/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: ISC
 */

#include "dp_dal_tx.h"
#include "hal_tx.h"
#include "qdf_mem.h"

/**
 *dp_dal_tx_cmp_isr_vendor_cb - tx cmpl ISR vendor callback
 *@ring_num: tx completion ring number
 *@priv: pointer to dp dal context
 *
 * Return: 0 on success
 */
int dp_dal_tx_cmp_isr_vendor_cb(int ring_num, void *priv)
{
	struct dp_dal_ctx *dal_ctx = (struct dp_dal_ctx *)priv;
	struct dp_soc *soc;
	int grp_id;
	QDF_STATUS status;

	if (!dal_ctx) {
		dp_err("DAL context is NULL");
		return -EINVAL;
	}

	soc = dal_ctx->soc;
	if (!soc) {
		dp_err("SOC is NULL");
		return -EINVAL;
	}

	grp_id = dp_dal_get_ext_grp_id(dal_ctx, ring_num, COMP_RING_TYPE);
	if (grp_id >= HIF_MAX_GROUP) {
		dp_err("invalid group id:%d ring_num:%d ring_type:%s",
		       grp_id, ring_num, "COMP_RING_TYPE");
		QDF_BUG(0);
		return -EINVAL;
	}

	status = hif_ext_grp_napi_schedule(soc->hif_handle, grp_id);
	if (status != QDF_STATUS_SUCCESS) {
		dp_err("Failed to sched NAPI for grp_id:%d ring:%d status:%d",
		       grp_id, ring_num, status);
		return qdf_status_to_os_return(status);
	}

	return 0;
}

/**
 * dp_dal_tx_bypass_mode() - Skeleton for platform bus tx in bypass mode
 *
 * @priv: private data
 * @pkt: tx packet
 * @ifidx: interface index
 * @desc: TX descriptor
 *
 * Return: 0 on success
 */
int dp_dal_tx_bypass_mode(void *priv, void *pkt, u32 ifidx, void *desc)
{
	return 0;
}

/**
 * dp_dal_tx_cpl_bypass_mode() - Skeleton for platform bus tx completion
 * handler in bypass mode
 *
 * @priv: private data
 * @cnt: packet count
 *
 * Return: true on success
 */
bool dp_dal_tx_cpl_bypass_mode(void *priv, u32 *cnt)
{
	return true;
}

/**
 * dp_dal_tx_queue_active_bypass_mode() - Skeleton for platform tx queue active
 * in bypass mode
 *
 * @priv: private data
 * @flowid: flow id
 * @enable: enable or not
 *
 * Return: 0 on success
 */
int dp_dal_tx_queue_active_bypass_mode(void *priv, u16 flowid, bool enable)
{
	return 0;
}

/**
 * dp_dal_tx_hw_enqueue - DAL layer's HW enqueue function for TX packets
 * @soc: DP SOC context.
 * @vdev: DP VDEV context.
 * @tx_desc: TX descriptor for the packet.
 * @fw_metadata: Firmware metadata associated with the packet.
 * @metadata: Exception metadata for TX path.
 * @msdu_info: MSDU information for the packet.
 *
 * This function implements the DAL layer's hardware enqueue logic.
 *
 * Return: QDF_STATUS_SUCCESS on success, error code on failure.
 */
QDF_STATUS dp_dal_tx_hw_enqueue(struct dp_soc *soc,
				struct dp_vdev *vdev,
				struct dp_tx_desc_s *tx_desc,
				uint16_t fw_metadata,
				struct cdp_tx_exception_metadata *metadata,
				struct dp_tx_msdu_info_s *msdu_info)
{
	struct dp_dal_ctx *dal_ctx;
	void *tcl_desc = NULL;
	qdf_nbuf_t nbuf;
	uint8_t vdev_id;
	int ret;
	QDF_STATUS status;

	dal_ctx = soc->dal_ctx;
	if (qdf_unlikely(!dal_ctx)) {
		dp_tx_err_rl("DAL context is NULL");
		return QDF_STATUS_E_NULL_VALUE;
	}

	nbuf = tx_desc->nbuf;
	vdev_id = tx_desc->vdev_id;

	if (qdf_unlikely(!nbuf)) {
		dp_tx_err_rl("nbuf is NULL");
		return QDF_STATUS_E_NULL_VALUE;
	}

	tcl_desc = qdf_mem_malloc(HAL_TX_DESC_LEN_BYTES);
	if (qdf_unlikely(!tcl_desc)) {
		dp_tx_err_rl("Failed to allocate TCL descriptor memory");
		return QDF_STATUS_E_NOMEM;
	}

	/* Generate TCL command via tx_gen_hw_desc soc_ops */
	status = soc->arch_ops.dp_tx_gen_hw_desc(soc, vdev, tx_desc,
						 fw_metadata, metadata,
						 msdu_info, tcl_desc);
	if (qdf_unlikely(QDF_IS_STATUS_ERROR(status))) {
		dp_tx_err_rl("Failed to generate HW descriptor, status: %d",
			     status);
		goto err_free_desc;
	}

	if (global_plat_ops && global_plat_ops->tx) {
		ret = global_plat_ops->tx(dal_ctx, nbuf, vdev_id, tcl_desc);
		if (qdf_unlikely(ret)) {
			dp_tx_err_rl("platform tx failed, ret: %d", ret);
			status = QDF_STATUS_E_FAILURE;
			goto err_free_desc;
		}
	} else {
		dp_tx_err_rl("Platform TX operation not available");
		status = QDF_STATUS_E_NOSUPPORT;
		goto err_free_desc;
	}

	tx_desc->flags |= DP_TX_DESC_FLAG_QUEUED_TX;
	status = QDF_STATUS_SUCCESS;

err_free_desc:
	qdf_mem_free(tcl_desc);
	return status;
}
