/*
 * aQuantia Corporation Network Driver
 * Copyright (C) 2017 aQuantia Corporation. All rights reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 */

#include <linux/ethtool.h>

#include "atl_common.h"
#include "atl_ring.h"

static uint32_t atl_ethtool_get_link(struct net_device *ndev)
{
	return ethtool_op_get_link(ndev);
}

static void atl_link_to_kernel(unsigned int bits, unsigned long *kernel,
	bool legacy)
{
	struct atl_link_type *type;
	int i;

	atl_for_each_rate(i, type) {
		if (legacy && type->ethtool_idx > 31)
			continue;

		if (bits & BIT(i))
			__set_bit(type->ethtool_idx, kernel);
	}
}

#define atl_ethtool_get_common(base, modes, lstate, legacy)		\
do {									\
	struct atl_fc_state *fc = &(lstate)->fc;			\
	(base)->port = PORT_TP;						\
	(base)->duplex = DUPLEX_FULL;					\
	(base)->autoneg = AUTONEG_DISABLE;				\
	(base)->eth_tp_mdix = ETH_TP_MDI_INVALID;			\
	(base)->eth_tp_mdix_ctrl = ETH_TP_MDI_INVALID;			\
									\
	atl_add_link_supported(modes, Autoneg);				\
	atl_add_link_supported(modes, TP);				\
	atl_add_link_supported(modes, Pause);				\
	atl_add_link_supported(modes, Asym_Pause);			\
	atl_add_link_advertised(modes, TP);				\
	atl_add_link_lpadvertised(modes, Autoneg);			\
									\
	if (lstate->autoneg) {						\
		(base)->autoneg = AUTONEG_ENABLE;			\
		atl_add_link_advertised(modes, Autoneg);		\
	}								\
									\
	if (fc->req & atl_fc_rx)					\
		atl_add_link_advertised(modes, Pause);			\
									\
	if (!!(fc->req & atl_fc_rx) ^ !!(fc->req & atl_fc_tx))		\
		atl_add_link_advertised(modes, Asym_Pause);		\
									\
	if (fc->cur & atl_fc_rx)					\
		atl_add_link_lpadvertised(modes, Pause);		\
									\
	if (!!(fc->cur & atl_fc_rx) ^ !!(fc->cur & atl_fc_tx))		\
		atl_add_link_lpadvertised(modes, Asym_Pause);		\
									\
	atl_link_to_kernel((lstate)->supported,				\
		(unsigned long *)&(modes)->link_modes.supported,	\
		legacy);						\
	atl_link_to_kernel((lstate)->advertized,			\
		(unsigned long *)&(modes)->link_modes.advertising,	\
		legacy);						\
	atl_link_to_kernel((lstate)->lp_advertized,			\
		(unsigned long *)&(modes)->link_modes.lp_advertising,	\
		legacy);						\
} while (0)

#define atl_add_link_supported(ptr, mode) \
	atl_add_link_mode(ptr, SUPPORTED, supported, mode)

#define atl_add_link_advertised(ptr, mode) \
	atl_add_link_mode(ptr, ADVERTISED, advertising, mode)

#define atl_add_link_lpadvertised(ptr, mode) \
	atl_add_link_mode(ptr, ADVERTISED, lp_advertising, mode)

#ifndef ATL_HAVE_ETHTOOL_KSETTINGS

struct atl_ethtool_compat {
	struct {
		unsigned long supported;
		unsigned long advertising;
		unsigned long lp_advertising;
	} link_modes;
};

#define atl_add_link_mode(ptr, nameuc, namelc, mode)	\
	do { \
		(ptr)->link_modes.namelc |= nameuc ## _ ## mode; \
	} while (0)

static int atl_ethtool_get_settings(struct net_device *ndev,
				 struct ethtool_cmd *cmd)
{
	struct atl_ethtool_compat cmd_compat = {0};
	struct atl_nic *nic = netdev_priv(ndev);
	struct atl_link_state *lstate = &nic->hw.link_state;

	atl_ethtool_get_common(cmd, &cmd_compat, lstate, true);
	cmd->supported = cmd_compat.link_modes.supported;
	cmd->advertising = cmd_compat.link_modes.advertising;
	cmd->lp_advertising = cmd_compat.link_modes.lp_advertising;

	ethtool_cmd_speed_set(cmd, lstate->link ? lstate->link->speed : 0);

	return 0;
}

#else

#define atl_add_link_mode(ptr, nameuc, namelc, mode)	\
	ethtool_link_ksettings_add_link_mode(ptr, namelc, mode)

static int atl_ethtool_get_ksettings(struct net_device *ndev,
	struct ethtool_link_ksettings *cmd)
{
	struct atl_nic *nic = netdev_priv(ndev);
	struct atl_link_state *lstate = &nic->hw.link_state;

	ethtool_link_ksettings_zero_link_mode(cmd, supported);
	ethtool_link_ksettings_zero_link_mode(cmd, advertising);
	ethtool_link_ksettings_zero_link_mode(cmd, lp_advertising);

	atl_ethtool_get_common(&cmd->base, cmd, lstate, false);

	cmd->base.speed = lstate->link ? lstate->link->speed : 0;

	return 0;
}

#endif

#undef atl_add_link_supported
#undef atl_add_link_advertised
#undef atl_add_link_lpadvertised
#undef atl_add_link_mode

static unsigned int atl_kernel_to_link(const unsigned long int *bits,
	bool legacy)
{
	unsigned int ret = 0;
	int i;
	struct atl_link_type *type;

	atl_for_each_rate(i, type) {
		if (legacy && type->ethtool_idx > 31)
			continue;

		if (test_bit(type->ethtool_idx, bits))
			ret |= BIT(i);
	}

	return ret;
}

static int atl_set_fixed_speed(struct atl_hw *hw, unsigned int speed)
{
	struct atl_link_state *lstate = &hw->link_state;
	struct atl_link_type *type;
	int i;

	atl_for_each_rate(i, type)
		if (type->speed == speed) {
			if (!(lstate->supported & BIT(i)))
				return -EINVAL;

			lstate->advertized = BIT(i);
			break;
		}

	lstate->autoneg = false;
	hw->mcp.ops->set_link(hw);
	return 0;
}

