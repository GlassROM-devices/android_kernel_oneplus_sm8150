/*
 * aQuantia Corporation Network Driver
 * Copyright (C) 2019 aQuantia Corporation. All rights reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 */

#include <linux/dma-mapping.h>
#include <linux/if_vlan.h>
#include <linux/init.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <net/genetlink.h>

#include "atl_fwdnl.h"

#include "atl_common.h"
#include "atl_compat.h"
#include "atl_ring.h"

#define ATL_FWDNL_PREFIX "atl_fwdnl: "
/* Forward declaration, actual definition is at the bottom of the file */
static struct genl_family atlfwd_nl_family;
static struct atl_fwd_ring *get_fwd_ring(struct net_device *netdev,
					 const int ring_index,
					 struct genl_info *info);
static unsigned int atlfwd_nl_dev_cache_index(const struct net_device *netdev);
static int atlfwd_nl_transmit_skb_ring(struct atl_fwd_ring *ring,
				       struct sk_buff *skb);

/* Register generic netlink family upon module initialization */
int __init atlfwd_nl_init(void)
{
	return genl_register_family(&atlfwd_nl_family);
}

#define MAX_NUM_ATLFWD_DEVICES 16 /* Maximum number of entries in cache */
/* ATL devices cache
 *
 * To make sure we are trying to communicate with supported devices only.
 */
static struct {
	int ifindex;
	/* State of forced redirections */
	int force_icmp_via;
	int force_tx_via;
} s_atlfwd_devices[MAX_NUM_ATLFWD_DEVICES];
static int s_atlfwd_devices_cnt; /* Total number of entries in cache */

/* Remember the ATL device on probe */
void atlfwd_nl_on_probe(const struct net_device *ndev)
{
	if (s_atlfwd_devices_cnt >= MAX_NUM_ATLFWD_DEVICES) {
		pr_warn(ATL_FWDNL_PREFIX
			"Device cache exceeded, consider increasing MAX_NUM_ATLFWD_DEVICES\n");
		return;
	}

	int i;

	for (i = 0; i != MAX_NUM_ATLFWD_DEVICES; i++) {
		if (s_atlfwd_devices[i].ifindex == 0) {
			s_atlfwd_devices[i].ifindex = ndev->ifindex;
			s_atlfwd_devices[i].force_icmp_via = S32_MIN;
			s_atlfwd_devices[i].force_tx_via = S32_MIN;
			s_atlfwd_devices_cnt++;
			break;
		}
	}

	if (i == MAX_NUM_ATLFWD_DEVICES)
		pr_warn(ATL_FWDNL_PREFIX
			"Device cache has issues: counter and the actual cache contents are inconsistent\n");
}

/* Remove the ATL device from the cache on remove request */
void atlfwd_nl_on_remove(const struct net_device *ndev)
{
	int i;

	for (i = 0; i != MAX_NUM_ATLFWD_DEVICES; i++) {
		if (s_atlfwd_devices[i].ifindex == ndev->ifindex) {
			s_atlfwd_devices_cnt--;
			s_atlfwd_devices[i].ifindex = 0;
			break;
		}
	}

	if (i == MAX_NUM_ATLFWD_DEVICES)
		pr_warn(ATL_FWDNL_PREFIX "Device cache miss for '%s' (%d)\n",
			ndev->name, ndev->ifindex);
}

/* Unregister generic netlink family upon module exit */
void __exit atlfwd_nl_exit(void)
{
	genl_unregister_family(&atlfwd_nl_family);
}

/* Returns true, if skb should be sent via FWD. false otherwise. */
bool atlfwd_nl_is_redirected(const struct sk_buff *skb, struct net_device *ndev)
{
	const unsigned int idx = atlfwd_nl_dev_cache_index(ndev);

	if (idx == MAX_NUM_ATLFWD_DEVICES) {
		pr_warn(ATL_FWDNL_PREFIX
			"%s called for non-ATL device (ifindex %d)\n",
			__func__, ndev->ifindex);
		return false;
	}

	if (unlikely(s_atlfwd_devices[idx].force_tx_via != S32_MIN)) {
		/* Redirect everything, but:
		 * . LSO is not supported at the moment
		 * . VLAN is not supported at the moment
		 */
		return (skb_shinfo(skb)->gso_size == 0 &&
			!skb_vlan_tag_present(skb));
	}

	if (unlikely(s_atlfwd_devices[idx].force_icmp_via != S32_MIN)) {
		uint8_t l4_proto = 0;

		switch (skb->protocol) {
		case htons(ETH_P_IP):
			l4_proto = ip_hdr(skb)->protocol;
			break;
		case htons(ETH_P_IPV6):
			l4_proto = ipv6_hdr(skb)->nexthdr;
			break;
		}

		switch (l4_proto) {
		case IPPROTO_ICMP:
			return true;
		}
	}

	return false;
}

