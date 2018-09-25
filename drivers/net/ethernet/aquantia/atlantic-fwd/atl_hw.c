/*
 * aQuantia Corporation Network Driver
 * Copyright (C) 2017 aQuantia Corporation. All rights reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 */

#include <linux/interrupt.h>

#include "atl_common.h"
#include "atl_hw.h"
#include "atl_ring.h"

static void atl_restore_vlan_filters(struct atl_hw *hw);

struct atl_board_info {
	unsigned int link_mask;
};

static struct atl_board_info atl_boards[] = {
	[ATL_UNKNOWN] = {
		.link_mask = 0x1f,
	},
	[ATL_AQC107] = {
		.link_mask = 0x1f,
	},
	[ATL_AQC108] = {
		.link_mask = 0xf,
	},
	[ATL_AQC109] = {
		.link_mask = 7,
	},
	[ATL_AQC100] = {
		.link_mask = 0x1f,
	},
};

static void atl_unplugged(struct atl_hw *hw)
{
	if (!hw->regs)
		return;
	hw->regs = 0;
	dev_err(&hw->pdev->dev, "Device removed\n");
}

void atl_check_unplug(struct atl_hw *hw, uint32_t addr)
{
	uint32_t val;

	if (addr == ATL_GLOBAL_MIF_ID) {
		atl_unplugged(hw);
		return;
	}

	val = atl_read(hw, ATL_GLOBAL_MIF_ID);
	if (val == 0xffffffff)
		atl_unplugged(hw);
}

bool atl_read_mcp_mem(struct atl_hw *hw, uint32_t mcp_addr, void *host_addr,
		      unsigned int size)
{
	uint32_t *addr = (uint32_t *)host_addr;

	size = (size + 3) & ~3u;
	atl_write(hw, ATL_GLOBAL_MBOX_ADDR, mcp_addr);
	while (size) {
		uint32_t next;

		atl_write(hw, ATL_GLOBAL_MBOX_CTRL, 0x8000);

		busy_wait(100, udelay(10), next,
			  atl_read(hw, ATL_GLOBAL_MBOX_ADDR), next == mcp_addr);
		if (next == mcp_addr) {
			atl_dev_err("mcp mem read timed out (%d remaining)\n",
				    size);
			return false;
		}
		*addr = atl_read(hw, ATL_GLOBAL_MBOX_DATA);
		mcp_addr += 4;
		addr++;
		size -= 4;
	}
	return true;
}


static inline void atl_glb_soft_reset(struct atl_hw *hw)
{
	atl_write_bit(hw, ATL_GLOBAL_STD_CTRL, 14, 0);
	atl_write_bit(hw, ATL_GLOBAL_STD_CTRL, 15, 1);
}

static inline void atl_glb_soft_reset_full(struct atl_hw *hw)
{
	atl_write_bit(hw, ATL_TX_CTRL1, 29, 0);
	atl_write_bit(hw, ATL_RX_CTRL1, 29, 0);
	atl_write_bit(hw, ATL_INTR_CTRL, 29, 0);
	atl_write_bit(hw, ATL_MPI_CTRL1, 29, 0);
	atl_glb_soft_reset(hw);
}

static int atl_hw_reset_nonrbl(struct atl_hw *hw)
{
	uint32_t tries;
	uint32_t reg = atl_read(hw, ATL_GLOBAL_DAISY_CHAIN_STS1);

	bool daisychain_running = (reg & 0x30) != 0x30;

	if (daisychain_running)
		atl_dev_dbg("AQDBG: daisychain running (0x18: %#x)\n",
			    atl_read(hw, ATL_GLOBAL_FW_IMAGE_ID));

	atl_write(hw, 0x404, 0x40e1);
	mdelay(50);

	atl_write(hw, 0x534, 0xa0);
	atl_write(hw, 0x100, 0x9f);
	atl_write(hw, 0x100, 0x809f);
	mdelay(50);

	atl_glb_soft_reset(hw);

	atl_write(hw, 0x404, 0x80e0);
	atl_write(hw, 0x32a8, 0);
	atl_write(hw, 0x520, 1);
	mdelay(50);
	atl_write(hw, 0x404, 0x180e0);

	tries = busy_wait(10000, mdelay(1), reg, atl_read(hw, 0x704),
		!(reg & 0x10));
	if (!(reg & 0x10)) {
		atl_dev_err("FLB kickstart timed out: %#x\n", reg);
		return -EIO;
	}
	atl_dev_dbg("FLB kickstart took %d ms\n", tries);

	atl_write(hw, 0x404, 0x80e0);
	mdelay(50);
	atl_write(hw, 0x3a0, 1);

	atl_glb_soft_reset_full(hw);

	return atl_fw_init(hw);
}

