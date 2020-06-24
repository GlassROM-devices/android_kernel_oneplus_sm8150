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

#define ATL_PTP_TX_TIMEOUT        (HZ *  10)

enum ptp_speed_offsets {
	ptp_offset_idx_10 = 0,
	ptp_offset_idx_100,
	ptp_offset_idx_1000,
	ptp_offset_idx_2500,
	ptp_offset_idx_5000,
	ptp_offset_idx_10000,
};

struct ptp_skb_ring {
	struct sk_buff **buff;
	spinlock_t lock;
	unsigned int size;
	unsigned int head;
	unsigned int tail;
};

struct ptp_tx_timeout {
	spinlock_t lock;
	bool active;
	unsigned long tx_start;
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

	atomic_t offset_egress;
	atomic_t offset_ingress;

	struct ptp_tx_timeout ptp_tx_timeout;

	struct napi_struct *napi;
	unsigned int idx_vector;

	struct atl_queue_vec qvec[ATL_PTPQ_NUM];

	struct ptp_skb_ring skb_ring;
};

#define atl_for_each_ptp_qvec(ptp, qvec)			\
	for (qvec = &ptp->qvec[0];				\
	     qvec < &ptp->qvec[ATL_PTPQ_NUM]; qvec++)

struct ptp_tm_offset {
	unsigned int mbps;
	int egress;
	int ingress;
};

static struct ptp_tm_offset ptp_offset[6];

void atl_ptp_tm_offset_set(struct atl_nic *nic, unsigned int mbps)
{
	struct atl_ptp *ptp = nic->ptp;
	int i, egress, ingress;

	if (!ptp)
		return;

	egress = 0;
	ingress = 0;

	for (i = 0; i < ARRAY_SIZE(ptp_offset); i++) {
		if (mbps == ptp_offset[i].mbps) {
			egress = ptp_offset[i].egress;
			ingress = ptp_offset[i].ingress;
			break;
		}
	}

	atomic_set(&ptp->offset_egress, egress);
	atomic_set(&ptp->offset_ingress, ingress);
}

static int __atl_ptp_skb_put(struct ptp_skb_ring *ring, struct sk_buff *skb)
{
	unsigned int next_head = (ring->head + 1) % ring->size;

	if (next_head == ring->tail)
		return -ENOMEM;

	ring->buff[ring->head] = skb_get(skb);
	ring->head = next_head;

	return 0;
}

static int atl_ptp_skb_put(struct ptp_skb_ring *ring, struct sk_buff *skb)
{
	unsigned long flags;
	int ret;

	spin_lock_irqsave(&ring->lock, flags);
	ret = __atl_ptp_skb_put(ring, skb);
	spin_unlock_irqrestore(&ring->lock, flags);

	return ret;
}

static struct sk_buff *__atl_ptp_skb_get(struct ptp_skb_ring *ring)
{
	struct sk_buff *skb;

	if (ring->tail == ring->head)
		return NULL;

	skb = ring->buff[ring->tail];
	ring->tail = (ring->tail + 1) % ring->size;

	return skb;
}

static struct sk_buff *atl_ptp_skb_get(struct ptp_skb_ring *ring)
{
	unsigned long flags;
	struct sk_buff *skb;

	spin_lock_irqsave(&ring->lock, flags);
	skb = __atl_ptp_skb_get(ring);
	spin_unlock_irqrestore(&ring->lock, flags);

	return skb;
}

static unsigned int atl_ptp_skb_buf_len(struct ptp_skb_ring *ring)
{
	unsigned long flags;
	unsigned int len;

	spin_lock_irqsave(&ring->lock, flags);
	len = (ring->head >= ring->tail) ?
	ring->head - ring->tail :
	ring->size - ring->tail + ring->head;
	spin_unlock_irqrestore(&ring->lock, flags);

	return len;
}

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

static void atl_ptp_skb_ring_clean(struct ptp_skb_ring *ring)
{
	struct sk_buff *skb;

	while ((skb = atl_ptp_skb_get(ring)) != NULL)
		dev_kfree_skb_any(skb);
}

