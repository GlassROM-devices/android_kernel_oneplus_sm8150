/*
 * aQuantia Corporation Network Driver
 * Copyright (C) 2019 aQuantia Corporation. All rights reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 */

#include "atl_common.h"
#ifdef NETIF_F_HW_MACSEC
#include <net/macsec.h>

#include "macsec/macsec_api.h"
#define ATL_MACSEC_KEY_LEN_128_BIT 16
#define ATL_MACSEC_KEY_LEN_192_BIT 24
#define ATL_MACSEC_KEY_LEN_256_BIT 32

static void ether_addr_to_mac(uint32_t mac[2], unsigned char *emac)
{
	uint32_t tmp[2] = {0};

	memcpy(((uint8_t*)tmp) + 2, emac, ETH_ALEN);

	mac[0] = swab32(tmp[1]);
	mac[1] = swab32(tmp[0]);
}

int atl_get_sc_idx_from_secy(struct atl_hw *hw,
			     const struct macsec_secy *secy)
{
	int i;

	for (i = 0; i < ATL_MACSEC_MAX_SECY; i++) {
		if (hw->macsec_cfg.secys[i].secy == secy) {
			return i;
			break;
		}
	}
	return -1;
}

/* Rotate keys uint32_t[8] */
static void atl_rotate_keys(uint32_t (*key)[8], int key_len)
{
	uint32_t tmp[8] = {0};

	memcpy(&tmp, key, sizeof(tmp));
	memset(*key, 0, sizeof(*key));

	if (key_len == ATL_MACSEC_KEY_LEN_128_BIT) {
		(*key)[0] = swab32(tmp[3]);
		(*key)[1] = swab32(tmp[2]);
		(*key)[2] = swab32(tmp[1]);
		(*key)[3] = swab32(tmp[0]);
	} else if (key_len == ATL_MACSEC_KEY_LEN_192_BIT) {
		(*key)[0] = swab32(tmp[5]);
		(*key)[1] = swab32(tmp[4]);
		(*key)[2] = swab32(tmp[3]);
		(*key)[3] = swab32(tmp[0]);
		(*key)[4] = swab32(tmp[1]);
		(*key)[5] = swab32(tmp[0]);
	} else if (key_len == ATL_MACSEC_KEY_LEN_256_BIT) {
		(*key)[0] = swab32(tmp[7]);
		(*key)[1] = swab32(tmp[6]);
		(*key)[2] = swab32(tmp[5]);
		(*key)[3] = swab32(tmp[4]);
		(*key)[4] = swab32(tmp[3]);
		(*key)[5] = swab32(tmp[2]);
		(*key)[6] = swab32(tmp[1]);
		(*key)[7] = swab32(tmp[0]);
	} else {
		pr_warn("Rotate_keys: invalid key_len\n");
	}

}

static unsigned int atl_macsec_sa_idx(enum ast_macsec_sc_sa sc_sa, int sc_idx,
				      int an)
{
	switch(sc_sa) {
	case atl_macses_sa_sc_4sa_8sc:
		return sc_idx << 2 | an;
	case atl_macses_sa_sc_2sa_16sc:
		return sc_idx << 1 | an;
	case atl_macses_sa_sc_1sa_32sc:
		return sc_idx << 0 | an;
	default:
		WARN_ONCE(1, "Invalid sc_sa");
	}
	return an;
}

static int atl_macsec_apply_cfg(struct atl_hw *hw);

