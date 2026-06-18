/*
 * Copyright (c) 2012-2015,2020-2021 The Linux Foundation. All rights reserved.
 * Copyright (c) 2021-2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/**
 * DOC: Implements general SM framework for connection manager roaming sm
 */

#include "wlan_cm_main.h"
#include "wlan_cm_roam_sm.h"
#include "wlan_cm_sm.h"
#include "wlan_cm_main_api.h"
#include "wlan_cm_roam.h"
#ifdef WLAN_FEATURE_11BE_MLO_ADV_FEATURE
#include "wlan_mlo_mgr_roam.h"
#endif
#include "wlan_smd_roam.h"
#include "wlan_cm_tgt_if_tx_api.h"

#if defined(WLAN_FEATURE_HOST_ROAM) || defined(WLAN_FEATURE_ROAM_OFFLOAD)
void cm_state_roaming_entry(void *ctx)
{
	struct cnx_mgr *cm_ctx = ctx;

	cm_sm_state_update(cm_ctx, WLAN_CM_S_ROAMING, WLAN_CM_SS_IDLE);
}

void cm_state_roaming_exit(void *ctx)
{
}

#ifdef WLAN_FEATURE_ROAM_OFFLOAD
static
bool cm_handle_fw_roaming_event(struct cnx_mgr *cm_ctx, uint16_t event,
				uint16_t data_len, void *data)
{
	bool event_handled = true;
	QDF_STATUS status;

	switch (event) {
	case WLAN_CM_SM_EV_ROAM_INVOKE:
		status = cm_add_fw_roam_cmd_to_list_n_ser(cm_ctx, data);
		if (QDF_IS_STATUS_ERROR(status)) {
			event_handled = false;
			break;
		}
		cm_sm_transition_to(cm_ctx, WLAN_CM_SS_ROAM_STARTED);
		cm_sm_deliver_event_sync(cm_ctx,
					 WLAN_CM_SM_EV_ROAM_INVOKE,
					 data_len, data);
		break;
	case WLAN_CM_SM_EV_ROAM_START:
		status = cm_add_fw_roam_cmd_to_list_n_ser(cm_ctx, data);
		if (QDF_IS_STATUS_ERROR(status)) {
			event_handled = false;
			break;
		}
		cm_sm_transition_to(cm_ctx, WLAN_CM_SS_ROAM_STARTED);
		cm_sm_deliver_event_sync(cm_ctx,
					 WLAN_CM_SM_EV_ROAM_START,
					 data_len, data);
		break;
	case WLAN_CM_SM_EV_ROAM_ABORT:
		cm_sm_transition_to(cm_ctx, WLAN_CM_S_CONNECTED);
		cm_sm_deliver_event_sync(cm_ctx, event,
					 data_len, data);
		break;
	case WLAN_CM_SM_EV_ROAM_SYNC:
		cm_sm_transition_to(cm_ctx, WLAN_CM_SS_ROAM_SYNC);
		status = cm_sm_deliver_event_sync(cm_ctx, event,
						  data_len, data);
		if (QDF_IS_STATUS_ERROR(status))
			event_handled = false;
		break;
	default:
		event_handled = false;
		break;
	}

	return event_handled;
}
#else
static inline
bool cm_handle_fw_roaming_event(struct cnx_mgr *cm_ctx, uint16_t event,
				uint16_t data_len, void *data)
{
	return false;
}
#endif

