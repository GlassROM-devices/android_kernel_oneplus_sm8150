/*
 * aQuantia Corporation Network Driver
 * Copyright (C) 2017 aQuantia Corporation. All rights reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 */

#ifndef _ATL_COMMON_H_
#define _ATL_COMMON_H_

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/pci.h>
#include <linux/list.h>
#include <linux/dma-mapping.h>
#include <linux/netdevice.h>
#include <linux/moduleparam.h>

#define ATL_VERSION "1.0.9"

struct atl_nic;

#include "atl_compat.h"
#include "atl_hw.h"

#define ATL_MAX_QUEUES 8

#include "atl_fwd.h"

struct atl_rx_ring_stats {
	uint64_t packets;
	uint64_t bytes;
	uint64_t linear_dropped;
	uint64_t alloc_skb_failed;
	uint64_t reused_head_page;
	uint64_t reused_data_page;
	uint64_t alloc_head_page;
	uint64_t alloc_data_page;
	uint64_t alloc_head_page_failed;
	uint64_t alloc_data_page_failed;
	uint64_t non_eop_descs;
	uint64_t mac_err;
	uint64_t csum_err;
	uint64_t multicast;
};

struct atl_tx_ring_stats {
	uint64_t packets;
	uint64_t bytes;
	uint64_t tx_busy;
	uint64_t tx_restart;
	uint64_t dma_map_failed;
};

struct atl_ring_stats {
	union {
		struct atl_rx_ring_stats rx;
		struct atl_tx_ring_stats tx;
	};
};

struct atl_msm_stats {
	uint32_t rx_pause;
	uint32_t tx_pause;
};

struct atl_global_stats {
	struct atl_rx_ring_stats rx;
	struct atl_tx_ring_stats tx;
	struct atl_msm_stats msm;
};

enum {
	ATL_RXF_VLAN_BASE = 0,
	ATL_RXF_VLAN_MAX = 8,
	ATL_RXF_ETYPE_BASE = ATL_RXF_VLAN_BASE + ATL_RXF_VLAN_MAX,
	ATL_RXF_ETYPE_MAX = 16,
	ATL_RXF_NTUPLE_BASE = ATL_RXF_ETYPE_BASE + ATL_RXF_ETYPE_MAX,
	ATL_RXF_NTUPLE_MAX = 8,
};

enum atl_rxf_common_cmd {
	ATL_RXF_EN = BIT(31),
	ATL_RXF_RXQ_MSK = BIT(5) - 1,
};

enum atl_ntuple_cmd {
	ATL_NTC_EN = ATL_RXF_EN, /* Filter enabled */
	ATL_NTC_V6 = BIT(30),	/* IPv6 mode -- only valid in filters
				 * 0 and 4 */
	ATL_NTC_SA = BIT(29),	/* Match source address */
	ATL_NTC_DA = BIT(28),	/* Match destination address */
	ATL_NTC_SP = BIT(27),	/* Match source port */
	ATL_NTC_DP = BIT(26),	/* Match destination port */
	ATL_NTC_PROTO = BIT(25), /* Match L4 proto */
	ATL_NTC_ARP = BIT(24),
	ATL_NTC_RXQ = BIT(23),	/* Assign Rx queue */
	ATL_NTC_ACT_SHIFT = 16,
	ATL_NTC_RXQ_SHIFT = 8,
	ATL_NTC_RXQ_MASK = ATL_RXF_RXQ_MSK << ATL_NTC_RXQ_SHIFT,
	ATL_NTC_L4_MASK = BIT(3) - 1,
	ATL_NTC_L4_TCP = 0,
	ATL_NTC_L4_UDP = 1,
	ATL_NTC_L4_SCTP = 2,
	ATL_NTC_L4_ICMP = 3,
};