int atl_init_macsec(struct atl_hw *hw)
{
	struct macsec_msg_fw_request msg = { 0 };
	struct macsec_msg_fw_response resp = { 0 };
	int num_ctl_ether_types = 0;
	int index = 0, tbl_idx;
	int ret;

	if (hw->mcp.ops->send_macsec_req != NULL) {
		struct macsec_cfg cfg = { 0 };

		cfg.enabled = 1;
		cfg.egress_threshold = 0xffffffff;
		cfg.ingress_threshold = 0xffffffff;
		cfg.interrupts_enabled = 1;

		msg.msg_type = macsec_cfg_msg;
		msg.cfg = cfg;

		ret = hw->mcp.ops->send_macsec_req(hw, &msg, &resp);
	}


	/* Init Ethertype bypass filters */
	uint32_t ctl_ether_types[1] = { 0x888e };
	for (index = 0; index < ARRAY_SIZE(ctl_ether_types); index++) {
		if (ctl_ether_types[index] == 0) 
			continue;
		AQ_API_SEC_EgressCTLFRecord egressCTLFRecord = {0};
		egressCTLFRecord.eth_type = ctl_ether_types[index];
		egressCTLFRecord.match_type = 4; /* Match eth_type only */
		egressCTLFRecord.match_mask = 0xf; /* match for eth_type */
		egressCTLFRecord.action = 0; /* Bypass MACSEC modules */
		tbl_idx = NUMROWS_EGRESSCTLFRECORD - num_ctl_ether_types - 1;
		AQ_API_SetEgressCTLFRecord(hw, &egressCTLFRecord, tbl_idx);

		AQ_API_SEC_IngressPreCTLFRecord ingressPreCTLFRecord = {0};
		ingressPreCTLFRecord.eth_type = ctl_ether_types[index];
		ingressPreCTLFRecord.match_type = 4; /* Match eth_type only */
		ingressPreCTLFRecord.match_mask = 0xf; /* match for eth_type */
		ingressPreCTLFRecord.action = 0; /* Bypass MACSEC modules */
		tbl_idx = NUMROWS_INGRESSPRECTLFRECORD - num_ctl_ether_types - 1;
		AQ_API_SetIngressPreCTLFRecord(hw, &ingressPreCTLFRecord,
					       tbl_idx);

		num_ctl_ether_types++;
	}

	return atl_macsec_apply_cfg(hw);
}

static int atl_macsec_apply_secy_cfg(struct atl_hw *hw, int sc_idx);

static int atl_mdo_dev_open(struct macsec_context *ctx)
{
	struct atl_nic *nic = netdev_priv(ctx->netdev);
	int sc_idx = atl_get_sc_idx_from_secy(&nic->hw, ctx->secy);
	int ret = 0;

	if (ctx->prepare)
		return 0;

	if (netif_carrier_ok(nic->ndev))
		ret = atl_macsec_apply_secy_cfg(&nic->hw, sc_idx);

	return ret;
}

static int atl_mdo_dev_stop(struct macsec_context *ctx)
{
	struct atl_nic *nic = netdev_priv(ctx->netdev);
	int sc_idx = atl_get_sc_idx_from_secy(&nic->hw, ctx->secy);
	struct atl_hw *hw = &nic->hw;
	int ret = 0;

	AQ_API_SEC_EgressSCRecord matchSCRecord = {0};

	matchSCRecord.fresh = 1;
	ret = AQ_API_SetEgressSCRecord(hw, &matchSCRecord, sc_idx);

	return ret;
}