bool cm_state_roaming_event(void *ctx, uint16_t event,
			    uint16_t data_len, void *data)
{
	struct cnx_mgr *cm_ctx = ctx;
	bool event_handled = true;
	struct wlan_objmgr_psoc *psoc;

	switch (event) {
	case WLAN_CM_SM_EV_ROAM_REQ:
		psoc = wlan_vdev_get_psoc(cm_ctx->vdev);
		if (!psoc) {
			event_handled = false;
			break;
		}
		if (cm_roam_offload_enabled(psoc)) {
			cm_sm_deliver_event_sync(cm_ctx,
						 WLAN_CM_SM_EV_ROAM_INVOKE,
						 data_len, data);
		} else {
			cm_add_roam_req_to_list(cm_ctx, data);
			cm_sm_transition_to(cm_ctx, WLAN_CM_SS_PREAUTH);
			cm_sm_deliver_event_sync(cm_ctx,
						 WLAN_CM_SM_EV_ROAM_START,
						 data_len, data);
		}
		break;
	case WLAN_CM_SM_EV_ROAM_START:
		/*
		 * FW sent a new ROAM_START while already in ROAMING/SS_IDLE
		 * (e.g. SMD prep completed but FW roamed to legacy instead of
		 * sending ROAM_SYNC). An SMD roam command may already be
		 * serialized from the earlier ROAM_START — remove it before
		 * serializing the new one, otherwise two commands coexist and
		 * cm_fw_roam_start() picks the stale one.
		 */
		smd_remove_roam_cmd(cm_ctx);
		fallthrough;
	default:
		event_handled = cm_handle_fw_roaming_event(cm_ctx, event,
							   data_len, data);
		break;
	}

	return event_handled;
}

static bool cm_handle_connect_disconnect_in_roam(struct cnx_mgr *cm_ctx,
						 uint16_t event,
						 uint16_t data_len, void *data)
{
	QDF_STATUS status;

	switch (event) {
	case WLAN_CM_SM_EV_CONNECT_REQ:
		status = cm_handle_connect_req_in_non_init_state(cm_ctx, data,
							WLAN_CM_S_ROAMING);
		if (QDF_IS_STATUS_ERROR(status))
			return false;
		cm_sm_transition_to(cm_ctx, WLAN_CM_S_CONNECTING);
		cm_sm_deliver_event_sync(cm_ctx, WLAN_CM_SM_EV_CONNECT_START,
					 data_len, data);
		break;
	case WLAN_CM_SM_EV_DISCONNECT_REQ:
		status = cm_handle_discon_req_in_non_connected_state(cm_ctx,
						data, WLAN_CM_S_ROAMING);
		if (QDF_IS_STATUS_ERROR(status))
			return false;
		cm_sm_transition_to(cm_ctx, WLAN_CM_S_DISCONNECTING);
		cm_sm_deliver_event_sync(cm_ctx, WLAN_CM_SM_EV_DISCONNECT_START,
					 data_len, data);
		break;
	case WLAN_CM_SM_EV_DISCONNECT_ACTIVE:
		cm_disconnect_active(cm_ctx, data);
		break;
	default:
		return false;
		break;
	}

	return true;
}
#endif

#ifdef WLAN_FEATURE_HOST_ROAM
void cm_subst_preauth_entry(void *ctx)
{
	struct cnx_mgr *cm_ctx = ctx;

	if (cm_get_state(cm_ctx) != WLAN_CM_S_ROAMING)
		QDF_BUG(0);

	cm_set_substate(cm_ctx, WLAN_CM_SS_PREAUTH);
	/* set preauth to true when we enter preauth state */
	cm_ctx->preauth_in_progress = true;
}

void cm_subst_preauth_exit(void *ctx)
{
}

#ifdef WLAN_FEATURE_PREAUTH_ENABLE
static bool
cm_handle_preauth_event(struct cnx_mgr *cm_ctx, uint16_t event,
			uint16_t data_len, void *data)
{
	bool event_handled = true;

	switch (event) {
	case WLAN_CM_SM_EV_PREAUTH_ACTIVE:
		if (!cm_check_cmid_match_list_head(cm_ctx, data)) {
			event_handled = false;
			break;
		}
		cm_preauth_active(cm_ctx, data);
		break;
	case WLAN_CM_SM_EV_PREAUTH_RESP:
		cm_preauth_done_resp(cm_ctx, data);
		break;
	case WLAN_CM_SM_EV_PREAUTH_DONE:
		cm_sm_transition_to(cm_ctx, WLAN_CM_SS_REASSOC);
		cm_preauth_success(cm_ctx, data);
		break;
	case WLAN_CM_SM_EV_PREAUTH_FAIL:
		cm_sm_transition_to(cm_ctx, WLAN_CM_S_CONNECTED);
		cm_preauth_fail(cm_ctx, data);
		break;
	default:
		event_handled = false;
	}

	return event_handled;
}
#else
static inline bool
cm_handle_preauth_event(struct cnx_mgr *cm_ctx, uint16_t event,
			uint16_t data_len, void *data)
{
	return false;
}
#endif

