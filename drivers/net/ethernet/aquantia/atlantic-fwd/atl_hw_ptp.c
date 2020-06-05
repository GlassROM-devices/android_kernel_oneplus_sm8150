// SPDX-License-Identifier: GPL-2.0-only
/* Atlantic Network Driver
 *
 * Copyright (C) 2017 aQuantia Corporation
 * Copyright (C) 2019-2020 Marvell International Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "atl_hw_ptp.h"

#define FRAC_PER_NS 0x100000000LL

#define ATL_HW_MAC_COUNTER_HZ   312500000ll
#define ATL_HW_PHY_COUNTER_HZ   160000000ll

/* register address for bitfield PTP Digital Clock Read Enable */
#define HW_ATL_PCS_PTP_CLOCK_READ_ENABLE_ADR 0x00004628
/* lower bit position of bitfield PTP Digital Clock Read Enable */
#define HW_ATL_PCS_PTP_CLOCK_READ_ENABLE_SHIFT 4
/* width of bitfield PTP Digital Clock Read Enable */
#define HW_ATL_PCS_PTP_CLOCK_READ_ENABLE_WIDTH 1

/* register address for ptp counter reading */
#define HW_ATL_PCS_PTP_TS_VAL_ADDR(index) (0x00004900 + (index) * 0x4)

static void hw_atl_pcs_ptp_clock_read_enable(struct atl_hw *hw,
				      u32 ptp_clock_read_enable)
{
	atl_write_bits(hw, HW_ATL_PCS_PTP_CLOCK_READ_ENABLE_ADR,
			    HW_ATL_PCS_PTP_CLOCK_READ_ENABLE_SHIFT,
			    HW_ATL_PCS_PTP_CLOCK_READ_ENABLE_WIDTH,
			    ptp_clock_read_enable);
}

static u32 hw_atl_pcs_ptp_clock_get(struct atl_hw *hw, u32 index)
{
	return atl_read(hw, HW_ATL_PCS_PTP_TS_VAL_ADDR(index));
}

#define get_ptp_ts_val_u64(self, indx) \
	((u64)(hw_atl_pcs_ptp_clock_get(self, indx) & 0xffff))

void hw_atl_get_ptp_ts(struct atl_hw *hw, u64 *stamp)
{
	u64 ns;

	hw_atl_pcs_ptp_clock_read_enable(hw, 1);
	hw_atl_pcs_ptp_clock_read_enable(hw, 0);
	ns = (get_ptp_ts_val_u64(hw, 0) +
	      (get_ptp_ts_val_u64(hw, 1) << 16)) * NSEC_PER_SEC +
	     (get_ptp_ts_val_u64(hw, 3) +
	      (get_ptp_ts_val_u64(hw, 4) << 16));

	*stamp = ns + hw->ptp_clk_offset;
}

static void hw_atl_adj_params_get(u64 freq, s64 adj, u32 *ns, u32 *fns)
{
	/* For accuracy, the digit is extended */
	s64 base_ns = ((adj + NSEC_PER_SEC) * NSEC_PER_SEC);
	u64 nsi_frac = 0;
	u64 nsi;

	base_ns = div64_s64(base_ns, freq);
	nsi = div64_u64(base_ns, NSEC_PER_SEC);

	if (base_ns != nsi * NSEC_PER_SEC) {
		s64 divisor = div64_s64((s64)NSEC_PER_SEC * NSEC_PER_SEC,
					base_ns - nsi * NSEC_PER_SEC);
		nsi_frac = div64_s64(FRAC_PER_NS * NSEC_PER_SEC, divisor);
	}

	*ns = (u32)nsi;
	*fns = (u32)nsi_frac;
}

static void
hw_atl_mac_adj_param_calc(struct ptp_adj_freq *ptp_adj_freq, u64 phyfreq,
			  u64 macfreq)
{
	s64 adj_fns_val;
	s64 fns_in_sec_phy = phyfreq * (ptp_adj_freq->fns_phy +
					FRAC_PER_NS * ptp_adj_freq->ns_phy);
	s64 fns_in_sec_mac = macfreq * (ptp_adj_freq->fns_mac +
					FRAC_PER_NS * ptp_adj_freq->ns_mac);
	s64 fault_in_sec_phy = FRAC_PER_NS * NSEC_PER_SEC - fns_in_sec_phy;
	s64 fault_in_sec_mac = FRAC_PER_NS * NSEC_PER_SEC - fns_in_sec_mac;
	/* MAC MCP counter freq is macfreq / 4 */
	s64 diff_in_mcp_overflow = (fault_in_sec_mac - fault_in_sec_phy) *
				   4 * FRAC_PER_NS;

	diff_in_mcp_overflow = div64_s64(diff_in_mcp_overflow,
					 ATL_HW_MAC_COUNTER_HZ);
	adj_fns_val = (ptp_adj_freq->fns_mac + FRAC_PER_NS *
		       ptp_adj_freq->ns_mac) + diff_in_mcp_overflow;

	ptp_adj_freq->mac_ns_adj = div64_s64(adj_fns_val, FRAC_PER_NS);
	ptp_adj_freq->mac_fns_adj = adj_fns_val - ptp_adj_freq->mac_ns_adj *
				    FRAC_PER_NS;
}

int hw_atl_adj_sys_clock(struct atl_hw *hw, s64 delta)
{
	hw->ptp_clk_offset += delta;

	return 0;
}

int hw_atl_set_sys_clock(struct atl_hw *hw, u64 time, u64 ts)
{
	s64 delta = time - (hw->ptp_clk_offset + ts);

	return hw_atl_adj_sys_clock(hw, delta);
}

int hw_atl_adj_clock_freq(struct atl_hw *hw, s32 ppb)
{
	struct ptp_msg_fw_request fwreq;
	struct atl_mcp *mcp = &hw->mcp;

	memset(&fwreq, 0, sizeof(fwreq));

	fwreq.msg_id = ptp_adj_freq_msg;
	hw_atl_adj_params_get(ATL_HW_MAC_COUNTER_HZ, ppb,
				 &fwreq.adj_freq.ns_mac,
				 &fwreq.adj_freq.fns_mac);
	hw_atl_adj_params_get(ATL_HW_PHY_COUNTER_HZ, ppb,
				 &fwreq.adj_freq.ns_phy,
				 &fwreq.adj_freq.fns_phy);
	hw_atl_mac_adj_param_calc(&fwreq.adj_freq,
				     ATL_HW_PHY_COUNTER_HZ,
				     ATL_HW_MAC_COUNTER_HZ);

	return mcp->ops->send_ptp_req(hw, &fwreq);
}