struct atl_rxf_ntuple {
	union {
		struct {
			__be32 dst_ip4[ATL_RXF_NTUPLE_MAX];
			__be32 src_ip4[ATL_RXF_NTUPLE_MAX];
		};
		struct {
			__be32 dst_ip6[ATL_RXF_NTUPLE_MAX / 4][4];
			__be32 src_ip6[ATL_RXF_NTUPLE_MAX / 4][4];
		};
	};
	__be16 dst_port[ATL_RXF_NTUPLE_MAX];
	__be16 src_port[ATL_RXF_NTUPLE_MAX];
	uint32_t cmd[ATL_RXF_NTUPLE_MAX];
	int count;
};

enum atl_vlan_cmd {
	ATL_VLAN_EN = ATL_RXF_EN,
	ATL_VLAN_RXQ = BIT(28),
	ATL_VLAN_RXQ_SHIFT = 20,
	ATL_VLAN_RXQ_MASK = ATL_RXF_RXQ_MSK << ATL_VLAN_RXQ_SHIFT,
	ATL_VLAN_ACT_SHIFT = 16,
	ATL_VLAN_VID_MASK = BIT(12) - 1,
};

struct atl_rxf_vlan {
	uint32_t cmd[ATL_RXF_VLAN_MAX];
	int count;
};

enum atl_etype_cmd {
	ATL_ETYPE_EN = ATL_RXF_EN,
	ATL_ETYPE_RXQ = BIT(29),
	ATL_ETYPE_RXQ_SHIFT = 20,
	ATL_ETYPE_RXQ_MASK = ATL_RXF_RXQ_MSK << ATL_ETYPE_RXQ_SHIFT,
	ATL_ETYPE_ACT_SHIFT = 16,
	ATL_ETYPE_VAL_MASK = BIT(16) - 1,
};

struct atl_rxf_etype {
	uint32_t cmd[ATL_RXF_ETYPE_MAX];
	int count;
};

struct atl_queue_vec;

#define ATL_NUM_FWD_RINGS ATL_MAX_QUEUES
#define ATL_FWD_RING_BASE ATL_MAX_QUEUES /* Use TC 1 for offload
					  * engine rings */
#define ATL_NUM_MSI_VECS 32
#define ATL_NUM_NON_RING_IRQS 1

#define ATL_FWD_MSI_BASE (ATL_MAX_QUEUES + ATL_NUM_NON_RING_IRQS)

struct atl_fwd {
	unsigned long ring_map[2];
	struct atl_fwd_ring *rings[2][ATL_NUM_FWD_RINGS];
	unsigned long msi_map;
};

struct atl_nic {
	struct net_device *ndev;

	struct atl_queue_vec *qvecs;
	int nvecs;
	struct atl_hw hw;
	unsigned flags;
	unsigned long state;
	uint32_t priv_flags;
	struct timer_list link_timer;
	int max_mtu;
	int requested_nvecs;
	int requested_rx_size;
	int requested_tx_size;
	int rx_intr_delay;
	int tx_intr_delay;
	struct atl_global_stats stats;
	spinlock_t stats_lock;

	struct atl_fwd fwd;

	struct atl_rxf_ntuple rxf_ntuple;
	struct atl_rxf_vlan rxf_vlan;
	struct atl_rxf_etype rxf_etype;
};

enum atl_nic_flags {
	ATL_FL_MULTIPLE_VECTORS = BIT(0),
};

enum atl_nic_state {
	ATL_ST_UP,
	ATL_ST_CONFIGURED,
};

#define ATL_PF(_name) ATL_PF_ ## _name
#define ATL_PF_BIT(_name) ATL_PF_ ## _name ## _BIT
#define ATL_DEF_PF_BIT(_name) ATL_PF_BIT(_name) = BIT(ATL_PF(_name))

enum atl_priv_flags {
	ATL_PF_LPB_SYS_PB,
	ATL_PF_LPB_SYS_DMA,
	/* ATL_PF_LPB_NET_DMA, */
};

enum atl_priv_flag_bits {
	ATL_DEF_PF_BIT(LPB_SYS_PB),
	ATL_DEF_PF_BIT(LPB_SYS_DMA),
	/* ATL_DEF_PF_BIT(LPB_NET_DMA), */

