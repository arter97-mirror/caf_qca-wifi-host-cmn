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
 * dp_dal_tx_override_ring_id_bypass_mode() - Override ring ID for bypass mode
 * @soc: DP SOC handle
 * @ring_id: Pointer to ring ID to be updated
 *
 * When DAL is enabled(compile time), SW2TCL0 & SW2TCL1 is always reserved.
 * In Offload mode, these two rings are used for STA and SAP respectively,
 * but in the bypass mode, these rings are reserved/unused.
 *
 * Therefore, If ring_id is less than 2, add +2 to the ring_id.
 *
 * Return: QDF_STATUS_SUCCESS on success, QDF_STATUS_E_INVAL on error
 */
static inline QDF_STATUS
dp_dal_tx_override_ring_id_bypass_mode(struct dp_soc *soc, uint8_t *ring_id)
{
	if (qdf_unlikely(!soc || !ring_id))
		return QDF_STATUS_E_INVAL;

	/* DAL requires minimum 3 TCL rings */
	if (qdf_unlikely(soc->num_tcl_data_rings < 3))
		return QDF_STATUS_E_INVAL;

	if (*ring_id >= 2)
		return QDF_STATUS_SUCCESS;

	/* For bypass mode, if ring_id < 2, add +2 */
	*ring_id += 2;

	/* Limit it to remaining rings (2 to num_tcl_data_rings-1) */
	if (*ring_id >= soc->num_tcl_data_rings)
		*ring_id = (*ring_id % (soc->num_tcl_data_rings - 2)) + 2;

	return QDF_STATUS_SUCCESS;
}

/**
 * dp_dal_tx_bypass_mode() - Platform bus tx in bypass mode
 *
 * @priv: dal private data
 * @ring_id: ring ID for TCL descriptor enqueue
 * @ifidx: interface index (vdev_id)
 * @desc: TCL descriptor
 * @tx_metadata: pointer to dp_dal_tx_metadata structure containing MSDU info
 *
 * This function implements the TX hardware enqueue functionality for
 * bypass mode.
 *
 * Return: 0 on success, negative error code on failure
 */
int dp_dal_tx_bypass_mode(void *priv, u8 ring_id, u32 ifidx, void *desc,
			  void *tx_metadata)
{
	struct dp_dal_ctx *dal_ctx = (struct dp_dal_ctx *)priv;
	struct dp_dal_tx_metadata *metadata =
		(struct dp_dal_tx_metadata *)tx_metadata;
	struct dp_soc *soc = dal_ctx->soc;
	struct dp_vdev *vdev = metadata->vdev;
	struct dp_tx_desc_s *tx_desc = metadata->tx_desc;
	struct dp_tx_msdu_info_s *msdu_info = metadata->msdu_info;
	struct dp_tx_queue *txq = &msdu_info->tx_queue;
	uint8_t tid = msdu_info->tid;
	uint32_t *hal_tx_desc_cached = desc;
	hal_ring_handle_t hal_ring_hdl = NULL;
	void *hal_tx_desc;
	int coalesce = 0;
	uint8_t num_desc_bytes = HAL_TX_DESC_LEN_BYTES;
	uint32_t hp;
	QDF_STATUS status;

	/* Override ring ID for bypass mode */
	status = dp_dal_tx_override_ring_id_bypass_mode(soc, &txq->ring_id);
	if (qdf_unlikely(status != QDF_STATUS_SUCCESS)) {
		dp_tx_err_rl("Failed to override ring_id, status: %d", status);
		return -EINVAL;
	}

	ring_id = txq->ring_id;
	hal_ring_hdl = dp_tx_get_hal_ring_hdl(soc, ring_id);

	if (qdf_unlikely(dp_tx_hal_ring_access_start(soc, hal_ring_hdl))) {
		dp_tx_err_rl("HAL RING Access Failed -- %pK", hal_ring_hdl);
		DP_STATS_INC(soc, tx.tcl_ring_full[ring_id], 1);
		return -ENOSPC;
	}

	hal_tx_desc = hal_srng_src_get_next(soc->hal_soc, hal_ring_hdl);
	if (qdf_unlikely(!hal_tx_desc)) {
		dp_verbose_debug("TCL ring full ring_id:%d", ring_id);
		DP_STATS_INC(soc, tx.tcl_ring_full[ring_id], 1);
		goto ring_access_fail;
	}

	/* Sync cached descriptor with HW */
	soc->arch_ops.dp_tx_hw_desc_sync(hal_tx_desc_cached, hal_tx_desc,
					 num_desc_bytes);

	coalesce = dp_tx_attempt_coalescing_wrapper(soc, vdev, tx_desc, tid,
						    msdu_info, ring_id);

	if (qdf_unlikely(dp_tx_pkt_tracepoints_enabled())) {
		hp = hal_srng_src_get_hp(hal_ring_hdl);
		qdf_trace_dp_tx_enqueue(tx_desc->nbuf, hp, ring_id, coalesce);
	}

	dp_tx_update_stats(soc, tx_desc, ring_id);

	dp_tx_hw_desc_update_evt((uint8_t *)hal_tx_desc_cached,
				 hal_ring_hdl, soc, ring_id);

ring_access_fail:
	dp_tx_ring_access_end_wrapper(soc, hal_ring_hdl, coalesce);

	return (hal_tx_desc) ? 0 : -ENOSPC;
}