static int atl_hw_reset(struct atl_hw *hw)
{
	uint32_t reg = atl_read(hw, ATL_MCP_SCRATCH(RBL_STS));
	uint32_t flb_stat = atl_read(hw, ATL_GLOBAL_DAISY_CHAIN_STS1);
	int tries = 0;
	/* bool host_load_done = false; */

	while (!reg && flb_stat == 0x6000000 && tries++ < 1000) {
		mdelay(1);
		reg = atl_read(hw, ATL_MCP_SCRATCH(RBL_STS));
		flb_stat = atl_read(hw, ATL_GLOBAL_DAISY_CHAIN_STS1);
	}

	atl_dev_dbg("0x388: %#x 0x704: %#x\n", reg, flb_stat);
	if (tries >= 1000) {
		atl_dev_err("Timeout waiting to choose RBL or FLB path\n");
		return -EIO;
	}

	if (!reg)
		return atl_hw_reset_nonrbl(hw);

	atl_write(hw, 0x404, 0x40e1);
	atl_write(hw, 0x3a0, 1);
	atl_write(hw, 0x32a8, 0);

	atl_write(hw, ATL_MCP_SCRATCH(RBL_STS), 0xdead);

	atl_glb_soft_reset_full(hw);

	atl_write(hw, ATL_GLOBAL_CTRL2, 0x40e0);

	for (tries = 0; tries < 10000; mdelay(1)) {
		tries++;
		reg = atl_read(hw, ATL_MCP_SCRATCH(RBL_STS)) & 0xffff;

		if (!reg || reg == 0xdead)
			continue;

		/* if (reg != 0xf1a7) */
			break;

		/* if (host_load_done) */
		/* 	continue; */

		/* ret = atl_load_mac_fw(hw); */
		/* if (ret) { */
		/* 	atl_dev_err("MAC FW host load failed\n"); */
		/* 	return ret; */
		/* } */
		/* host_load_done = true; */
	}

	if (reg == 0xf1a7) {
		atl_dev_err("MAC FW Host load not supported yet\n");
		return -EIO;
	}
	if (!reg || reg == 0xdead) {
		atl_dev_err("RBL restart timeout: %#x\n", reg);
		return -EIO;
	}
	atl_dev_dbg("RBL restart took %d ms result %#x\n", tries, reg);

	/* if (host_load_done) { */
	/* 	// Wait for MAC FW to decide whether it wants to reload the PHY FW */
	/* 	busy_wait(10, mdelay(1), reg, atl_read(hw, 0x340), !(reg & (1 << 9 | 1 << 1 | 1 << 0))); */

	/* 	if (reg & 1 << 9) { */
	/* 		ret = atl_load_phy_fw(hw); */
	/* 		if (ret) { */
	/* 			atl_dev_err("PHY FW host load failed\n"); */
	/* 			return ret; */
	/* 		} */
	/* 	} */
	/* } */

	return atl_fw_init(hw);
}

static bool atl_get_mac_addr(struct atl_hw *hw, uint8_t *buf)
{
	uint32_t efuse_shadow_addr =
		atl_read(hw, hw->mcp.ops->efuse_shadow_addr_reg);
	uint8_t tmp[8];
	bool ret;

	if (!efuse_shadow_addr)
		return false;

	ret = atl_read_mcp_mem(hw, efuse_shadow_addr + 40 * 4, tmp, 8);
	*(uint32_t *)buf = htonl(*(uint32_t *)tmp);
	*(uint16_t *)&buf[4] = (uint16_t)htonl(*(uint32_t *)&tmp[4]);

	return ret;
}

