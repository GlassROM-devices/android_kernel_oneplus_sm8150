/* SPDX-License-Identifier: GPL-2.0-only */
/* Atlantic Network Driver
 *
 * Copyright (C) 2017 aQuantia Corporation
 * Copyright (C) 2019-2020 Marvell International Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _ATL_HW_PTP_H_
#define _ATL_HW_PTP_H_

#include "atl_hw.h"

void hw_atl_get_ptp_ts(struct atl_hw *hw, u64 *stamp);
int hw_atl_adj_sys_clock(struct atl_hw *hw, s64 delta);
int hw_atl_set_sys_clock(struct atl_hw *hw, u64 time, u64 ts);
int hw_atl_adj_clock_freq(struct atl_hw *hw, s32 ppb);

#endif