/**
 * dp_dal_tx_cpl_bypass_mode() - Skeleton for platform bus tx completion
 * handler in bypass mode
 *
 * @priv: private data
 * @cnt: packet count
 * @ring_id: Ring Id of the completion ring
 *
 * Return: true on success
 */
bool dp_dal_tx_cpl_bypass_mode(void *priv, u32 *cnt, u16 ring_id)
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
	struct dp_dal_tx_metadata tx_metadata = {};
	void *tcl_desc = NULL;
	qdf_nbuf_t nbuf;
	uint8_t vdev_id;
	uint8_t ring_id;
	int ret;
	QDF_STATUS status;

	dal_ctx = soc->dal_ctx;
	if (qdf_unlikely(!dal_ctx)) {
		dp_tx_err_rl("DAL context is NULL");
		return QDF_STATUS_E_NULL_VALUE;
	}

	nbuf = tx_desc->nbuf;
	vdev_id = tx_desc->vdev_id;
	ring_id = msdu_info->tx_queue.ring_id;

	if (qdf_unlikely(!nbuf)) {
		dp_tx_err_rl("nbuf is NULL");
		return QDF_STATUS_E_NULL_VALUE;
	}

	tx_metadata.msdu_info = msdu_info;
	tx_metadata.vdev = vdev;
	tx_metadata.tx_desc = tx_desc;

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

	if (!dp_tx_desc_set_ktimestamp(vdev, tx_desc))
		dp_tx_desc_set_timestamp(tx_desc);

	if (global_plat_ops->tx) {
		ret = global_plat_ops->tx(dal_ctx, ring_id, vdev_id, tcl_desc,
					  &tx_metadata);
		if (qdf_unlikely(ret)) {
			dp_tx_err_rl("platform tx failed, ret: %d", ret);
			DP_STATS_INC(vdev,
				     tx_i[msdu_info->xmit_type].dropped.enqueue_fail,
				     1);
			status = QDF_STATUS_E_FAILURE;
			goto err_free_desc;
		}
	} else {
		dp_tx_err_rl("Platform TX operation not available");
		status = QDF_STATUS_E_NOSUPPORT;
		goto err_free_desc;
	}

	/* Re-init ring_id as platform_bus_tx() might override it */
	ring_id = msdu_info->tx_queue.ring_id;

	tx_desc->flags |= DP_TX_DESC_FLAG_QUEUED_TX;
	dp_tx_update_proto_stats(vdev, tx_desc->nbuf, ring_id, TX_ENQUEUE_HW);

	DP_STATS_INC_PKT(vdev, tx_i[msdu_info->xmit_type].processed, 1,
			 dp_tx_get_pkt_len(tx_desc));
	DP_STATS_INC(soc, tx.tcl_enq[ring_id], 1);

	dp_pkt_add_timestamp(vdev, QDF_PKT_TX_DRIVER_EXIT,
			     qdf_get_log_timestamp(), tx_desc->nbuf);
	status = QDF_STATUS_SUCCESS;