static int atl_update_secy(struct atl_hw *hw, int sc_idx)
{
	const struct macsec_secy *secy = hw->macsec_cfg.secys[sc_idx].secy;
	int ret = 0;

	AQ_API_SEC_EgressClassRecord matchEgressClassRecord = {0};

	ether_addr_to_mac(matchEgressClassRecord.mac_sa,
			  secy->netdev->dev_addr);

	dev_dbg(&hw->pdev->dev, "set secy: sci %#llx, sc_idx=%d, protect=%d, curr_an=%d \n",
		secy->sci, sc_idx, secy->protect_frames,
		secy->tx_sc.encoding_sa);

	matchEgressClassRecord.sci[1] = swab32(secy->sci & 0xffffffff);
	matchEgressClassRecord.sci[0] = swab32(secy->sci >> 32);
	matchEgressClassRecord.sci_mask = 0;

	matchEgressClassRecord.sa_mask = 0x3f;

	matchEgressClassRecord.action = 0; /* forward to SA/SC table */
	matchEgressClassRecord.valid = 1;

	matchEgressClassRecord.sc_idx = sc_idx;

	matchEgressClassRecord.sc_sa = hw->macsec_cfg.sc_sa;

	ret = AQ_API_SetEgressClassRecord(hw, &matchEgressClassRecord, sc_idx);
	if (ret)
		return ret;

	AQ_API_SEC_EgressSCRecord matchSCRecord = {0};

	matchSCRecord.protect = secy->protect_frames;
	if (secy->tx_sc.encrypt)
		matchSCRecord.tci |=  BIT(1);
	if (secy->tx_sc.scb)
		matchSCRecord.tci |=  BIT(2);
	if (secy->tx_sc.send_sci)
		matchSCRecord.tci |=  BIT(3);
	if (secy->tx_sc.end_station)
		matchSCRecord.tci |=  BIT(4);
	/* The C bit is clear if and only if the Secure Data is
	 * exactly the same as the User Data and the ICV is 16 octets long.
	 */
	if (!(secy->icv_len == 16 && !secy->tx_sc.encrypt))
		matchSCRecord.tci |=  BIT(0);

	matchSCRecord.an_roll = 0;

	switch (secy->key_len) {
	case 16:
		matchSCRecord.sak_len = 0;
		break;
	case 24:
		matchSCRecord.sak_len = 1;
		break;
	case 32:
		matchSCRecord.sak_len = 2;
		break;
	default:
		return -EINVAL;
	}

	matchSCRecord.curr_an = secy->tx_sc.encoding_sa;
	matchSCRecord.valid = 1;
	matchSCRecord.fresh = 1;

	return AQ_API_SetEgressSCRecord(hw, &matchSCRecord, sc_idx);
}

static int atl_mdo_add_secy(struct macsec_context *ctx)
{
	struct atl_nic *nic = netdev_priv(ctx->netdev);
	const struct macsec_secy *secy = ctx->secy;
	uint32_t sc_idx_max = ATL_MACSEC_MAX_SECY;
	struct atl_hw *hw = &nic->hw;
	uint32_t sc_idx;
	int ret = 0;

	switch(MACSEC_NUM_AN){
		case 4:
			hw->macsec_cfg.sc_sa = atl_macses_sa_sc_4sa_8sc;
			sc_idx_max = 8;
			break;
		case 2:
			hw->macsec_cfg.sc_sa = atl_macses_sa_sc_2sa_16sc;
			sc_idx_max = 16;
			break;
		case 1:
			hw->macsec_cfg.sc_sa = atl_macses_sa_sc_1sa_32sc;
			sc_idx_max = 32;
			break;
		default:
			return -EINVAL;
			break;
	}

	if (hweight32(hw->macsec_cfg.sc_idx_busy) >= sc_idx_max)
		return -ENOSPC;

	sc_idx = ffz (hw->macsec_cfg.sc_idx_busy);
	if (sc_idx == ATL_MACSEC_MAX_SECY)
		return -ENOSPC;

	if (ctx->prepare)
		return 0;

	hw->macsec_cfg.secys[sc_idx].sc_idx = sc_idx;
	hw->macsec_cfg.secys[sc_idx].secy = secy;
	dev_dbg(&hw->pdev->dev, "add secy: sc_idx=%d\n", sc_idx);

	if (netif_carrier_ok(nic->ndev) && netif_running(secy->netdev))
		ret = atl_update_secy(hw, sc_idx);

	set_bit(sc_idx, &hw->macsec_cfg.sc_idx_busy);

	return 0;
}

static int atl_mdo_upd_secy(struct macsec_context *ctx)
{
	struct atl_nic *nic = netdev_priv(ctx->netdev);
	int sc_idx = atl_get_sc_idx_from_secy(&nic->hw, ctx->secy);
	struct atl_hw *hw = &nic->hw;
	const struct macsec_secy *secy = hw->macsec_cfg.secys[sc_idx].secy;
	int ret = 0;

	if (sc_idx < 0)
		return -ENOENT;

	if (ctx->prepare)
		return 0;

	if (netif_carrier_ok(nic->ndev) && netif_running(secy->netdev))
		ret = atl_update_secy(hw, sc_idx);

	return ret;
}

