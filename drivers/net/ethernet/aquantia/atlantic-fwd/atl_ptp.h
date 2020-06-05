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
struct atl_queue_vec;

#if IS_REACHABLE(CONFIG_PTP_1588_CLOCK)

/* Common functions */
int atl_ptp_init(struct atl_nic *nic);

void atl_ptp_unregister(struct atl_nic *nic);
void atl_ptp_free(struct atl_nic *nic);

int atl_ptp_ring_alloc(struct atl_nic *nic);
void atl_ptp_ring_free(struct atl_nic *nic);

int atl_ptp_ring_start(struct atl_nic *nic);
void atl_ptp_ring_stop(struct atl_nic *nic);

void atl_ptp_clock_init(struct atl_nic *nic);

int atl_ptp_qvec_intr(struct atl_queue_vec *qvec);

#else

static inline int atl_ptp_init(struct atl_nic *nic)
{
	return 0;
}

static inline void atl_ptp_unregister(struct atl_nic *nic) {}
static inline void atl_ptp_free(struct atl_nic *nic) {}

static inline int atl_ptp_ring_alloc(struct atl_nic *nic)
{
	return 0;
}

static inline void atl_ptp_ring_free(struct atl_nic *nic) {}

static inline int atl_ptp_ring_start(struct atl_nic *nic)
{
	return 0;
}

static inline void atl_ptp_ring_stop(struct atl_nic *nic) {}

static inline void atl_ptp_clock_init(struct atl_nic *nic) {}

#endif

#endif /* ATL_PTP_H */