netdev_tx_t atlfwd_nl_xmit(struct sk_buff *skb, struct net_device *ndev)
{
	const unsigned int idx = atlfwd_nl_dev_cache_index(ndev);
	struct atl_fwd_ring *ring = NULL;
	int ring_index = S32_MIN;

	if (idx == MAX_NUM_ATLFWD_DEVICES) {
		pr_warn(ATL_FWDNL_PREFIX
			"%s called for non-ATL device (ifindex %d)\n",
			__func__, ndev->ifindex);
		return -EFAULT;
	}

	if (s_atlfwd_devices[idx].force_tx_via != S32_MIN)
		ring_index = s_atlfwd_devices[idx].force_tx_via;
	else if (s_atlfwd_devices[idx].force_icmp_via != S32_MIN)
		ring_index = s_atlfwd_devices[idx].force_icmp_via;
	else
		return -EFAULT;

	ring = get_fwd_ring(ndev, ring_index, NULL);
	if (unlikely(ring == NULL))
		return -EFAULT;

	return atlfwd_nl_transmit_skb_ring(ring, skb);
}

/* Set the netlink error message
 *
 * Before Linux 4.12 we could only put it in kernel logs.
 * Starting with 4.12 we can also use extack to pass the message to user-mode.
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 12, 0)
#define ATLFWD_NL_SET_ERR_MSG(info, msg) GENL_SET_ERR_MSG(info, msg)
#else
#define ATLFWD_NL_SET_ERR_MSG(info, msg) pr_warn(ATL_FWDNL_PREFIX "%s\n", msg)
#endif

/* Attempt to auto-deduce the device
 *
 * This is possible if and only if there's exactly 1 ATL device in the system.
 *
 * Returns NULL on error, net_device pointer otherwise.
 */
static struct net_device *atlfwd_nl_auto_deduce_dev(struct genl_info *info)
{
	int i;

	if (s_atlfwd_devices_cnt > 1) {
		ATLFWD_NL_SET_ERR_MSG(
			info,
			"Device name is required since there are several ATL devices");
		return NULL;
	}

	for (i = 0; i != MAX_NUM_ATLFWD_DEVICES; i++) {
		const int ifindex = s_atlfwd_devices[i].ifindex;

		if (ifindex != 0) {
			pr_debug(ATL_FWDNL_PREFIX
				 "Found ifindex %d (auto-deduced)\n",
				 ifindex);
			return dev_get_by_index(genl_info_net(info), ifindex);
		}
	}

	ATLFWD_NL_SET_ERR_MSG(info, "No ATL devices found at the moment");
	return NULL;
}

/* Get the index (in cache) of the given ATL device.
 *
 * Returns MAX_NUM_ATLFWD_DEVICES on error, valid index otherwise.
 */
static unsigned int atlfwd_nl_dev_cache_index(const struct net_device *netdev)
{
	int i;

	for (i = 0; i != MAX_NUM_ATLFWD_DEVICES; i++) {
		if (s_atlfwd_devices[i].ifindex != 0 &&
		    s_atlfwd_devices[i].ifindex == netdev->ifindex)
			break;
	}

	return i;
}

/* Get net_device by name
 *
 * The name is usually provided by the user-mode tool via the IFNAME attribute.
 * But it can also be null, if the tool hasn't provided the attribute and wants
 * us to attempt to auto deduce the device.
 *
 * Returns NULL on error, net_device pointer otherwise.
 */
static struct net_device *atlfwd_nl_get_dev_by_name(const char *dev_name,
						    struct genl_info *info)
{
	unsigned int idx = MAX_NUM_ATLFWD_DEVICES;
	struct net_device *netdev = NULL;

	if (dev_name == NULL) {
		/* No dev_name provided in request, try to auto-deduce. */
		return atlfwd_nl_auto_deduce_dev(info);
	}

	netdev = dev_get_by_name(genl_info_net(info), dev_name);
	if (netdev == NULL) {
		ATLFWD_NL_SET_ERR_MSG(info, "No matching device found");
		return NULL;
	}

