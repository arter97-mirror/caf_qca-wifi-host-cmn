/*
 * Copyright (c) 2016-2020 The Linux Foundation. All rights reserved.
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * Permission to use, copy, modify, and/or distribute this software for
 * any purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all
 * copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/**
 *  DOC:    wlan_mgmt_txrx_main.c
 *  This file contains mgmt txrx private API definitions for
 *  mgmt txrx component.
 */

#include "wlan_mgmt_txrx_main_i.h"
#include "qdf_nbuf.h"
#include "wlan_objmgr_pdev_obj.h"
#include "wlan_objmgr_psoc_obj.h"

QDF_STATUS wlan_mgmt_txrx_desc_pool_init(
			struct mgmt_txrx_priv_pdev_context *mgmt_txrx_pdev_ctx)
{
	struct wlan_objmgr_pdev *pdev;
	struct wlan_objmgr_psoc *psoc;
	uint32_t i;
	uint8_t pdev_id;
	uint8_t psoc_id;

	pdev = mgmt_txrx_pdev_ctx->pdev;
	if (!pdev) {
		mgmt_txrx_err("pdev context passed is NULL");
		return QDF_STATUS_E_INVAL;
	}

	psoc = wlan_pdev_get_psoc(pdev);

	if (!psoc) {
		mgmt_txrx_err("psoc context in pdev is NULL");
		return QDF_STATUS_E_INVAL;
	}

	pdev_id = wlan_objmgr_pdev_get_pdev_id(pdev);

	psoc_id = wlan_psoc_get_id(psoc);

	mgmt_txrx_debug(
			"mgmt_txrx ctx: %pK pdev: %pK pdev_id: %d psoc_id: %d mgmt desc pool size %d",
			mgmt_txrx_pdev_ctx, pdev,
			pdev_id, psoc_id,
			MGMT_DESC_POOL_MAX);
	mgmt_txrx_pdev_ctx->mgmt_desc_pool.pool = qdf_mem_malloc(
			MGMT_DESC_POOL_MAX *
			sizeof(struct mgmt_txrx_desc_elem_t));

	if (!mgmt_txrx_pdev_ctx->mgmt_desc_pool.pool)
		return QDF_STATUS_E_NOMEM;

	qdf_list_create(&mgmt_txrx_pdev_ctx->mgmt_desc_pool.free_list,
					MGMT_DESC_POOL_MAX);

	for (i = 0; i < MGMT_DESC_POOL_MAX; i++) {
		mgmt_txrx_pdev_ctx->mgmt_desc_pool.pool[i].desc_id = i;
		mgmt_txrx_pdev_ctx->mgmt_desc_pool.pool[i].in_use = false;
		qdf_list_insert_front(
				&mgmt_txrx_pdev_ctx->mgmt_desc_pool.free_list,
				&mgmt_txrx_pdev_ctx->mgmt_desc_pool.pool[i].entry);
	}

	qdf_spinlock_create(
		&mgmt_txrx_pdev_ctx->mgmt_desc_pool.desc_pool_lock);

	mgmt_txrx_debug("exit pdev_id:%d psoc_id:%d", pdev_id, psoc_id);

	return QDF_STATUS_SUCCESS;
}

void wlan_mgmt_txrx_desc_pool_deinit(
			struct mgmt_txrx_priv_pdev_context *mgmt_txrx_pdev_ctx)
{
	uint32_t i;
	uint32_t pool_size;
	QDF_STATUS status;

	if (!mgmt_txrx_pdev_ctx->mgmt_desc_pool.pool) {
		mgmt_txrx_err("Empty mgmt descriptor pool");
		qdf_assert_always(0);
		return;
	}

	pool_size = mgmt_txrx_pdev_ctx->mgmt_desc_pool.free_list.max_size;
	for (i = 0; i < pool_size; i++) {
		status = qdf_list_remove_node(
				&mgmt_txrx_pdev_ctx->mgmt_desc_pool.free_list,
				&mgmt_txrx_pdev_ctx->mgmt_desc_pool.pool[i].entry);
		if (status != QDF_STATUS_SUCCESS)
			mgmt_txrx_err(
				"Failed to get mgmt desc from freelist, desc id: %d: status %d",
				i, status);
	}

	qdf_list_destroy(&mgmt_txrx_pdev_ctx->mgmt_desc_pool.free_list);
	qdf_mem_free(mgmt_txrx_pdev_ctx->mgmt_desc_pool.pool);
	mgmt_txrx_pdev_ctx->mgmt_desc_pool.pool = NULL;