bool cm_subst_preauth_event(void *ctx, uint16_t event,
			    uint16_t data_len, void *data)
{
	struct cnx_mgr *cm_ctx = ctx;
	bool event_handled = true;

	switch (event) {
	case WLAN_CM_SM_EV_CONNECT_REQ:
	case WLAN_CM_SM_EV_DISCONNECT_REQ:
	case WLAN_CM_SM_EV_DISCONNECT_ACTIVE:
		event_handled =
			cm_handle_connect_disconnect_in_roam(cm_ctx, event,
							     data_len, data);
		break;
	case WLAN_CM_SM_EV_ROAM_START:
		cm_host_roam_start_req(cm_ctx, data);
		break;
	case WLAN_CM_SM_EV_START_REASSOC:
		cm_sm_transition_to(cm_ctx, WLAN_CM_SS_REASSOC);
		cm_sm_deliver_event_sync(cm_ctx, event, data_len, data);
		break;
	case WLAN_CM_SM_EV_REASSOC_FAILURE:
		cm_reassoc_complete(cm_ctx, data);
		break;
	default:
		event_handled = cm_handle_preauth_event(cm_ctx, event,
							data_len, data);
		break;
	}

	return event_handled;
}

void cm_subst_reassoc_entry(void *ctx)
{
	struct cnx_mgr *cm_ctx = ctx;

	if (cm_get_state(cm_ctx) != WLAN_CM_S_ROAMING)
		QDF_BUG(0);

	cm_set_substate(cm_ctx, WLAN_CM_SS_REASSOC);
	/* set preauth to false as soon as we move to reassoc state */
	cm_ctx->preauth_in_progress = false;
}

void cm_subst_reassoc_exit(void *ctx)
{
}

#ifdef WLAN_FEATURE_PREAUTH_ENABLE
static bool
cm_handle_reassoc_event(struct cnx_mgr *cm_ctx, uint16_t event,
			uint16_t data_len, void *data)
{
	bool event_handled = true;
	QDF_STATUS status;

	switch (event) {
	case WLAN_CM_SM_EV_REASSOC_TIMER:
		status = cm_handle_reassoc_timer(cm_ctx, data);
		if (QDF_IS_STATUS_ERROR(status))
			event_handled = false;
		break;
	default:
		event_handled = false;
	}

	return event_handled;
}
#else
static inline bool
cm_handle_reassoc_event(struct cnx_mgr *cm_ctx, uint16_t event,
			uint16_t data_len, void *data)
{
	return false;
}
#endif

bool cm_subst_reassoc_event(void *ctx, uint16_t event,
			    uint16_t data_len, void *data)
{
	struct cnx_mgr *cm_ctx = ctx;
	bool event_handled = true;

	switch (event) {
	case WLAN_CM_SM_EV_CONNECT_REQ:
	case WLAN_CM_SM_EV_DISCONNECT_REQ:
	case WLAN_CM_SM_EV_DISCONNECT_ACTIVE:
		event_handled =
			cm_handle_connect_disconnect_in_roam(cm_ctx, event,
							     data_len, data);
		break;
	case WLAN_CM_SM_EV_START_REASSOC:
		cm_reassoc_start(cm_ctx, data);
		break;
	case WLAN_CM_SM_EV_REASSOC_ACTIVE:
		if (!cm_check_cmid_match_list_head(cm_ctx, data)) {
			event_handled = false;
			break;
		}
		cm_reassoc_active(cm_ctx, data);
		break;
	case WLAN_CM_SM_EV_HO_ROAM_DISCONNECT_DONE:
		cm_reassoc_disconnect_complete(cm_ctx, data);
		break;
	case WLAN_CM_SM_EV_BSS_CREATE_PEER_SUCCESS:
		if (!cm_check_cmid_match_list_head(cm_ctx, data)) {
			event_handled = false;
			break;
		}
		cm_resume_reassoc_after_peer_create(cm_ctx, data);
		break;
	case WLAN_CM_SM_EV_REASSOC_DONE:
		if (!cm_roam_resp_cmid_match_list_head(cm_ctx, data)) {
			event_handled = false;
			break;
		}
		cm_sm_transition_to(cm_ctx, WLAN_CM_S_CONNECTED);
		cm_sm_deliver_event_sync(cm_ctx, event, data_len, data);
		break;
	case WLAN_CM_SM_EV_REASSOC_FAILURE:
		cm_reassoc_complete(cm_ctx, data);
		break;
	case WLAN_CM_SM_EV_HW_MODE_SUCCESS:
	case WLAN_CM_SM_EV_HW_MODE_FAILURE:
		/* check if cm id is valid for the current req */
		if (!cm_check_cmid_match_list_head(cm_ctx, data)) {
			event_handled = false;
			break;
		}
		cm_handle_reassoc_hw_mode_change(cm_ctx, data, event);
		break;
	default:
		event_handled = cm_handle_reassoc_event(cm_ctx, event,
							data_len, data);
		break;
	}

	return event_handled;
}