	/* Check ATL device cache to make sure an ATL device was requested */
	idx = atlfwd_nl_dev_cache_index(netdev);
	if (idx != MAX_NUM_ATLFWD_DEVICES) {
		pr_debug(ATL_FWDNL_PREFIX "Found ifindex %d for '%s'\n",
			 s_atlfwd_devices[idx].ifindex, dev_name);
		return netdev;
	}

	ATLFWD_NL_SET_ERR_MSG(info, "Requested device is not an ATL device");
	dev_put(netdev);
	return NULL;
}

/* Helper function to obtain the netlink attribute string data
 *
 * Returns NULL, if the attribute doesn't exist.
 */
static const char *
atlfwd_attr_to_str_or_null(struct genl_info *info,
			   const enum atlfwd_nl_attribute attr)
{
	if (likely(!info->attrs[attr]))
		return NULL;

	return (char *)nla_data(info->attrs[attr]);
}

/* Helper function to obtain the netlink attribute s32 data
 *
 * Returns S32_MIN on error, actual attribute value otherwise.
 */
static int atlfwd_attr_to_s32(struct genl_info *info,
			      const enum atlfwd_nl_attribute attr)
{
	if (info->attrs[attr])
		return nla_get_s32(info->attrs[attr]);

	pr_warn(ATL_FWDNL_PREFIX "attribute %d check is missing in pre_doit\n",
		attr);
	ATLFWD_NL_SET_ERR_MSG(
		info, "Required attribute is missing (and internal error)");
	return S32_MIN;
}

static bool is_tx_ring(const int ring_index)
{
	return ring_index % 2;
}

static int nl_ring_index(const struct atl_fwd_ring *ring, const bool is_tx)
{
	int idx = 0;

	if (ring->idx < ATL_FWD_RING_BASE) {
		pr_warn(ATL_FWDNL_PREFIX
			"Got an unexpected ring index %d (was expecting %d or greater)\n",
			ring->idx, ATL_FWD_RING_BASE);
		return S32_MIN;
	}

	/* Expose a 0-based index to the user-mode */
	idx = ring->idx - ATL_FWD_RING_BASE;

	/* Use even (0,2,...) indexes for RX rings, odd - for TX. */
	idx *= 2;
	if (is_tx)
		idx++;
	pr_debug(ATL_FWDNL_PREFIX "Normalized ring index %d\n", idx);

	return idx;
}

static int send_nl_reply(struct genl_info *info,
			 const enum atlfwd_nl_command reply_cmd,
			 const enum atlfwd_nl_attribute attr, const int value)
{
	struct sk_buff *msg = nlmsg_new(NLMSG_DEFAULT_SIZE, GFP_KERNEL);
	void *hdr = NULL;

	if (msg == NULL)
		return -ENOBUFS;

	hdr = genlmsg_put(msg, info->snd_portid, info->snd_seq,
			  &atlfwd_nl_family, 0, reply_cmd);
	if (hdr == NULL) {
		ATLFWD_NL_SET_ERR_MSG(info, "Reply message creation failed");
		nlmsg_free(msg);
		return -EMSGSIZE;
	}

	if (nla_put_s32(msg, attr, value) != 0) {
		ATLFWD_NL_SET_ERR_MSG(info,
				      "Failed to put the reply attribute");
		genlmsg_cancel(msg, hdr);
		nlmsg_free(msg);
		return -EMSGSIZE;
	}

	genlmsg_end(msg, hdr);
	return genlmsg_reply(msg, info);
}

/* Get the FWD ring by index.
 *
 * This function obtains the FWD ring descriptor/pointer from the private
 * part of net_device data.
 *
 * Returns NULL on error.
 */
static struct atl_fwd_ring *get_fwd_ring(struct net_device *netdev,
					 const int ring_index,
					 struct genl_info *info)
{
	const int dir_tx = is_tx_ring(ring_index);
	const int norm_index = (dir_tx ? ring_index - 1 : ring_index) / 2;
	struct atl_nic *nic = netdev_priv(netdev);
	struct atl_fwd_ring *ring = nic->fwd.rings[dir_tx][norm_index];

	pr_debug(ATL_FWDNL_PREFIX "index=%d/n%d\n", ring_index, norm_index);

	if (unlikely(norm_index < 0 || norm_index >= ATL_NUM_FWD_RINGS)) {
		if (info)
			ATLFWD_NL_SET_ERR_MSG(info,
					      "Ring index is out of bounds");
		return NULL;
	}