	qdf_spinlock_destroy(
		&mgmt_txrx_pdev_ctx->mgmt_desc_pool.desc_pool_lock);
}

struct mgmt_txrx_desc_elem_t *wlan_mgmt_txrx_desc_get(
			struct mgmt_txrx_priv_pdev_context *mgmt_txrx_pdev_ctx)
{
	QDF_STATUS status;
	qdf_list_node_t *desc_node;
	struct mgmt_txrx_desc_elem_t *mgmt_txrx_desc;

	qdf_spin_lock_bh(&mgmt_txrx_pdev_ctx->mgmt_desc_pool.desc_pool_lock);
	if (qdf_list_peek_front(&mgmt_txrx_pdev_ctx->mgmt_desc_pool.free_list,
			    &desc_node)
			!= QDF_STATUS_SUCCESS) {
		qdf_spin_unlock_bh(
			&mgmt_txrx_pdev_ctx->mgmt_desc_pool.desc_pool_lock);
		mgmt_txrx_err_rl("Descriptor freelist empty for mgmt_txrx_ctx %pK",
				 mgmt_txrx_pdev_ctx);
		return NULL;
	}

	status = qdf_list_remove_node(
				&mgmt_txrx_pdev_ctx->mgmt_desc_pool.free_list,
				desc_node);
	if (status != QDF_STATUS_SUCCESS) {
		qdf_spin_unlock_bh(
			&mgmt_txrx_pdev_ctx->mgmt_desc_pool.desc_pool_lock);
		mgmt_txrx_err("Failed to get descriptor from list: status %d",
					status);
		qdf_assert_always(0);
	}

	mgmt_txrx_desc = qdf_container_of(desc_node,
					  struct mgmt_txrx_desc_elem_t,
					  entry);
	mgmt_txrx_desc->in_use = true;

	qdf_spin_unlock_bh(&mgmt_txrx_pdev_ctx->mgmt_desc_pool.desc_pool_lock);

	/* acquire the wakelock when there are pending mgmt tx frames */
	qdf_wake_lock_timeout_acquire(&mgmt_txrx_pdev_ctx->wakelock_tx_cmp,
				      MGMT_TXRX_WAKELOCK_TIMEOUT_TX_CMP);
	qdf_runtime_pm_prevent_suspend(
		&mgmt_txrx_pdev_ctx->wakelock_tx_runtime_cmp);


	return mgmt_txrx_desc;
}

void wlan_mgmt_txrx_desc_put(
			struct mgmt_txrx_priv_pdev_context *mgmt_txrx_pdev_ctx,
			uint32_t desc_id)
{
	struct mgmt_txrx_desc_elem_t *desc;
	bool release_wakelock = false;

	desc = &mgmt_txrx_pdev_ctx->mgmt_desc_pool.pool[desc_id];
	qdf_spin_lock_bh(&mgmt_txrx_pdev_ctx->mgmt_desc_pool.desc_pool_lock);
	if (!desc->in_use) {
		qdf_spin_unlock_bh(&mgmt_txrx_pdev_ctx->mgmt_desc_pool.
				   desc_pool_lock);
		mgmt_txrx_err("desc %d is freed", desc_id);
		return;
	}
	desc->in_use = false;
	desc->context = NULL;
	desc->peer = NULL;
	desc->nbuf = NULL;
	desc->tx_dwnld_cmpl_cb = NULL;
	desc->tx_ota_cmpl_cb = NULL;
	desc->vdev_id = WLAN_UMAC_VDEV_ID_MAX;

	qdf_list_insert_front(&mgmt_txrx_pdev_ctx->mgmt_desc_pool.free_list,
			      &desc->entry);

	/* release the wakelock if there are no pending mgmt tx frames */
	if (mgmt_txrx_pdev_ctx->mgmt_desc_pool.free_list.count ==
	    mgmt_txrx_pdev_ctx->mgmt_desc_pool.free_list.max_size)
		release_wakelock = true;

	qdf_spin_unlock_bh(&mgmt_txrx_pdev_ctx->mgmt_desc_pool.desc_pool_lock);

	if (release_wakelock) {
		qdf_runtime_pm_allow_suspend(
			&mgmt_txrx_pdev_ctx->wakelock_tx_runtime_cmp);
		qdf_wake_lock_release(&mgmt_txrx_pdev_ctx->wakelock_tx_cmp,
				      MGMT_TXRX_WAKELOCK_REASON_TX_CMP);
	}
}