static void atl_ptp_skb_ring_release(struct ptp_skb_ring *ring)
{
	if (ring->buff) {
		atl_ptp_skb_ring_clean(ring);
		kfree(ring->buff);
		ring->buff = NULL;
	}
}

static void atl_ptp_tx_timeout_init(struct ptp_tx_timeout *timeout)
{
	spin_lock_init(&timeout->lock);
	timeout->active = false;
}

static void atl_ptp_tx_timeout_start(struct atl_ptp *ptp)
{
	struct ptp_tx_timeout *timeout = &ptp->ptp_tx_timeout;
	unsigned long flags;

	spin_lock_irqsave(&timeout->lock, flags);
	timeout->active = true;
	timeout->tx_start = jiffies;
	spin_unlock_irqrestore(&timeout->lock, flags);
}

static void atl_ptp_tx_timeout_update(struct atl_ptp *ptp)
{
	if (!atl_ptp_skb_buf_len(&ptp->skb_ring)) {
		struct ptp_tx_timeout *timeout = &ptp->ptp_tx_timeout;
		unsigned long flags;

		spin_lock_irqsave(&timeout->lock, flags);
		timeout->active = false;
		spin_unlock_irqrestore(&timeout->lock, flags);
	}
}

static void atl_ptp_tx_timeout_check(struct atl_ptp *ptp)
{
	struct ptp_tx_timeout *timeout = &ptp->ptp_tx_timeout;
	struct atl_nic *nic = ptp->nic;
	unsigned long flags;
	bool timeout_flag;

	timeout_flag = false;

	spin_lock_irqsave(&timeout->lock, flags);
	if (timeout->active) {
		timeout_flag = time_is_before_jiffies(timeout->tx_start +
						      ATL_PTP_TX_TIMEOUT);
		/* reset active flag if timeout detected */
		if (timeout_flag)
			timeout->active = false;
	}
	spin_unlock_irqrestore(&timeout->lock, flags);

	if (timeout_flag) {
		atl_nic_err("PTP Timeout. Clearing Tx Timestamp SKBs\n");
		atl_ptp_skb_ring_clean(&ptp->skb_ring);
	}
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

static void atl_ptp_convert_to_hwtstamp(struct skb_shared_hwtstamps *hwtstamp,
					u64 timestamp)
{
	memset(hwtstamp, 0, sizeof(*hwtstamp));
	hwtstamp->hwtstamp = ns_to_ktime(timestamp);
}

/* atl_ptp_tx_hwtstamp - utility function which checks for TX time stamp
 *
 * if the timestamp is valid, we convert it into the timecounter ns
 * value, then store that result into the hwtstamps structure which
 * is passed up the network stack
 */
void atl_ptp_tx_hwtstamp(struct atl_nic *nic, u64 timestamp)
{
	struct atl_ptp *ptp = nic->ptp;
	struct sk_buff *skb = atl_ptp_skb_get(&ptp->skb_ring);
	struct skb_shared_hwtstamps hwtstamp;

	if (!skb) {
		atl_nic_err("have timestamp but tx_queues empty\n");
		return;
	}

	timestamp += atomic_read(&ptp->offset_egress);
	atl_ptp_convert_to_hwtstamp(&hwtstamp, timestamp);
	skb_tstamp_tx(skb, &hwtstamp);
	dev_kfree_skb_any(skb);

	atl_ptp_tx_timeout_update(ptp);
}

/* atl_ptp_rx_hwtstamp - utility function which checks for RX time stamp
 * @skb: particular skb to send timestamp with
 *
 * if the timestamp is valid, we convert it into the timecounter ns
 * value, then store that result into the hwtstamps structure which
 * is passed up the network stack
 */
static void atl_ptp_rx_hwtstamp(struct atl_ptp *ptp, struct sk_buff *skb,
				u64 timestamp)
{
	timestamp -= atomic_read(&ptp->offset_ingress);
	atl_ptp_convert_to_hwtstamp(skb_hwtstamps(skb), timestamp);
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
			return ATL_NUM_NON_RING_IRQS - 1;
	}

	WARN_ONCE(1, "Not a PTP queue vector");
	return ATL_NUM_NON_RING_IRQS;
}