	if (unlikely(ring == NULL && info))
		ATLFWD_NL_SET_ERR_MSG(info,
				      "Requested ring is NULL / released");

	return ring;
}

static int atlfwd_nl_tx_ring_head(struct atl_fwd_ring *ring)
{
	/* Assume there are no other users for now */
	return atl_read(&ring->nic->hw, ATL_TX_RING_HEAD(ring)) & 0x1fff;
}

static int atlfwd_nl_tx_ring_tail(struct atl_fwd_ring *ring)
{
	/* Assume there are no other users for now */
	return atl_read(&ring->nic->hw, ATL_TX_RING_TAIL(ring));
}

static bool atlfwd_nl_tx_full(struct atl_fwd_ring *ring, const int needed)
{
	int space =
		atlfwd_nl_tx_ring_head(ring) - atlfwd_nl_tx_ring_tail(ring) - 1;

	if (space < 0)
		space += ring->hw.size;

	if (likely(space >= needed))
		return false;

	return true;
}

/* Returns checksum offload flags for TX descriptor */
static unsigned int
atlfwd_nl_skb_checksum_offload_cmd(const struct sk_buff *skb)
{
	unsigned int cmd = 0;

	if (skb->ip_summed == CHECKSUM_PARTIAL) {
		/* Checksum offload has been requested by the stack */
		uint8_t l4_proto = 0;

		switch (skb->protocol) {
		case htons(ETH_P_IP):
			cmd |= tx_desc_cmd_ipv4cs;
			l4_proto = ip_hdr(skb)->protocol;
			break;
		case htons(ETH_P_IPV6):
			l4_proto = ipv6_hdr(skb)->nexthdr;
			break;
		}

		switch (l4_proto) {
		case IPPROTO_TCP:
		case IPPROTO_UDP:
			cmd |= tx_desc_cmd_l4cs;
			break;
		}
	}

	return cmd;
}

static int atlfwd_nl_num_txd_for_skb(struct sk_buff *skb)
{
	unsigned int len = skb_headlen(skb);
	unsigned int num_txd =
		(len - len % ATL_DATA_PER_TXD) / ATL_DATA_PER_TXD + 1;
	unsigned int frag;

	for (frag = 0; frag != skb_shinfo(skb)->nr_frags; frag++) {
		len = skb_frag_size(&skb_shinfo(skb)->frags[frag]);
		num_txd +=
			(len - len % ATL_DATA_PER_TXD) / ATL_DATA_PER_TXD + 1;
	}

	return num_txd;
}

static int atlfwd_nl_transmit_skb_ring(struct atl_fwd_ring *ring,
				       struct sk_buff *skb)
{
	unsigned int frags = skb_shinfo(skb)->nr_frags;
	int idx = atlfwd_nl_tx_ring_tail(ring);
	unsigned int len = skb_headlen(skb);
	skb_frag_t *frag = NULL;
	struct atl_hw *hw = &ring->nic->hw;
	struct device *dev = &hw->pdev->dev;
	struct atl_tx_desc desc;
	dma_addr_t daddr = 0;

	if (atlfwd_nl_tx_full(ring, atlfwd_nl_num_txd_for_skb(skb)))
		return NETDEV_TX_BUSY;

	memset(&desc, 0, sizeof(desc));
	desc.cmd = tx_desc_cmd_fcs;
	desc.cmd |= atlfwd_nl_skb_checksum_offload_cmd(skb);
	desc.ct_en = 0;
	desc.type = tx_desc_type_desc;
	desc.pay_len = skb->len;

	daddr = dma_map_single(dev, skb->data, len, DMA_TO_DEVICE);
	for (frag = &skb_shinfo(skb)->frags[0];; frag++) {
		if (dma_mapping_error(dev, daddr))
			goto err_dma;

		desc.daddr = cpu_to_le64(daddr);
		while (len > ATL_DATA_PER_TXD) {
			desc.len = cpu_to_le16(ATL_DATA_PER_TXD);
			WRITE_ONCE(ring->hw.descs[idx].tx, desc);
			bump_ptr(idx, ring, 1);
			daddr += ATL_DATA_PER_TXD;
			len -= ATL_DATA_PER_TXD;
			desc.daddr = cpu_to_le64(daddr);
		}
		desc.len = cpu_to_le16(len);

		if (!frags)
			break;

		WRITE_ONCE(ring->hw.descs[idx].tx, desc);
		bump_ptr(idx, ring, 1);
		len = skb_frag_size(frag);
		daddr = skb_frag_dma_map(dev, frag, 0, len, DMA_TO_DEVICE);

		frags--;
	}