#ifdef WLAN_IOT_SIM_SUPPORT
QDF_STATUS iot_sim_mgmt_tx_update(struct wlan_objmgr_psoc *psoc,
				  struct wlan_objmgr_vdev *vdev,
				  qdf_nbuf_t buf)
{
	struct wlan_lmac_if_rx_ops *rx_ops;
	QDF_STATUS status = QDF_STATUS_SUCCESS;

	rx_ops = wlan_psoc_get_lmac_if_rxops(psoc);
	if (!rx_ops) {
		mgmt_txrx_err("rx_ops is NULL");
		return QDF_STATUS_E_NULL_VALUE;
	}
	if (rx_ops->iot_sim_rx_ops.iot_sim_cmd_handler) {
		status = rx_ops->iot_sim_rx_ops.iot_sim_cmd_handler(vdev,
								    buf,
								    NULL,
								    true,
								    NULL);
		if (status == QDF_STATUS_E_NULL_VALUE)
			mgmt_txrx_err("iot_sim frame drop");
		else
			status = QDF_STATUS_SUCCESS;
	}

	return status;
}
#else
QDF_STATUS iot_sim_mgmt_tx_update(struct wlan_objmgr_psoc *psoc,
				  struct wlan_objmgr_vdev *vdev,
				  qdf_nbuf_t buf)
{
	return QDF_STATUS_SUCCESS;
}
#endif