int atl_hwinit(struct atl_nic *nic, enum atl_board brd_id)
{
	struct atl_hw *hw = &nic->hw;
	struct atl_board_info *brd = &atl_boards[brd_id];
	int ret;

	/* Default supported speed set based on device id. */
	hw->link_state.supported = brd->link_mask;

	ret = atl_hw_reset(hw);

	atl_dev_info("rev 0x%x chip 0x%x FW img 0x%x\n",
		 atl_read(hw, ATL_GLOBAL_CHIP_REV) & 0xffff,
		 atl_read(hw, ATL_GLOBAL_CHIP_ID) & 0xffff,
		 atl_read(hw, ATL_GLOBAL_FW_IMAGE_ID));

	if (ret)
		return ret;

	if (!atl_get_mac_addr(hw, hw->mac_addr)) {
		atl_dev_err("couldn't read MAC address\n");
		return -EIO;
	}

	return hw->mcp.ops->get_link_caps(hw);
}

void atl_refresh_link(struct atl_nic *nic)
{
	struct atl_hw *hw = &nic->hw;
	struct atl_link_type *link, *prev_link = hw->link_state.link;

	link = hw->mcp.ops->check_link(hw);

	if (link != prev_link) {
		if (link) {
			atl_nic_info("Link up: %s\n", link->name);
			netif_carrier_on(nic->ndev);
		} else {
			atl_nic_info("Link down\n");
			netif_carrier_off(nic->ndev);
		}
	}
}

static irqreturn_t atl_link_irq(int irq, void *priv)
{
	struct atl_nic *nic = (struct atl_nic *)priv;

	atl_refresh_link(nic);
	atl_intr_enable(&nic->hw, BIT(0));
	return IRQ_HANDLED;
}

static irqreturn_t atl_legacy_irq(int irq, void *priv)
{
	struct atl_nic *nic = priv;
	struct atl_hw *hw = &nic->hw;
	uint32_t stat;


	stat = atl_read(hw, ATL_INTR_STS);

	/* Mask asserted intr sources */
	atl_intr_disable(hw, stat);

	if (!(stat & hw->intr_mask))
		/* Interrupt from another device on a shared int
		 * line. As no status bits were set, nothing was
		 * masked above, so no need to unmask anything. */
		return IRQ_NONE;

	if (likely(stat & BIT(ATL_NUM_NON_RING_IRQS)))
		/* Only one qvec when using legacy interrupts */
		atl_ring_irq(irq, &nic->qvecs[0].napi);

	if (unlikely(stat & BIT(0)))
		atl_link_irq(irq, nic);
	return IRQ_HANDLED;
}

static int atl_intr_init(struct atl_nic *nic)
{
	struct atl_hw *hw = &nic->hw;
	struct pci_dev *pdev = hw->pdev;
	int ret;

	/* Map link interrupt to cause 0 */
	atl_write(hw, ATL_INTR_GEN_INTR_MAP4, BIT(7) | (0 << 0));

	if (nic->flags & ATL_FL_MULTIPLE_VECTORS) {
		ret = request_irq(pci_irq_vector(pdev, 0), atl_link_irq, 0,
		nic->ndev->name, nic);
		if (ret) {
			atl_nic_err("request MSI link vector failed: %d\n",
				-ret);
			return ret;
		}

		return atl_init_ring_interrupts(nic);
	}

	ret = request_irq(pci_irq_vector(pdev, 0), atl_legacy_irq, IRQF_SHARED,
			  nic->ndev->name, nic);
	if (ret) {
		atl_nic_err("request legacy irq failed: %d\n", -ret);
		return ret;
	}

	/* Enable intr status clear-on-read */
	atl_write_bit(hw, ATL_INTR_CTRL, 7, 1);

	/* Map rx[0] and tx[0] into cause 1*/
	atl_write(hw, ATL_INTR_RING_INTR_MAP(0),
		  BIT(31) | (1 << 0x18) | BIT(15) | (1 << 8));

	return 0;
}

static void atl_intr_release(struct atl_nic *nic)
{
	struct pci_dev *pdev = nic->hw.pdev;

	atl_intr_disable_all(&nic->hw);

	free_irq(pci_irq_vector(pdev, 0), nic);
	if (!(nic->flags & ATL_FL_MULTIPLE_VECTORS))
		return;

	atl_release_ring_interrupts(nic);
}

void atl_set_uc_flt(struct atl_hw *hw, int idx, uint8_t mac_addr[ETH_ALEN])
{
	atl_write(hw, ATL_RX_UC_FLT_REG1(idx),
		be32_to_cpu(*(uint32_t *)&mac_addr[2]));
	atl_write(hw, ATL_RX_UC_FLT_REG2(idx),
		(uint32_t)be16_to_cpu(*(uint16_t *)mac_addr) |
		1 << 16 | 1 << 31);
}