#endif /* WLAN_FEATURE_HOST_ROAM */

#ifdef WLAN_FEATURE_ROAM_OFFLOAD
void cm_subst_roam_start_entry(void *ctx)
{
	struct cnx_mgr *cm_ctx = ctx;

	if (cm_get_state(cm_ctx) != WLAN_CM_S_ROAMING)
		QDF_BUG(0);

	cm_set_substate(cm_ctx, WLAN_CM_SS_ROAM_STARTED);
}

void cm_subst_roam_start_exit(void *ctx)
{
}

bool cm_subst_roam_start_event(void *ctx, uint16_t event,
			       uint16_t data_len, void *data)
{
	bool event_handled = true;
	struct cnx_mgr *cm_ctx = ctx;
	QDF_STATUS status;

	switch (event) {
	case WLAN_CM_SM_EV_CONNECT_REQ:
	case WLAN_CM_SM_EV_DISCONNECT_REQ:
	case WLAN_CM_SM_EV_DISCONNECT_ACTIVE:
		event_handled =
			cm_handle_connect_disconnect_in_roam(cm_ctx, event,
							     data_len, data);
		break;
	case WLAN_CM_SM_EV_ROAM_START:
		cm_fw_roam_start(ctx);
		break;
	case WLAN_CM_SM_EV_ROAM_INVOKE:
		cm_send_roam_invoke_req(cm_ctx, data);
		break;
	case WLAN_CM_SM_EV_ROAM_ABORT:
	case WLAN_CM_SM_EV_ROAM_INVOKE_FAIL:
	case WLAN_CM_SM_EV_ROAM_HO_FAIL:
		cm_sm_transition_to(cm_ctx, WLAN_CM_S_CONNECTED);
		cm_sm_deliver_event_sync(cm_ctx, event,
					 data_len, data);
		break;
	case WLAN_CM_SM_EV_ROAM_SYNC:
		cm_sm_transition_to(cm_ctx, WLAN_CM_SS_ROAM_SYNC);
		status = cm_sm_deliver_event_sync(cm_ctx, event,
						  data_len, data);
		if (QDF_IS_STATUS_ERROR(status))
			event_handled = false;
		break;
	default:
		event_handled = false;
		break;
	}

	return event_handled;
}

void cm_subst_roam_sync_entry(void *ctx)
{
	struct cnx_mgr *cm_ctx = ctx;

	if (cm_get_state(cm_ctx) != WLAN_CM_S_ROAMING)
		QDF_BUG(0);

	cm_set_substate(cm_ctx, WLAN_CM_SS_ROAM_SYNC);
}

void cm_subst_roam_sync_exit(void *ctx)
{
}

