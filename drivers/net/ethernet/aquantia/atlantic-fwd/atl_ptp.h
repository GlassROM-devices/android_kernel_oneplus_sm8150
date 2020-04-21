/* SPDX-License-Identifier: GPL-2.0-only */
/* Atlantic Network Driver
 *
 * Copyright (C) 2014-2019 aQuantia Corporation
 * Copyright (C) 2019-2020 Marvell International Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef ATL_PTP_H
#define ATL_PTP_H

#include "atl_compat.h"

struct atl_nic;

#if IS_REACHABLE(CONFIG_PTP_1588_CLOCK)

/* Common functions */
int atl_ptp_init(struct atl_nic *nic);

void atl_ptp_unregister(struct atl_nic *nic);
void atl_ptp_free(struct atl_nic *nic);

#else

static inline int atl_ptp_init(struct atl_nic *nic)
{
	return 0;
}

static inline void atl_ptp_unregister(struct atl_nic *nic) {}
static inline void atl_ptp_free(struct atl_nic *nic) {}

#endif

#endif /* ATL_PTP_H */