err_free_desc:
	qdf_mem_free(tcl_desc);
	return status;
}

/**
 * dp_dal_tx_cpl_cb() - DAL TX completion callback handler
 * @priv: DAL context (dal_ctx)
 * @desc: TX completion descriptor
 * @ring_id: Ring ID
 *
 * This callback handler processes TX completion descriptors received from
 * the platform layer. It replicates all descriptor validation checks from
 * dp_tx_comp_handler and adds valid descriptors to a dedicated list within
 * dal_ctx for later processing.
 *
 * Return: 0 for successful processing
 */
int dp_dal_tx_cpl_cb(void *priv, void *desc, u16 ring_id)
{
	struct dp_dal_ctx *dal_ctx = (struct dp_dal_ctx *)priv;
	struct dp_soc *soc;
	void *tx_comp_hal_desc = desc;
	struct dp_tx_desc_s *tx_desc = NULL;
	uint8_t buffer_src;
	QDF_STATUS status;
	hal_soc_handle_t hal_soc;
	uint8_t tx_status;

	if (qdf_unlikely(!dal_ctx)) {
		dp_err("DAL context is NULL");
		return 0;
	}

	soc = dal_ctx->soc;
	if (qdf_unlikely(!soc)) {
		dp_err("SOC is NULL");
		return 0;
	}

	if (qdf_unlikely(!tx_comp_hal_desc)) {
		dp_err("TX completion descriptor is NULL");
		return 0;
	}

	if (qdf_unlikely(ring_id >= MAX_TCL_DATA_RINGS)) {
		dp_err("Invalid ring_id %u", ring_id);
		return 0;
	}

	hal_soc = soc->hal_soc;

	buffer_src = hal_tx_comp_get_buffer_source(hal_soc, tx_comp_hal_desc);

	/* If this buffer was not released by TQM or FW, then it is not
	 * Tx completion indication, assert.
	 */
	if (qdf_unlikely(buffer_src != HAL_TX_COMP_RELEASE_SOURCE_TQM) &&
	    (qdf_unlikely(buffer_src != HAL_TX_COMP_RELEASE_SOURCE_FW))) {
		uint8_t wbm_internal_error;

		dp_err_rl("Tx comp release_src != TQM | FW but from %d",
			  buffer_src);
		hal_dump_comp_desc(tx_comp_hal_desc);
		DP_STATS_INC(soc, tx.invalid_release_source, 1);

		/* When WBM sees NULL buffer_addr_info in any of
		 * ingress rings it sends an error indication,
		 * with wbm_internal_error=1, to a specific ring.
		 * The WBM2SW ring used to indicate these errors is
		 * fixed in HW, and that ring is being used as Tx
		 * completion ring. These errors are not related to
		 * Tx completions, and should just be ignored
		 */
		wbm_internal_error =
			hal_get_wbm_internal_error(hal_soc, tx_comp_hal_desc);

		if (wbm_internal_error) {
			dp_err_rl("Tx comp wbm_internal_error!!");
			DP_STATS_INC(soc, tx.wbm_internal_error[WBM_INT_ERROR_ALL], 1);

			if (HAL_TX_COMP_RELEASE_SOURCE_REO == buffer_src)
				dp_handle_wbm_internal_error(soc,
							     tx_comp_hal_desc,
							     hal_tx_comp_get_buffer_type(
							     tx_comp_hal_desc));
		} else {
			dp_err_rl("Tx comp wbm_internal_error false");
			DP_STATS_INC(soc, tx.non_wbm_internal_err, 1);
		}
		return 0;
	}

	status = soc->arch_ops.tx_comp_get_params_from_hal_desc(
				soc, tx_comp_hal_desc, &tx_desc);
	if (qdf_unlikely(!tx_desc)) {
		if (QDF_IS_STATUS_SUCCESS(
			dp_tx_comp_stale_entry_handle(soc, ring_id, status))) {
			return 0;
		}

		dp_err("unable to retrieve tx_desc!");
		hal_dump_comp_desc(tx_comp_hal_desc);
		DP_STATS_INC(soc, tx.invalid_tx_comp_desc, 1);
		QDF_BUG(0);
		return 0;
	}

	dp_tx_comp_reset_stale_entry_detection(soc, ring_id);
	tx_desc->buffer_src = buffer_src;
	tx_status = hal_tx_comp_get_tx_status(tx_comp_hal_desc);

	/*
	 * If the release source is FW, process the HTT status
	 */
	if (qdf_unlikely(buffer_src == HAL_TX_COMP_RELEASE_SOURCE_FW) ||
	    dp_tx_fw_release_reason(tx_status)) {
		uint8_t htt_tx_status[HAL_TX_COMP_HTT_STATUS_LEN];

		hal_tx_comp_get_htt_desc(tx_comp_hal_desc, htt_tx_status);

		if (qdf_unlikely(!tx_desc->pdev))
			dp_tx_dump_tx_desc(tx_desc);

		soc->arch_ops.dp_tx_process_htt_completion(soc, tx_desc,
							   htt_tx_status,
							   ring_id);
	} else {
		tx_desc->tx_status = tx_status;
		tx_desc->buffer_src = buffer_src;

		/*
		 * If the descriptor is already freed in vdev_detach,
		 * continue to next descriptor
		 */
		if (qdf_unlikely(tx_desc->vdev_id == DP_INVALID_VDEV_ID &&
				 !tx_desc->flags)) {
			dp_tx_comp_info_rl("Descriptor freed in vdev_detach %d",
					   tx_desc->id);
			DP_STATS_INC(soc, tx.tx_comp_exception, 1);
			dp_tx_desc_check_corruption(tx_desc);
			return 0;
		}

		if (qdf_unlikely(!tx_desc->pdev)) {
			dp_tx_comp_warn("pdev is NULL in TX desc, ignored.");
			dp_tx_dump_tx_desc(tx_desc);
			DP_STATS_INC(soc, tx.tx_comp_exception, 1);
			return 0;
		}

		if (qdf_unlikely(tx_desc->pdev->is_pdev_down)) {
			dp_tx_comp_info_rl("pdev in down state %d",
					   tx_desc->id);
			tx_desc->flags |= DP_TX_DESC_FLAG_TX_COMP_ERR;
			dp_tx_comp_free_buf(soc, tx_desc, false);
			dp_tx_desc_release(soc, tx_desc, tx_desc->pool_id);
			return 0;
		}

		if (!(tx_desc->flags & DP_TX_DESC_FLAG_ALLOCATED) ||
		    !(tx_desc->flags & DP_TX_DESC_FLAG_QUEUED_TX)) {
			dp_tx_comp_alert("Txdesc invalid, flgs = %x,id = %d",
					 tx_desc->flags, tx_desc->id);
			qdf_assert_always(0);
		}

		if (qdf_unlikely(tx_desc->flags & DP_TX_DESC_FLAG_REAPED)) {
			dp_tx_comp_alert("Txdesc duplicate entry, flags = %x,id = %d",
					 tx_desc->flags, tx_desc->id);
			qdf_assert_always(0);
		}

		tx_desc->flags |= DP_TX_DESC_FLAG_REAPED;

		hal_tx_comp_desc_sync_wrapper(tx_comp_hal_desc, NULL,
					      tx_desc, buffer_src, 0, 1);

		/* Add valid descriptor to the dedicated list for processing */
		qdf_spin_lock_bh(&dal_ctx->dal_tx_cpl_lock);

		if (!dal_ctx->tx_cpl_desc_list[ring_id]) {
			dal_ctx->tx_cpl_desc_list[ring_id] = tx_desc;
			dal_ctx->tx_cpl_desc_tail[ring_id] = tx_desc;
		} else {
			dal_ctx->tx_cpl_desc_tail[ring_id]->next = tx_desc;
			dal_ctx->tx_cpl_desc_tail[ring_id] = tx_desc;
		}
		tx_desc->next = NULL;
		dal_ctx->tx_cpl_desc_count[ring_id]++;

		qdf_spin_unlock_bh(&dal_ctx->dal_tx_cpl_lock);
	}

	return 0;
}