bool cm_subst_roam_sync_event(void *ctx, uint16_t event,
			      uint16_t data_len, void *data)
{
	bool event_handled = true;
	struct cnx_mgr *cm_ctx = ctx;
	QDF_STATUS status;

	switch (event) {
	case WLAN_CM_SM_EV_CONNECT_REQ:
	case WLAN_CM_SM_EV_DISCONNECT_REQ:
	case WLAN_CM_SM_EV_DISCONNECT_ACTIVE:
		event_handled =
			cm_handle_connect_disconnect_in_roam(cm_ctx, event,
							     data_len, data);
		break;
	case WLAN_CM_SM_EV_ROAM_SYNC:
#ifdef WLAN_FEATURE_11BE_MLO_ADV_FEATURE
		status = mlo_cm_roam_sync_cb(cm_ctx->vdev, data, data_len);
		if (status == QDF_STATUS_E_PENDING) {
			/* SMD async execution started. CM stays in ROAM_SYNC
			 * substate; Link Recfg SM delivers ROAM_DONE when all
			 * link switches complete.
			 */
			break;
		}
		if (QDF_IS_STATUS_ERROR(status)) {
			event_handled = false;
			break;
		}
#endif
		status = cm_fw_send_vdev_roam_event(cm_ctx, data_len, data);
		if (QDF_IS_STATUS_ERROR(status))
			event_handled = false;
		break;
	case WLAN_CM_SM_EV_ROAM_DONE:
		cm_sm_transition_to(cm_ctx, WLAN_CM_S_CONNECTED);
		cm_sm_deliver_event_sync(cm_ctx, event,
					 data_len, data);
		break;
	case WLAN_CM_SM_EV_ROAM_ABORT:
	case WLAN_CM_SM_EV_ROAM_HO_FAIL:
		cm_sm_transition_to(cm_ctx, WLAN_CM_S_CONNECTED);
		cm_sm_deliver_event_sync(cm_ctx, event,
					 data_len, data);
		break;
	default:
		event_handled = false;
		break;
	}

	return event_handled;
}
#endif /* WLAN_FEATURE_ROAM_OFFLOAD */

#ifdef WLAN_FEATURE_11BN_SMD
void cm_subst_smd_roam_sync_entry(void *ctx)
{
	struct cnx_mgr *cm_ctx = ctx;

	if (cm_get_state(cm_ctx) != WLAN_CM_S_CONNECTED)
		QDF_BUG(0);

	/*
	 * Only the assoc vdev enters this sub-state.
	 * Link vdevs follow the IDLE_DUE_TO_LINK_SWITCH path.
	 */
	if (wlan_vdev_mlme_is_mlo_vdev(cm_ctx->vdev) &&
	    !wlan_vdev_mlme_is_assoc_sta_vdev(cm_ctx->vdev))
		QDF_BUG(0);

	cm_set_substate(cm_ctx, WLAN_CM_SS_SMD_ROAM_SYNC);
}

void cm_subst_smd_roam_sync_exit(void *ctx)
{
}