uint8_t *mgmt_txrx_get_frm_type_string(enum mgmt_frame_type frm_type)
{
	switch (frm_type) {
	CASE_RETURN_STRING(MGMT_ASSOC_REQ);
	CASE_RETURN_STRING(MGMT_ASSOC_RESP);
	CASE_RETURN_STRING(MGMT_REASSOC_REQ);
	CASE_RETURN_STRING(MGMT_REASSOC_RESP);
	CASE_RETURN_STRING(MGMT_PROBE_REQ);
	CASE_RETURN_STRING(MGMT_PROBE_RESP);
	CASE_RETURN_STRING(MGMT_BEACON);
	CASE_RETURN_STRING(MGMT_ATIM);
	CASE_RETURN_STRING(MGMT_DISASSOC);
	CASE_RETURN_STRING(MGMT_AUTH);
	CASE_RETURN_STRING(MGMT_DEAUTH);
	CASE_RETURN_STRING(MGMT_ACTION_MEAS_REQUEST);
	CASE_RETURN_STRING(MGMT_ACTION_MEAS_REPORT);
	CASE_RETURN_STRING(MGMT_ACTION_TPC_REQUEST);
	CASE_RETURN_STRING(MGMT_ACTION_TPC_REPORT);
	CASE_RETURN_STRING(MGMT_ACTION_CHAN_SWITCH);
	CASE_RETURN_STRING(MGMT_ACTION_QOS_ADD_TS_REQ);
	CASE_RETURN_STRING(MGMT_ACTION_QOS_ADD_TS_RSP);
	CASE_RETURN_STRING(MGMT_ACTION_QOS_DEL_TS_REQ);
	CASE_RETURN_STRING(MGMT_ACTION_QOS_SCHEDULE);
	CASE_RETURN_STRING(MGMT_ACTION_QOS_MAP_CONFIGURE);
	CASE_RETURN_STRING(MGMT_ACTION_DLS_REQUEST);
	CASE_RETURN_STRING(MGMT_ACTION_DLS_RESPONSE);
	CASE_RETURN_STRING(MGMT_ACTION_DLS_TEARDOWN);
	CASE_RETURN_STRING(MGMT_ACTION_BA_ADDBA_REQUEST);
	CASE_RETURN_STRING(MGMT_ACTION_BA_ADDBA_RESPONSE);
	CASE_RETURN_STRING(MGMT_ACTION_BA_DELBA);
	CASE_RETURN_STRING(MGMT_ACTION_2040_BSS_COEXISTENCE);
	CASE_RETURN_STRING(MGMT_ACTION_CATEGORY_VENDOR_SPECIFIC);
	CASE_RETURN_STRING(MGMT_ACTION_CATEGORY_VENDOR_SPECIFIC_PROTECTED);
	CASE_RETURN_STRING(MGMT_ACTION_EXT_CHANNEL_SWITCH_ID);
	CASE_RETURN_STRING(MGMT_ACTION_VENDOR_SPECIFIC);
	CASE_RETURN_STRING(MGMT_ACTION_TDLS_DISCRESP);
	CASE_RETURN_STRING(MGMT_ACTION_RRM_RADIO_MEASURE_REQ);
	CASE_RETURN_STRING(MGMT_ACTION_RRM_RADIO_MEASURE_RPT);
	CASE_RETURN_STRING(MGMT_ACTION_RRM_LINK_MEASUREMENT_REQ);
	CASE_RETURN_STRING(MGMT_ACTION_RRM_LINK_MEASUREMENT_RPT);
	CASE_RETURN_STRING(MGMT_ACTION_RRM_NEIGHBOR_REQ);
	CASE_RETURN_STRING(MGMT_ACTION_RRM_NEIGHBOR_RPT);
	CASE_RETURN_STRING(MGMT_ACTION_FT_REQUEST);
	CASE_RETURN_STRING(MGMT_ACTION_FT_RESPONSE);
	CASE_RETURN_STRING(MGMT_ACTION_FT_CONFIRM);
	CASE_RETURN_STRING(MGMT_ACTION_FT_ACK);
	CASE_RETURN_STRING(MGMT_ACTION_HT_NOTIFY_CHANWIDTH);
	CASE_RETURN_STRING(MGMT_ACTION_HT_SMPS);
	CASE_RETURN_STRING(MGMT_ACTION_HT_PSMP);
	CASE_RETURN_STRING(MGMT_ACTION_HT_PCO_PHASE);
	CASE_RETURN_STRING(MGMT_ACTION_HT_CSI);
	CASE_RETURN_STRING(MGMT_ACTION_HT_NONCOMPRESSED_BF);
	CASE_RETURN_STRING(MGMT_ACTION_HT_COMPRESSED_BF);
	CASE_RETURN_STRING(MGMT_ACTION_HT_ASEL_IDX_FEEDBACK);
	CASE_RETURN_STRING(MGMT_ACTION_SA_QUERY_REQUEST);
	CASE_RETURN_STRING(MGMT_ACTION_SA_QUERY_RESPONSE);
	CASE_RETURN_STRING(MGMT_ACTION_PDPA_GAS_INIT_REQ);
	CASE_RETURN_STRING(MGMT_ACTION_PDPA_GAS_INIT_RSP);
	CASE_RETURN_STRING(MGMT_ACTION_PDPA_GAS_COMEBACK_REQ);
	CASE_RETURN_STRING(MGMT_ACTION_PDPA_GAS_COMEBACK_RSP);
	CASE_RETURN_STRING(MGMT_ACTION_WNM_BSS_TM_QUERY);
	CASE_RETURN_STRING(MGMT_ACTION_WNM_BSS_TM_REQUEST);
	CASE_RETURN_STRING(MGMT_ACTION_WNM_BSS_TM_RESPONSE);
	CASE_RETURN_STRING(MGMT_ACTION_WNM_NOTIF_REQUEST);
	CASE_RETURN_STRING(MGMT_ACTION_WNM_NOTIF_RESPONSE);
	CASE_RETURN_STRING(MGMT_ACTION_WNM_FMS_REQ);
	CASE_RETURN_STRING(MGMT_ACTION_WNM_FMS_RESP);
	CASE_RETURN_STRING(MGMT_ACTION_WNM_TFS_REQ);
	CASE_RETURN_STRING(MGMT_ACTION_WNM_TFS_RESP);
	CASE_RETURN_STRING(MGMT_ACTION_WNM_TFS_NOTIFY);
	CASE_RETURN_STRING(MGMT_ACTION_WNM_SLEEP_REQ);
	CASE_RETURN_STRING(MGMT_ACTION_WNM_SLEEP_RESP);
	CASE_RETURN_STRING(MGMT_ACTION_WNM_TIM_REQ);
	CASE_RETURN_STRING(MGMT_ACTION_WNM_TIM_RESP);
	CASE_RETURN_STRING(MGMT_ACTION_TDLS_SETUP_REQ);
	CASE_RETURN_STRING(MGMT_ACTION_TDLS_SETUP_RSP);
	CASE_RETURN_STRING(MGMT_ACTION_TDLS_SETUP_CNF);
	CASE_RETURN_STRING(MGMT_ACTION_TDLS_TEARDOWN);
	CASE_RETURN_STRING(MGMT_ACTION_TDLS_PEER_TRAFFIC_IND);
	CASE_RETURN_STRING(MGMT_ACTION_TDLS_CH_SWITCH_REQ);
	CASE_RETURN_STRING(MGMT_ACTION_TDLS_CH_SWITCH_RSP);
	CASE_RETURN_STRING(MGMT_ACTION_TDLS_PEER_PSM_REQUEST);
	CASE_RETURN_STRING(MGMT_ACTION_TDLS_PEER_PSM_RESPONSE);
	CASE_RETURN_STRING(MGMT_ACTION_TDLS_PEER_TRAFFIC_RSP);
	CASE_RETURN_STRING(MGMT_ACTION_TDLS_DIS_REQ);
	CASE_RETURN_STRING(MGMT_ACTION_MESH_LINK_METRIC_REPORT);
	CASE_RETURN_STRING(MGMT_ACTION_MESH_HWMP_PATH_SELECTION);
	CASE_RETURN_STRING(MGMT_ACTION_MESH_GATE_ANNOUNCEMENT);
	CASE_RETURN_STRING(MGMT_ACTION_MESH_CONGESTION_CONTROL_NOTIFICATION);
	CASE_RETURN_STRING(MGMT_ACTION_MESH_MCCA_SETUP_REQUEST);
	CASE_RETURN_STRING(MGMT_ACTION_MESH_MCCA_SETUP_REPLY);
	CASE_RETURN_STRING(MGMT_ACTION_MESH_MCCA_ADVERTISEMENT_REQUEST);
	CASE_RETURN_STRING(MGMT_ACTION_MESH_MCCA_ADVERTISEMENT);
	CASE_RETURN_STRING(MGMT_ACTION_MESH_MCCA_TEARDOWN);
	CASE_RETURN_STRING(MGMT_ACTION_MESH_TBTT_ADJUSTMENT_REQUEST);
	CASE_RETURN_STRING(MGMT_ACTION_MESH_TBTT_ADJUSTMENT_RESPONSE);
	CASE_RETURN_STRING(MGMT_ACTION_SP_MESH_PEERING_OPEN);
	CASE_RETURN_STRING(MGMT_ACTION_SP_MESH_PEERING_CONFIRM);
	CASE_RETURN_STRING(MGMT_ACTION_SP_MESH_PEERING_CLOSE);
	CASE_RETURN_STRING(MGMT_ACTION_SP_MGK_INFORM);
	CASE_RETURN_STRING(MGMT_ACTION_SP_MGK_ACK);
	CASE_RETURN_STRING(MGMT_ACTION_WMM_QOS_SETUP_REQ);
	CASE_RETURN_STRING(MGMT_ACTION_WMM_QOS_SETUP_RESP);
	CASE_RETURN_STRING(MGMT_ACTION_WMM_QOS_TEARDOWN);
	CASE_RETURN_STRING(MGMT_ACTION_VHT_COMPRESSED_BF);
	CASE_RETURN_STRING(MGMT_ACTION_VHT_GID_NOTIF);
	CASE_RETURN_STRING(MGMT_ACTION_VHT_OPMODE_NOTIF);
	CASE_RETURN_STRING(MGMT_ACTION_GAS_INITIAL_REQUEST);
	CASE_RETURN_STRING(MGMT_ACTION_GAS_INITIAL_RESPONSE);
	CASE_RETURN_STRING(MGMT_ACTION_GAS_COMEBACK_REQUEST);
	CASE_RETURN_STRING(MGMT_ACTION_GAS_COMEBACK_RESPONSE);
	CASE_RETURN_STRING(MGMT_ACTION_FST_SETUP_REQ);
	CASE_RETURN_STRING(MGMT_ACTION_FST_SETUP_RSP);
	CASE_RETURN_STRING(MGMT_ACTION_FST_TEAR_DOWN);
	CASE_RETURN_STRING(MGMT_ACTION_FST_ACK_REQ);
	CASE_RETURN_STRING(MGMT_ACTION_FST_ACK_RSP);
	CASE_RETURN_STRING(MGMT_ACTION_FST_ON_CHANNEL_TUNNEL);
	CASE_RETURN_STRING(MGMT_ACTION_SCS_REQ);
	CASE_RETURN_STRING(MGMT_ACTION_SCS_RSP);
	CASE_RETURN_STRING(MGMT_ACTION_GROUP_MEMBERSHIP_REQ);
	CASE_RETURN_STRING(MGMT_ACTION_GROUP_MEMBERSHIP_RSP);
	CASE_RETURN_STRING(MGMT_ACTION_MCSC_REQ);
	CASE_RETURN_STRING(MGMT_ACTION_MCSC_RSP);
	CASE_RETURN_STRING(MGMT_FRAME_TYPE_ALL);
	CASE_RETURN_STRING(MGMT_CTRL_FRAME);
	CASE_RETURN_STRING(MGMT_ACTION_TWT_SETUP);
	CASE_RETURN_STRING(MGMT_ACTION_TWT_TEARDOWN);
	CASE_RETURN_STRING(MGMT_ACTION_TWT_INFORMATION);
	CASE_RETURN_STRING(MGMT_ACTION_EHT_T2LM_REQUEST);
	CASE_RETURN_STRING(MGMT_ACTION_EHT_T2LM_RESPONSE);
	CASE_RETURN_STRING(MGMT_ACTION_EHT_T2LM_TEARDOWN);
	CASE_RETURN_STRING(MGMT_ACTION_EHT_EPCS_REQUEST);
	CASE_RETURN_STRING(MGMT_ACTION_EHT_EPCS_RESPONSE);
	CASE_RETURN_STRING(MGMT_ACTION_EHT_EPCS_TEARDOWN);
	CASE_RETURN_STRING(MGMT_ACTION_FTM_REQUEST);
	CASE_RETURN_STRING(MGMT_ACTION_FTM_RESPONSE);
	CASE_RETURN_STRING(MGMT_ACTION_FILS_DISCOVERY);
	CASE_RETURN_STRING(MGMT_ACTION_EHT_LINK_RECONFIG_NOTIFY);
	CASE_RETURN_STRING(MGMT_ACTION_EHT_LINK_RECONFIG_REQUEST);
	CASE_RETURN_STRING(MGMT_ACTION_EHT_LINK_RECONFIG_RESPONSE);
	default:
		break;
	}

	return (uint8_t *)"MGMT_UNKNOWN";
}