bool atl_is_ptp_ring(struct atl_nic *nic, struct atl_desc_ring *ring)
{
	struct atl_ptp *ptp = nic->ptp;

	if (!ptp)
		return false;

	return &ptp->qvec[ATL_PTPQ_PTP].tx == ring ||
	       &ptp->qvec[ATL_PTPQ_PTP].rx == ring ||
	       &ptp->qvec[ATL_PTPQ_HWTS].rx == ring;
}

u16 atl_ptp_extract_ts(struct atl_nic *nic, struct sk_buff *skb, u8 *p,
		       unsigned int len)
{
	struct atl_ptp *ptp = nic->ptp;
	u64 timestamp = 0;
	u16 ret = hw_atl_rx_extract_ts(&nic->hw, p, len, &timestamp);

	if (ret > 0)
		atl_ptp_rx_hwtstamp(ptp, skb, timestamp);

	return ret;
}

static int atl_ptp_poll(struct napi_struct *napi, int budget)
{
	struct atl_queue_vec *qvec = container_of(napi, struct atl_queue_vec, napi);
	struct atl_ptp *ptp = qvec->nic->ptp;
	int work_done = 0;

	/* Processing PTP TX and RX traffic */
	work_done = atl_poll_qvec(&ptp->qvec[ATL_PTPQ_PTP], budget);

	/* Processing HW_TIMESTAMP RX traffic */
	atl_clean_hwts_rx(&ptp->qvec[ATL_PTPQ_HWTS].rx, budget);

	return work_done;
}

static irqreturn_t atl_ptp_irq(int irq, void *private)
{
	struct atl_ptp *ptp = private;
	int err = 0;

	if (!ptp) {
		err = -EINVAL;
		goto err_exit;
	}
	napi_schedule_irqoff(ptp->napi);

err_exit:
	return err >= 0 ? IRQ_HANDLED : IRQ_NONE;
}

netdev_tx_t atl_ptp_start_xmit(struct atl_nic *nic, struct sk_buff *skb)
{
	struct atl_ptp *ptp = nic->ptp;
	netdev_tx_t err = NETDEV_TX_OK;
	struct atl_desc_ring *ring;
	unsigned long irq_flags;

	ring = &ptp->qvec[ATL_PTPQ_PTP].tx;

	if (skb->len <= 0) {
		dev_kfree_skb_any(skb);
		goto err_exit;
	}

	if (atl_tx_full(ring, skb_shinfo(skb)->nr_frags + 4)) {
		/* Drop packet because it doesn't make sence to delay it */
		dev_kfree_skb_any(skb);
		goto err_exit;
	}

	err = atl_ptp_skb_put(&ptp->skb_ring, skb);
	if (err) {
		atl_nic_err("SKB Ring is overflow!\n");
		return NETDEV_TX_BUSY;
	}
	skb_shinfo(skb)->tx_flags |= SKBTX_IN_PROGRESS;
	atl_ptp_tx_timeout_start(ptp);
	skb_tx_timestamp(skb);

	spin_lock_irqsave(&ptp->ptp_ring_lock, irq_flags);
	err = atl_map_skb(skb, ring);
	spin_unlock_irqrestore(&ptp->ptp_ring_lock, irq_flags);

err_exit:
	return err;
}

void atl_ptp_work(struct atl_nic *nic)
{
	struct atl_ptp *ptp = nic->ptp;

	if (!ptp)
		return;

	atl_ptp_tx_timeout_check(ptp);
}

