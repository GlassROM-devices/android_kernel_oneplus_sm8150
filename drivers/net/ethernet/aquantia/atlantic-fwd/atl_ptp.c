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

#if IS_REACHABLE(CONFIG_PTP_1588_CLOCK)

struct atl_ptp {
	struct atl_nic *nic;
	struct ptp_clock *ptp_clock;
	struct ptp_clock_info ptp_info;
};

static struct ptp_clock_info atl_ptp_clock = {
	.owner		= THIS_MODULE,
	.name		= "atlantic ptp",
	.n_ext_ts	= 0,
	.pps		= 0,
	.n_per_out	= 0,
	.n_pins		= 0,
	.pin_config	= NULL,
};

int atl_ptp_init(struct atl_nic *nic)
{
	struct atl_mcp *mcp = &nic->hw.mcp;
	struct ptp_clock *clock;
	struct atl_ptp *ptp;
	int err = 0;

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

	ptp->ptp_info = atl_ptp_clock;
	clock = ptp_clock_register(&ptp->ptp_info, &nic->ndev->dev);
	if (IS_ERR_OR_NULL(clock)) {
		netdev_err(nic->ndev, "ptp_clock_register failed\n");
		err = PTR_ERR(clock);
		goto err_exit;
	}
	ptp->ptp_clock = clock;

	nic->ptp = ptp;

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
	struct atl_ptp *ptp = nic->ptp;

	if (!ptp)
		return;

	kfree(ptp);
	nic->ptp = NULL;
}
#endif