/* MGMT frame threshold time in ms */
#define MGMT_FRAME_THRESHOLD_TIME 1000

/**
 * mgmt_txrx_get_frame_info_for_dup_detect - get frame info for peer from PSOC
 * private object.
 * @psoc: pointer to PSOC object
 * @peer_addr: Peer MAC address
 * @mgmt_frame_info: pointer to management frame info structure
 *
 * Return: frame info structure for duplicate detection
 */
static struct mgmt_frame_dup_det_info *
mgmt_txrx_get_frame_info_for_dup_detect(
				struct wlan_objmgr_psoc *psoc,
				struct qdf_mac_addr *peer_addr,
				struct mgmt_frame_dup_det_info *mgmt_frame_info)
{
	uint8_t i;

	for (i = 0; i < MAX_PEER_NUM; i++) {
		if (qdf_is_macaddr_equal(peer_addr,
					 &mgmt_frame_info[i].peer_addr))
			return &mgmt_frame_info[i];
	}

	return NULL;
}

/**
 * mgmt_txrx_cache_frame_info_for_dup_detect - cache frame info for peer in PSOC
 * private object.
 * @psoc: pointer to PSOC object
 * @mgmt_frame_info: pointer to frame info to be cached
 * @new_frame: pointer to New management frame parameters
 *
 * Return: None
 */
