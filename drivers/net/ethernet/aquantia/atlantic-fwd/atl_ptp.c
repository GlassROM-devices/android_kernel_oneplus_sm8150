// SPDX-License-Identifier: GPL-2.0-only
/* Atlantic Network Driver
 *
 * Copyright (C) 2014-2019 aQuantia Corporation
 * Copyright (C) 2019-2020 Marvell International Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/ptp_clock_kernel.h>
#include <linux/clocksource.h>

#include "atl_ptp.h"
#include "atl_common.h"
#include "atl_hw_ptp.h"

#if IS_REACHABLE(CONFIG_PTP_1588_CLOCK)

struct atl_ptp {
	struct atl_nic *nic;
	spinlock_t ptp_lock;
	struct ptp_clock *ptp_clock;
	struct ptp_clock_info ptp_info;
};

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 3, 0)
/* atl_ptp_adjfine
 * @ptp_info: the ptp clock structure
 * @ppb: parts per billion adjustment from base
 *
 * adjust the frequency of the ptp cycle counter by the
 * indicated ppb from the base frequency.
 */
static int atl_ptp_adjfine(struct ptp_clock_info *ptp_info, long scaled_ppm)
{
	struct atl_ptp *ptp = container_of(ptp_info, struct atl_ptp, ptp_info);
	struct atl_nic *nic = ptp->nic;

	hw_atl_adj_clock_freq(&nic->hw, scaled_ppm_to_ppb(scaled_ppm));

	return 0;
}
#endif

/* atl_ptp_adjtime
 * @ptp_info: the ptp clock structure
 * @delta: offset to adjust the cycle counter by
 *
 * adjust the timer by resetting the timecounter structure.
 */
static int atl_ptp_adjtime(struct ptp_clock_info *ptp_info, s64 delta)
{
	struct atl_ptp *ptp = container_of(ptp_info, struct atl_ptp, ptp_info);
	struct atl_nic *nic = ptp->nic;
	unsigned long flags;

	spin_lock_irqsave(&ptp->ptp_lock, flags);
	hw_atl_adj_sys_clock(&nic->hw, delta);
	spin_unlock_irqrestore(&ptp->ptp_lock, flags);

	return 0;
}

/* atl_ptp_gettime
 * @ptp_info: the ptp clock structure
 * @ts: timespec structure to hold the current time value
 *
 * read the timecounter and return the correct value on ns,
 * after converting it into a struct timespec.
 */
static int atl_ptp_gettime(struct ptp_clock_info *ptp_info, struct timespec64 *ts)
{
	struct atl_ptp *ptp = container_of(ptp_info, struct atl_ptp, ptp_info);
	struct atl_nic *nic = ptp->nic;
	unsigned long flags;
	u64 ns;

	spin_lock_irqsave(&ptp->ptp_lock, flags);
	hw_atl_get_ptp_ts(&nic->hw, &ns);
	spin_unlock_irqrestore(&ptp->ptp_lock, flags);

	*ts = ns_to_timespec64(ns);

	return 0;
}

/* atl_ptp_settime
 * @ptp_info: the ptp clock structure
 * @ts: the timespec containing the new time for the cycle counter
 *
 * reset the timecounter to use a new base value instead of the kernel
 * wall timer value.
 */
static int atl_ptp_settime(struct ptp_clock_info *ptp_info,
			  const struct timespec64 *ts)
{
	struct atl_ptp *ptp = container_of(ptp_info, struct atl_ptp, ptp_info);
	struct atl_nic *nic = ptp->nic;
	unsigned long flags;
	u64 ns = timespec64_to_ns(ts);
	u64 now;

	spin_lock_irqsave(&ptp->ptp_lock, flags);
	hw_atl_get_ptp_ts(&nic->hw, &now);
	hw_atl_adj_sys_clock(&nic->hw, (s64)ns - (s64)now);

	spin_unlock_irqrestore(&ptp->ptp_lock, flags);

	return 0;
}

static struct ptp_clock_info atl_ptp_clock = {
	.owner		= THIS_MODULE,
	.name		= "atlantic ptp",
	.max_adj	= 999999999,
	.n_ext_ts	= 0,
	.pps		= 0,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 3, 0)
	.adjfine	= atl_ptp_adjfine,
#endif
	.adjtime	= atl_ptp_adjtime,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 1, 0)
	.gettime64	= atl_ptp_gettime,
	.settime64	= atl_ptp_settime,
#else
	.gettime	= atl_ptp_gettime,
	.settime	= atl_ptp_settime,
#endif
	.n_per_out	= 0,
	.n_pins		= 0,
	.pin_config	= NULL,
};

void atl_ptp_clock_init(struct atl_nic *nic)
{
	struct atl_ptp *ptp = nic->ptp;
	struct timespec64 ts;

	ktime_get_real_ts64(&ts);
	atl_ptp_settime(&ptp->ptp_info, &ts);
}

int atl_ptp_init(struct atl_nic *nic)
{
	struct atl_mcp *mcp = &nic->hw.mcp;
	struct ptp_clock *clock;
	struct atl_ptp *ptp;
	int err = 0;

	if (!mcp->ops->set_ptp) {
		nic->ptp = NULL;
		return 0;
	}

	if (!(mcp->caps_ex & atl_fw2_ex_caps_phy_ptp_en)) {
		nic->ptp = NULL;
		return 0;
	}

	ptp = kzalloc(sizeof(*ptp), GFP_KERNEL);
	if (!ptp) {
		err = -ENOMEM;
		goto err_exit;
	}

	ptp->nic = nic;

	spin_lock_init(&ptp->ptp_lock);

	ptp->ptp_info = atl_ptp_clock;
	clock = ptp_clock_register(&ptp->ptp_info, &nic->ndev->dev);
	if (IS_ERR_OR_NULL(clock)) {
		netdev_err(nic->ndev, "ptp_clock_register failed\n");
		err = PTR_ERR(clock);
		goto err_exit;
	}
	ptp->ptp_clock = clock;

	nic->ptp = ptp;

	/* enable ptp counter */
	nic->hw.link_state.ptp_available = true;
	mcp->ops->set_ptp(&nic->hw, true);
	atl_ptp_clock_init(nic);

	return 0;

err_exit:
	kfree(ptp);
	nic->ptp = NULL;
	return err;
}

void atl_ptp_unregister(struct atl_nic *nic)
{
	struct atl_ptp *ptp = nic->ptp;

	if (!ptp)
		return;

	ptp_clock_unregister(ptp->ptp_clock);
}

void atl_ptp_free(struct atl_nic *nic)
{
	struct atl_mcp *mcp = &nic->hw.mcp;
	struct atl_ptp *ptp = nic->ptp;

	if (!ptp)
		return;

	/* disable ptp */
	mcp->ops->set_ptp(&nic->hw, false);

	kfree(ptp);
	nic->ptp = NULL;
}
#endif