#define atl_ethtool_set_common(base, lstate, advertise, tmp, legacy, speed) \
do {									\
	struct atl_fc_state *fc = &lstate->fc;				\
									\
	if ((base)->port != PORT_TP || (base)->duplex != DUPLEX_FULL)	\
		return -EINVAL;						\
									\
	if ((base)->autoneg != AUTONEG_ENABLE)				\
		return atl_set_fixed_speed(hw, speed);			\
									\
	atl_add_link_bit(tmp, Autoneg);					\
	atl_add_link_bit(tmp, TP);					\
	atl_add_link_bit(tmp, Pause);					\
	atl_add_link_bit(tmp, Asym_Pause);				\
	atl_link_to_kernel((lstate)->supported, tmp, legacy);		\
									\
	if (atl_complement_intersect(advertise, tmp)) {			\
		atl_nic_dbg("Unsupported advertising bits from ethtool\n"); \
		return -EINVAL;						\
	}								\
									\
	lstate->autoneg = true;						\
	(lstate)->advertized &= ATL_EEE_MASK;				\
	(lstate)->advertized |= atl_kernel_to_link(advertise, legacy);	\
									\
	fc->req = 0;							\
	if (atl_test_link_bit(advertise, Pause))			\
		fc->req	|= atl_fc_full;					\
									\
	if (atl_test_link_bit(advertise, Asym_Pause))			\
		fc->req ^= atl_fc_tx;					\
									\
} while (0)

#ifndef ATL_HAVE_ETHTOOL_KSETTINGS