/**
 * dp_dal_tx_comp_handler() - DAL TX completion handler
 * @soc: DP SOC context
 * @ring_id: Ring ID
 * @dp_budget: NAPI budget
 *
 * This is the primary API for processing TX completions in the DAL module.
 * It invokes platform_tx_cpl to get completions from the platform layer,
 * then processes the descriptor list accumulated via dp_dal_tx_cpl_cb by
 * calling dp_tx_comp_process_desc_list().
 *
 * Return: Number of completions processed
 */
uint32_t dp_dal_tx_comp_handler(struct dp_soc *soc, u16 ring_id,
				uint32_t dp_budget)
{
	struct dp_dal_ctx *dal_ctx;
	struct dp_tx_desc_s *head_desc = NULL;
	uint32_t cnt = 0;
	bool ret;

	DP_HIST_INIT();

	if (qdf_unlikely(!soc)) {
		dp_tx_err_rl("SOC is NULL");
		return 0;
	}

	dal_ctx = soc->dal_ctx;
	if (qdf_unlikely(!dal_ctx)) {
		dp_tx_err_rl("DAL context is NULL");
		return 0;
	}

	if (qdf_unlikely(ring_id >= MAX_TCL_DATA_RINGS)) {
		dp_tx_err_rl("Invalid ring_id %u", ring_id);
		return 0;
	}

	/* Invoke platform_tx_cpl to get completions from DAL layer */
	if (global_plat_ops->tx_cpl) {
		ret = global_plat_ops->tx_cpl(dal_ctx, &cnt, ring_id);
		if (qdf_unlikely(!ret)) {
			dp_debug("No TX completions available for ring %u",
				 ring_id);
			return 0;
		}
	} else {
		dp_tx_err_rl("Platform TX CPL operation not available");
		return 0;
	}

	/* Process the descriptor list accumulated via dp_dal_tx_cpl_cb */
	qdf_spin_lock_bh(&dal_ctx->dal_tx_cpl_lock);
	head_desc = dal_ctx->tx_cpl_desc_list[ring_id];
	if (head_desc) {
		dal_ctx->tx_cpl_desc_list[ring_id] = NULL;
		dal_ctx->tx_cpl_desc_tail[ring_id] = NULL;
		cnt = dal_ctx->tx_cpl_desc_count[ring_id];
		dal_ctx->tx_cpl_desc_count[ring_id] = 0;
	}
	qdf_spin_unlock_bh(&dal_ctx->dal_tx_cpl_lock);

	/* pdev_id is always zero. Pass zero instead of
	 * iterating the tx_desc list for pdev id.
	 */
	DP_HIST_PACKET_COUNT_ADD(0, cnt);

	/* Process the descriptor list using the standard DP function */
	if (head_desc)
		dp_tx_comp_process_desc_list(soc, head_desc, ring_id);

	DP_STATS_INC(soc, tx.tx_comp[ring_id], cnt);

	DP_TX_HIST_STATS_PER_PDEV();
	return cnt;
}
