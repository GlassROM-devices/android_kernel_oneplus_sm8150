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
#include "atl_ring.h"
#include "atl_ring_desc.h"

struct ptp_skb_ring {
	struct sk_buff **buff;
	spinlock_t lock;
	unsigned int size;
	unsigned int head;
	unsigned int tail;
};

enum atl_ptp_queue {
	ATL_PTPQ_PTP = 0,
	ATL_PTPQ_HWTS = 1,
	ATL_PTPQ_NUM,
};

#if IS_REACHABLE(CONFIG_PTP_1588_CLOCK)

struct atl_ptp {
	struct atl_nic *nic;
	spinlock_t ptp_lock;
	spinlock_t ptp_ring_lock;
	struct ptp_clock *ptp_clock;
	struct ptp_clock_info ptp_info;

	struct atl_queue_vec qvec[ATL_PTPQ_NUM];

	struct ptp_skb_ring skb_ring;
};

#define atl_for_each_ptp_qvec(ptp, qvec)			\
	for (qvec = &ptp->qvec[0];				\
	     qvec < &ptp->qvec[ATL_PTPQ_NUM]; qvec++)

static int atl_ptp_skb_ring_init(struct ptp_skb_ring *ring, unsigned int size)
{
	struct sk_buff **buff = kmalloc(sizeof(*buff) * size, GFP_KERNEL);

	if (!buff)
		return -ENOMEM;

	spin_lock_init(&ring->lock);

	ring->buff = buff;
	ring->size = size;
	ring->head = 0;
	ring->tail = 0;

	return 0;
}

static void atl_ptp_skb_ring_release(struct ptp_skb_ring *ring)
{
	kfree(ring->buff);
	ring->buff = NULL;
}

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

#define PTP_8TC_RING_IDX             8
#define PTP_4TC_RING_IDX            16
#define PTP_HWTS_RING_IDX           31

int atl_ptp_ring_index(enum atl_ptp_queue ptp_queue)
{
	switch (ptp_queue) {
	case ATL_PTPQ_PTP:
		/* multi-TC is not supported in FWD driver, so tc mode is
		 * always set to 4 TCs (each with 8 queues) for now
		 */
		return PTP_4TC_RING_IDX;
	case ATL_PTPQ_HWTS:
		return PTP_HWTS_RING_IDX;
	default:
		break;
	}

	WARN_ONCE(1, "Invalid ptp_queue");
	return 0;
}

int atl_ptp_qvec_intr(struct atl_queue_vec *qvec)
{
	int i;

	for (i = 0; i != ATL_PTPQ_NUM; i++) {
		if (qvec->idx == atl_ptp_ring_index(i))
			return i + qvec->nic->nvecs + ATL_NUM_NON_RING_IRQS;
	}

	WARN_ONCE(1, "Not a PTP queue vector");
	return ATL_NUM_NON_RING_IRQS;
}

int atl_ptp_ring_alloc(struct atl_nic *nic)
{
	struct atl_ptp *ptp = nic->ptp;
	struct atl_queue_vec *qvec;
	int err = 0;
	int i;

	if (!ptp)
		return 0;

	for (i = 0; i != ATL_PTPQ_NUM; i++) {
		qvec = &ptp->qvec[i];

		atl_init_qvec(nic, qvec, atl_ptp_ring_index(i));
	}

	atl_for_each_ptp_qvec(ptp, qvec) {
		err = atl_alloc_qvec(qvec);
		if (err)
			goto free;
	}

	err = atl_ptp_skb_ring_init(&ptp->skb_ring, nic->requested_rx_size);
	if (err != 0) {
		err = -ENOMEM;
		goto free;
	}

	return 0;

free:
	while (--qvec >= &ptp->qvec[0])
		atl_free_qvec(qvec);

	return err;
}

int atl_ptp_ring_start(struct atl_nic *nic)
{
	struct atl_ptp *ptp = nic->ptp;
	struct atl_queue_vec *qvec;
	int err = 0;

	if (!ptp)
		return 0;

	atl_for_each_ptp_qvec(ptp, qvec) {
		err = atl_start_qvec(qvec);
		if (err)
			goto stop;
	}

	return 0;

stop:
	while (--qvec >= &ptp->qvec[0])
		atl_stop_qvec(qvec);

	return err;
}

void atl_ptp_ring_stop(struct atl_nic *nic)
{
	struct atl_ptp *ptp = nic->ptp;
	struct atl_queue_vec *qvec;

	if (!ptp)
		return;

	atl_for_each_ptp_qvec(ptp, qvec)
		atl_stop_qvec(qvec);
}

void atl_ptp_ring_free(struct atl_nic *nic)
{
	struct atl_ptp *ptp = nic->ptp;

	if (!ptp)
		return;

	atl_free_qvec(&ptp->qvec[ATL_PTPQ_PTP]);

	atl_ptp_skb_ring_release(&ptp->skb_ring);
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

	ptp->qvec[ATL_PTPQ_HWTS].is_hwts = true;

	spin_lock_init(&ptp->ptp_lock);
	spin_lock_init(&ptp->ptp_ring_lock);

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