#define atl_add_link_bit(ptr, name)		\
	(*(ptr) |= SUPPORTED_ ## name)

#define atl_test_link_bit(ptr, name)		\
	(*(ptr) & SUPPORTED_ ## name)

static inline bool atl_complement_intersect(const unsigned long *advertised,
	unsigned long *supported)
{
	return !!(*(uint32_t *)advertised & ~*(uint32_t *)supported);
}

static int atl_ethtool_set_settings(struct net_device *ndev,
	struct ethtool_cmd *cmd)
{
	struct atl_nic *nic = netdev_priv(ndev);
	struct atl_hw *hw = &nic->hw;
	struct atl_link_state *lstate = &hw->link_state;
	unsigned long tmp = 0;
	uint32_t speed = ethtool_cmd_speed(cmd);

	atl_ethtool_set_common(cmd, lstate,
		(unsigned long *)&cmd->advertising, &tmp, true, speed);
	hw->mcp.ops->set_link(hw);
	return 0;
}

#else

#define atl_add_link_bit(ptr, name)				\
	__set_bit(ETHTOOL_LINK_MODE_ ## name ## _BIT, ptr)

#define atl_test_link_bit(ptr, name)				\
	test_bit(ETHTOOL_LINK_MODE_ ## name ## _BIT, ptr)

static inline bool atl_complement_intersect(const unsigned long *advertised,
	unsigned long *supported)
{
	bitmap_complement(supported, supported,
		__ETHTOOL_LINK_MODE_MASK_NBITS);
	return bitmap_intersects(advertised, supported,
		__ETHTOOL_LINK_MODE_MASK_NBITS);
}

static int atl_ethtool_set_ksettings(struct net_device *ndev,
	const struct ethtool_link_ksettings *cmd)
{
	struct atl_nic *nic = netdev_priv(ndev);
	struct atl_hw *hw = &nic->hw;
	struct atl_link_state *lstate = &hw->link_state;
	const struct ethtool_link_settings *base = &cmd->base;
	__ETHTOOL_DECLARE_LINK_MODE_MASK(tmp);

	bitmap_zero(tmp, __ETHTOOL_LINK_MODE_MASK_NBITS);

	atl_ethtool_set_common(base, lstate, cmd->link_modes.advertising, tmp,
		false, cmd->base.speed);
	hw->mcp.ops->set_link(hw);
	return 0;
}

#endif

#undef atl_add_link_bit
#undef atl_test_link_bit

static uint32_t atl_rss_tbl_size(struct net_device *ndev)
{
	return ATL_RSS_TBL_SIZE;
}

static uint32_t atl_rss_key_size(struct net_device *ndev)
{
	return ATL_RSS_KEY_SIZE;
}

static int atl_rss_get_rxfh(struct net_device *ndev, uint32_t *tbl,
	uint8_t *key, uint8_t *htype)
{
	struct atl_hw *hw = &((struct atl_nic *)netdev_priv(ndev))->hw;
	int i;

	if (htype)
		*htype = ETH_RSS_HASH_TOP;

	if (key)
		memcpy(key, hw->rss_key, atl_rss_key_size(ndev));

	if (tbl)
		for (i = 0; i < atl_rss_tbl_size(ndev); i++)
			tbl[i] = hw->rss_tbl[i];

	return 0;
}

static int atl_rss_set_rxfh(struct net_device *ndev, const uint32_t *tbl,
	const uint8_t *key, const uint8_t htype)
{
	struct atl_nic *nic = netdev_priv(ndev);
	struct atl_hw *hw = &nic->hw;
	int i;
	uint32_t tbl_size = atl_rss_tbl_size(ndev);

	if (htype && htype != ETH_RSS_HASH_TOP)
		return -EINVAL;

	if (tbl) {
		for (i = 0; i < tbl_size; i++)
			if (tbl[i] >= nic->nvecs)
				return -EINVAL;

		for (i = 0; i < tbl_size; i++)
			hw->rss_tbl[i] = tbl[i];
	}

	if (key) {
		memcpy(hw->rss_key, key, atl_rss_key_size(ndev));
		atl_set_rss_key(hw);
	}

	if (tbl)
		atl_set_rss_tbl(hw);

	return 0;
}

static void atl_get_channels(struct net_device *ndev,
	struct ethtool_channels *chan)
{
	struct atl_nic *nic = netdev_priv(ndev);

	chan->max_combined = ATL_MAX_QUEUES;
	chan->combined_count = nic->nvecs;
	if (nic->flags & ATL_FL_MULTIPLE_VECTORS)
		chan->max_other = chan->other_count = ATL_NUM_NON_RING_IRQS;
}

static int atl_set_channels(struct net_device *ndev,
			    struct ethtool_channels *chan)
{
	struct atl_nic *nic = netdev_priv(ndev);
	unsigned int nvecs = chan->combined_count;

	if (!nvecs || chan->rx_count || chan->tx_count)
		return -EINVAL;

	if (nic->flags & ATL_FL_MULTIPLE_VECTORS &&
		chan->other_count != ATL_NUM_NON_RING_IRQS)
		return -EINVAL;

	if (!(nic->flags & ATL_FL_MULTIPLE_VECTORS) &&
		chan->other_count)
		return -EINVAL;

	if (nvecs > atl_max_queues)
		return -EINVAL;

	nic->requested_nvecs = nvecs;

	return atl_reconfigure(nic);
}

static void atl_get_pauseparam(struct net_device *ndev,
	struct ethtool_pauseparam *pause)
{
	struct atl_nic *nic = netdev_priv(ndev);
	struct atl_fc_state *fc = &nic->hw.link_state.fc;

	pause->autoneg = 1;
	pause->rx_pause = !!(fc->cur & atl_fc_rx);
	pause->tx_pause = !!(fc->cur & atl_fc_tx);
}

static int atl_set_pauseparam(struct net_device *ndev,
	struct ethtool_pauseparam *pause)
{
	struct atl_nic *nic = netdev_priv(ndev);
	struct atl_hw *hw = &nic->hw;
	struct atl_link_state *lstate = &hw->link_state;
	struct atl_fc_state *fc = &lstate->fc;

	if (atl_fw_major(hw) < 2)
		return -EOPNOTSUPP;

	if (pause->autoneg && !lstate->autoneg)
		return -EINVAL;

	fc->req = pause->autoneg ? atl_fc_full :
		(!!pause->rx_pause << atl_fc_rx_shift) |
		(!!pause->tx_pause << atl_fc_tx_shift);

	hw->mcp.ops->set_link(hw);
	return 0;
}

static int atl_get_eee(struct net_device *ndev, struct ethtool_eee *eee)
{
	struct atl_nic *nic = netdev_priv(ndev);
	struct atl_link_state *lstate = &nic->hw.link_state;

	eee->supported = eee->advertised = eee->lp_advertised = 0;

	/* Casting to unsigned long is safe, as atl_link_to_kernel()
	 * will only access low 32 bits when called with legacy == true
	 */
	atl_link_to_kernel(lstate->supported >> ATL_EEE_BIT_OFFT,
		(unsigned long *)&eee->supported, true);
	atl_link_to_kernel(lstate->advertized >> ATL_EEE_BIT_OFFT,
		(unsigned long *)&eee->advertised, true);
	atl_link_to_kernel(lstate->lp_advertized >> ATL_EEE_BIT_OFFT,
		(unsigned long *)&eee->lp_advertised, true);

	eee->eee_enabled = eee->tx_lpi_enabled = lstate->eee_enabled;
	eee->eee_active = lstate->eee;
	eee->tx_lpi_timer = 0;
	return 0;
}

static int atl_set_eee(struct net_device *ndev, struct ethtool_eee *eee)
{
	struct atl_nic *nic = netdev_priv(ndev);
	struct atl_hw *hw = &nic->hw;
	struct atl_link_state *lstate = &hw->link_state;
	uint32_t tmp = 0;

	if (atl_fw_major(hw) < 2)
		return -EOPNOTSUPP;

	if (eee->tx_lpi_timer != 0)
		return -EOPNOTSUPP;

	lstate->eee_enabled = eee->eee_enabled;

	if (lstate->eee_enabled) {
		atl_link_to_kernel(lstate->supported >> ATL_EEE_BIT_OFFT,
			(unsigned long *)&tmp, true);
		if (eee->advertised & ~tmp)
			return -EINVAL;

		tmp = atl_kernel_to_link((unsigned long *)&eee->advertised,
			true);
	}

	lstate->advertized &= ~ATL_EEE_MASK;
	if (lstate->eee_enabled)
		lstate->advertized |= tmp << ATL_EEE_BIT_OFFT;

	hw->mcp.ops->set_link(hw);
	return 0;
}

static void atl_get_drvinfo(struct net_device *ndev,
	struct ethtool_drvinfo *drvinfo)
{
	struct atl_nic *nic = netdev_priv(ndev);
	uint32_t fw_rev = nic->hw.mcp.fw_rev;

	strlcpy(drvinfo->driver, atl_driver_name, sizeof(drvinfo->driver));
	strlcpy(drvinfo->version, ATL_VERSION, sizeof(drvinfo->version));
	snprintf(drvinfo->fw_version, sizeof(drvinfo->fw_version),
		"%d.%d.%d", fw_rev >> 24, fw_rev >> 16 & 0xff,
		fw_rev & 0xffff);
	strlcpy(drvinfo->bus_info, pci_name(nic->hw.pdev),
		sizeof(drvinfo->bus_info));
}

static int atl_nway_reset(struct net_device *ndev)
{
	struct atl_nic *nic = netdev_priv(ndev);
	struct atl_hw *hw = &nic->hw;

	return hw->mcp.ops->restart_aneg(hw);
}

static void atl_get_ringparam(struct net_device *ndev,
	struct ethtool_ringparam *rp)
{
	struct atl_nic *nic = netdev_priv(ndev);

	rp->rx_mini_max_pending = rp->rx_mini_pending = 0;
	rp->rx_jumbo_max_pending = rp->rx_jumbo_pending = 0;

	rp->rx_max_pending = rp->tx_max_pending = ATL_MAX_RING_SIZE;

	rp->rx_pending = nic->requested_rx_size;
	rp->tx_pending = nic->requested_tx_size;
}

static int atl_set_ringparam(struct net_device *ndev,
	struct ethtool_ringparam *rp)
{
	struct atl_nic *nic = netdev_priv(ndev);

	if (rp->rx_mini_pending || rp->rx_jumbo_pending)
		return -EINVAL;

	if (rp->rx_pending < 8 || rp->tx_pending < 8)
		return -EINVAL;

	nic->requested_rx_size = rp->rx_pending & ~7;
	nic->requested_tx_size = rp->tx_pending & ~7;

	return atl_reconfigure(nic);
}

struct atl_stat_desc {
	char stat_name[ETH_GSTRING_LEN];
	int idx;
};

#define ATL_TX_STAT(_name, _field)				\
{								\
	.stat_name = #_name,					\
	.idx = offsetof(struct atl_tx_ring_stats, _field) /	\
		sizeof(uint64_t),				\
}

#define ATL_RX_STAT(_name, _field)				\
{								\
	.stat_name = #_name,					\
	.idx = offsetof(struct atl_rx_ring_stats, _field) /	\
		sizeof(uint64_t),				\
}

#define ATL_MSM_STAT(_name, _field)				\
{								\
	.stat_name = #_name,					\
	.idx = offsetof(struct atl_msm_stats, _field) /		\
		sizeof(uint32_t),				\
}

static const struct atl_stat_desc tx_stat_descs[] = {
	ATL_TX_STAT(tx_packets, packets),
	ATL_TX_STAT(tx_bytes, bytes),
	ATL_TX_STAT(tx_busy, tx_busy),
	ATL_TX_STAT(tx_queue_restart, tx_restart),
	ATL_TX_STAT(tx_dma_map_failed, dma_map_failed),
};

static const struct atl_stat_desc rx_stat_descs[] = {
	ATL_RX_STAT(rx_packets, packets),
	ATL_RX_STAT(rx_bytes, bytes),
	ATL_RX_STAT(rx_multicast_packets, multicast),
	ATL_RX_STAT(rx_lin_skb_overrun, linear_dropped),
	ATL_RX_STAT(rx_skb_alloc_failed, alloc_skb_failed),
	ATL_RX_STAT(rx_head_page_reused, reused_head_page),
	ATL_RX_STAT(rx_data_page_reused, reused_data_page),
	ATL_RX_STAT(rx_head_page_allocated, alloc_head_page),
	ATL_RX_STAT(rx_data_page_allocated, alloc_data_page),
	ATL_RX_STAT(rx_head_page_alloc_failed, alloc_head_page_failed),
	ATL_RX_STAT(rx_data_page_alloc_failed, alloc_data_page_failed),
	ATL_RX_STAT(rx_non_eop_descs, non_eop_descs),
	ATL_RX_STAT(rx_mac_err, mac_err),
	ATL_RX_STAT(rx_checksum_err, csum_err),
};

static const struct atl_stat_desc msm_stat_descs[] = {
	ATL_MSM_STAT(tx_pause, tx_pause),
	ATL_MSM_STAT(rx_pause, rx_pause),
};

#define ATL_PRIV_FLAG(_name, _bit)		\
	[ATL_PF(_bit)] = #_name

static const char atl_priv_flags[][ETH_GSTRING_LEN] = {
	ATL_PRIV_FLAG(PKTSystemLoopback, LPB_SYS_PB),
	ATL_PRIV_FLAG(DMASystemLoopback, LPB_SYS_DMA),
	/* ATL_PRIV_FLAG(DMANetworkLoopback, LPB_NET_DMA), */
};

static int atl_get_sset_count(struct net_device *ndev, int sset)
{
	struct atl_nic *nic = netdev_priv(ndev);

	switch (sset) {
	case ETH_SS_STATS:
		return ARRAY_SIZE(tx_stat_descs) * (nic->nvecs + 1) +
			ARRAY_SIZE(rx_stat_descs) * (nic->nvecs + 1) +
			ARRAY_SIZE(msm_stat_descs);

	case ETH_SS_PRIV_FLAGS:
		return ARRAY_SIZE(atl_priv_flags);

	default:
		return -EOPNOTSUPP;
	}
}

static void atl_copy_stats_strings(char **data, char *prefix,
	const struct atl_stat_desc *descs, int len)
{
	int i;

	for (i = 0; i < len; i++) {
		snprintf(*data, ETH_GSTRING_LEN, "%s%s",
			prefix, descs[i].stat_name);
		*data += ETH_GSTRING_LEN;
	}
}

static void atl_copy_stats_string_set(char **data, char *prefix)
{
	atl_copy_stats_strings(data, prefix, tx_stat_descs,
		ARRAY_SIZE(tx_stat_descs));
	atl_copy_stats_strings(data, prefix, rx_stat_descs,
		ARRAY_SIZE(rx_stat_descs));
}

static void atl_get_strings(struct net_device *ndev, uint32_t sset,
	uint8_t *data)
{
	struct atl_nic *nic = netdev_priv(ndev);
	int i;
	char prefix[16];
	char *p = data;

	switch (sset) {
	case ETH_SS_STATS:
		atl_copy_stats_string_set(&p, "");

		atl_copy_stats_strings(&p, "", msm_stat_descs,
			ARRAY_SIZE(msm_stat_descs));

		for (i = 0; i < nic->nvecs; i++) {
			snprintf(prefix, sizeof(prefix), "ring_%d_", i);
			atl_copy_stats_string_set(&p, prefix);
		}
		return;

	case ETH_SS_PRIV_FLAGS:
		memcpy(p, atl_priv_flags, sizeof(atl_priv_flags));
		return;
	}
}

#define atl_write_stats(stats, descs, data, type)	\
do {							\
	type *_stats = (type *)(stats);			\
	int i;						\
							\
	for (i = 0; i < ARRAY_SIZE(descs); i++)		\
		*(data)++ = _stats[descs[i].idx];	\
} while (0)


static void atl_get_ethtool_stats(struct net_device *ndev,
	struct ethtool_stats *stats, u64 *data)
{
	struct atl_nic *nic = netdev_priv(ndev);
	int i;

	atl_update_global_stats(nic);

	atl_write_stats(&nic->stats.tx, tx_stat_descs, data, uint64_t);
	atl_write_stats(&nic->stats.rx, rx_stat_descs, data, uint64_t);

	atl_write_stats(&nic->stats.msm, msm_stat_descs, data, uint32_t);

	for (i = 0; i < nic->nvecs; i++) {
		struct atl_queue_vec *qvec = &nic->qvecs[i];
		struct atl_ring_stats tmp;

		atl_get_ring_stats(&qvec->tx, &tmp);
		atl_write_stats(&tmp.tx, tx_stat_descs, data, uint64_t);
		atl_get_ring_stats(&qvec->rx, &tmp);
		atl_write_stats(&tmp.rx, rx_stat_descs, data, uint64_t);
	}
}


static uint32_t atl_get_priv_flags(struct net_device *ndev)
{
	struct atl_nic *nic = netdev_priv(ndev);

	return nic->priv_flags;
}

static int atl_set_priv_flags(struct net_device *ndev, uint32_t flags)
{
	struct atl_nic *nic = netdev_priv(ndev);

	if (flags & ~ATL_PF_LPB_MASK)
		return -EOPNOTSUPP;

	flags &= ATL_PF_LPB_MASK;
	if (hweight32(flags) > 1) {
		atl_nic_err("Can't enable more than one loopback simultaneously\n");
		return -EINVAL;
	}

	if (flags & ATL_PF_BIT(LPB_SYS_DMA) && !atl_rx_linear) {
		atl_nic_err("System DMA loopback suported only in rx_linear mode\n");
		return -EINVAL;
	}

	if (nic->priv_flags)
		atl_set_loopback(nic, ffs(nic->priv_flags) - 1, false);

	nic->priv_flags = flags;
	if (flags)
		atl_set_loopback(nic, ffs(flags) - 1, true);

	return 0;
}

static int atl_get_coalesce(struct net_device *ndev,
			    struct ethtool_coalesce *ec)
{
	struct atl_nic *nic = netdev_priv(ndev);

	memset(ec, 0, sizeof(*ec));
	ec->rx_coalesce_usecs = nic->rx_intr_delay;
	ec->tx_coalesce_usecs = nic->tx_intr_delay;

	return 0;
}

static int atl_set_coalesce(struct net_device *ndev,
			    struct ethtool_coalesce *ec)
{
	struct atl_nic *nic = netdev_priv(ndev);

	if (ec->use_adaptive_rx_coalesce || ec->use_adaptive_tx_coalesce ||
		ec->rx_max_coalesced_frames || ec->tx_max_coalesced_frames ||
		ec->rx_max_coalesced_frames_irq || ec->rx_coalesce_usecs_irq ||
		ec->tx_max_coalesced_frames_irq || ec->tx_coalesce_usecs_irq)
		return -EOPNOTSUPP;

	if (ec->rx_coalesce_usecs < atl_min_intr_delay ||
		ec->tx_coalesce_usecs < atl_min_intr_delay) {
		atl_nic_err("Interrupt coalescing delays less than min_intr_delay (%d uS) not supported\n",
			atl_min_intr_delay);
		return -EINVAL;
	}

	nic->rx_intr_delay = ec->rx_coalesce_usecs;
	nic->tx_intr_delay = ec->tx_coalesce_usecs;

	atl_set_intr_mod(nic);

	return 0;
}

struct atl_rxf_flt_desc {
	int base;
	int count;
	uint32_t rxq_bit;
	int rxq_shift;
	size_t cmd_offt;
	size_t count_offt;
	int (*get_rxf)(const struct atl_rxf_flt_desc *desc,
		struct atl_nic *nic, struct ethtool_rx_flow_spec *fsp);
	int (*set_rxf)(const struct atl_rxf_flt_desc *desc,
		struct atl_nic *nic, struct ethtool_rx_flow_spec *fsp);
	void (*update_rxf)(struct atl_nic *nic, int idx);
};

static inline int atl_rxf_idx(const struct atl_rxf_flt_desc *desc,
	struct ethtool_rx_flow_spec *fsp)
{
	return fsp->location - desc->base;
}

static inline uint64_t atl_ring_cookie(const struct atl_rxf_flt_desc *desc,
	uint32_t cmd)
{
	return (cmd & desc->rxq_bit) ?
		(cmd >> desc->rxq_shift) & ATL_RXF_RXQ_MSK :
		RX_CLS_FLOW_DISC;
}

static int atl_rxf_get_vlan(const struct atl_rxf_flt_desc *desc,
	struct atl_nic *nic, struct ethtool_rx_flow_spec *fsp)
{
	struct atl_rxf_vlan *vlan = &nic->rxf_vlan;
	int idx = atl_rxf_idx(desc, fsp);
	uint32_t cmd = vlan->cmd[idx];

	if (!(cmd & ATL_RXF_EN))
		return -EINVAL;

	fsp->flow_type = ETHER_FLOW | FLOW_EXT;
	fsp->h_ext.vlan_tci = htons(cmd & ATL_VLAN_VID_MASK);
	fsp->m_ext.vlan_tci = htons(BIT(12) - 1);
	fsp->ring_cookie = atl_ring_cookie(desc, cmd);

	return 0;
}

static int atl_rxf_get_etype(const struct atl_rxf_flt_desc *desc,
	struct atl_nic *nic, struct ethtool_rx_flow_spec *fsp)
{
	struct atl_rxf_etype *etype = &nic->rxf_etype;
	int idx = atl_rxf_idx(desc, fsp);
	uint32_t cmd = etype->cmd[idx];

	if (!(cmd & ATL_RXF_EN))
		return -EINVAL;

	fsp->flow_type = ETHER_FLOW;
	fsp->m_u.ether_spec.h_proto = 0xffff;
	fsp->h_u.ether_spec.h_proto = htons(cmd & ATL_ETYPE_VAL_MASK);
	fsp->ring_cookie = atl_ring_cookie(desc, cmd);

	return 0;
}

static inline void atl_ntuple_swap_v6(__be32 dst[4], __be32 src[4])
{
	int i;

	for (i = 0; i < 4; i++)
		dst[i] = src[3 - i];
}

static int atl_rxf_get_ntuple(const struct atl_rxf_flt_desc *desc,
	struct atl_nic *nic, struct ethtool_rx_flow_spec *fsp)
{
	struct atl_rxf_ntuple *ntuples = &nic->rxf_ntuple;
	uint32_t idx = atl_rxf_idx(desc, fsp);
	uint32_t cmd = ntuples->cmd[idx];

	if (!(cmd & ATL_RXF_EN))
		return -EINVAL;

	if (cmd & ATL_NTC_PROTO) {
		switch (cmd & ATL_NTC_L4_MASK) {
		case ATL_NTC_L4_TCP:
			fsp->flow_type = cmd & ATL_NTC_V6 ?
				TCP_V6_FLOW : TCP_V4_FLOW;
			break;

		case ATL_NTC_L4_UDP:
			fsp->flow_type = cmd & ATL_NTC_V6 ?
				UDP_V6_FLOW : UDP_V4_FLOW;
			break;

		case ATL_NTC_L4_SCTP:
			fsp->flow_type = cmd & ATL_NTC_V6 ?
				SCTP_V6_FLOW : SCTP_V4_FLOW;
			break;

		default:
			return -EINVAL;
		}
	} else {
#ifdef ATL_HAVE_IPV6_NTUPLE
		if (cmd & ATL_NTC_V6) {
			fsp->flow_type = IPV6_USER_FLOW;
		} else
#endif
		{
			fsp->flow_type = IPV4_USER_FLOW;
			fsp->h_u.usr_ip4_spec.ip_ver = ETH_RX_NFC_IP4;
		}
	}

#ifdef ATL_HAVE_IPV6_NTUPLE
	if (cmd & ATL_NTC_V6) {
		struct ethtool_tcpip6_spec *rule = &fsp->h_u.tcp_ip6_spec;
		struct ethtool_tcpip6_spec *mask = &fsp->m_u.tcp_ip6_spec;

		if (cmd & ATL_NTC_SA) {
			atl_ntuple_swap_v6(rule->ip6src,
				ntuples->src_ip6[idx / 4]);
			memset(mask->ip6src, 0xff, sizeof(mask->ip6src));
		}

		if (cmd & ATL_NTC_DA) {
			atl_ntuple_swap_v6(rule->ip6dst,
				ntuples->dst_ip6[idx / 4]);
			memset(mask->ip6dst, 0xff, sizeof(mask->ip6dst));
		}

		if (cmd & ATL_NTC_SP) {
			rule->psrc = ntuples->src_port[idx];
			mask->psrc = -1;
		}

		if (cmd & ATL_NTC_DP) {
			rule->pdst = ntuples->dst_port[idx];
			mask->pdst = -1;
		}
	} else
#endif
	{
		struct ethtool_tcpip4_spec *rule = &fsp->h_u.tcp_ip4_spec;
		struct ethtool_tcpip4_spec *mask = &fsp->m_u.tcp_ip4_spec;

		if (cmd & ATL_NTC_SA) {
			rule->ip4src = ntuples->src_ip4[idx];
			mask->ip4src = -1;
		}

		if (cmd & ATL_NTC_DA) {
			rule->ip4dst = ntuples->dst_ip4[idx];
			mask->ip4dst = -1;
		}

		if (cmd & ATL_NTC_SP) {
			rule->psrc = ntuples->src_port[idx];
			mask->psrc = -1;
		}

		if (cmd & ATL_NTC_DP) {
			rule->pdst = ntuples->dst_port[idx];
			mask->pdst = -1;
		}
	}

	fsp->ring_cookie = atl_ring_cookie(desc, cmd);

	return 0;
}

static int atl_get_rxf_locs(struct atl_nic *nic, struct ethtool_rxnfc *rxnfc,
	uint32_t *rule_locs)
{
	struct atl_rxf_ntuple *ntuple = &nic->rxf_ntuple;
	struct atl_rxf_vlan *vlan = &nic->rxf_vlan;
	struct atl_rxf_etype *etype = &nic->rxf_etype;
	int count = ntuple->count + vlan->count + etype->count;
	int i;

	if (rxnfc->rule_cnt < count)
		return -EMSGSIZE;

	for (i = 0; i < ATL_RXF_VLAN_MAX; i++)
		if (vlan->cmd[i] & ATL_RXF_EN)
			*rule_locs++ = i + ATL_RXF_VLAN_BASE;

	for (i = 0; i < ATL_RXF_ETYPE_MAX; i++)
		if (etype->cmd[i] & ATL_RXF_EN)
			*rule_locs++ = i + ATL_RXF_ETYPE_BASE;

	for (i = 0; i < ATL_RXF_NTUPLE_MAX; i++)
		if (ntuple->cmd[i] & ATL_RXF_EN)
			*rule_locs++ = i + ATL_RXF_NTUPLE_BASE;

	rxnfc->rule_cnt = count;
	return 0;
}

static int atl_check_mask(uint8_t *mask, int len, uint32_t *cmd, uint32_t flag)
{
	uint8_t first = mask[0];
	uint8_t *p;

	if (first != 0 && first != 0xff)
		return -EINVAL;

	for (p = mask; p < &mask[len]; p++)
		if (*p != first)
			return -EINVAL;

	if (first == 0xff) {
		if (cmd)
			*cmd |= flag;
		else
			return -EINVAL;
	}

	return 0;
}

static int atl_rxf_set_ring(const struct atl_rxf_flt_desc *desc,
	struct atl_nic *nic, struct ethtool_rx_flow_spec *fsp, uint32_t *cmd)
{
	uint64_t ring_cookie = fsp->ring_cookie;
	uint32_t ring;

	if (ring_cookie == RX_CLS_FLOW_DISC)
		return 0;

	ring = ethtool_get_flow_spec_ring(ring_cookie);
	if (ring >= nic->nvecs && !test_bit(ring, &nic->fwd.ring_map[0])) {
		atl_nic_err("Invalid Rx filter queue %d\n", ring);
		return -EINVAL;
	}

	if (ethtool_get_flow_spec_ring_vf(ring_cookie)) {
		atl_nic_err("Rx filter queue VF must be zero");
		return -EINVAL;
	}

	*cmd |= ring << desc->rxq_shift | desc->rxq_bit;

	return 0;
}

static int atl_check_vlan_etype_common(struct ethtool_rx_flow_spec *fsp)
{
	int ret;

	ret = atl_check_mask((uint8_t *)&fsp->m_u.ether_spec.h_source,
		sizeof(fsp->m_u.ether_spec.h_source), NULL, 0);
	if (ret)
		return ret;

	ret = atl_check_mask((uint8_t *)&fsp->m_ext.data,
		sizeof(fsp->m_ext.data), NULL, 0);
	if (ret)
		return ret;

	ret = atl_check_mask((uint8_t *)&fsp->m_ext.vlan_etype,
		sizeof(fsp->m_ext.vlan_etype), NULL, 0);

	return ret;
}

static int atl_rxf_set_vlan(const struct atl_rxf_flt_desc *desc,
	struct atl_nic *nic, struct ethtool_rx_flow_spec *fsp)
{
	struct atl_rxf_vlan *vlan = &nic->rxf_vlan;
	int idx = atl_rxf_idx(desc, fsp);
	int ret;
	uint32_t cmd = ATL_RXF_EN;
	uint16_t vid, mask;

	if (fsp->flow_type != (ETHER_FLOW | FLOW_EXT)) {
		atl_nic_err("Only ether flow-type supported for VLAN filters\n");
		return -EINVAL;
	}

	ret = atl_check_vlan_etype_common(fsp);
	if (ret)
		return ret;

	if (fsp->m_u.ether_spec.h_proto)
		return -EINVAL;

	vid = ntohs(fsp->h_ext.vlan_tci);
	mask = ntohs(fsp->m_ext.vlan_tci);

	if (mask & 0xf000 && vid & 0xf000 & mask)
		return -EINVAL;

	if ((mask & 0xfff) != 0xfff)
		return -EINVAL;

	cmd |= vid & 0xfff;

	ret = atl_rxf_set_ring(desc, nic, fsp, &cmd);
	if (ret)
		return ret;

	vlan->cmd[idx] = cmd;

	return 0;
}

static int atl_rxf_set_etype(const struct atl_rxf_flt_desc *desc,
	struct atl_nic *nic, struct ethtool_rx_flow_spec *fsp)
{
	struct atl_rxf_etype *etype = &nic->rxf_etype;
	int idx = atl_rxf_idx(desc, fsp);
	int ret;
	uint32_t cmd = ATL_RXF_EN;

	if (fsp->flow_type != (ETHER_FLOW)) {
		atl_nic_err("Only ether flow-type supported for ethertype filters\n");
		return -EINVAL;
	}

	ret = atl_check_vlan_etype_common(fsp);
	if (ret)
		return ret;

	if (fsp->m_ext.vlan_tci)
		return -EINVAL;

	if (fsp->m_u.ether_spec.h_proto != 0xffff)
		return -EINVAL;

	cmd |= ntohs(fsp->h_u.ether_spec.h_proto);

	ret = atl_rxf_set_ring(desc, nic, fsp, &cmd);
	if (ret)
		return ret;

	etype->cmd[idx] = cmd;

	return 0;
}

static int atl_rxf_set_ntuple(const struct atl_rxf_flt_desc *desc,
	struct atl_nic *nic, struct ethtool_rx_flow_spec *fsp)
{
	struct atl_rxf_ntuple *ntuple = &nic->rxf_ntuple;
	int idx = atl_rxf_idx(desc, fsp);
	uint32_t cmd = ATL_NTC_EN;
	int ret;
	__be16 sport, dport;

	ret = atl_rxf_set_ring(desc, nic, fsp, &cmd);
	if (ret)
		return ret;

	switch (fsp->flow_type) {
#ifdef ATL_HAVE_IPV6_NTUPLE
	case TCP_V6_FLOW:
	case UDP_V6_FLOW:
	case SCTP_V6_FLOW:
		if (fsp->m_u.tcp_ip6_spec.tclass != 0) {
			atl_nic_err("Unsupported match field\n");
			return -EINVAL;
		}
		cmd |= ATL_NTC_PROTO | ATL_NTC_V6;
		break;

	case IPV6_USER_FLOW:
		if (fsp->m_u.usr_ip6_spec.l4_4_bytes != 0 ||
			fsp->m_u.usr_ip6_spec.tclass != 0 ||
			fsp->m_u.usr_ip6_spec.l4_proto != 0) {
			atl_nic_err("Unsupported match field\n");
			return -EINVAL;
		}
		cmd |= ATL_NTC_V6;
		break;
#endif

	case TCP_V4_FLOW:
	case UDP_V4_FLOW:
	case SCTP_V4_FLOW:
		if (fsp->m_u.tcp_ip4_spec.tos != 0) {
			atl_nic_err("Unsupported match field\n");
			return -EINVAL;
		}
		cmd |= ATL_NTC_PROTO;
		break;

	case IPV4_USER_FLOW:
		if (fsp->m_u.usr_ip4_spec.l4_4_bytes != 0 ||
			fsp->m_u.usr_ip4_spec.tos != 0 ||
			fsp->h_u.usr_ip4_spec.ip_ver != ETH_RX_NFC_IP4 ||
			fsp->h_u.usr_ip4_spec.proto != 0) {
			atl_nic_err("Unsupported match field\n");
			return -EINVAL;
		}
		break;

	default:
		return -EINVAL;
	}

	switch (fsp->flow_type) {
	case TCP_V6_FLOW:
	case TCP_V4_FLOW:
		cmd |= ATL_NTC_L4_TCP;
		break;

	case UDP_V6_FLOW:
	case UDP_V4_FLOW:
		cmd |= ATL_NTC_L4_UDP;
		break;

	case SCTP_V6_FLOW:
	case SCTP_V4_FLOW:
		cmd |= ATL_NTC_L4_SCTP;
		break;
	}

#ifdef ATL_HAVE_IPV6_NTUPLE
	if (cmd & ATL_NTC_V6) {
		int i;

		if (idx & 3) {
			atl_nic_err("IPv6 filters only supported in locations 8 and 12\n");
			return -EINVAL;
		}

		for (i = idx + 1; i < idx + 4; i++)
			if (ntuple->cmd[i] & ATL_NTC_EN) {
				atl_nic_err("IPv6 filter %d overlaps an IPv4 filter %d\n",
					    idx, i);
				return -EINVAL;
			}

		ret = atl_check_mask((uint8_t *)fsp->m_u.tcp_ip6_spec.ip6src,
			sizeof(fsp->m_u.tcp_ip6_spec.ip6src), &cmd, ATL_NTC_SA);
		if (ret)
			return ret;

		ret = atl_check_mask((uint8_t *)fsp->m_u.tcp_ip6_spec.ip6dst,
			sizeof(fsp->m_u.tcp_ip6_spec.ip6dst), &cmd, ATL_NTC_DA);
		if (ret)
			return ret;

		sport = fsp->h_u.tcp_ip6_spec.psrc;
		ret = atl_check_mask((uint8_t *)&fsp->m_u.tcp_ip6_spec.psrc,
			sizeof(fsp->m_u.tcp_ip6_spec.psrc), &cmd, ATL_NTC_SP);
		if (ret)
			return ret;

		dport = fsp->h_u.tcp_ip6_spec.pdst;
		ret = atl_check_mask((uint8_t *)&fsp->m_u.tcp_ip6_spec.pdst,
			sizeof(fsp->m_u.tcp_ip6_spec.pdst), &cmd, ATL_NTC_DP);
		if (ret)
			return ret;

		if (cmd & ATL_NTC_SA)
			atl_ntuple_swap_v6(ntuple->src_ip6[idx / 4],
				fsp->h_u.tcp_ip6_spec.ip6src);

		if (cmd & ATL_NTC_DA)
			atl_ntuple_swap_v6(ntuple->dst_ip6[idx / 4],
				fsp->h_u.tcp_ip6_spec.ip6dst);

	} else
#endif
	{

		ret = atl_check_mask((uint8_t *)&fsp->m_u.tcp_ip4_spec.ip4src,
			sizeof(fsp->m_u.tcp_ip4_spec.ip4src), &cmd, ATL_NTC_SA);
		if (ret)
			return ret;

		ret = atl_check_mask((uint8_t *)&fsp->m_u.tcp_ip4_spec.ip4dst,
			sizeof(fsp->m_u.tcp_ip4_spec.ip4dst), &cmd, ATL_NTC_DA);
		if (ret)
			return ret;

		sport = fsp->h_u.tcp_ip4_spec.psrc;
		ret = atl_check_mask((uint8_t *)&fsp->m_u.tcp_ip4_spec.psrc,
			sizeof(fsp->m_u.tcp_ip4_spec.psrc), &cmd, ATL_NTC_SP);
		if (ret)
			return ret;

		dport = fsp->h_u.tcp_ip4_spec.pdst;
		ret = atl_check_mask((uint8_t *)&fsp->m_u.tcp_ip4_spec.pdst,
			sizeof(fsp->m_u.tcp_ip4_spec.psrc), &cmd, ATL_NTC_DP);
		if (ret)
			return ret;

		if (cmd & ATL_NTC_SA)
			ntuple->src_ip4[idx] = fsp->h_u.tcp_ip4_spec.ip4src;

		if (cmd & ATL_NTC_DA)
			ntuple->dst_ip4[idx] = fsp->h_u.tcp_ip4_spec.ip4dst;
	}

	if (cmd & ATL_NTC_SP)
		ntuple->src_port[idx] = sport;

	if (cmd & ATL_NTC_DP)
		ntuple->dst_port[idx] = dport;

	ntuple->cmd[idx] = cmd;

	return 0;
}

static void atl_rxf_update_vlan(struct atl_nic *nic, int idx)
{
	atl_write(&nic->hw, ATL_RX_VLAN_FLT(idx), nic->rxf_vlan.cmd[idx]);
}

static void atl_rxf_update_etype(struct atl_nic *nic, int idx)
{
	atl_write(&nic->hw, ATL_RX_ETYPE_FLT(idx), nic->rxf_etype.cmd[idx]);
}

static const struct atl_rxf_flt_desc atl_rxf_descs[] = {
	{
		.base = ATL_RXF_VLAN_BASE,
		.count = ATL_RXF_VLAN_MAX,
		.rxq_bit = ATL_VLAN_RXQ,
		.rxq_shift = ATL_VLAN_RXQ_SHIFT,
		.cmd_offt = offsetof(struct atl_nic, rxf_vlan.cmd),
		.count_offt = offsetof(struct atl_nic, rxf_vlan.count),
		.get_rxf = atl_rxf_get_vlan,
		.set_rxf = atl_rxf_set_vlan,
		.update_rxf = atl_rxf_update_vlan,
	},
	{
		.base = ATL_RXF_ETYPE_BASE,
		.count = ATL_RXF_ETYPE_MAX,
		.rxq_bit = ATL_ETYPE_RXQ,
		.rxq_shift = ATL_ETYPE_RXQ_SHIFT,
		.cmd_offt = offsetof(struct atl_nic, rxf_etype.cmd),
		.count_offt = offsetof(struct atl_nic, rxf_etype.count),
		.get_rxf = atl_rxf_get_etype,
		.set_rxf = atl_rxf_set_etype,
		.update_rxf = atl_rxf_update_etype,
	},
	{
		.base = ATL_RXF_NTUPLE_BASE,
		.count = ATL_RXF_NTUPLE_MAX,
		.rxq_bit = ATL_NTC_RXQ,
		.rxq_shift = ATL_NTC_RXQ_SHIFT,
		.cmd_offt = offsetof(struct atl_nic, rxf_ntuple.cmd),
		.count_offt = offsetof(struct atl_nic, rxf_ntuple.count),
		.get_rxf = atl_rxf_get_ntuple,
		.set_rxf = atl_rxf_set_ntuple,
		.update_rxf = atl_update_ntuple_flt,
	},
};

static uint32_t *atl_rxf_cmd(const struct atl_rxf_flt_desc *desc,
	struct atl_nic *nic)
{
	return (uint32_t *)((char *)nic + desc->cmd_offt);
}

static int *atl_rxf_count(const struct atl_rxf_flt_desc *desc, struct atl_nic *nic)
{
	return (int *)((char *)nic + desc->count_offt);
}

static const struct atl_rxf_flt_desc *atl_rxf_desc(struct ethtool_rx_flow_spec *fsp)
{
	uint32_t loc = fsp->location;
	const struct atl_rxf_flt_desc *desc = atl_rxf_descs;

	while (desc < atl_rxf_descs + ARRAY_SIZE(atl_rxf_descs)) {
		if (loc < desc->base)
			return NULL;

		if (loc < desc->base + desc->count)
			return desc;

		desc++;
	}

	return NULL;
}

static int atl_set_rxf(struct atl_nic *nic,
	struct ethtool_rx_flow_spec *fsp, bool delete)
{
	const struct atl_rxf_flt_desc *desc;
	uint32_t *cmd;
	int *count, ret, idx;
	bool present;

	desc = atl_rxf_desc(fsp);
	if (!desc)
		return -EINVAL;

	idx = atl_rxf_idx(desc, fsp);
	cmd = &atl_rxf_cmd(desc, nic)[idx];
	count = atl_rxf_count(desc, nic);
	present = !!(*cmd & ATL_RXF_EN);

	if (!delete) {
		ret = desc->set_rxf(desc, nic, fsp);
		if (ret)
			return ret;

		if (!present)
			(*count)++;
	} else if (!present)
		/* Attempting to delete non-existent filter */
		return -EINVAL;
	else {
		*cmd = 0;
		(*count)--;
	}

	desc->update_rxf(nic, idx);

	return 0;
}

static int atl_get_rxnfc(struct net_device *ndev, struct ethtool_rxnfc *rxnfc,
	uint32_t *rule_locs)
{
	struct atl_nic *nic = netdev_priv(ndev);
	struct ethtool_rx_flow_spec *fsp = &rxnfc->fs;
	int ret = -ENOTSUPP;
	const struct atl_rxf_flt_desc *desc;

	switch (rxnfc->cmd) {
	case ETHTOOL_GRXRINGS:
		rxnfc->data = nic->nvecs;
		return 0;

	case ETHTOOL_GRXCLSRLCNT:
		rxnfc->rule_cnt = nic->rxf_ntuple.count + nic->rxf_vlan.count +
			nic->rxf_etype.count;
		return 0;

	case ETHTOOL_GRXCLSRULE:
		desc = atl_rxf_desc(fsp);
		if (!desc)
			return -EINVAL;

		memset(&fsp->h_u, 0, sizeof(fsp->h_u));
		memset(&fsp->m_u, 0, sizeof(fsp->m_u));
		memset(&fsp->h_ext, 0, sizeof(fsp->h_ext));
		memset(&fsp->m_ext, 0, sizeof(fsp->m_ext));

		ret = desc->get_rxf(desc, nic, fsp);
		break;

	case ETHTOOL_GRXCLSRLALL:
		ret = atl_get_rxf_locs(nic, rxnfc, rule_locs);
		break;

	default:
		break;
	}

	return ret;
}

static int atl_set_rxnfc(struct net_device *ndev, struct ethtool_rxnfc *rxnfc)
{
	struct atl_nic *nic = netdev_priv(ndev);
	struct ethtool_rx_flow_spec *fsp = &rxnfc->fs;

	switch (rxnfc->cmd) {
	case ETHTOOL_SRXCLSRLINS:
		return atl_set_rxf(nic, fsp, false);

	case ETHTOOL_SRXCLSRLDEL:
		return atl_set_rxf(nic, fsp, true);
	}

	return -ENOTSUPP;
}

const struct ethtool_ops atl_ethtool_ops = {
	.get_link = atl_ethtool_get_link,
#ifndef ATL_HAVE_ETHTOOL_KSETTINGS
	.get_settings = atl_ethtool_get_settings,
	.set_settings = atl_ethtool_set_settings,
#else
	.get_link_ksettings = atl_ethtool_get_ksettings,
	.set_link_ksettings = atl_ethtool_set_ksettings,
#endif

	.get_rxfh_indir_size = atl_rss_tbl_size,
	.get_rxfh_key_size = atl_rss_key_size,
	.get_rxfh = atl_rss_get_rxfh,
	.set_rxfh = atl_rss_set_rxfh,
	.get_channels = atl_get_channels,
	.set_channels = atl_set_channels,
	.get_rxnfc = atl_get_rxnfc,
	.set_rxnfc = atl_set_rxnfc,
	.get_pauseparam = atl_get_pauseparam,
	.set_pauseparam = atl_set_pauseparam,
	.get_eee = atl_get_eee,
	.set_eee = atl_set_eee,
	.get_drvinfo = atl_get_drvinfo,
	.nway_reset = atl_nway_reset,
	.get_ringparam = atl_get_ringparam,
	.set_ringparam = atl_set_ringparam,
	.get_sset_count = atl_get_sset_count,
	.get_strings = atl_get_strings,
	.get_ethtool_stats = atl_get_ethtool_stats,
	.get_priv_flags = atl_get_priv_flags,
	.set_priv_flags = atl_set_priv_flags,
	.get_coalesce = atl_get_coalesce,
	.set_coalesce = atl_set_coalesce,
};