static void atl_disable_uc_flt(struct atl_hw *hw, int idx)
{
	atl_write(hw, ATL_RX_UC_FLT_REG2(idx), 0);
}

void atl_set_rss_key(struct atl_hw *hw)
{
	int i;
	uint32_t val;

	for (i = 0; i < ATL_RSS_KEY_SIZE / 4; i++) {
		val = swab32(((uint32_t *)hw->rss_key)[i]);
		atl_write(hw, ATL_RX_RSS_KEY_WR_DATA, val);
		atl_write(hw, ATL_RX_RSS_KEY_ADDR, i | BIT(5));
		busy_wait(100, udelay(1), val,
			atl_read(hw, ATL_RX_RSS_KEY_ADDR),
			val & BIT(5));
		if (val & BIT(5)) {
			atl_dev_err("Timeout writing RSS key[%d]: %#x\n",
				i, val);
			return;
		}
	}
}

void atl_set_rss_tbl(struct atl_hw *hw)
{
	int i, shift = 0, addr = 0;
	uint32_t val = 0, stat;

	for (i = 0; i < ATL_RSS_TBL_SIZE; i++) {
		val |= (uint32_t)(hw->rss_tbl[i]) << shift;
		shift += 3;

		if (shift < 16)
			continue;

		atl_write(hw, ATL_RX_RSS_TBL_WR_DATA, val & 0xffff);
		atl_write(hw, ATL_RX_RSS_TBL_ADDR, addr | BIT(4));

		busy_wait(100, udelay(1), stat,
			atl_read(hw, ATL_RX_RSS_TBL_ADDR), stat & BIT(4));
		if (stat & BIT(4)) {
			atl_dev_err("Timeout writing RSS redir table[%d] (addr %d): %#x\n",
				    i, addr, stat);
			return;
		}

		shift -= 16;
		val >>= 16;
		addr++;
	}
}

unsigned int atl_fwd_rx_buf_reserve = 0, atl_fwd_tx_buf_reserve = 0;
module_param_named(fwd_tx_buf_reserve, atl_fwd_tx_buf_reserve, uint, 0444);
module_param_named(fwd_rx_buf_reserve, atl_fwd_rx_buf_reserve, uint, 0444);

int atl_start_hw(struct atl_nic *nic)
{
	struct atl_hw *hw = &nic->hw;
	int ret;

	ret = atl_intr_init(nic);
	if (ret)
		return ret;

	/* Enable RPF2, TPO2 */
	atl_write(hw, 0x7040, 0x10000);
	atl_write(hw, 0x5040, 0xf0000);

	/* Alloc TPB */
	/* TC1: space for offload engine iface */
	atl_write(hw, ATL_TX_PBUF_REG1(1), atl_fwd_tx_buf_reserve);
	/* TC0: 160k minus TC1 size */
	atl_write(hw, ATL_TX_PBUF_REG1(0), 160 - atl_fwd_tx_buf_reserve);
	/* 4-TC | Enable TPB */
	atl_set_bits(hw, ATL_TX_PBUF_CTRL1, BIT(8) | BIT(0));

	/* Alloc RPB */
	/* TC1: space for offload engine iface */
	atl_write(hw, ATL_RX_PBUF_REG1(1), atl_fwd_rx_buf_reserve);
	atl_write(hw, ATL_RX_PBUF_REG2(1), BIT(31) |
		(atl_fwd_rx_buf_reserve * 32 * 66 / 100) << 16 |
		(atl_fwd_rx_buf_reserve * 32 * 50 / 100));
	/* TC1: 320k minus TC1 size */
	atl_write(hw, ATL_RX_PBUF_REG1(0), 320 - atl_fwd_rx_buf_reserve);
	atl_write(hw, ATL_RX_PBUF_REG2(0), BIT(31) |
		((320 - atl_fwd_rx_buf_reserve) * 32 * 66 / 100) << 16 |
		((320 - atl_fwd_rx_buf_reserve) * 32 * 50 / 100));
	/* 4-TC | Enable RPB */
	atl_set_bits(hw, ATL_RX_PBUF_CTRL1, BIT(8) | BIT(4) | BIT(0));

	/* TPO */
	/* Enable L3 | L4 chksum */
	atl_set_bits(hw, ATL_TX_PO_CTRL1, 3);
	/* TSO TCP flags bitmask first / middle */
	atl_write(hw, ATL_TX_LSO_TCP_CTRL1, 0x0ff60ff6);
	/* TSO TCP flags bitmask last */
	atl_write(hw, ATL_TX_LSO_TCP_CTRL2, 0xf7f);

	/* RPO */
	/* Enable  L3 | L4 chksum */
	atl_set_bits(hw, ATL_RX_PO_CTRL1, 3);

	/* RPF */
	/* Default RPF2 parser options */
	atl_write(hw, ATL_RX_FLT_CTRL2, 0x0);
	atl_set_uc_flt(hw, 0, hw->mac_addr);
	/* BC action host */
	atl_write_bits(hw, ATL_RX_FLT_CTRL1, 12, 3, 1);
	/* Enable BC */
	atl_write_bit(hw, ATL_RX_FLT_CTRL1, 0, 1);
	/* BC thresh */
	atl_write_bits(hw, ATL_RX_FLT_CTRL1, 16, 16, 0x1000);

	/* Enable untagged packets */
	atl_write(hw, ATL_RX_VLAN_FLT_CTRL1, 1 << 2 | 1 << 3);
	atl_restore_vlan_filters(hw);

	atl_set_rss_key(hw);
	atl_set_rss_tbl(hw);
	/* Enable RSS | 8 queues per TC */
	atl_write(hw, ATL_RX_RSS_CTRL, BIT(31) | 3);

	/* Reset Rx/Tx on unexpected PERST# */
	atl_write_bit(hw, 0x1000, 29, 0);
	atl_write(hw, 0x448, 3);

	return 0;
}