	/* Last descriptor */
	desc.eop = 1;
#if defined(ATL_TX_DESC_WB) || defined(ATL_TX_HEAD_WB)
	desc.cmd |= tx_desc_cmd_wb;
#endif
	WRITE_ONCE(ring->hw.descs[idx].tx, desc);
	bump_ptr(idx, ring, 1);

	wmb();
	atl_write(hw, ATL_TX_RING_TAIL(ring), idx);

	return NETDEV_TX_OK;

err_dma:
	dev_err(dev, "%s failed\n", __func__);
	return -EFAULT;
}

/* ATL_FWD_CMD_REQUEST_RING handler */
static int atlfwd_nl_request_ring(struct sk_buff *skb, struct genl_info *info)
{
	struct net_device *netdev = NULL;
	struct atl_fwd_ring *ring = NULL;
	const char *ifname = NULL;
	int page_order = S32_MIN;
	int ring_index = S32_MIN;
	int ring_size = S32_MIN;
	int buf_size = S32_MIN;
	int flags = S32_MIN;
	int result = 0;

	/* 1. get net_device */
	ifname = atlfwd_attr_to_str_or_null(info, ATL_FWD_ATTR_IFNAME);
	netdev = atlfwd_nl_get_dev_by_name(ifname, info);
	if (netdev == NULL)
		return -ENODEV;

	/* 2. parse mandatory attributes */
	flags = atlfwd_attr_to_s32(info, ATL_FWD_ATTR_FLAGS);
	ring_size = atlfwd_attr_to_s32(info, ATL_FWD_ATTR_RING_SIZE);
	buf_size = atlfwd_attr_to_s32(info, ATL_FWD_ATTR_BUF_SIZE);
	page_order = atlfwd_attr_to_s32(info, ATL_FWD_ATTR_PAGE_ORDER);
	if (unlikely(flags == S32_MIN || ring_size == S32_MIN ||
		     buf_size == S32_MIN || page_order == S32_MIN)) {
		result = -EINVAL;
		goto err_netdev;
	}

	pr_debug(ATL_FWDNL_PREFIX "flags=%d\n", flags);
	pr_debug(ATL_FWDNL_PREFIX "ring_size=%d\n", ring_size);
	pr_debug(ATL_FWDNL_PREFIX "buf_size=%d\n", buf_size);
	pr_debug(ATL_FWDNL_PREFIX "page_order=%d\n", page_order);

	ring = atl_fwd_request_ring(netdev, flags, ring_size, buf_size,
				    page_order, NULL);
	if (IS_ERR_OR_NULL(ring)) {
		result = PTR_ERR(ring);
		goto err_netdev;
	}

	pr_debug(ATL_FWDNL_PREFIX "Got ring %d (%p)\n", ring->idx, ring);
	ring_index = nl_ring_index(ring, !!(flags & ATL_FWR_TX));
	if (ring_index == S32_MIN) {
		ATLFWD_NL_SET_ERR_MSG(info, "Internal error");
		atl_fwd_release_ring(ring);
		result = -EINVAL;
		goto err_netdev;
	}

	result = send_nl_reply(info, ATL_FWD_CMD_RELEASE_RING,
			       ATL_FWD_ATTR_RING_INDEX, ring_index);

err_netdev:
	dev_put(netdev);
	return result;
}

/* ATL_FWD_CMD_RELEASE_RING handler */
static int atlfwd_nl_release_ring(struct sk_buff *skb, struct genl_info *info)
{
	struct net_device *netdev = NULL;
	struct atl_fwd_ring *ring = NULL;
	const char *ifname = NULL;
	int ring_index = S32_MIN;
	int result = 0;

	/* 1. get net_device */
	ifname = atlfwd_attr_to_str_or_null(info, ATL_FWD_ATTR_IFNAME);
	netdev = atlfwd_nl_get_dev_by_name(ifname, info);
	if (netdev == NULL)
		return -ENODEV;

	/* 2. parse mandatory attributes */
	ring_index = atlfwd_attr_to_s32(info, ATL_FWD_ATTR_RING_INDEX);
	if (unlikely(ring_index == S32_MIN)) {
		result = -EINVAL;
		goto err_netdev;
	}

	ring = get_fwd_ring(netdev, ring_index, info);
	if (unlikely(ring == NULL)) {
		result = -EINVAL;
		goto err_netdev;
	}

	pr_debug(ATL_FWDNL_PREFIX "Releasing ring %d (%p)\n", ring_index, ring);
	atl_fwd_release_ring(ring);

err_netdev:
	dev_put(netdev);
	return result;
}