	ATL_PF_LPB_MASK = ATL_PF_BIT(LPB_SYS_DMA) | ATL_PF_BIT(LPB_SYS_PB)
		/* | ATL_PF_BIT(LPB_NET_DMA) */,
};

#define ATL_MAX_MTU (16352 - ETH_FCS_LEN - ETH_HLEN)

#define ATL_MAX_RING_SIZE (8192 - 8)
#define ATL_RING_SIZE 4096

extern const char atl_driver_name[];

extern const struct ethtool_ops atl_ethtool_ops;

extern int atl_max_queues;
extern unsigned atl_rx_linear;
extern unsigned atl_min_intr_delay;

/* Logging conviniency macros.
 *
 * atl_dev_xxx are for low-level contexts and implicitly reference
 * struct atl_hw *hw;
 *
 * atl_nic_xxx are for high-level contexts and implicitly reference
 * struct atl_nic *nic; */
#define atl_dev_dbg(fmt, args...)			\
	dev_dbg(&hw->pdev->dev, fmt, ## args)
#define atl_dev_info(fmt, args...)			\
	dev_info(&hw->pdev->dev, fmt, ## args)
#define atl_dev_warn(fmt, args...)			\
	dev_warn(&hw->pdev->dev, fmt, ## args)
#define atl_dev_err(fmt, args...)			\
	dev_err(&hw->pdev->dev, fmt, ## args)

#define atl_nic_dbg(fmt, args...)		\
	dev_dbg(&nic->hw.pdev->dev, fmt, ## args)
#define atl_nic_info(fmt, args...)		\
	dev_info(&nic->hw.pdev->dev, fmt, ## args)
#define atl_nic_warn(fmt, args...)		\
	dev_warn(&nic->hw.pdev->dev, fmt, ## args)
#define atl_nic_err(fmt, args...)		\
	dev_err(&nic->hw.pdev->dev, fmt, ## args)

netdev_tx_t atl_start_xmit(struct sk_buff *skb, struct net_device *ndev);
int atl_vlan_rx_add_vid(struct net_device *ndev, __be16 proto, u16 vid);
int atl_vlan_rx_kill_vid(struct net_device *ndev, __be16 proto, u16 vid);
void atl_set_rx_mode(struct net_device *ndev);
int atl_set_features(struct net_device *ndev, netdev_features_t features);
void atl_get_stats64(struct net_device *ndev,
	struct rtnl_link_stats64 *stats);
int atl_setup_datapath(struct atl_nic *nic);
void atl_clear_datapath(struct atl_nic *nic);
int atl_start_rings(struct atl_nic *nic);
void atl_stop_rings(struct atl_nic *nic);
int atl_init_ring_interrupts(struct atl_nic *nic);
void atl_release_ring_interrupts(struct atl_nic *nic);
irqreturn_t atl_ring_irq(int irq, void *priv);
int atl_start_hw(struct atl_nic *nic);
void atl_stop_hw(struct atl_nic *nic);
int atl_fw_init(struct atl_hw *hw);
int atl_reconfigure(struct atl_nic *nic);
void atl_update_global_stats(struct atl_nic *nic);
void atl_set_loopback(struct atl_nic *nic, int idx, bool on);
void atl_set_intr_mod(struct atl_nic *nic);
void atl_update_ntuple_flt(struct atl_nic *nic, int idx);
int atl_hwsem_get(struct atl_hw *hw, int idx);
void atl_hwsem_put(struct atl_hw *hw, int idx);
int __atl_msm_read(struct atl_hw *hw, uint32_t addr, uint32_t *val);
int atl_msm_read(struct atl_hw *hw, uint32_t addr, uint32_t *val);
int __atl_msm_write(struct atl_hw *hw, uint32_t addr, uint32_t val);
int atl_msm_write(struct atl_hw *hw, uint32_t addr, uint32_t val);
int atl_update_msm_stats(struct atl_nic *nic);
void atl_fwd_release_rings(struct atl_nic *nic);


#endif