static int atl_mdo_del_secy(struct macsec_context *ctx)
{
	struct atl_nic *nic = netdev_priv(ctx->netdev);
	int sc_idx = atl_get_sc_idx_from_secy(&nic->hw, ctx->secy);
	struct atl_hw *hw = &nic->hw;
	int ret = 0;

	if (ctx->prepare)
		return 0;

	clear_bit(sc_idx, &hw->macsec_cfg.sc_idx_busy);
	hw->macsec_cfg.secys[sc_idx].secy = NULL;

	if (netif_carrier_ok(nic->ndev)) {
		AQ_API_SEC_EgressSCRecord matchSCRecord = {0};

		matchSCRecord.fresh = 1;
		ret = AQ_API_SetEgressSCRecord(hw, &matchSCRecord, sc_idx);
	}
	return ret;
}

static int atl_update_txsa(struct atl_hw *hw,
			   const struct macsec_secy *secy,
			   const struct macsec_tx_sa *tx_sa,
			   const unsigned char *key,
			   int an)
{
	int sc_idx = atl_get_sc_idx_from_secy(hw, secy);
	int ret = 0;
	int sa_idx;

	dev_dbg(&hw->pdev->dev, "set tx_sa %d: active=%d, next_pn=%d \n", an,
			tx_sa->active, tx_sa->next_pn);

 	sa_idx = atl_macsec_sa_idx(hw->macsec_cfg.sc_sa, sc_idx, an);

	AQ_API_SEC_EgressSARecord matchSARecord = {0};
	matchSARecord.valid = tx_sa->active;
	matchSARecord.fresh = 1;
	matchSARecord.next_pn = tx_sa->next_pn;

	ret = AQ_API_SetEgressSARecord(hw, &matchSARecord, sa_idx);
	if (ret) {
		dev_err(&hw->pdev->dev,
			"AQ_API_SetEgressSARecord failed with %d\n", ret);
		return ret;
	}

	AQ_API_SEC_EgressSAKeyRecord matchKeyRecord = {0};
	memcpy(&matchKeyRecord.key, key, secy->key_len);

	atl_rotate_keys(&matchKeyRecord.key, secy->key_len);

	ret = AQ_API_SetEgressSAKeyRecord(hw, &matchKeyRecord, sa_idx);
	if (ret)
		dev_err(&hw->pdev->dev,
			"AQ_API_SetEgressSAKeyRecord failed with %d\n", ret);

	return ret;

}

static int atl_mdo_add_txsa(struct macsec_context *ctx)
{
	struct atl_nic *nic = netdev_priv(ctx->netdev);
	int sc_idx = atl_get_sc_idx_from_secy(&nic->hw, ctx->secy);
	const struct macsec_secy *secy = ctx->secy;

	if (ctx->prepare)
		return 0;

	memcpy(nic->hw.macsec_cfg.secys[sc_idx].tx_sa_key[ctx->sa.assoc_num],
	       ctx->sa.key, secy->key_len);

	if (netif_carrier_ok(nic->ndev) && netif_running(secy->netdev))
		return atl_update_txsa(&nic->hw, secy,
				       ctx->sa.tx_sa,
				       ctx->sa.key,
				       ctx->sa.assoc_num);

	return 0;
}

static int atl_mdo_upd_txsa(struct macsec_context *ctx)
{
	struct atl_nic *nic = netdev_priv(ctx->netdev);
	int sc_idx = atl_get_sc_idx_from_secy(&nic->hw, ctx->secy);
	const struct macsec_secy *secy = ctx->secy;

	if (ctx->prepare)
		return 0;

	memcpy(nic->hw.macsec_cfg.secys[sc_idx].tx_sa_key[ctx->sa.assoc_num],
	       ctx->sa.key, secy->key_len);

	if (netif_carrier_ok(nic->ndev) && netif_running(secy->netdev))
		return atl_update_txsa(&nic->hw, secy,
				       ctx->sa.tx_sa,
				       ctx->sa.key,
				       ctx->sa.assoc_num);

	return 0;
}