/* ATL_FWD_CMD_ENABLE_RING handler */
static int atlfwd_nl_enable_ring(struct sk_buff *skb, struct genl_info *info)
{
	struct net_device *netdev = NULL;
	struct atl_fwd_ring *ring = NULL;
	const char *ifname = NULL;
	int ring_index = S32_MIN;
	int result = 0;

	/* 1. get net_device */
	ifname = atlfwd_attr_to_str_or_null(info, ATL_FWD_ATTR_IFNAME);
	netdev = atlfwd_nl_get_dev_by_name(ifname, info);
	if (netdev == NULL)
		return -ENODEV;

	/* 2. parse mandatory attributes */
	ring_index = atlfwd_attr_to_s32(info, ATL_FWD_ATTR_RING_INDEX);
	if (unlikely(ring_index == S32_MIN)) {
		result = -EINVAL;
		goto err_netdev;
	}

	ring = get_fwd_ring(netdev, ring_index, info);
	if (unlikely(ring == NULL)) {
		result = -EINVAL;
		goto err_netdev;
	}

	pr_debug(ATL_FWDNL_PREFIX "Enabling ring %d (%p)\n", ring_index, ring);
	atl_fwd_enable_ring(ring);

err_netdev:
	dev_put(netdev);
	return result;
}

/* ATL_FWD_CMD_DISABLE_RING handler */
static int atlfwd_nl_disable_ring(struct sk_buff *skb, struct genl_info *info)
{
	struct net_device *netdev = NULL;
	struct atl_fwd_ring *ring = NULL;
	const char *ifname = NULL;
	int ring_index = S32_MIN;
	int result = 0;

	/* 1. get net_device */
	ifname = atlfwd_attr_to_str_or_null(info, ATL_FWD_ATTR_IFNAME);
	netdev = atlfwd_nl_get_dev_by_name(ifname, info);
	if (netdev == NULL)
		return -ENODEV;

	/* 2. parse mandatory attributes */
	ring_index = atlfwd_attr_to_s32(info, ATL_FWD_ATTR_RING_INDEX);
	if (unlikely(ring_index == S32_MIN)) {
		result = -EINVAL;
		goto err_netdev;
	}

	ring = get_fwd_ring(netdev, ring_index, info);
	if (unlikely(ring == NULL)) {
		result = -EINVAL;
		goto err_netdev;
	}

	pr_debug(ATL_FWDNL_PREFIX "Disabling ring %d (%p)\n", ring_index, ring);
	atl_fwd_disable_ring(ring);

err_netdev:
	dev_put(netdev);
	return result;
}

/* ATL_FWD_CMD_DISABLE_REDIRECTIONS handler */
static int atlfwd_nl_disable_redirections(struct sk_buff *skb,
					  struct genl_info *info)
{
	unsigned int idx = MAX_NUM_ATLFWD_DEVICES;
	struct net_device *netdev = NULL;
	const char *ifname = NULL;
	int result = 0;

	/* 1. get net_device */
	ifname = atlfwd_attr_to_str_or_null(info, ATL_FWD_ATTR_IFNAME);
	netdev = atlfwd_nl_get_dev_by_name(ifname, info);
	if (netdev == NULL)
		return -ENODEV;

	idx = atlfwd_nl_dev_cache_index(netdev);
	if (idx == MAX_NUM_ATLFWD_DEVICES) {
		pr_warn(ATL_FWDNL_PREFIX
			"%s called for non-ATL device (ifindex %d)\n",
			__func__, netdev->ifindex);
		return -EFAULT;
	}

	pr_debug(ATL_FWDNL_PREFIX "All forced redirections are disabled now\n");
	s_atlfwd_devices[idx].force_icmp_via = S32_MIN;
	s_atlfwd_devices[idx].force_tx_via = S32_MIN;

	dev_put(netdev);
	return result;
}

/* ATL_FWD_CMD_FORCE_ICMP_TX_VIA handler */
static int atlfwd_nl_force_icmp_tx_via(struct sk_buff *skb,
				       struct genl_info *info)
{
	unsigned int idx = MAX_NUM_ATLFWD_DEVICES;
	struct net_device *netdev = NULL;
	struct atl_fwd_ring *ring = NULL;
	const char *ifname = NULL;
	int ring_index = S32_MIN;
	int result = -EINVAL;