static void mgmt_txrx_cache_frame_info_for_dup_detect(
				struct wlan_objmgr_psoc *psoc,
				struct mgmt_frame_dup_det_info *mgmt_frame_info,
				struct mgmt_frame_dup_det_info *new_frame)
{
	uint8_t i, override_idx = 0;
	bool empty_slot_found = false;
	qdf_time_t oldest_entry_time = 0;

	for (i = 0; i < MAX_PEER_NUM; i++) {
		if (qdf_is_macaddr_equal(&new_frame->peer_addr,
					 &mgmt_frame_info[i].peer_addr)) {
			mgmt_frame_info[i] = *new_frame;
			return;
		}

		if (!empty_slot_found &&
		    qdf_is_macaddr_zero(&mgmt_frame_info[i].peer_addr)) {
			override_idx = i;
			empty_slot_found = true;
		}

		/*
		 * Update at 1st empty index, or the oldest index
		 * if no empty index found
		 */
		if (!empty_slot_found &&
		    (!oldest_entry_time ||
		     oldest_entry_time < mgmt_frame_info[i].time)) {
			oldest_entry_time = mgmt_frame_info[i].time;
			override_idx = i;
		}
	}

	mgmt_frame_info[override_idx] = *new_frame;
}

/**
 * mgmt_txrx_fill_frame_info_for_dup_detect - fill frame info new frame from the
 * frame header.
 *
 * @wlan_hdr: Frame header
 * @frm_type: Frame subtype
 * @new_frame: pointer to frame info to be filled
 *
 * Return: none
 */
