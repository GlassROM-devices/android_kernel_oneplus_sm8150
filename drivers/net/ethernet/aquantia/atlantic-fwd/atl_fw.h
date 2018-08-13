/*
 * aQuantia Corporation Network Driver
 * Copyright (C) 2017 aQuantia Corporation. All rights reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 */

#ifndef _ATL_FW_H_
#define _ATL_FW_H_

struct atl_hw;

struct atl_link_type {
	unsigned speed;
	unsigned ethtool_idx;
	uint32_t fw_bits[2];
	const char *name;
};

extern struct atl_link_type atl_link_types[];
extern const int atl_num_rates;

#define atl_for_each_rate(idx, type)		\
	for (idx = 0, type = atl_link_types;	\
	     idx < atl_num_rates;		\
	     idx++, type++)

enum atl_fw2_opts {
	atl_fw2_pause_shift = 3,
	atl_fw2_asym_pause_shift = 4,
	atl_fw2_pause = BIT(atl_fw2_pause_shift),
	atl_fw2_asym_pause = BIT(atl_fw2_asym_pause_shift),
	atl_fw2_pause_mask = atl_fw2_pause | atl_fw2_asym_pause,
};

enum atl_fc_mode {
	atl_fc_none = 0,
	atl_fc_rx_shift = 0,
	atl_fc_tx_shift = 1,
	atl_fc_rx = BIT(atl_fc_rx_shift),
	atl_fc_tx = BIT(atl_fc_tx_shift),
	atl_fc_full = atl_fc_rx | atl_fc_tx,
};

struct atl_fc_state {
	enum atl_fc_mode req;
	enum atl_fc_mode prev_req;
	enum atl_fc_mode cur;
};

#define ATL_EEE_BIT_OFFT 16
#define ATL_EEE_MASK ~(BIT(ATL_EEE_BIT_OFFT) - 1)

struct atl_link_state{
	/* The following three bitmaps use alt_link_types[] indices
	 * as link bit positions. Conversion to/from ethtool bits is
	 * done in atl_ethtool.c. */
	unsigned supported;
	unsigned advertized;
	unsigned lp_advertized;
	unsigned prev_advertized;
	bool force_off;
	bool autoneg;
	bool eee;
	bool eee_enabled;
	struct atl_link_type *link;
	struct atl_fc_state fc;
};

struct atl_fw_ops {
	void (*set_link)(struct atl_hw *hw);
	struct atl_link_type *(*check_link)(struct atl_hw *hw);
	int (*wait_fw_init)(struct atl_hw *hw);
	int (*get_link_caps)(struct atl_hw *hw);
	int (*restart_aneg)(struct atl_hw *hw);
	void (*set_default_link)(struct atl_hw *hw);
	unsigned efuse_shadow_addr_reg;
};

#define ATL_FW_STAT_LINK_CAPS	0x84

#endif