	/* 1. get net_device */
	ifname = atlfwd_attr_to_str_or_null(info, ATL_FWD_ATTR_IFNAME);
	netdev = atlfwd_nl_get_dev_by_name(ifname, info);
	if (netdev == NULL)
		return -ENODEV;

	idx = atlfwd_nl_dev_cache_index(netdev);
	if (idx == MAX_NUM_ATLFWD_DEVICES) {
		pr_warn(ATL_FWDNL_PREFIX
			"%s called for non-ATL device (ifindex %d)\n",
			__func__, netdev->ifindex);
		return -EFAULT;
	}

	/* 2. parse mandatory attributes */
	ring_index = atlfwd_attr_to_s32(info, ATL_FWD_ATTR_RING_INDEX);
	if (unlikely(ring_index == S32_MIN))
		goto err_netdev;
	if (unlikely(!is_tx_ring(ring_index))) {
		ATLFWD_NL_SET_ERR_MSG(info, "Expected TX ring");
		goto err_netdev;
	}

	ring = get_fwd_ring(netdev, ring_index, info);
	if (unlikely(ring == NULL))
		goto err_netdev;

	result = 0;
	pr_debug(ATL_FWDNL_PREFIX
		 "All egress ICMP traffic is now forced via ring %d (%p)\n",
		 ring_index, ring);
	s_atlfwd_devices[idx].force_icmp_via = ring_index;

err_netdev:
	dev_put(netdev);
	return result;
}

/* ATL_FWD_CMD_FORCE_TX_VIA handler */
static int atlfwd_nl_force_tx_via(struct sk_buff *skb, struct genl_info *info)
{
	unsigned int idx = MAX_NUM_ATLFWD_DEVICES;
	struct net_device *netdev = NULL;
	struct atl_fwd_ring *ring = NULL;
	const char *ifname = NULL;
	int ring_index = S32_MIN;
	int result = -EINVAL;

	/* 1. get net_device */
	ifname = atlfwd_attr_to_str_or_null(info, ATL_FWD_ATTR_IFNAME);
	netdev = atlfwd_nl_get_dev_by_name(ifname, info);
	if (netdev == NULL)
		return -ENODEV;

	idx = atlfwd_nl_dev_cache_index(netdev);
	if (idx == MAX_NUM_ATLFWD_DEVICES) {
		pr_warn(ATL_FWDNL_PREFIX
			"%s called for non-ATL device (ifindex %d)\n",
			__func__, netdev->ifindex);
		return -EFAULT;
	}

	/* 2. parse mandatory attributes */
	ring_index = atlfwd_attr_to_s32(info, ATL_FWD_ATTR_RING_INDEX);
	if (unlikely(ring_index == S32_MIN))
		goto err_netdev;
	if (unlikely(!is_tx_ring(ring_index))) {
		ATLFWD_NL_SET_ERR_MSG(info, "Expected TX ring");
		goto err_netdev;
	}

	ring = get_fwd_ring(netdev, ring_index, info);
	if (unlikely(ring == NULL))
		goto err_netdev;

	result = 0;
	pr_debug(ATL_FWDNL_PREFIX
		 "All egress traffic is now forced via ring %d (%p)\n",
		 ring_index, ring);
	s_atlfwd_devices[idx].force_tx_via = ring_index;

err_netdev:
	dev_put(netdev);
	return result;
}

/* This handler is called before the actual command handler.
 *
 * At the moment we simply check that all mandatory attributes are present.
 *
 * Returns 0 on success, error otherwise.
 */
static int atlfwd_nl_pre_doit(const struct genl_ops *ops, struct sk_buff *skb,
			      struct genl_info *info)
{
	enum atlfwd_nl_attribute missing_attr = ATL_FWD_ATTR_INVALID;
	int ret = 0;