void atl_stop_hw(struct atl_nic *nic)
{
	atl_intr_release(nic);
	atl_hw_reset(&nic->hw);
}

#define atl_vlan_flt_val(vid) ((uint32_t)(vid) | 1 << 16 | 1 << 31)
#define atl_set_vlan_promisc(hw, promisc)				\
	atl_write_bit((hw), ATL_RX_VLAN_FLT_CTRL1, 1, (promisc))

static inline bool atl_vlan_promisc_needed(struct atl_hw *hw)
{
	return hw->vlans_active > (ATL_VLAN_FLT_NUM - ATL_RXF_VLAN_MAX);
}

static void atl_restore_vlan_filters(struct atl_hw *hw)
{
	int i;
	int num = hw->vlans_active;
	long vid = -1;
	bool promisc = atl_vlan_promisc_needed(hw);

	atl_set_vlan_promisc(hw, promisc);
	if (promisc)
		return;

	for (i = ATL_RXF_VLAN_MAX; i < num + ATL_RXF_VLAN_MAX; i++) {
		vid = find_next_bit(hw->vlan_map, BIT(12), vid + 1);
		atl_write(hw, ATL_RX_VLAN_FLT(i),
			atl_vlan_flt_val(vid));
	}

	for (; i < ATL_VLAN_FLT_NUM; i++)
		atl_write(hw, ATL_RX_VLAN_FLT(i), 0);
}

int atl_vlan_rx_add_vid(struct net_device *ndev, __be16 proto, u16 vid)
{
	struct atl_nic *nic = netdev_priv(ndev);
	struct atl_hw *hw = &nic->hw;

	atl_dev_dbg("Add vlan id %hd\n", vid);

	vid &= 0xfff;
	if (!__test_and_set_bit(vid, hw->vlan_map)) {
		hw->vlans_active++;
		atl_restore_vlan_filters(hw);
	}

	return 0;
}

int atl_vlan_rx_kill_vid(struct net_device *ndev, __be16 proto, u16 vid)
{
	struct atl_nic *nic = netdev_priv(ndev);
	struct atl_hw *hw = &nic->hw;

	atl_dev_dbg("Kill vlan id %hd\n", vid);

	vid &= 0xfff;
	if (!__test_and_clear_bit(vid, hw->vlan_map))
		return -EINVAL;

	hw->vlans_active--;
	atl_restore_vlan_filters(hw);

	return 0;
}

static void atl_set_all_multi(struct atl_hw *hw, bool all_multi)
{
	atl_write_bit(hw, ATL_RX_MC_FLT_MSK, 14, all_multi);
	atl_write(hw, ATL_RX_MC_FLT(0), all_multi ? 0x80010000 : 0);
}