static int atl_mdo_del_txsa(struct macsec_context *ctx)
{
	struct atl_nic *nic = netdev_priv(ctx->netdev);
	struct atl_hw *hw = &nic->hw;
	int ret = 0;

	if (ctx->prepare)
		return 0;

	if (netif_carrier_ok(nic->ndev)) {
		AQ_API_SEC_EgressSARecord matchSARecord = {0};
		matchSARecord.fresh = 1;

		ret = AQ_API_SetEgressSARecord(hw, &matchSARecord,
					       ctx->sa.assoc_num);
		if (ret)
			return ret;

		AQ_API_SEC_EgressSAKeyRecord matchKeyRecord = {0};

		return AQ_API_SetEgressSAKeyRecord(hw, &matchKeyRecord,
						   ctx->sa.assoc_num);
	}

	return 0;
}

static int atl_mdo_add_rxsc(struct macsec_context *ctx)
{
	pr_info("%s %s\n", __FUNCTION__, ctx->prepare?"prepare":"do");
	return 0;
}

static int atl_mdo_upd_rxsc(struct macsec_context *ctx)
{
	pr_info("%s %s\n", __FUNCTION__, ctx->prepare?"prepare":"do");
	return 0;
}

static int atl_mdo_del_rxsc(struct macsec_context *ctx)
{
	pr_info("%s %s\n", __FUNCTION__, ctx->prepare?"prepare":"do");
	return 0;
}

static int atl_mdo_add_rxsa(struct macsec_context *ctx)
{
	pr_info("%s %s\n", __FUNCTION__, ctx->prepare?"prepare":"do");
	return 0;
}

static int atl_mdo_upd_rxsa(struct macsec_context *ctx)
{
	pr_info("%s %s\n", __FUNCTION__, ctx->prepare?"prepare":"do");
	return 0;
}

static int atl_mdo_del_rxsa(struct macsec_context *ctx)
{
	pr_info("%s %s\n", __FUNCTION__, ctx->prepare?"prepare":"do");
	return 0;
}

static int atl_macsec_apply_cfg(struct atl_hw *hw)
{
	int i;
	int ret = 0;
	for (i = 0; i < ATL_MACSEC_MAX_SECY; i++) {
		if (hw->macsec_cfg.sc_idx_busy & BIT(i))
			if (netif_running(hw->macsec_cfg.secys[i].secy->netdev))
				ret = atl_macsec_apply_secy_cfg(hw, i);
		if (ret)
			break;
	}

	return ret;
}

static int atl_macsec_apply_secy_cfg(struct atl_hw *hw, int sc_idx)
{
	const struct macsec_secy *secy = hw->macsec_cfg.secys[sc_idx].secy;
	int i;
	int ret = 0;

	atl_update_secy(hw, sc_idx);

	if (!netif_running(secy->netdev))
		return ret;

	for (i = 0; i < MACSEC_NUM_AN; i++) {
		if (secy->tx_sc.sa[i])
			ret =  atl_update_txsa(hw, secy,
				secy->tx_sc.sa[i],
				hw->macsec_cfg.secys[sc_idx].tx_sa_key[i],
				i);
	}

	return ret;
}

const struct macsec_ops atl_macsec_ops = {
	.mdo_dev_open = atl_mdo_dev_open,
	.mdo_dev_stop = atl_mdo_dev_stop,
	.mdo_add_secy = atl_mdo_add_secy,
	.mdo_upd_secy = atl_mdo_upd_secy,
	.mdo_del_secy = atl_mdo_del_secy,
	.mdo_add_rxsc = atl_mdo_add_rxsc,
	.mdo_upd_rxsc = atl_mdo_upd_rxsc,
	.mdo_del_rxsc = atl_mdo_del_rxsc,
	.mdo_add_rxsa = atl_mdo_add_rxsa,
	.mdo_upd_rxsa = atl_mdo_upd_rxsa,
	.mdo_del_rxsa = atl_mdo_del_rxsa,
	.mdo_add_txsa = atl_mdo_add_txsa,
	.mdo_upd_txsa = atl_mdo_upd_txsa,
	.mdo_del_txsa = atl_mdo_del_txsa,
};
#endif