static void mgmt_txrx_fill_frame_info_for_dup_detect(
				struct ieee80211_frame *wlan_hdr,
				enum mgmt_frame_type frm_type,
				struct mgmt_frame_dup_det_info *new_frame)
{
	new_frame->subtype = frm_type;
	new_frame->retry_bit = wlan_hdr->i_fc[1] & IEEE80211_FC1_RETRY;
	new_frame->seq_num =
		(le16toh(*(uint16_t *)wlan_hdr->i_seq) >> WLAN_SEQ_SEQ_SHIFT);
	new_frame->time = qdf_get_time_of_the_day_ms();
	qdf_mem_copy(new_frame->peer_addr.bytes, wlan_hdr->i_addr2,
		     QDF_MAC_ADDR_SIZE);
}

/**
 * mgmt_txrx_clean_dup_det_frame_info_on_timeout - clean the duplicate detection
 * frame information if difference between current time and previous frame time
 * is greater than threshold time.
 * @psoc: pointer to PSOC object
 * @mgmt_frame_info: pointer to management frame info structure
 *
 * Return: none
 */
static void mgmt_txrx_clean_dup_det_frame_info_on_timeout(
				struct wlan_objmgr_psoc *psoc,
				struct mgmt_frame_dup_det_info *mgmt_frame_info)
{
	uint8_t i;
	uint64_t curr_time;

	curr_time = qdf_get_time_of_the_day_ms();

	for (i = 0; i < MAX_PEER_NUM; i++) {
		if (qdf_system_time_after(curr_time, mgmt_frame_info[i].time +
					  MGMT_FRAME_THRESHOLD_TIME))
			qdf_mem_zero(&mgmt_frame_info[i],
				     sizeof(mgmt_frame_info[i]));
	}
}

bool mgmt_txrx_frame_is_duplicate(struct wlan_objmgr_psoc *psoc,
				  struct ieee80211_frame *wlan_hdr,
				  enum mgmt_frame_type frm_type)
{
	struct mgmt_frame_dup_det_info new_frame, *prev_frame;
	struct mgmt_txrx_priv_psoc_context *psoc_priv_obj;
	struct mgmt_frame_dup_det_info *mgmt_frame_info;

	psoc_priv_obj = wlan_objmgr_psoc_get_comp_private_obj(
						psoc,
						WLAN_UMAC_COMP_MGMT_TXRX);
	if (!psoc_priv_obj) {
		mgmt_txrx_err("psoc priv obj is NULL");
		return false;
	}

	mgmt_frame_info = psoc_priv_obj->mgmt_frame_param;

	mgmt_txrx_clean_dup_det_frame_info_on_timeout(psoc, mgmt_frame_info);
	mgmt_txrx_fill_frame_info_for_dup_detect(wlan_hdr, frm_type,
						 &new_frame);
	prev_frame = mgmt_txrx_get_frame_info_for_dup_detect(
							psoc,
							&new_frame.peer_addr,
							mgmt_frame_info);
	if (!prev_frame)
		goto end;

	if (prev_frame->seq_num == new_frame.seq_num &&
	    prev_frame->subtype == new_frame.subtype && new_frame.retry_bit) {
		mgmt_txrx_debug(QDF_MAC_ADDR_FMT " : Drop duplicate frame type %d (%s) seq %d, is retry %d",
				QDF_MAC_ADDR_REF(new_frame.peer_addr.bytes),
				new_frame.subtype,
				mgmt_txrx_get_frm_type_string(frm_type),
				new_frame.seq_num, new_frame.retry_bit);
		return true;
	}

end:
	mgmt_txrx_cache_frame_info_for_dup_detect(psoc, mgmt_frame_info,
						  &new_frame);

	return false;
}
