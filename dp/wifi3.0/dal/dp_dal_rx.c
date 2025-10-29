/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: ISC
 */

#include "dp_dal_rx.h"
#include "dp_rx.h"
#include "dp_dal.h"

/**
 *dp_dal_rx_isr_vendor_cb - rx ISR vendor callback
 *@ring_num: rx ring number
 *@priv: pointer to dp dal context
 *
 * Return: 0 on success
 */
int dp_dal_rx_isr_vendor_cb(int ring_num, void *priv)
{
	struct dp_dal_ctx *dal_ctx = (struct dp_dal_ctx *)priv;
	struct dp_soc *soc;
	int grp_id;

	if (!dal_ctx) {
		dp_err("dal_ctx is NULL");
		return -EINVAL;
	}

	soc = dal_ctx->soc;
	if (!soc) {
		dp_err("soc is NULL");
		return -EINVAL;
	}

	grp_id = dp_dal_get_ext_grp_id(dal_ctx, ring_num, REO_DST);
	if (grp_id >= HIF_MAX_GROUP) {
		dp_err("invalid group id:%d ring_num:%d ring_type:%s",
		       grp_id, ring_num, " REO_DEST");
		QDF_BUG(0);
		return -EINVAL;
	}

	hif_ext_grp_napi_schedule(soc->hif_handle, grp_id);

	return 0;
}

/**
 * dp_dal_rx_bypass_mode() - Skeleton for platform bus rx in bypass mode
 *
 * @priv: private data
 * @cnt: count
 *
 * Return: false
 */
bool dp_dal_rx_bypass_mode(void *priv, u32 *cnt)
{
	return false;
}

/**
 * dp_dal_rx_replenish_bypass_mode() - replenish rx buffers
 * in bypass mode
 *
 * @priv: private data
 * @cnt: count
 * @use_rsv_pktid: use reserved packet id
 *
 * Return: 0 on success
 */
int dp_dal_rx_replenish_bypass_mode(void *priv, u32 cnt, bool use_rsv_pktid)
{
	struct dp_dal_ctx *dal_ctx = (struct dp_dal_ctx *)priv;
	struct dp_soc *soc;
	struct rx_desc_pool *rx_desc_pool;
	union dp_rx_desc_list_elem_t *desc_list_head = NULL;
	union dp_rx_desc_list_elem_t *desc_list_tail = NULL;
	int mac_id = 0;

	if (!dal_ctx) {
		dp_err("DAL context is NULL");
		return -EINVAL;
	}

	soc = dal_ctx->soc;
	if (!soc) {
		dp_err("SOC is NULL in DAL context");
		return -EINVAL;
	}

	rx_desc_pool = &soc->rx_desc_buf[mac_id];
	dp_rx_buffers_replenish(soc, mac_id,
				&soc->rx_refill_buf_ring[mac_id],
				rx_desc_pool, cnt,
				&desc_list_head, &desc_list_tail, true);

	return 0;
}

/**
 * dp_dal_rx_rxbm_sync_bypass_mode() - Skeleton for platform bus rxbm sync
 * in bypass mode
 *
 * @priv: private data
 * @cnt: count
 * @rxbm: rxbm
 *
 * Return: 0 on success
 */
int dp_dal_rx_rxbm_sync_bypass_mode(void *priv, u32 cnt, void **rxbm)
{
	return 0;
}

/**
 * dp_dal_rx_process_nbuf_list - Process a list of qdf_nbufs for RX path.
 * @soc: DP SOC context.
 * @nbuf_list: Head of the qdf_nbuf list to be processed.
 *
 * This is a placeholder (skeleton) implementation that iterates over the
 * provided nbuf list. The actual processing logic should be added by the
 * driver developer.
 *
 * Return: %QDF_STATUS_SUCCESS on success, or an appropriate error code.
 */
QDF_STATUS dp_dal_rx_process_nbuf_list(struct dp_soc *soc,
				       qdf_nbuf_t nbuf_list)
{
	/* TODO: Implement RX processing of the nbuf list */
	return QDF_STATUS_SUCCESS;
}