void atl_set_rx_mode(struct net_device *ndev)
{
	struct atl_nic *nic = netdev_priv(ndev);
	struct atl_hw *hw = &nic->hw;
	int uc_count = netdev_uc_count(ndev), mc_count = netdev_mc_count(ndev);
	int promisc_needed = !!(ndev->flags & IFF_PROMISC);
	int all_multi_needed = !!(ndev->flags & IFF_ALLMULTI);
	int i = 1; /* UC filter 0 reserved for MAC address */
	struct netdev_hw_addr *hwaddr;

	if (uc_count > ATL_UC_FLT_NUM - 1)
		promisc_needed |= 1;
	else if (uc_count + mc_count > ATL_UC_FLT_NUM - 1)
		all_multi_needed |= 1;


	/* Enable promisc VLAN mode iff IFF_PROMISC explicitly
	 * requested or too many VIDs registered
	 */
	atl_set_vlan_promisc(hw,
		ndev->flags & IFF_PROMISC || atl_vlan_promisc_needed(hw));

	atl_write_bit(hw, ATL_RX_FLT_CTRL1, 3, promisc_needed);
	if (promisc_needed)
		return;

	netdev_for_each_uc_addr(hwaddr, ndev)
		atl_set_uc_flt(hw, i++, hwaddr->addr);

	atl_set_all_multi(hw, all_multi_needed);

	if (!all_multi_needed)
		netdev_for_each_mc_addr(hwaddr, ndev)
			atl_set_uc_flt(hw, i++, hwaddr->addr);

	while (i < ATL_UC_FLT_NUM)
		atl_disable_uc_flt(hw, i++);
}

int atl_alloc_descs(struct atl_nic *nic, struct atl_hw_ring *ring)
{
	struct device *dev = &nic->hw.pdev->dev;

	ring->descs = dma_alloc_coherent(dev, ring->size * sizeof(*ring->descs),
					 &ring->daddr, GFP_KERNEL);

	if (!ring->descs)
		return -ENOMEM;

	return 0;
}

void atl_free_descs(struct atl_nic *nic, struct atl_hw_ring *ring)
{
	struct device *dev = &nic->hw.pdev->dev;

	if (!ring->descs)
		return;

	dma_free_coherent(dev, ring->size * sizeof(*ring->descs),
		ring->descs, ring->daddr);
	ring->descs = 0;
}

void atl_set_intr_bits(struct atl_hw *hw, int idx, int rxbit, int txbit)
{
	int shift = idx & 1 ? 0 : 8;
	uint32_t clear_mask = 0;
	uint32_t set_mask = 0;
	uint32_t val;

	if (rxbit >= 0) {
		clear_mask |= BIT(7) | (BIT(5) - 1);
		if (rxbit < ATL_NUM_MSI_VECS)
			set_mask |= BIT(7) | rxbit;
	}
	if (txbit >= 0) {
		clear_mask |= (BIT(7) | (BIT(5) - 1)) << 0x10;
		if (txbit < ATL_NUM_MSI_VECS)
			set_mask |= (BIT(7) | txbit) << 0x10;
	}

	val = atl_read(hw, ATL_INTR_RING_INTR_MAP(idx));
	val &= ~(clear_mask << shift);
	val |= set_mask << shift;
	atl_write(hw, ATL_INTR_RING_INTR_MAP(idx), val);
}

void atl_set_loopback(struct atl_nic *nic, int idx, bool on)
{
	struct atl_hw *hw = &nic->hw;

	switch (idx) {
	case ATL_PF_LPB_SYS_DMA:
		atl_write_bit(hw, ATL_TX_CTRL1, 6, on);
		atl_write_bit(hw, ATL_RX_CTRL1, 6, on);
		break;
	case ATL_PF_LPB_SYS_PB:
		atl_write_bit(hw, ATL_TX_CTRL1, 7, on);
		atl_write_bit(hw, ATL_RX_CTRL1, 8, on);
		break;
	/* case ATL_PF_LPB_NET_DMA: */
	/* 	atl_write_bit(hw, ATL_TX_CTRL1, 4, on); */
	/* 	atl_write_bit(hw, ATL_RX_CTRL1, 4, on); */
	/* 	break; */
	}
}