	switch (ops->cmd) {
	case ATL_FWD_CMD_REQUEST_RING:
		if (!info->attrs[ATL_FWD_ATTR_FLAGS])
			missing_attr = ATL_FWD_ATTR_FLAGS;
		else if (!info->attrs[ATL_FWD_ATTR_RING_SIZE])
			missing_attr = ATL_FWD_ATTR_RING_SIZE;
		else if (!info->attrs[ATL_FWD_ATTR_BUF_SIZE])
			missing_attr = ATL_FWD_ATTR_BUF_SIZE;
		else if (!info->attrs[ATL_FWD_ATTR_PAGE_ORDER])
			missing_attr = ATL_FWD_ATTR_PAGE_ORDER;
		break;
	case ATL_FWD_CMD_RELEASE_RING:
	case ATL_FWD_CMD_ENABLE_RING:
	case ATL_FWD_CMD_DISABLE_RING:
	case ATL_FWD_CMD_FORCE_ICMP_TX_VIA:
	case ATL_FWD_CMD_FORCE_TX_VIA:
		if (!info->attrs[ATL_FWD_ATTR_RING_INDEX])
			missing_attr = ATL_FWD_ATTR_RING_INDEX;
		break;
	case ATL_FWD_CMD_DISABLE_REDIRECTIONS:
		/* no attributes => nothing to check */
		break;
	default:
		ATLFWD_NL_SET_ERR_MSG(info, "Unknown command");
		return -EINVAL;
	}

	if (unlikely(missing_attr != ATL_FWD_ATTR_INVALID)) {
		pr_warn(ATL_FWDNL_PREFIX "Required attribute is missing: %d\n",
			missing_attr);
		ATLFWD_NL_SET_ERR_MSG(info, "Required attribute is missing");
		ret = -EINVAL;
	}
	return ret;
}

/* netlink-specific structures */
static const struct nla_policy atlfwd_nl_policy[NUM_ATL_FWD_ATTR] = {
	[ATL_FWD_ATTR_IFNAME] = { .type = NLA_NUL_STRING, .len = IFNAMSIZ - 1 },
	[ATL_FWD_ATTR_FLAGS] = { .type = NLA_S32 },
	[ATL_FWD_ATTR_RING_SIZE] = { .type = NLA_S32 },
	[ATL_FWD_ATTR_BUF_SIZE] = { .type = NLA_S32 },
	[ATL_FWD_ATTR_PAGE_ORDER] = { .type = NLA_S32 },
	[ATL_FWD_ATTR_RING_INDEX] = { .type = NLA_S32 },
};

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 2, 0)
#define ATLFWD_NL_OP_POLICY(op_policy) .policy = op_policy
#else
#define ATLFWD_NL_OP_POLICY(op_policy)
#endif

static const struct genl_ops atlfwd_nl_ops[] = {
	{ .cmd = ATL_FWD_CMD_REQUEST_RING,
	  .doit = atlfwd_nl_request_ring,
	  ATLFWD_NL_OP_POLICY(atlfwd_nl_policy) },
	{ .cmd = ATL_FWD_CMD_RELEASE_RING,
	  .doit = atlfwd_nl_release_ring,
	  ATLFWD_NL_OP_POLICY(atlfwd_nl_policy) },
	{ .cmd = ATL_FWD_CMD_ENABLE_RING,
	  .doit = atlfwd_nl_enable_ring,
	  ATLFWD_NL_OP_POLICY(atlfwd_nl_policy) },
	{ .cmd = ATL_FWD_CMD_DISABLE_RING,
	  .doit = atlfwd_nl_disable_ring,
	  ATLFWD_NL_OP_POLICY(atlfwd_nl_policy) },
	{ .cmd = ATL_FWD_CMD_DISABLE_REDIRECTIONS,
	  .doit = atlfwd_nl_disable_redirections,
	  ATLFWD_NL_OP_POLICY(atlfwd_nl_policy) },
	{ .cmd = ATL_FWD_CMD_FORCE_ICMP_TX_VIA,
	  .doit = atlfwd_nl_force_icmp_tx_via,
	  ATLFWD_NL_OP_POLICY(atlfwd_nl_policy) },
	{ .cmd = ATL_FWD_CMD_FORCE_TX_VIA,
	  .doit = atlfwd_nl_force_tx_via,
	  ATLFWD_NL_OP_POLICY(atlfwd_nl_policy) },
};

static struct genl_family atlfwd_nl_family = {
	.hdrsize = 0, /* no private header */
	.name = ATL_FWD_GENL_NAME, /* have users key off the name instead */
	.version = 1, /* no particular meaning now */
	.maxattr = ATL_FWD_ATTR_MAX,
	.netnsok = false,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 2, 0)
	.policy = atlfwd_nl_policy,
#endif
	.pre_doit = atlfwd_nl_pre_doit,
	.ops = atlfwd_nl_ops,
	.n_ops = ARRAY_SIZE(atlfwd_nl_ops),
	.module = THIS_MODULE,
};