bool cm_subst_smd_roam_sync_event(void *ctx, uint16_t event,
				  uint16_t data_len, void *data)
{
	struct cnx_mgr *cm_ctx = ctx;
	bool event_handled = true;
	struct cm_req *roam_cm_req;
	QDF_STATUS status;

	switch (event) {
	case WLAN_CM_SM_EV_SMD_EXEC_COMPLETE:
		/*
		 * data = struct wlan_roam_synch_complete_params * built by
		 * smd_exec_complete() with vdev_repurpose_resp[] populated.
		 * Send WMI_ROAM_SYNCH_COMPLETE first, then run cleanup.
		 * Delivery is synchronous — the stack pointer from
		 * smd_exec_complete() is still valid here.
		 */
		status = wlan_cm_tgt_send_roam_sync_complete_cmd(
					wlan_vdev_get_psoc(cm_ctx->vdev),
					(struct wlan_roam_synch_complete_params *)data);

		/* ALWAYS release PM, even on error */
		wlan_cm_tgt_allow_pm_after_roam_sync(
					wlan_vdev_get_psoc(cm_ctx->vdev),
					wlan_vdev_get_id(cm_ctx->vdev));
		/* smd_roam_update_sta_ctx_links() was already called in
		 * smd_exec_complete() before target_bss_ctx was freed.
		 */
		smd_roam_update_deflink(cm_ctx->vdev);

		if (QDF_IS_STATUS_ERROR(status)) {
			event_handled = false;
			break;
		}

		smd_remove_roam_cmd(cm_ctx);

		/* Notify supplicant */
		mlme_cm_osif_roam_complete(cm_ctx->vdev);

		cm_sm_transition_to(cm_ctx, WLAN_CM_SS_IDLE);
		break;

	case WLAN_CM_SM_EV_ROAM_SYNC:
		/*
		 * Continuation ROAM_SYNC (vdev_repurpose_req > 0, ML→ML):
		 * route through mlo_cm_roam_sync_cb() which returns E_PENDING
		 * and triggers smd_trigger_link_recfg_sm() for next link.
		 * Unexpected ROAM_SYNC with no vdev_repurpose (race) → warn.
		 */
		status = mlo_cm_roam_sync_cb(cm_ctx->vdev, data, data_len);
		if (status == QDF_STATUS_E_PENDING)
			break; /* Link Recfg SM handles next link */
		if (QDF_IS_STATUS_ERROR(status)) {
			event_handled = false;
			break;
		}
		mlme_warn("vdev %d: unexpected ROAM_SYNC in SMD_ROAM_SYNC",
			  wlan_vdev_get_id(cm_ctx->vdev));
		break;

	case WLAN_CM_SM_EV_ROAM_START:
		/*
		 * FW triggered a legacy ROAM after the SMD prep phase.
		 * Remove the current SMD roam request from serialization,
		 * prepare a new roam cmd, then hand off to the normal
		 * FW-roam path via cm_handle_fw_roaming_event() which
		 * serializes it and transitions to WLAN_CM_SS_ROAM_STARTED.
		 */
		{
			struct cm_roam_req *old_roam_req;
			wlan_cm_id cm_id = CM_ID_INVALID;

			old_roam_req = cm_get_first_roam_command(cm_ctx->vdev);
			if (old_roam_req)
				cm_id = old_roam_req->cm_id;
			if (cm_id != CM_ID_INVALID)
				cm_remove_cmd(cm_ctx, &cm_id);
		}

		status = cm_prepare_roam_cmd(cm_ctx, &roam_cm_req,
					     CM_ROAMING_FW, data);
		if (QDF_IS_STATUS_ERROR(status)) {
			mlme_err("CM vdev %d: failed to prepare roam cmd",
				 wlan_vdev_get_id(cm_ctx->vdev));
			event_handled = false;
			break;
		}

		cm_sm_transition_to(cm_ctx, WLAN_CM_S_ROAMING);
		cm_sm_deliver_event_sync(cm_ctx, event,
					 sizeof(*roam_cm_req), roam_cm_req);
		break;

	case WLAN_CM_SM_EV_DISCONNECT_REQ:
		/* Disconnect while waiting: abort, release PM, disconnect */
		smd_abort_roam_sync(cm_ctx->vdev);
		mlme_cm_rso_stop_req(cm_ctx->vdev);
		wlan_cm_tgt_allow_pm_after_roam_sync(
					wlan_vdev_get_psoc(cm_ctx->vdev),
					wlan_vdev_get_id(cm_ctx->vdev));
		status = cm_add_disconnect_req_to_list(cm_ctx, data);
		if (QDF_IS_STATUS_ERROR(status)) {
			event_handled = false;
			break;
		}
		cm_sm_transition_to(cm_ctx, WLAN_CM_S_DISCONNECTING);
		cm_sm_deliver_event_sync(cm_ctx,
					 WLAN_CM_SM_EV_DISCONNECT_START,
					 data_len, data);
		break;

	case WLAN_CM_SM_EV_ROAM_ABORT:
	case WLAN_CM_SM_EV_ROAM_HO_FAIL:
		/*
		 * FW aborted after link switch completed.
		 * Cleanup without SYNCH_COMPLETE — FW already terminated.
		 * Update sta_ctx links from the partial target context before
		 * freeing it, so any completed link state is reflected.
		 * Free the prepared target context that
		 * will never be activated.
		 */
		smd_roam_update_sta_ctx_links(cm_ctx->vdev);
		smd_roam_update_deflink(cm_ctx->vdev);
		wlan_cm_tgt_allow_pm_after_roam_sync(
					wlan_vdev_get_psoc(cm_ctx->vdev),
					wlan_vdev_get_id(cm_ctx->vdev));
		cm_sm_transition_to(cm_ctx, WLAN_CM_SS_IDLE);
		break;

	default:
		event_handled = false;
		break;
	}

	return event_handled;
}
#endif /* WLAN_FEATURE_11BN_SMD */
