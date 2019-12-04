/*
 * Marvell Semiconductor Altantic Network Driver
 * Copyright (C) 2019 Marvell Semiconductor. All rights reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 */
#include "atl_common.h"
#include "atl_hw.h"
#include "atl2_fw.h"

#define ATL2_FW_READ_TRY_MAX 1000

#define atl2_shared_buffer_write(HW, ITEM, VARIABLE) \
	atl2_mif_shared_buf_write(HW,\
		(offsetof(struct fw_interface_in, ITEM) / sizeof(u32)),\
		(u32 *)&VARIABLE, sizeof(VARIABLE) / sizeof(u32))

#define atl2_shared_buffer_get(HW, ITEM, VARIABLE) \
	atl2_mif_shared_buf_get(HW, \
		(offsetof(struct fw_interface_in, ITEM) / sizeof(u32)),\
		(u32 *)&VARIABLE, \
		sizeof(VARIABLE) / sizeof(u32))

/* This should never be used on non atomic fields,
 * treat any > u32 read as non atomic.
 */
#define atl2_shared_buffer_read(HW, ITEM, VARIABLE) \
{\
	BUILD_BUG_ON_MSG((offsetof(struct fw_interface_out, ITEM) % \
			 sizeof(u32)) != 0,\
			 "Non aligned read " # ITEM);\
	BUILD_BUG_ON_MSG(sizeof(VARIABLE) > sizeof(u32),\
			 "Non atomic read " # ITEM);\
	atl2_mif_shared_buf_read(HW, \
		(offsetof(struct fw_interface_out, ITEM) / sizeof(u32)),\
		(u32 *)&VARIABLE, \
		sizeof(VARIABLE) / sizeof(u32));\
}

#define atl2_shared_buffer_read_item_fn_decl(ITEM) \
static int atl2_mif_shared_buf_read_item_##ITEM(struct atl_hw *hw,\
						void *data)\
{\
	atl2_mif_shared_buf_read(hw, \
		(offsetof(struct fw_interface_out, ITEM) / sizeof(u32)),\
		(u32 *)data,\
		sizeof(((struct fw_interface_out *)0)->ITEM) / sizeof(u32));\
	return 0;\
}

#define  atl2_shared_buffer_read_item_fn_name(ITEM) \
	atl2_mif_shared_buf_read_item_##ITEM


static void atl2_mif_shared_buf_get(struct atl_hw *hw, int offset,
				       u32 *data, int len)
{
	int i;
	int j = 0;

	for (i = offset; i < offset + len; i++, j++)
		data[j] = atl_read(hw, ATL2_MIF_SHARED_BUFFER_IN_ADR(i));
}

static void atl2_mif_shared_buf_write(struct atl_hw *hw, int offset,
				  u32 *data, int len)
{
	int i;
	int j = 0;

	for (i = offset; i < offset + len; i++, j++)
		atl_write(hw, ATL2_MIF_SHARED_BUFFER_IN_ADR(i),
				data[j]);
}

static void atl2_mif_shared_buf_read(struct atl_hw *hw, int offset,
				     u32 *data, int len)
{
	int i;
	int j = 0;

	for (i = offset; i < offset + len; i++, j++)
		data[j] = atl_read(hw, ATL2_MIF_SHARED_BUFFER_OUT_ADR(i));
}

static void atl2_mif_host_finished_write_set(struct atl_hw *hw, u32 finish)
{
	atl_write_bits(hw, ATL2_MIF_HOST_FINISHED_WRITE_ADR,
			    ATL2_MIF_HOST_FINISHED_WRITE_SHIFT,
			    ATL2_MIF_HOST_FINISHED_WRITE_WIDTH,
			    finish);
}

static u32 atl2_mif_mcp_finished_read_get(struct atl_hw *hw)
{
	return (atl_read(hw, ATL2_MIF_MCP_FINISHED_READ_ADR) &
		ATL2_MIF_MCP_FINISHED_READ_MSK) >>
		ATL2_MIF_MCP_FINISHED_READ_SHIFT;
}
static int atl2_shared_buffer_read_safe(struct atl_hw *hw,
		     int (*read_item_fn)(struct atl_hw *hw, void *data),
		     void *data)
{
	struct transaction_counter_s tid1, tid2;
	int cnt = 0;

	do {
		do {
			atl2_shared_buffer_read(hw, transaction_id, tid1);
			cnt++;
			if (cnt > ATL2_FW_READ_TRY_MAX)
				return -ETIME;
			if (tid1.transaction_cnt_a != tid1.transaction_cnt_b)
				udelay(1);
		} while (tid1.transaction_cnt_a != tid1.transaction_cnt_b);

		read_item_fn(hw, data);

		atl2_shared_buffer_read(hw, transaction_id, tid2);

		cnt++;
		//TODO: no much sense to handle these errors
		// therefore put WARN here to make it visible
		if (cnt > ATL2_FW_READ_TRY_MAX)
			return -ETIME;
	} while (tid2.transaction_cnt_a != tid2.transaction_cnt_b ||
		 tid1.transaction_cnt_a != tid2.transaction_cnt_a);

	return 0;
}

static inline int atl2_shared_buffer_finish_ack(struct atl_hw *hw)
{
	u32 val;
	int err = 0;

	atl2_mif_host_finished_write_set(hw, 1U);

	busy_wait(1000, udelay(100), val,
		  atl2_mif_mcp_finished_read_get(hw),
		  val != 0);

	if (val != 0)
		err = -ETIME;
	WARN(err, "atl2_shared_buffer_finish_ack");

	return err;
}

static int __a2_fw_wait_init(struct atl_hw *hw)
{
	struct link_control_s link_control;
	uint32_t mtu;

	BUILD_BUG_ON_MSG(sizeof(struct link_options_s) != 0x4,
			 "linkOptions invalid size");
	BUILD_BUG_ON_MSG(sizeof(struct thermal_shutdown_s) != 0x4,
			 "thermalShutdown invalid size");
	BUILD_BUG_ON_MSG(sizeof(struct sleep_proxy_s) != 0x958,
			 "sleepProxy invalid size");
	BUILD_BUG_ON_MSG(sizeof(struct pause_quanta_s) != 0x18,
			 "pauseQuanta invalid size");
	BUILD_BUG_ON_MSG(sizeof(struct cable_diag_control_s) != 0x4,
			 "cableDiagControl invalid size");

	BUILD_BUG_ON_MSG(offsetof(struct fw_interface_in, mtu) != 0,
			 "mtu invalid offset");
	BUILD_BUG_ON_MSG(offsetof(struct fw_interface_in, mac_address) != 0x8,
			 "macAddress invalid offset");
	BUILD_BUG_ON_MSG(offsetof(struct fw_interface_in,
				  link_control) != 0x10,
			 "linkControl invalid offset");
	BUILD_BUG_ON_MSG(offsetof(struct fw_interface_in,
				  link_options) != 0x18,
			 "linkOptions invalid offset");
	BUILD_BUG_ON_MSG(offsetof(struct fw_interface_in,
				  thermal_shutdown) != 0x20,
			 "thermalShutdown invalid offset");
	BUILD_BUG_ON_MSG(offsetof(struct fw_interface_in, sleep_proxy) != 0x28,
			 "sleepProxy invalid offset");
	BUILD_BUG_ON_MSG(offsetof(struct fw_interface_in,
				  pause_quanta) != 0x984,
			 "pauseQuanta invalid offset");
	BUILD_BUG_ON_MSG(offsetof(struct fw_interface_in,
				  cable_diag_control) != 0xA44,
			 "cableDiagControl invalid offset");

	BUILD_BUG_ON_MSG(offsetof(struct fw_interface_out, version) != 0x04,
			 "version invalid offset");
	BUILD_BUG_ON_MSG(offsetof(struct fw_interface_out, link_status) != 0x14,
			 "linkStatus invalid offset");
	BUILD_BUG_ON_MSG(offsetof(struct fw_interface_out,
				  wol_status) != 0x18,
			 "wolStatus invalid offset");
	BUILD_BUG_ON_MSG(offsetof(struct fw_interface_out,
				  mac_health_monitor) != 0x610,
			 "macHealthMonitor invalid offset");
	BUILD_BUG_ON_MSG(offsetof(struct fw_interface_out,
				  phy_health_monitor) != 0x620,
			 "phyHealthMonitor invalid offset");
	BUILD_BUG_ON_MSG(offsetof(struct fw_interface_out,
				  cable_diag_status) != 0x630,
			 "cableDiagStatus invalid offset");
	BUILD_BUG_ON_MSG(offsetof(struct fw_interface_out,
				  device_link_caps) != 0x648,
			 "deviceLinkCaps invalid offset");
	BUILD_BUG_ON_MSG(offsetof(struct fw_interface_out,
				 sleep_proxy_caps) != 0x650,
			 "sleepProxyCaps invalid offset");
	BUILD_BUG_ON_MSG(offsetof(struct fw_interface_out,
				  lkp_link_caps) != 0x660,
			 "lkpLinkCaps invalid offset");
	BUILD_BUG_ON_MSG(offsetof(struct fw_interface_out, core_dump) != 0x668,
			 "coreDump invalid offset");
	BUILD_BUG_ON_MSG(offsetof(struct fw_interface_out, stats) != 0x700,
			 "stats invalid offset");
	BUILD_BUG_ON_MSG(offsetof(struct fw_interface_out, trace) != 0x800,
			 "trace invalid offset");

	atl2_shared_buffer_get(hw, link_control, link_control);
	link_control.mode = ATL2_HOST_MODE_ACTIVE;
	atl2_shared_buffer_write(hw, link_control, link_control);

	atl2_shared_buffer_get(hw, mtu, mtu);
	mtu = ATL_MAX_MTU;
	atl2_shared_buffer_write(hw, mtu, mtu);

	return atl2_shared_buffer_finish_ack(hw);
}

static int atl2_fw_deinit(struct atl_hw *hw)
{
	struct link_control_s link_control;
	uint32_t val;
	int err = 0;

	atl2_shared_buffer_get(hw, link_control, link_control);
	link_control.mode = ATL2_HOST_MODE_SHUTDOWN;

	atl2_shared_buffer_write(hw, link_control, link_control);
	atl2_mif_host_finished_write_set(hw, 1U);
	busy_wait(1000, udelay(100), val,
		  atl2_mif_mcp_finished_read_get(hw),
		  val != 0);

	if (val != 0)
		err = -ETIME;

	return err;
}

int atl2_get_fw_version(struct atl_hw *hw, u32 *fw_version)
{
	*fw_version = atl_read(hw, 0x13008);

	return 0;
}

static struct atl_fw_ops atl2_fw_ops = {
		.__wait_fw_init = __a2_fw_wait_init,
		.deinit = atl2_fw_deinit,
/*		.set_link = atl2_fw_set_link,
		.check_link = atl2_fw_check_link,
		.__get_link_caps = __atl2_fw_get_link_caps,
		.restart_aneg = atl2_fw_restart_aneg,
		.set_default_link = atl2_fw_set_default_link,
		.enable_wol = atl2_fw_enable_wol,
		.get_phy_temperature = atl2_fw_get_phy_temperature,
		.dump_cfg = atl2_fw_dump_cfg,
		.restore_cfg = atl2_fw_restore_cfg,
		.set_phy_loopback = atl2_fw_set_phy_loopback,
		.set_mediadetect = atl2_fw_set_mediadetect,
		.send_macsec_req = atl2_fw_send_macsec_request,
		.__get_hbeat = __atl2_fw_get_hbeat,
		.get_mac_addr = atl2_fw_get_mac_addr,
*/
};

int atl2_fw_init(struct atl_hw *hw)
{
	struct atl_mcp *mcp = &hw->mcp;
	int ret;

	atl2_get_fw_version(hw, &mcp->fw_rev);

	mcp->ops = &atl2_fw_ops;
	atl_dev_dbg("Detect ATL2FW %x\n", mcp->fw_rev);

	ret = mcp->ops->__wait_fw_init(hw);
	if (ret)
		return ret;

	mcp->fw_stat_addr = 0;
	mcp->rpc_addr = 0;

	ret = mcp->ops->__get_hbeat(hw, &mcp->phy_hbeat);
	if (ret)
		return ret;
	mcp->next_wdog = jiffies + 2 * HZ;

	ret = mcp->ops->__get_link_caps(hw);
	if (ret)
		return ret;

	if (!(mcp->caps_high & atl_fw2_set_thermal)) {
		if (hw->thermal.flags & atl_thermal_monitor)
			atl_dev_warn("Thermal monitoring not supported by firmware\n");
		hw->thermal.flags &=
			~(atl_thermal_monitor | atl_thermal_throttle);
	} else
		ret = mcp->ops->__update_thermal(hw);


	return ret;
}