int atl_ptp_irq_alloc(struct atl_nic *nic)
{
	struct pci_dev *pdev = nic->hw.pdev;
	struct atl_ptp *ptp = nic->ptp;

	if (!ptp)
		return 0;

	if (nic->flags & ATL_FL_MULTIPLE_VECTORS) {
		return request_irq(pci_irq_vector(pdev, ptp->idx_vector),
				  atl_ptp_irq, 0, nic->ndev->name, ptp);
	}

	return -EINVAL;
}

void atl_ptp_irq_free(struct atl_nic *nic)
{
	struct atl_hw *hw = &nic->hw;
	struct atl_ptp *ptp = nic->ptp;

	if (!ptp)
		return;

	atl_intr_disable(hw, BIT(ptp->idx_vector));
	free_irq(pci_irq_vector(hw->pdev, ptp->idx_vector), ptp);
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

	napi_enable(ptp->napi);

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

	napi_disable(ptp->napi);

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

#define ptp_offset_init(__idx, __mbps, __egress, __ingress)   do { \
		ptp_offset[__idx].mbps = (__mbps); \
		ptp_offset[__idx].egress = (__egress); \
		ptp_offset[__idx].ingress = (__ingress); } \
		while (0)

static void atl_ptp_offset_init_from_fw(const struct atl_ptp_offset_info *offsets)
{
	int i;

	/* Load offsets for PTP */
	for (i = 0; i < ARRAY_SIZE(ptp_offset); i++) {
		switch (i) {
		/* 100M */
		case ptp_offset_idx_100:
			ptp_offset_init(i, 100,
					offsets->egress_100,
					offsets->ingress_100);
			break;
		/* 1G */
		case ptp_offset_idx_1000:
			ptp_offset_init(i, 1000,
					offsets->egress_1000,
					offsets->ingress_1000);
			break;
		/* 2.5G */
		case ptp_offset_idx_2500:
			ptp_offset_init(i, 2500,
					offsets->egress_2500,
					offsets->ingress_2500);
			break;
		/* 5G */
		case ptp_offset_idx_5000:
			ptp_offset_init(i, 5000,
					offsets->egress_5000,
					offsets->ingress_5000);
			break;
		/* 10G */
		case ptp_offset_idx_10000:
			ptp_offset_init(i, 10000,
					offsets->egress_10000,
					offsets->ingress_10000);
			break;
		}
	}
}

static void atl_ptp_offset_init(const struct atl_ptp_offset_info *offsets)
{
	memset(ptp_offset, 0, sizeof(ptp_offset));

	atl_ptp_offset_init_from_fw(offsets);
}

void atl_ptp_clock_init(struct atl_nic *nic)
{
	struct atl_ptp *ptp = nic->ptp;
	struct timespec64 ts;

	ktime_get_real_ts64(&ts);
	atl_ptp_settime(&ptp->ptp_info, &ts);
}

int atl_ptp_init(struct atl_nic *nic)
{
	struct atl_ptp_offset_info ptp_offset_info;
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

	err = atl_read_mcp_mem(&nic->hw, mcp->fw_stat_addr + atl_fw2_stat_ptp_offset,
		&ptp_offset_info, sizeof(ptp_offset_info));
	if (err)
		return err;

	atl_ptp_offset_init(&ptp_offset_info);

	ptp = kzalloc(sizeof(*ptp), GFP_KERNEL);
	if (!ptp) {
		err = -ENOMEM;
		goto err_exit;
	}

	ptp->nic = nic;

	ptp->qvec[ATL_PTPQ_PTP].is_ptp = true;
	ptp->qvec[ATL_PTPQ_HWTS].is_ptp = true;
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
	atl_ptp_tx_timeout_init(&ptp->ptp_tx_timeout);

	atomic_set(&ptp->offset_egress, 0);
	atomic_set(&ptp->offset_ingress, 0);

	ptp->napi = &ptp->qvec[ATL_PTPQ_PTP].napi;
	netif_napi_add(nic->ndev, ptp->napi, atl_ptp_poll, 64);

	ptp->idx_vector = ATL_IRQ_PTP;

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

	netif_napi_del(ptp->napi);
	kfree(ptp);
	nic->ptp = NULL;
}
#endif