void atl_update_ntuple_flt(struct atl_nic *nic, int idx)
{
	struct atl_hw *hw = &nic->hw;
	struct atl_rxf_ntuple *ntuple = &nic->rxf_ntuple;
	uint32_t cmd = ntuple->cmd[idx];
	int i, len = 1;

	if (!(cmd & ATL_NTC_EN)) {
		atl_write(hw, ATL_NTUPLE_CTRL(idx), cmd);
		return;
	}

	if (cmd & ATL_NTC_V6)
		len = 4;

	for (i = idx; i < idx + len; i++) {
		if (cmd & ATL_NTC_SA)
			atl_write(hw, ATL_NTUPLE_SADDR(i),
				swab32(ntuple->src_ip4[i]));

		if (cmd & ATL_NTC_DA)
			atl_write(hw, ATL_NTUPLE_DADDR(i),
				swab32(ntuple->dst_ip4[i]));
	}

	if (cmd & ATL_NTC_SP)
		atl_write(hw, ATL_NTUPLE_SPORT(idx),
			swab16(ntuple->src_port[idx]));

	if (cmd & ATL_NTC_DP)
		atl_write(hw, ATL_NTUPLE_DPORT(idx),
			swab16(ntuple->dst_port[idx]));

	if (cmd & ATL_NTC_RXQ)
		cmd |= 1 << ATL_NTC_ACT_SHIFT;

	atl_write(hw, ATL_NTUPLE_CTRL(idx), cmd);
}

int atl_hwsem_get(struct atl_hw *hw, int idx)
{
	uint32_t val;

	busy_wait(10000, udelay(1), val, atl_read(hw, ATL_MCP_SEM(idx)), !val);

	if (!val)
		return -ETIME;

	return 0;
}

void atl_hwsem_put(struct atl_hw *hw, int idx)
{
	atl_write(hw, ATL_MCP_SEM(idx), 1);
}

static int atl_msm_wait(struct atl_hw *hw)
{
	uint32_t val;

	busy_wait(10, udelay(1), val, atl_read(hw, ATL_MPI_MSM_ADDR),
		val & BIT(12));
	if (val & BIT(12))
		return -ETIME;

	return 0;
}

int __atl_msm_read(struct atl_hw *hw, uint32_t addr, uint32_t *val)
{
	int ret;

	ret = atl_msm_wait(hw);
	if (ret)
		return ret;

	atl_write(hw, ATL_MPI_MSM_ADDR, (addr >> 2) | BIT(9));
	ret = atl_msm_wait(hw);
	if (ret)
		return ret;

	*val = atl_read(hw, ATL_MPI_MSM_RD);
	return 0;
}

int atl_msm_read(struct atl_hw *hw, uint32_t addr, uint32_t *val)
{
	int ret;

	ret = atl_hwsem_get(hw, ATL_MCP_SEM_MSM);
	if (ret)
		return -1;

	ret = __atl_msm_read(hw, addr, val);
	atl_hwsem_put(hw, ATL_MCP_SEM_MSM);

	return ret;
}

int __atl_msm_write(struct atl_hw *hw, uint32_t addr, uint32_t val)
{
	int ret;

	ret = atl_msm_wait(hw);
	if (ret)
		return ret;

	atl_write(hw, ATL_MPI_MSM_WR, val);
	atl_write(hw, ATL_MPI_MSM_ADDR, addr | BIT(8));
	ret = atl_msm_wait(hw);
	if (ret)
		return ret;

	return 0;
}

int atl_msm_write(struct atl_hw *hw, uint32_t addr, uint32_t val)
{
	int ret;

	ret = atl_hwsem_get(hw, ATL_MCP_SEM_MSM);
	if (ret)
		return ret;

	ret = __atl_msm_write(hw, addr, val);
	atl_hwsem_put(hw, ATL_MCP_SEM_MSM);

	return ret;
}

int atl_update_msm_stats(struct atl_nic *nic)
{
	struct atl_hw *hw = &nic->hw;
	struct atl_msm_stats stats = {0};
	int ret;

	ret = atl_hwsem_get(hw, ATL_MCP_SEM_MSM);
	if (ret)
		return ret;

	ret = __atl_msm_read(hw, ATL_MSM_CTR_TX_PAUSE, &stats.tx_pause);
	if (ret)
		goto hwsem_put;

	ret = __atl_msm_read(hw, ATL_MSM_CTR_RX_PAUSE, &stats.rx_pause);
	if (ret)
		goto hwsem_put;

	nic->stats.msm = stats;
	ret = 0;

hwsem_put:
	atl_hwsem_put(hw, ATL_MCP_SEM_MSM);
	return ret;
}
