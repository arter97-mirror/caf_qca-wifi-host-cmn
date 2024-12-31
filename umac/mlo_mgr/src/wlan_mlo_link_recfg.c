/*
 * Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
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

/*
 * DOC: contains MLO manager Link Reconfiguration related functionality
 */
#include <wlan_mlo_mgr_link_switch.h>
#include <wlan_mlo_link_recfg.h>
#include <wlan_mlo_mgr_main.h>
#include <wlan_mlo_mgr_sta.h>
#include <wlan_serialization_api.h>
#include <wlan_cm_api.h>
#include <wlan_crypto_def_i.h>
#include <wlan_sm_engine.h>
#ifdef WLAN_FEATURE_11BE_MLO
#include "wlan_cm_roam_api.h"
#include <wlan_mlo_mgr_roam.h>
#include "wlan_dlm_api.h"
#include "wlan_dp_ucfg_api.h"
#include "target_if_mlo_mgr.h"
#include "utils_mlo.h"
#include <../../core/src/wlan_cm_vdev_api.h>
#endif
#include "host_diag_core_event.h"
#include "lim_types.h"

static struct mlo_link_recfg_state_tran *
mlo_link_recfg_get_curr_tran_req(struct mlo_link_recfg_context *recfg_ctx);

static struct wlan_mlo_dev_context *
mlo_link_recfg_get_mlo_ctx(struct mlo_link_recfg_context *recfg_ctx)
{
	return recfg_ctx->ml_dev;
}

static struct wlan_objmgr_psoc *
mlo_link_recfg_get_psoc(struct mlo_link_recfg_context *recfg_ctx)
{
	return recfg_ctx->psoc;
}

bool mlo_is_link_recfg_in_progress(struct wlan_objmgr_vdev *vdev)
{
	enum wlan_link_recfg_sm_state curr_state;

	if (!vdev || !vdev->mlo_dev_ctx || !vdev->mlo_dev_ctx->link_recfg_ctx)
		return false;

	ml_link_recfg_sm_lock_acquire(vdev->mlo_dev_ctx);
	curr_state = vdev->mlo_dev_ctx->link_recfg_ctx->sm.link_recfg_state;
	ml_link_recfg_sm_lock_release(vdev->mlo_dev_ctx);

	if (curr_state != WLAN_LINK_RECFG_S_INIT)
		return true;

	return false;
}

void mlo_link_recfg_init_state(struct wlan_mlo_dev_context *mlo_dev_ctx)
{
	ml_link_recfg_sm_lock_acquire(mlo_dev_ctx);

	ml_link_recfg_sm_lock_release(mlo_dev_ctx);
}

void
mlo_link_recfg_trans_abort_state(struct wlan_mlo_dev_context *mlo_dev_ctx)
{
	ml_link_recfg_sm_lock_acquire(mlo_dev_ctx);

	ml_link_recfg_sm_lock_release(mlo_dev_ctx);
}

enum wlan_link_recfg_sm_state
mlo_link_recfg_get_state(struct wlan_mlo_dev_context *mlo_dev_ctx)
{
	enum wlan_link_recfg_sm_state curr_state;

	if (!mlo_dev_ctx)
		return WLAN_LINK_RECFG_S_MAX;

	ml_link_recfg_sm_lock_acquire(mlo_dev_ctx);
	curr_state = mlo_dev_ctx->link_recfg_ctx->sm.link_recfg_state;
	ml_link_recfg_sm_lock_release(mlo_dev_ctx);

	return curr_state;
}

enum wlan_link_recfg_sm_state
mlo_link_recfg_get_substate(struct wlan_mlo_dev_context *mlo_dev_ctx)
{
	enum wlan_link_recfg_sm_state curr_substate;

	if (!mlo_dev_ctx)
		return WLAN_LINK_RECFG_SS_MAX;

	ml_link_recfg_sm_lock_acquire(mlo_dev_ctx);
	curr_substate = mlo_dev_ctx->link_recfg_ctx->sm.link_recfg_substate;
	ml_link_recfg_sm_lock_release(mlo_dev_ctx);

	return curr_substate;
}

QDF_STATUS
mlo_link_recfg_sm_deliver_event_sync(struct wlan_mlo_dev_context *mlo_dev_ctx,
				     enum wlan_link_recfg_sm_evt event,
				     uint16_t data_len, void *data)
{
	return wlan_sm_dispatch(mlo_dev_ctx->link_recfg_ctx->sm.sm_hdl,
				event, data_len, data);
}

QDF_STATUS
mlo_link_recfg_sm_deliver_event(struct wlan_mlo_dev_context *mlo_dev_ctx,
				enum wlan_link_recfg_sm_evt event,
				uint16_t data_len, void *data)
{
	QDF_STATUS status;

	if (!mlo_dev_ctx)
		return QDF_STATUS_E_NULL_VALUE;

	ml_link_recfg_sm_lock_acquire(mlo_dev_ctx);
	status = mlo_link_recfg_sm_deliver_event_sync(mlo_dev_ctx,
						      event,
						      data_len, data);
	ml_link_recfg_sm_lock_release(mlo_dev_ctx);

	return status;
}

static void
mlo_link_recfg_sm_transition_to(struct mlo_link_recfg_context *recfg_ctx,
				enum wlan_link_recfg_sm_state state)
{
	wlan_sm_transition_to(recfg_ctx->sm.sm_hdl, state);
}

uint8_t
mlo_link_recfg_dialog_token(struct mlo_link_recfg_state_req *req)
{
	if (!req)
		return 0;

	if (req->dialog_token == WLAN_MAX_DIALOG_TOKEN)
		/* wrap is ok */
		req->dialog_token = WLAN_MIN_DIALOG_TOKEN;
	else
		req->dialog_token += 1;

	mlo_debug("gen dialog token %d", req->dialog_token);
	return req->dialog_token;
}

/**
 * mlo_remove_link_recfg_cmd() - The API will remove the link reconfig
 * command from active serialization queue.
 * @recfg_ctx: link recfg context
 *
 * Once link reconfig process on @vdev is completed either in success of failure
 * case, the API removes the link reconfig command from serialization queue.
 *
 * Return: QDF_STATUS
 */
static QDF_STATUS
mlo_remove_link_recfg_cmd(struct mlo_link_recfg_context *recfg_ctx)
{
	struct wlan_serialization_queued_cmd_info cmd = {0};
	struct wlan_mlo_link_recfg_req *recfg_req;
	struct wlan_objmgr_psoc *psoc;
	struct wlan_objmgr_vdev *vdev;

	psoc = mlo_link_recfg_get_psoc(recfg_ctx);
	if (!psoc) {
		mlo_err("psoc null");
		return QDF_STATUS_E_INVAL;
	}
	recfg_req = &recfg_ctx->curr_recfg_req;
	vdev = wlan_objmgr_get_vdev_by_id_from_psoc(
					psoc,
					recfg_req->vdev_id,
					WLAN_LINK_RECFG_ID);
	if (!vdev) {
		mlo_debug("invalid vdev for id %d",
			  recfg_req->vdev_id);
		return QDF_STATUS_E_INVAL;
	}

	cmd.cmd_id = 0;
	cmd.req_type = WLAN_SER_CANCEL_NON_SCAN_CMD;
	cmd.cmd_type = WLAN_SER_CMD_LINK_RECFG;
	cmd.vdev = vdev;
	cmd.queue_type = WLAN_SERIALIZATION_ACTIVE_QUEUE;
	cmd.requestor = WLAN_UMAC_COMP_MLO_MGR;
	wlan_serialization_remove_cmd(&cmd);

	wlan_objmgr_vdev_release_ref(vdev, WLAN_LINK_RECFG_ID);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS
mlo_link_recfg_ser_active(struct wlan_objmgr_vdev *vdev)
{
	struct wlan_mlo_dev_context *mlo_dev_ctx;

	mlo_dev_ctx = vdev->mlo_dev_ctx;
	if (!mlo_dev_ctx) {
		mlo_err("mlo_ctx null");
		return QDF_STATUS_E_INVAL;
	}

	return mlo_link_recfg_sm_deliver_event(mlo_dev_ctx,
					       WLAN_LINK_RECFG_SM_EV_ACTIVE,
					       0, NULL);
}

static QDF_STATUS
mlo_link_recfg_ser_timeout(struct wlan_objmgr_vdev *vdev)
{
	struct wlan_mlo_dev_context *mlo_dev_ctx;

	mlo_dev_ctx = vdev->mlo_dev_ctx;
	if (!mlo_dev_ctx) {
		mlo_err("mlo_ctx null");
		return QDF_STATUS_E_INVAL;
	}

	return mlo_link_recfg_sm_deliver_event(
				mlo_dev_ctx,
				WLAN_LINK_RECFG_SM_EV_SER_TIMEOUT,
				0, NULL);
}

/**
 * mlo_ser_link_recfg_cb() - Link recfg Serialization callback
 * @cmd: Serialization command info
 * @reason: Serialization reason for callback execution
 *
 * Return: Status of callback execution
 */
static QDF_STATUS
mlo_ser_link_recfg_cb(struct wlan_serialization_command *cmd,
		      enum wlan_serialization_cb_reason reason)
{
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	struct wlan_objmgr_vdev *vdev;

	if (!cmd || !cmd->vdev)
		return QDF_STATUS_E_FAILURE;

	vdev = cmd->vdev;
	switch (reason) {
	case WLAN_SER_CB_ACTIVATE_CMD:
		mlo_link_recfg_ser_active(vdev);
		break;
	case WLAN_SER_CB_CANCEL_CMD:
		/* command removed from pending list. */
		break;
	case WLAN_SER_CB_ACTIVE_CMD_TIMEOUT:
		mlo_link_recfg_ser_timeout(vdev);
		break;
	case WLAN_SER_CB_RELEASE_MEM_CMD:
		wlan_objmgr_vdev_release_ref(vdev, WLAN_LINK_RECFG_ID);
		break;
	default:
		QDF_ASSERT(0);
		status = QDF_STATUS_E_INVAL;
		break;
	}

	return status;
}

#define MLO_LINK_RECFG_MAX_TIMEOUT 35000

/**
 * mlo_ser_link_recfg_cmd() - The API will serialize link reconfig
 * command in serialization queue.
 * @recfg_ctx: link recfg ctx
 * @recfg_req: Link reconfig request parameters
 *
 * On receiving link reconfig request with valid parameters from FW or user,
 * this API will serialize the link reconfig command and later to procced for
 * link reconfig once the command comes to active queue.
 *
 * Return: QDF_STATUS
 */
static QDF_STATUS
mlo_ser_link_recfg_cmd(struct mlo_link_recfg_context *recfg_ctx,
		       struct wlan_mlo_link_recfg_req *recfg_req)
{
	struct wlan_mlo_dev_context *mlo_dev_ctx;
	struct wlan_objmgr_psoc *psoc;
	struct wlan_objmgr_vdev *vdev;
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	enum wlan_serialization_status ser_cmd_status;
	struct wlan_serialization_command cmd = {0};

	mlo_dev_ctx = mlo_link_recfg_get_mlo_ctx(recfg_ctx);
	if (!mlo_dev_ctx) {
		mlo_err("mlo_ctx null");
		return QDF_STATUS_E_INVAL;
	}
	psoc = mlo_link_recfg_get_psoc(recfg_ctx);
	if (!psoc) {
		mlo_err("psoc null");
		return QDF_STATUS_E_INVAL;
	}

	vdev = wlan_objmgr_get_vdev_by_id_from_psoc(
					psoc,
					recfg_req->vdev_id,
					WLAN_LINK_RECFG_ID);
	if (!vdev) {
		mlo_debug("invalid vdev for id %d",
			  recfg_req->vdev_id);
		return QDF_STATUS_E_INVAL;
	}

	mlo_dev_lock_acquire(mlo_dev_ctx);
	qdf_mem_copy(&recfg_ctx->curr_recfg_req, recfg_req,
		     sizeof(*recfg_req));
	mlo_dev_lock_release(mlo_dev_ctx);

	cmd.cmd_type = WLAN_SER_CMD_LINK_RECFG;
	cmd.cmd_id = 0;
	cmd.cmd_cb = mlo_ser_link_recfg_cb;
	cmd.source = WLAN_UMAC_COMP_MLO_MGR;
	cmd.is_high_priority = false;
	cmd.cmd_timeout_duration = MLO_LINK_RECFG_MAX_TIMEOUT;
	cmd.vdev = vdev;
	cmd.is_blocking = true;
	cmd.umac_cmd = mlo_dev_ctx;

	ser_cmd_status = wlan_serialization_request(&cmd);
	switch (ser_cmd_status) {
	case WLAN_SER_CMD_PENDING:
		mlo_debug("Link recfg cmd in pending queue");
		break;
	case WLAN_SER_CMD_ACTIVE:
		mlo_debug("Link recfg cmd in active queue");
		break;
	default:
		status = QDF_STATUS_E_INVAL;
		break;
	}

	if (QDF_IS_STATUS_SUCCESS(status))
		return status;

	wlan_objmgr_vdev_release_ref(vdev, WLAN_LINK_RECFG_ID);

	return status;
}

QDF_STATUS mlo_link_recfg_notify(struct wlan_objmgr_vdev *vdev,
				 struct wlan_mlo_link_recfg_req *req)
{
	return QDF_STATUS_SUCCESS;
}

static inline QDF_STATUS
mlo_link_recfg_update_channel_freq(struct wlan_objmgr_psoc *psoc,
				   struct wlan_mlo_dev_context *mlo_dev_ctx,
				   struct wlan_mlo_link_recfg_req *recfg_req)
{
	struct wlan_objmgr_pdev *pdev;
	struct scan_cache_entry *scan_entry;
	uint8_t i;
	struct wlan_mlo_link_recfg_bss_info *link;
	struct mlo_link_info *ap_link_info;
	QDF_STATUS status = QDF_STATUS_SUCCESS;

	pdev = wlan_objmgr_get_pdev_by_id(psoc, 0, WLAN_LINK_RECFG_ID);
	if (!pdev) {
		mlo_err("Invalid pdev");
		return QDF_STATUS_E_INVAL;
	}
	if (!recfg_req) {
		mlo_err("recfg_req null");
		return QDF_STATUS_E_INVAL;
	}
	link = &recfg_req->add_link_info.link[0];
	for (i = 0; i < recfg_req->add_link_info.num_links &&
			i < WLAN_MAX_ML_BSS_LINKS; i++) {
		scan_entry = wlan_scan_get_entry_by_bssid(pdev,
							  &link[i].ap_link_addr);
		if (!scan_entry) {
			mlo_debug("add link " QDF_MAC_ADDR_FMT " scan entry not found",
				  QDF_MAC_ADDR_REF(link[i].ap_link_addr.bytes));
			status = QDF_STATUS_E_INVAL;
			break;
		}
		link[i].freq = scan_entry->channel.chan_freq;
		mlo_debug("add: freq %d link id %d " QDF_MAC_ADDR_FMT "",
			  link[i].freq, link[i].link_id,
			  QDF_MAC_ADDR_REF(link[i].ap_link_addr.bytes));

		util_scan_free_cache_entry(scan_entry);
	}
	link = &recfg_req->del_link_info.link[0];
	for (i = 0; i < recfg_req->del_link_info.num_links &&
			i < WLAN_MAX_ML_BSS_LINKS; i++) {
		ap_link_info =
		mlo_mgr_get_ap_link_by_link_id(mlo_dev_ctx,
					       link[i].link_id);
		if (!ap_link_info) {
			mlo_debug("del link " QDF_MAC_ADDR_FMT " link info not found",
				  QDF_MAC_ADDR_REF(link[i].ap_link_addr.bytes));
			status = QDF_STATUS_E_INVAL;
			break;
		}
		if (!ap_link_info->link_chan_info) {
			mlo_debug("del link " QDF_MAC_ADDR_FMT " ch info not found",
				  QDF_MAC_ADDR_REF(link[i].ap_link_addr.bytes));
			status = QDF_STATUS_E_INVAL;
			break;
		}

		link[i].freq = ap_link_info->link_chan_info->ch_freq;
		link[i].self_link_addr = ap_link_info->link_addr;
		mlo_debug("del: freq %d link id %d " QDF_MAC_ADDR_FMT " self addr " QDF_MAC_ADDR_FMT "",
			  link[i].freq, link[i].link_id,
			  QDF_MAC_ADDR_REF(link[i].ap_link_addr.bytes),
			  QDF_MAC_ADDR_REF(link[i].self_link_addr.bytes));
	}

	wlan_objmgr_pdev_release_ref(pdev, WLAN_LINK_RECFG_ID);

	return status;
}

QDF_STATUS
mlo_mgr_link_recfg_indication_event_handler(
			struct wlan_objmgr_psoc *psoc,
			struct wlan_mlo_link_recfg_ind_param *evt_params)
{
	struct wlan_objmgr_vdev *vdev;
	struct wlan_mlo_link_recfg_req recfg_req = {0};
	QDF_STATUS status;
	struct wlan_mlo_dev_context *mlo_dev_ctx;

	if (!evt_params) {
		mlo_err("Invalid params");
		return QDF_STATUS_E_INVAL;
	}

	vdev = wlan_objmgr_get_vdev_by_id_from_psoc(psoc, evt_params->vdev_id,
						    WLAN_LINK_RECFG_ID);
	if (!vdev) {
		mlo_err("Invalid link recfg VDEV %d", evt_params->vdev_id);
		return QDF_STATUS_E_INVAL;
	}
	mlo_dev_ctx = vdev->mlo_dev_ctx;
	if (!mlo_dev_ctx) {
		mlo_err("mlo_ctx null");
		status = QDF_STATUS_E_INVAL;
		goto end;
	}

	qdf_mem_zero(&recfg_req, sizeof(struct wlan_mlo_link_recfg_req));
	recfg_req.vdev_id = evt_params->vdev_id;
	recfg_req.is_user_req = evt_params->trigger_reason ==
				ROAM_TRIGGER_REASON_FORCED;
	recfg_req.is_fw_ind_received = true;
	recfg_req.add_link_info = evt_params->add_link;
	recfg_req.del_link_info = evt_params->del_link;
	recfg_req.fw_ind_param = *evt_params;
	/* update channel frequency of add/del link info, fw event will
	 * not include such info.
	 */
	status = mlo_link_recfg_update_channel_freq(
			psoc, mlo_dev_ctx, &recfg_req);
	if (QDF_IS_STATUS_ERROR(status)) {
		mlo_debug("failed to find link freq for fw link recfg ind event");
		goto end;
	}

	status = mlo_link_recfg_sm_deliver_event(
				mlo_dev_ctx,
				WLAN_LINK_RECFG_SM_EV_FW_IND,
				sizeof(recfg_req), &recfg_req);
end:
	wlan_objmgr_vdev_release_ref(vdev, WLAN_LINK_RECFG_ID);

	return status;
}

QDF_STATUS
mlo_link_recfg_linksw_start_indication(struct wlan_objmgr_vdev *vdev,
				       QDF_STATUS start_status)
{
	struct wlan_mlo_dev_context *mlo_dev_ctx;
	QDF_STATUS status;
	struct link_switch_ind link_switch_ind = {0};

	mlo_dev_ctx = vdev->mlo_dev_ctx;
	if (!mlo_dev_ctx) {
		mlo_err("mlo_ctx null");
		return QDF_STATUS_E_INVAL;
	}
	if (!mlo_dev_ctx->link_recfg_ctx)
		return QDF_STATUS_SUCCESS;

	link_switch_ind.status = start_status;
	status = mlo_link_recfg_sm_deliver_event(
				mlo_dev_ctx,
				WLAN_LINK_RECFG_SM_EV_LINK_SWITCH_IND,
				sizeof(link_switch_ind), &link_switch_ind);

	return status;
}

QDF_STATUS
mlo_link_recfg_linksw_completion_indication(struct wlan_objmgr_vdev *vdev,
					    QDF_STATUS comp_status)
{
	struct wlan_mlo_dev_context *mlo_dev_ctx;
	QDF_STATUS status;
	struct link_switch_rsp link_switch_rsp = {0};

	mlo_dev_ctx = vdev->mlo_dev_ctx;
	if (!mlo_dev_ctx) {
		mlo_err("mlo_ctx null");
		return QDF_STATUS_E_INVAL;
	}
	if (!mlo_dev_ctx->link_recfg_ctx)
		return QDF_STATUS_E_INVAL;

	link_switch_rsp.status = comp_status;
	status = mlo_link_recfg_sm_deliver_event(
				mlo_dev_ctx,
				WLAN_LINK_RECFG_SM_EV_LINK_SWITCH_RSP,
				sizeof(link_switch_rsp), &link_switch_rsp);

	return status;
}

QDF_STATUS
mlo_link_recfg_add_connect_done_indication(
				struct wlan_objmgr_vdev *vdev,
				QDF_STATUS comp_status)
{
	struct wlan_mlo_dev_context *mlo_dev_ctx;
	QDF_STATUS status;
	struct add_link_conn_rsp add_link_conn_rsp = {0};

	mlo_dev_ctx = vdev->mlo_dev_ctx;
	if (!mlo_dev_ctx) {
		mlo_err("mlo_ctx null");
		return QDF_STATUS_E_INVAL;
	}
	if (!mlo_dev_ctx->link_recfg_ctx)
		return QDF_STATUS_SUCCESS;

	add_link_conn_rsp.status = comp_status;
	status = mlo_link_recfg_sm_deliver_event(
				mlo_dev_ctx,
				WLAN_LINK_RECFG_SM_EV_ADD_CONN_RSP,
				sizeof(add_link_conn_rsp), &add_link_conn_rsp);

	return status;
}

QDF_STATUS
mlo_link_recfg_validate_request(struct wlan_objmgr_vdev *vdev,
				struct wlan_mlo_link_recfg_req *req)
{
	return QDF_STATUS_SUCCESS;
}

QDF_STATUS mlo_link_recfg_request_params(struct wlan_objmgr_psoc *psoc,
					 void *evt_params)
{
	QDF_STATUS status = QDF_STATUS_SUCCESS;

	if (!evt_params) {
		mlo_err("Invalid params");
		return QDF_STATUS_E_INVAL;
	}

	return status;
}

static QDF_STATUS
mlo_link_recfg_get_link_bitmap(struct mlo_link_recfg_context *recfg_ctx,
			       struct wlan_mlo_link_recfg_req *recfg_req,
			       uint32_t *add_link_set,
			       uint8_t *add_link_num,
			       uint32_t *del_link_set,
			       uint8_t *del_link_num,
			       uint32_t *curr_link_set,
			       uint8_t *curr_link_num,
			       uint32_t *curr_standby_set,
			       uint8_t *curr_standby_num)
{
	struct wlan_mlo_dev_context *mlo_dev_ctx;
	struct mlo_link_info *link_info;
	uint8_t i;
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	struct wlan_mlo_link_recfg_bss_info *link;

	mlo_dev_ctx = mlo_link_recfg_get_mlo_ctx(recfg_ctx);
	if (!mlo_dev_ctx) {
		mlo_err("mlo_ctx null");
		return QDF_STATUS_E_INVAL;
	}

	for (i = 0; i < WLAN_MAX_ML_BSS_LINKS; i++) {
		link_info = &mlo_dev_ctx->link_ctx->links_info[i];
		if (qdf_is_macaddr_zero(&link_info->ap_link_addr))
			continue;

		if (link_info->link_id == WLAN_INVALID_LINK_ID)
			continue;

		if (qdf_atomic_test_bit(
				LS_F_AP_REMOVAL_BIT,
				&link_info->link_status_flags)) {
			mlo_debug("skip del ap link addr: " QDF_MAC_ADDR_FMT" link flag 0x%x",
				  QDF_MAC_ADDR_REF(
					link_info->ap_link_addr.bytes),
				  (uint32_t)link_info->link_status_flags);
			continue;
		}
		*curr_link_set |= 1 << link_info->link_id;
		(*curr_link_num)++;

		if (link_info->vdev_id == WLAN_INVALID_VDEV_ID) {
			*curr_standby_set |= 1 << link_info->link_id;
			(*curr_standby_num)++;
		}
	}

	link = &recfg_req->add_link_info.link[0];
	for (i = 0; i < recfg_req->add_link_info.num_links; i++)
		*add_link_set |= 1 << link[i].link_id;
	*add_link_num = recfg_req->add_link_info.num_links;

	link = &recfg_req->del_link_info.link[0];
	for (i = 0; i < recfg_req->del_link_info.num_links; i++)
		*del_link_set |= 1 << link[i].link_id;
	*del_link_num = recfg_req->del_link_info.num_links;

	mlo_debug("add link num %d bitmap 0x%x del num %d bitmap 0x%x curr num %d bitmap 0x%x standby %d 0x%x",
		  *add_link_num, *add_link_set,
		  *del_link_num, *del_link_set,
		  *curr_link_num, *curr_link_set,
		  *curr_standby_num, *curr_standby_set);

	return status;

}

static bool
mlo_link_recfg_has_idle_vdev_for_add_link(
				struct mlo_link_recfg_context *recfg_ctx,
				struct mlo_link_recfg_state_req *req)
{
	struct wlan_objmgr_psoc *psoc;
	struct wlan_mlo_link_recfg_bss_info *link_add;
	struct wlan_mlo_link_recfg_bss_info tmp;
	bool idle_vdev_assigned = false;
	uint8_t i;
	struct wlan_objmgr_vdev *vdev;

	if (!req->add_link_info.num_links) {
		mlo_err("unexpected add link num 0");
		return false;
	}

	psoc = mlo_link_recfg_get_psoc(recfg_ctx);
	if (!psoc) {
		mlo_err("psoc null");
		return false;
	}

	for (i = 0; i < req->add_link_info.num_links; i++) {
		link_add = &req->add_link_info.link[i];

		if (link_add->vdev_id == WLAN_INVALID_VDEV_ID)
			continue;

		vdev = wlan_objmgr_get_vdev_by_id_from_psoc(
					psoc, link_add->vdev_id,
					WLAN_LINK_RECFG_ID);
		if (!vdev) {
			mlo_err("Invalid VDEV id %d", link_add->vdev_id);
			continue;
		}

		if (!cm_is_vdev_disconnected(vdev)) {
			wlan_objmgr_vdev_release_ref(vdev, WLAN_LINK_RECFG_ID);
			continue;
		}
		wlan_objmgr_vdev_release_ref(vdev, WLAN_LINK_RECFG_ID);
		/* todo: add validate vdev mac with link info link_add */

		idle_vdev_assigned = true;
		mlo_debug("idle vdev %d self mac " QDF_MAC_ADDR_FMT " assigned to add link %d ap addr " QDF_MAC_ADDR_FMT "",
			  link_add->vdev_id,
			  QDF_MAC_ADDR_REF(
				link_add->self_link_addr.bytes),
			  link_add->link_id,
			  QDF_MAC_ADDR_REF(
				link_add->ap_link_addr.bytes));
		/* move the link to first slot for trigger connect in
		 * mlo_link_recfg_add_link_connect.
		 */
		tmp = req->add_link_info.link[0];
		req->add_link_info.link[0] = *link_add;
		*link_add = tmp;
		break;
	}

	return idle_vdev_assigned;
}

static struct mlo_link_info *
mlo_link_recfg_find_link_info_with_active_vdev(
	struct wlan_objmgr_psoc *psoc,
	struct wlan_mlo_dev_context *mlo_dev_ctx,
	struct wlan_mlo_link_recfg_bss_info *link_add)
{
	struct wlan_objmgr_vdev *vdev;
	struct mlo_link_info *link_info;
	uint8_t *vdev_mac;
	uint8_t j;

	for (j = 0; j < WLAN_MAX_ML_BSS_LINKS; j++) {
		link_info = &mlo_dev_ctx->link_ctx->links_info[j];

		if (link_info->vdev_id == WLAN_INVALID_VDEV_ID)
			continue;

		if (link_info->link_id != link_add->link_id)
			continue;

		if (!qdf_is_macaddr_equal(&link_add->self_link_addr,
					  &link_info->link_addr)) {
			mlo_err("link %d info self " QDF_MAC_ADDR_FMT " not equal ADD_LINK: " QDF_MAC_ADDR_FMT "",
				link_info->link_id,
				QDF_MAC_ADDR_REF(link_add->self_link_addr.bytes),
				QDF_MAC_ADDR_REF(link_add->self_link_addr.bytes));
			continue;
		}

		vdev = wlan_objmgr_get_vdev_by_id_from_psoc(
					psoc, link_info->vdev_id,
					WLAN_LINK_RECFG_ID);
		if (!vdev) {
			mlo_err("Invalid VDEV id %d", link_info->vdev_id);
			continue;
		}

		if (!cm_is_vdev_connected(vdev)) {
			wlan_objmgr_vdev_release_ref(vdev, WLAN_LINK_RECFG_ID);
			continue;
		}

		vdev_mac = wlan_vdev_mlme_get_linkaddr(vdev);
		if (!qdf_is_macaddr_equal(&link_info->link_addr,
					  (struct qdf_mac_addr *)vdev_mac)) {
			mlo_err("vdev %d MAC address not equal " QDF_MAC_ADDR_FMT " link info self " QDF_MAC_ADDR_FMT "",
				link_info->vdev_id,
				QDF_MAC_ADDR_REF(vdev_mac),
				QDF_MAC_ADDR_REF(link_info->link_addr.bytes));
			wlan_objmgr_vdev_release_ref(vdev, WLAN_LINK_RECFG_ID);
			continue;
		}

		wlan_objmgr_vdev_release_ref(vdev, WLAN_LINK_RECFG_ID);
		return link_info;
	}

	return NULL;
}

static bool
mlo_link_recfg_has_active_vdev_for_add_link(
				struct mlo_link_recfg_context *recfg_ctx,
				struct mlo_link_recfg_state_req *req,
				struct wlan_mlo_link_switch_req *link_sw_req)
{
	struct wlan_objmgr_psoc *psoc;
	struct wlan_mlo_link_recfg_bss_info *link_add;
	uint8_t i;
	struct mlo_link_info *link_info;
	struct wlan_mlo_dev_context *mlo_dev_ctx;

	if (!req->add_link_info.num_links) {
		mlo_err("unexpected add link num 0");
		return false;
	}

	mlo_dev_ctx = mlo_link_recfg_get_mlo_ctx(recfg_ctx);
	if (!mlo_dev_ctx) {
		mlo_err("mlo_ctx null");
		return QDF_STATUS_E_INVAL;
	}

	psoc = mlo_link_recfg_get_psoc(recfg_ctx);
	if (!psoc) {
		mlo_err("psoc null");
		return QDF_STATUS_E_INVAL;
	}

	/* find an added link which has active vdev, trigger link switch
	 * disconnect and reconnect.
	 */
	for (i = 0; i < req->add_link_info.num_links; i++) {
		link_add = &req->add_link_info.link[i];

		/* Find the link info from mlo mgr for the added link.
		 * The link id and ap link address have been updated to
		 * mlo mgr after receive the recfg response frame.
		 * here only find the link info which has "connected" vdev
		 * (on an old deleted link), and then trigger link switch
		 * by host with reason MLO_LINK_SWITCH_REASON_HOST_ADD_LINK.
		 */
		link_info = mlo_link_recfg_find_link_info_with_active_vdev(
						psoc,
						mlo_dev_ctx,
						link_add);
		if (!link_info)
			continue;
		link_add->vdev_id = link_info->vdev_id;
		mlo_debug("assign active vdev %d self link addr: " QDF_MAC_ADDR_FMT " for add link %d freq %d",
			  link_info->vdev_id,
			  QDF_MAC_ADDR_REF(link_info->link_addr.bytes),
			  link_add->link_id,
			  link_add->freq);
		mlo_debug("old link id %d flag 0x%x on vdev %d ",
			  link_info->link_id,
			  (uint32_t)link_info->link_status_flags,
			  link_info->vdev_id);
		link_sw_req->vdev_id = link_add->vdev_id;
		link_sw_req->curr_ieee_link_id = link_info->link_id;
		link_sw_req->new_ieee_link_id = link_add->link_id;
		link_sw_req->new_primary_freq = link_add->freq;
		link_sw_req->new_phymode = 0;
		link_sw_req->reason = MLO_LINK_SWITCH_REASON_HOST_ADD_LINK;
		return true;
	}

	return false;
}

static bool
mlo_link_recfg_is_standby_link_del_only(
				struct mlo_link_recfg_context *recfg_ctx,
				struct mlo_link_recfg_state_req *req)
{
	struct wlan_mlo_dev_context *mlo_dev_ctx;
	uint8_t i;
	struct mlo_link_info *link_info;

	mlo_dev_ctx = mlo_link_recfg_get_mlo_ctx(recfg_ctx);
	if (!mlo_dev_ctx) {
		mlo_err("mlo_ctx null");
		return false;
	}

	for (i = 0; i < req->del_link_info.num_links; i++) {
		link_info = mlo_mgr_get_ap_link_by_link_id(
				mlo_dev_ctx,
				req->del_link_info.link[i].link_id);
		if (!link_info) {
			mlo_debug("unexpected link info null for link id %d",
				  req->del_link_info.link[i].link_id);
			continue;
		}

		if (link_info->vdev_id != WLAN_INVALID_VDEV_ID) {
			mlo_debug("del non standby link %d on vdev %d",
				  req->del_link_info.link[i].link_id,
				  link_info->vdev_id);
			return false;
		}
		mlo_debug("del standby link %d flag 0x%x vdev id %d",
			  req->del_link_info.link[i].link_id,
			  (uint32_t)link_info->link_status_flags,
			  link_info->vdev_id);
	}

	return true;
}

static bool
mlo_link_recfg_is_standby_link_present_for_link_switch(
				struct mlo_link_recfg_context *recfg_ctx)
{
	struct wlan_mlo_dev_context *mlo_dev_ctx;
	struct mlo_link_info *link_info;
	uint8_t i;

	mlo_dev_ctx = mlo_link_recfg_get_mlo_ctx(recfg_ctx);
	if (!mlo_dev_ctx) {
		mlo_err("mlo_ctx null");
		return false;
	}

	for (i = 0; i < WLAN_MAX_ML_BSS_LINKS; i++) {
		link_info = &mlo_dev_ctx->link_ctx->links_info[i];

		if (qdf_is_macaddr_zero(&link_info->ap_link_addr))
			continue;

		if (link_info->link_id == WLAN_INVALID_LINK_ID) {
			mlo_debug("invalid link id %d for ap link addr: " QDF_MAC_ADDR_FMT "",
				  link_info->link_id,
				  QDF_MAC_ADDR_REF(
				  link_info->ap_link_addr.bytes));
			continue;
		}

		if (qdf_atomic_test_bit(
				LS_F_AP_REMOVAL_BIT,
				&link_info->link_status_flags)) {
			mlo_debug("deleted link %d flag 0x%x ap link addr: " QDF_MAC_ADDR_FMT "",
				  link_info->link_id,
				  (uint32_t)link_info->link_status_flags,
				  QDF_MAC_ADDR_REF(
				  link_info->ap_link_addr.bytes));

			continue;
		}

		if (link_info->vdev_id == WLAN_INVALID_VDEV_ID) {
			mlo_debug("associated standby link %d present ap link addr: " QDF_MAC_ADDR_FMT "",
				  link_info->link_id,
				  QDF_MAC_ADDR_REF(
				  link_info->ap_link_addr.bytes));
			return true;
		}
	}
	mlo_debug("no standby link for link sw");

	return false;
}

static bool
mlo_link_recfg_is_link_switch_in_progress(
				struct mlo_link_recfg_context *recfg_ctx)
{
	struct wlan_mlo_dev_context *mlo_dev_ctx;
	uint8_t i;

	mlo_dev_ctx = mlo_link_recfg_get_mlo_ctx(recfg_ctx);
	if (!mlo_dev_ctx) {
		mlo_err("mlo_ctx null");
		return false;
	}

	for (i = 0; i < WLAN_UMAC_MLO_MAX_VDEVS; i++) {
		if (!mlo_dev_ctx->wlan_vdev_list[i])
			continue;
		if (mlo_mgr_is_link_switch_in_progress(
				mlo_dev_ctx->wlan_vdev_list[i]))
			return true;
	}

	return false;
}

static void
mlo_link_recfg_update_scan_mlme(struct wlan_objmgr_vdev *vdev,
				struct qdf_mac_addr *ap_link_addr,
				enum scan_entry_connection_state
				assoc_state)
{
	struct mlo_link_info *link_info;
	struct mlme_info mlme_info = {0};
	struct bss_info bss_info = {0};
	struct wlan_objmgr_vdev *assoc_vdev =
		wlan_mlo_get_assoc_link_vdev(vdev);
	QDF_STATUS status;

	if (!assoc_vdev) {
		mlo_debug("assoc vdev null");
		return;
	}
	link_info = mlo_mgr_get_ap_link_info(vdev, ap_link_addr);
	if (!link_info) {
		mlo_debug("link info null ap link addr " QDF_MAC_ADDR_FMT "",
			  QDF_MAC_ADDR_REF(ap_link_addr->bytes));
		return;
	}
	status = wlan_vdev_mlme_get_ssid(vdev, bss_info.ssid.ssid,
					 &bss_info.ssid.length);

	if (QDF_IS_STATUS_ERROR(status)) {
		mlo_debug("vdev %d failed to get ssid",
			  wlan_vdev_get_id(assoc_vdev));
		return;
	}

	mlme_info.assoc_state = assoc_state;
	qdf_copy_macaddr(&bss_info.bssid, ap_link_addr);
	bss_info.freq = link_info->link_chan_info->ch_freq;
	wlan_scan_update_mlme_by_bssinfo(wlan_vdev_get_pdev(vdev),
					 &bss_info, &mlme_info);
}

static void
mlo_link_recfg_remove_deleted_standby_in_mlo_mgr(
				struct mlo_link_recfg_context *recfg_ctx,
				struct mlo_link_recfg_state_req *req)
{
	struct wlan_mlo_dev_context *mlo_dev_ctx;
	uint8_t i;
	struct mlo_link_info *link_info;
	struct wlan_objmgr_vdev *vdev;
	struct wlan_objmgr_psoc *psoc;

	mlo_dev_ctx = mlo_link_recfg_get_mlo_ctx(recfg_ctx);
	if (!mlo_dev_ctx) {
		mlo_err("mlo_ctx null");
		return;
	}

	psoc = mlo_link_recfg_get_psoc(recfg_ctx);
	if (!psoc) {
		mlo_err("psoc is null");
		return;
	}

	vdev = wlan_objmgr_get_vdev_by_id_from_psoc(
				psoc, recfg_ctx->curr_recfg_req.vdev_id,
				WLAN_LINK_RECFG_ID);
	if (!vdev) {
		mlo_err("Invalid link recfg VDEV %d",
			recfg_ctx->curr_recfg_req.vdev_id);
		return;
	}

	for (i = 0; i < req->del_link_info.num_links; i++) {
		link_info = mlo_mgr_get_ap_link_by_link_id(
				mlo_dev_ctx,
				req->del_link_info.link[i].link_id);
		if (!link_info) {
			mlo_debug("unexpected link info null for link id %d",
				  req->del_link_info.link[i].link_id);
			continue;
		}

		if (!qdf_atomic_test_bit(
				LS_F_AP_REMOVAL_BIT,
				&link_info->link_status_flags))
			continue;

		if (link_info->vdev_id != WLAN_INVALID_VDEV_ID) {
			mlo_debug("skip deleted non standby link %d on vdev %d",
				  req->del_link_info.link[i].link_id,
				  link_info->vdev_id);
			continue;
		}

		mlo_debug("del standby link %d flag 0x%x vdev id %d from mlo mgr",
			  req->del_link_info.link[i].link_id,
			  (uint32_t)link_info->link_status_flags,
			  link_info->vdev_id);
		mlo_link_recfg_update_scan_mlme(vdev,
						&link_info->ap_link_addr,
						SCAN_ENTRY_CON_STATE_NONE);

		mlo_mgr_clear_ap_link_info(vdev, &link_info->ap_link_addr);
	}

	wlan_objmgr_vdev_release_ref(vdev, WLAN_LINK_RECFG_ID);
}

static QDF_STATUS
mlo_link_recfg_update_added_link_in_mlo_mgr(
				struct mlo_link_recfg_context *recfg_ctx,
				struct mlo_link_recfg_state_req *req)
{
	struct wlan_mlo_dev_context *mlo_dev_ctx;
	uint8_t i;
	struct mlo_link_info *link_info;
	struct wlan_objmgr_vdev *vdev = NULL;
	struct wlan_objmgr_psoc *psoc = NULL;
	struct wlan_mlo_link_recfg_bss_info *add_link;
	struct wlan_objmgr_pdev *pdev = NULL;
	struct scan_cache_entry *scan_entry;
	struct wlan_channel channel;
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	bool link_update_only = false;

	if (!req->add_link_info.num_links)
		return QDF_STATUS_SUCCESS;

	mlo_dev_ctx = mlo_link_recfg_get_mlo_ctx(recfg_ctx);
	if (!mlo_dev_ctx) {
		mlo_err("mlo_ctx null");
		return QDF_STATUS_E_INVAL;
	}

	psoc = mlo_link_recfg_get_psoc(recfg_ctx);
	if (!psoc) {
		mlo_err("psoc is null");
		return QDF_STATUS_E_INVAL;
	}
	pdev = wlan_objmgr_get_pdev_by_id(psoc, 0, WLAN_LINK_RECFG_ID);
	if (!pdev) {
		mlo_err("Invalid pdev");
		status = QDF_STATUS_E_INVAL;
		goto end;
	}

	vdev = wlan_objmgr_get_vdev_by_id_from_psoc(
				psoc, recfg_ctx->curr_recfg_req.vdev_id,
				WLAN_LINK_RECFG_ID);
	if (!vdev) {
		mlo_err("Invalid link recfg VDEV %d",
			recfg_ctx->curr_recfg_req.vdev_id);
		status = QDF_STATUS_E_INVAL;
		goto end;
	}

	for (i = 0; i < req->add_link_info.num_links; i++) {
		add_link = &req->add_link_info.link[i];

		/* todo: check link add status_code before update link info */

		link_info = mlo_mgr_get_link_info_by_self_addr(
				vdev,
				&add_link->self_link_addr);
		if (!link_info) {
			mlo_debug("unexpected link info null for link id %d self mac " QDF_MAC_ADDR_FMT "",
				  add_link->link_id,
				  QDF_MAC_ADDR_REF(add_link->self_link_addr.bytes));
			status = QDF_STATUS_E_INVAL;
			break;
		}
		if (link_info->link_id == add_link->link_id &&
		    recfg_ctx->curr_recfg_req.recfg_type ==
			link_recfg_del_add_no_common_link) {
			/* add special check for no common link case,
			 * in which we update mlo mgr before action frame sent
			 */
			if (recfg_ctx->curr_recfg_req.join_pending_vdev_id !=
				link_info->vdev_id) {
				mlo_debug("unexpected join pending vdev %d, link info %d link id %d",
					  recfg_ctx->curr_recfg_req.join_pending_vdev_id,
					  link_info->vdev_id,
					  link_info->link_id);
				status = QDF_STATUS_E_INVAL;
				break;
			}
			link_update_only = true;
		} else if (link_info->link_id != WLAN_INVALID_LINK_ID &&
			   !qdf_atomic_test_bit(
				LS_F_AP_REMOVAL_BIT,
				&link_info->link_status_flags)) {
			mlo_debug("can't updated link %d to mgr connected link id %d ap link " QDF_MAC_ADDR_FMT "",
				  add_link->link_id,
				  link_info->link_id,
				  QDF_MAC_ADDR_REF(add_link->ap_link_addr.bytes));
			status = QDF_STATUS_E_INVAL;
			break;
		} else if (link_info->link_id == add_link->link_id) {
			/* link delete and add back case, for example:
			 * AB->A->AB, do not flush cached link assoc respone,
			 * since the cache is updated after receive link recfg
			 * response for same link id.
			 */
			link_update_only = true;
		}

		scan_entry = wlan_scan_get_entry_by_bssid(pdev,
							  &add_link->ap_link_addr);
		if (!scan_entry) {
			mlo_debug("add link " QDF_MAC_ADDR_FMT " scan entry not found",
				  QDF_MAC_ADDR_REF(add_link->ap_link_addr.bytes));
			status = QDF_STATUS_E_INVAL;
			break;
		}
		qdf_mem_zero(&channel, sizeof(channel));
		channel.ch_freq = scan_entry->channel.chan_freq;
		channel.ch_ieee = wlan_reg_freq_to_chan(pdev, channel.ch_freq);
		channel.ch_phymode = scan_entry->phy_mode;
		channel.ch_cfreq1 = scan_entry->channel.cfreq0;
		channel.ch_cfreq2 = scan_entry->channel.cfreq1;
		channel.ch_width =
			wlan_mlme_get_ch_width_from_phymode(scan_entry->phy_mode);

		if (channel.ch_width == CH_WIDTH_20MHZ)
			channel.ch_cfreq1 = channel.ch_freq;
		if (!link_update_only)
			mlo_free_cache_link_assoc_rsp(vdev, link_info->link_id);

		mlo_debug("updated self link mac " QDF_MAC_ADDR_FMT " vdev %d ",
			  QDF_MAC_ADDR_REF(link_info->link_addr.bytes),
			  link_info->vdev_id);

		mlo_debug("delete old link id %d ap link " QDF_MAC_ADDR_FMT "",
			  link_info->link_id,
			  QDF_MAC_ADDR_REF(link_info->ap_link_addr.bytes));
		mlo_debug("update to added link id %d ap link " QDF_MAC_ADDR_FMT " ch %d phymode %d",
			  add_link->link_id,
			  QDF_MAC_ADDR_REF(add_link->ap_link_addr.bytes),
			  channel.ch_freq,
			  channel.ch_phymode);

		mlo_dev_lock_acquire(mlo_dev_ctx);
		qdf_mem_copy(&link_info->ap_link_addr, &add_link->ap_link_addr,
			     QDF_MAC_ADDR_SIZE);
		qdf_mem_copy(link_info->link_chan_info, &channel,
			     sizeof(struct wlan_channel));
		link_info->link_status_flags = 0;
		link_info->link_id = add_link->link_id;
		link_info->is_link_active = false;
		link_info->chan_freq = add_link->freq;
		link_info->link_status_code = STATUS_SUCCESS;
		mlo_dev_lock_release(mlo_dev_ctx);
		mlo_link_recfg_update_scan_mlme(vdev,
						&link_info->ap_link_addr,
						SCAN_ENTRY_CON_STATE_ASSOC);

		util_scan_free_cache_entry(scan_entry);
	}
end:
	if (vdev)
		wlan_objmgr_vdev_release_ref(vdev, WLAN_LINK_RECFG_ID);
	if (pdev)
		wlan_objmgr_pdev_release_ref(pdev, WLAN_LINK_RECFG_ID);

	return status;
}

QDF_STATUS
mlo_link_recfg_get_add_partner_links(
		struct wlan_objmgr_vdev *vdev,
		struct mlo_partner_info *ml_partner_info)
{
	struct wlan_mlo_dev_context *mlo_dev_ctx;
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	struct mlo_link_recfg_context *recfg_ctx;
	struct mlo_link_recfg_state_tran *tran;
	struct wlan_mlo_link_recfg_bss_info *link_add;
	struct mlo_link_info *link_info;
	uint8_t i;
	uint8_t idx = 0;

	mlo_dev_ctx = vdev->mlo_dev_ctx;
	if (!mlo_dev_ctx) {
		mlo_err("mlo_ctx null");
		return QDF_STATUS_E_INVAL;
	}
	if (!mlo_dev_ctx->link_recfg_ctx)
		return QDF_STATUS_E_INVAL;

	recfg_ctx = mlo_dev_ctx->link_recfg_ctx;

	ml_link_recfg_sm_lock_acquire(mlo_dev_ctx);
	tran = mlo_link_recfg_get_curr_tran_req(recfg_ctx);
	if (!tran) {
		mlo_err("curr tran null");
		ml_link_recfg_sm_lock_release(mlo_dev_ctx);
		return QDF_STATUS_E_INVAL;
	};

	for (i = 0; i < tran->req.add_link_info.num_links; i++) {
		link_add = &tran->req.add_link_info.link[i];
		if (link_add->status_code != STATUS_SUCCESS) {
			mlo_debug("ignore link for status_code %d",
				  link_add->status_code);
			continue;
		}
		if (link_add->link_id == wlan_vdev_get_link_id(vdev))
			continue;
		link_info = mlo_mgr_get_ap_link_by_link_id(
				mlo_dev_ctx,
				link_add->link_id);
		if (!link_info) {
			mlo_debug("unexpected link info null for link id %d",
				  link_add->link_id);
			status = QDF_STATUS_E_INVAL;
			break;
		}
		if (idx >= WLAN_MAX_ML_BSS_LINKS) {
			mlo_debug("unexpected no buff to add link id %d",
				  link_add->link_id);
			status = QDF_STATUS_E_INVAL;
			break;
		}
		ml_partner_info->partner_link_info[idx].link_addr =
			link_add->ap_link_addr;
		ml_partner_info->partner_link_info[idx].link_id =
			link_add->link_id;
		ml_partner_info->partner_link_info[idx].vdev_id =
			link_info->vdev_id;
		ml_partner_info->partner_link_info[idx].link_status_code =
			link_add->status_code;

		mlo_debug("add new partner link id %d ap bssid " QDF_MAC_ADDR_FMT " vdev %d ",
			  link_add->link_id,
			  QDF_MAC_ADDR_REF(link_info->ap_link_addr.bytes),
			  link_info->vdev_id);
		idx++;
	}
	ml_partner_info->num_partner_links = idx;
	ml_link_recfg_sm_lock_release(mlo_dev_ctx);

	return status;
}

static QDF_STATUS
mlo_link_recfg_update_partner_info(struct mlo_link_recfg_context *recfg_ctx)
{
	struct wlan_mlo_sta *sta_ctx;
	struct mlo_partner_info *ml_partner_info;
	struct mlo_link_info *link_info;
	uint8_t i, idx = 0;
	struct wlan_mlo_dev_context *mlo_dev_ctx;

	mlo_dev_ctx = mlo_link_recfg_get_mlo_ctx(recfg_ctx);
	if (!mlo_dev_ctx) {
		mlo_err("mlo_ctx null");
		return QDF_STATUS_E_NULL_VALUE;
	}
	sta_ctx = mlo_dev_ctx->sta_ctx;
	if (!sta_ctx) {
		mlo_err("sta_ctx null");
		return QDF_STATUS_E_NULL_VALUE;
	}
	ml_partner_info = &sta_ctx->ml_partner_info;
	for (i = 0; i < WLAN_MAX_ML_BSS_LINKS; i++) {
		if (idx >= QDF_ARRAY_SIZE(ml_partner_info->partner_link_info))
			break;
		link_info = &mlo_dev_ctx->link_ctx->links_info[i];

		if (qdf_is_macaddr_zero(&link_info->ap_link_addr))
			continue;

		if (link_info->link_id == WLAN_INVALID_LINK_ID)
			continue;

		if (link_info->link_status_code)
			continue;

		ml_partner_info->partner_link_info[idx].link_addr =
			link_info->ap_link_addr;
		ml_partner_info->partner_link_info[idx].link_id =
			link_info->link_id;
		ml_partner_info->partner_link_info[idx].vdev_id =
			link_info->vdev_id;
		ml_partner_info->partner_link_info[idx].link_status_code =
			link_info->link_status_code;
		ml_partner_info->partner_link_info[idx].chan_freq =
			link_info->link_chan_info->ch_freq;
		mlo_debug("[%d] update partner link id %d vdev %d",
			  idx,
			  ml_partner_info->partner_link_info[idx].link_id,
			  ml_partner_info->partner_link_info[idx].vdev_id);
		idx++;
	}
	ml_partner_info->num_partner_links = idx;

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS
mlo_link_recfg_del_standby_link(struct mlo_link_recfg_context *recfg_ctx,
				struct mlo_link_recfg_state_req *req)
{
	struct wlan_objmgr_psoc *psoc;
	struct wlan_mlo_dev_context *mlo_dev_ctx;
	struct wlan_lmac_if_mlo_tx_ops *mlo_tx_ops;
	QDF_STATUS status;
	struct mlo_link_bss_params params;
	struct wlan_channel chan = {0};
	struct mlo_link_info *link_info;

	psoc = mlo_link_recfg_get_psoc(recfg_ctx);
	if (!psoc) {
		mlo_err("psoc is null");
		return QDF_STATUS_E_NULL_VALUE;
	}

	mlo_dev_ctx = mlo_link_recfg_get_mlo_ctx(recfg_ctx);
	if (!mlo_dev_ctx) {
		mlo_err("mlo_ctx null");
		return QDF_STATUS_E_NULL_VALUE;
	}

	mlo_tx_ops = target_if_mlo_get_tx_ops(psoc);
	if (!mlo_tx_ops || !mlo_tx_ops->send_link_set_bss_params_cmd) {
		mlo_err("tx_ops or send_link_set_bss_params_cmd is null!");
		return QDF_STATUS_E_NULL_VALUE;
	}

	/* assumption is only one standby link existing */
	if (req->del_link_info.num_links != 1) {
		mlo_err("unexpected standby link del num %d",
			req->del_link_info.num_links);
		return QDF_STATUS_E_INVAL;
	}

	link_info = mlo_mgr_get_ap_link_by_link_id(
			mlo_dev_ctx,
			req->del_link_info.link[0].link_id);
	if (!link_info) {
		mlo_debug("unexpected link info null for link id %d",
			  req->del_link_info.link[0].link_id);
		return QDF_STATUS_E_INVAL;
	}
	if (!link_info->link_chan_info) {
		mlo_debug("unexpected link ch info null for link id %d",
			  req->del_link_info.link[0].link_id);
		return QDF_STATUS_E_INVAL;
	}

	/* update link with link deleted flag */
	mlo_mgr_update_link_state_delete_flag(
				mlo_dev_ctx,
				req->del_link_info.link[0].link_id,
				true);

	qdf_mem_zero(&params, sizeof(params));
	*(struct qdf_mac_addr *)&params.ap_mld_mac[0] =
		recfg_ctx->curr_recfg_req.fw_ind_param.ap_mld_addr;
	params.link_id = req->del_link_info.link[0].link_id;
	params.op_code = MLO_LINK_BSS_OP_DEL;
	params.chan = &chan;
	params.chan->ch_freq = link_info->link_chan_info->ch_freq;
	params.chan->ch_cfreq1 = link_info->link_chan_info->ch_cfreq1;
	params.chan->ch_cfreq2 = link_info->link_chan_info->ch_cfreq2;
	params.chan->ch_phymode = link_info->link_chan_info->ch_phymode;

	mlo_debug("link id %d chan freq %d cfreq1 %d cfreq2 %d host phymode %d ap mld mac " QDF_MAC_ADDR_FMT,
		  link_info->link_id, link_info->link_chan_info->ch_freq,
		  link_info->link_chan_info->ch_cfreq1,
		  link_info->link_chan_info->ch_cfreq2,
		  link_info->link_chan_info->ch_phymode,
		  QDF_MAC_ADDR_REF(&params.ap_mld_mac[0]));

	status = mlo_tx_ops->send_link_set_bss_params_cmd(psoc, &params);
	if (QDF_IS_STATUS_ERROR(status)) {
		mlo_err("failed to send link set bss request command to FW");
		return QDF_STATUS_E_NULL_VALUE;
	}

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS
mlo_link_recfg_add_standby_link(struct mlo_link_recfg_context *recfg_ctx,
				struct mlo_link_recfg_state_req *req)
{
	struct wlan_objmgr_psoc *psoc;
	struct wlan_mlo_dev_context *mlo_dev_ctx;
	struct wlan_lmac_if_mlo_tx_ops *mlo_tx_ops;
	QDF_STATUS status;
	struct mlo_link_bss_params params;
	struct mlo_link_info *link_info;
	struct wlan_objmgr_vdev *recfg_vdev = NULL;

	psoc = mlo_link_recfg_get_psoc(recfg_ctx);
	if (!psoc) {
		mlo_err("psoc is null");
		return QDF_STATUS_E_NULL_VALUE;
	}

	mlo_dev_ctx = mlo_link_recfg_get_mlo_ctx(recfg_ctx);
	if (!mlo_dev_ctx) {
		mlo_err("mlo_ctx null");
		return QDF_STATUS_E_NULL_VALUE;
	}

	mlo_tx_ops = target_if_mlo_get_tx_ops(psoc);
	if (!mlo_tx_ops || !mlo_tx_ops->send_link_set_bss_params_cmd) {
		mlo_err("tx_ops or send_link_set_bss_params_cmd is null!");
		return QDF_STATUS_E_NULL_VALUE;
	}

	/* assumption is only one standby link existing */
	if (req->add_link_info.num_links != 1) {
		mlo_err("unexpected standby link add num %d",
			req->add_link_info.num_links);
		return QDF_STATUS_E_INVAL;
	}

	link_info = mlo_mgr_get_ap_link_by_link_id(
			mlo_dev_ctx,
			req->add_link_info.link[0].link_id);
	if (!link_info) {
		mlo_debug("unexpected link info null for link id %d",
			  req->add_link_info.link[0].link_id);
		return QDF_STATUS_E_INVAL;
	}
	if (!link_info->link_chan_info) {
		mlo_debug("unexpected link ch info null for link id %d",
			  req->add_link_info.link[0].link_id);
		return QDF_STATUS_E_INVAL;
	}

	qdf_mem_zero(&params, sizeof(params));
	*(struct qdf_mac_addr *)&params.ap_mld_mac[0] =
		recfg_ctx->curr_recfg_req.fw_ind_param.ap_mld_addr;
	params.link_id = req->add_link_info.link[0].link_id;
	params.op_code = MLO_LINK_BSS_OP_ADD;
	params.chan = link_info->link_chan_info;
	params.ap_link_addr = req->add_link_info.link[0].ap_link_addr;
	params.self_link_addr = req->add_link_info.link[0].self_link_addr;

	mlo_debug("link id %d chan freq %d cfreq1 %d cfreq2 %d host phymode %d ap mld mac " QDF_MAC_ADDR_FMT,
		  link_info->link_id, link_info->link_chan_info->ch_freq,
		  link_info->link_chan_info->ch_cfreq1,
		  link_info->link_chan_info->ch_cfreq2,
		  link_info->link_chan_info->ch_phymode,
		  QDF_MAC_ADDR_REF(&params.ap_mld_mac[0]));

	status = mlo_tx_ops->send_link_set_bss_params_cmd(psoc, &params);
	if (QDF_IS_STATUS_ERROR(status)) {
		mlo_err("failed to send link set bss request command to FW");
		return QDF_STATUS_E_NULL_VALUE;
	}
	recfg_vdev = wlan_objmgr_get_vdev_by_id_from_psoc(
				psoc, recfg_ctx->curr_recfg_req.vdev_id,
				WLAN_LINK_RECFG_ID);
	if (!recfg_vdev) {
		mlo_err("Invalid link recfg VDEV %d",
			recfg_ctx->curr_recfg_req.vdev_id);
		wlan_objmgr_vdev_release_ref(recfg_vdev, WLAN_LINK_RECFG_ID);
		return QDF_STATUS_E_INVAL;
	}

	mlo_mgr_osif_update_connect_info(recfg_vdev,
					 req->add_link_info.link[0].link_id);
	wlan_objmgr_vdev_release_ref(recfg_vdev, WLAN_LINK_RECFG_ID);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS
mlo_link_recfg_add_link_connect(struct wlan_objmgr_vdev *recfg_vdev,
				struct mlo_link_recfg_state_req *req)
{
	struct wlan_objmgr_psoc *psoc;
	QDF_STATUS status = QDF_STATUS_E_INVAL;
	struct wlan_cm_connect_req conn_req = {0};
	struct mlo_link_info *mlo_link_info;
	uint8_t *vdev_mac;
	struct wlan_mlo_sta *sta_ctx;
	struct wlan_mlo_dev_context *mlo_dev_ctx;
	struct wlan_objmgr_vdev *assoc_vdev;
	struct wlan_objmgr_vdev *vdev = NULL;
	struct wlan_mlo_link_recfg_bss_info *link_add;

	psoc = wlan_vdev_get_psoc(recfg_vdev);
	if (!psoc) {
		mlo_err("psoc null");
		return QDF_STATUS_E_INVAL;
	}

	mlo_dev_ctx = recfg_vdev->mlo_dev_ctx;
	if (!mlo_dev_ctx) {
		mlo_err("mlo_ctx null");
		return QDF_STATUS_E_INVAL;
	}

	assoc_vdev = wlan_mlo_get_assoc_link_vdev(recfg_vdev);
	if (!assoc_vdev) {
		mlo_err("assoc_vdev not found for recfg vdev id %d",
			wlan_vdev_get_id(recfg_vdev));
		return QDF_STATUS_E_INVAL;
	}

	/* we have moved the added link to first slot */
	link_add = &req->add_link_info.link[0];
	mlo_link_info = mlo_mgr_get_ap_link_by_link_id(mlo_dev_ctx,
						       link_add->link_id);
	if (!mlo_link_info) {
		mlo_err("Add link info not found, id %d", link_add->link_id);
		return QDF_STATUS_E_INVAL;
	}

	vdev = wlan_objmgr_get_vdev_by_id_from_psoc(
				psoc, link_add->vdev_id,
				WLAN_LINK_RECFG_ID);
	if (!vdev) {
		mlo_err("Invalid link add VDEV %d",
			link_add->vdev_id);
		return QDF_STATUS_E_INVAL;
	}

	vdev_mac = wlan_vdev_mlme_get_linkaddr(vdev);
	if (!qdf_is_macaddr_equal(&mlo_link_info->link_addr,
				  (struct qdf_mac_addr *)vdev_mac)) {
		mlo_err("vdev MAC address not equal for the mlo mgr Link ID VDEV: " QDF_MAC_ADDR_FMT ", MLO_LINK: " QDF_MAC_ADDR_FMT"",
			QDF_MAC_ADDR_REF(vdev_mac),
			QDF_MAC_ADDR_REF(mlo_link_info->link_addr.bytes));
		goto out;
	}

	if (!qdf_is_macaddr_equal(&link_add->self_link_addr,
				  (struct qdf_mac_addr *)vdev_mac)) {
		mlo_err("vdev MAC address not equal for the Add Link ID VDEV: " QDF_MAC_ADDR_FMT ", ADD_LINK: " QDF_MAC_ADDR_FMT"",
			QDF_MAC_ADDR_REF(vdev_mac),
			QDF_MAC_ADDR_REF(link_add->ap_link_addr.bytes));
		goto out;
	}

	sta_ctx = mlo_dev_ctx->sta_ctx;
	copied_conn_req_lock_acquire(sta_ctx);
	if (sta_ctx->copied_conn_req) {
		qdf_mem_copy(&conn_req, sta_ctx->copied_conn_req,
			     sizeof(struct wlan_cm_connect_req));
	} else {
		copied_conn_req_lock_release(sta_ctx);
		goto out;
	}
	copied_conn_req_lock_release(sta_ctx);

	wlan_vdev_mlme_set_mlo_vdev(vdev);
	wlan_vdev_mlme_set_mlo_link_vdev(vdev);
	wlan_vdev_set_link_id(vdev, link_add->link_id);
	wlan_crypto_free_vdev_key(vdev);
	conn_req.vdev_id = wlan_vdev_get_id(vdev);
	conn_req.source = CM_MLO_LINK_ADD_CONNECT;
	conn_req.chan_freq = link_add->freq;
	conn_req.link_id = link_add->link_id;
	qdf_copy_macaddr(&conn_req.bssid, &link_add->ap_link_addr);
	wlan_vdev_mlme_get_ssid(assoc_vdev, conn_req.ssid.ssid,
				&conn_req.ssid.length);
	status = wlan_vdev_get_bss_peer_mld_mac(assoc_vdev, &conn_req.mld_addr);
	if (QDF_IS_STATUS_ERROR(status)) {
		mlo_debug("Get MLD addr failed");
		goto out;
	}

	conn_req.crypto.auth_type = 0;
	conn_req.ml_parnter_info = sta_ctx->ml_partner_info;
	mlo_allocate_and_copy_ies(&conn_req, sta_ctx->copied_conn_req);

	status = wlan_cm_start_connect(vdev, &conn_req);
	if (QDF_IS_STATUS_SUCCESS(status))
		mlo_update_connected_links(vdev, 1);

	wlan_cm_free_connect_req_param(&conn_req);

out:
	if (QDF_IS_STATUS_ERROR(status))
		mlo_err("VDEV %d link switch connect request failed",
			link_add->vdev_id);
	if (vdev)
		wlan_objmgr_vdev_release_ref(vdev, WLAN_LINK_RECFG_ID);

	return status;
}

struct add_link_msg {
	struct wlan_objmgr_vdev *recfg_vdev;
	struct mlo_link_recfg_state_req req;
};

static QDF_STATUS mlo_link_recfg_add_link_req_cb(struct scheduler_msg *msg)
{
	QDF_STATUS status = QDF_STATUS_E_INVAL;
	struct wlan_objmgr_vdev *recfg_vdev = NULL;
	struct add_link_msg *add_link_msg = msg->bodyptr;

	if (!add_link_msg) {
		mlo_err("add_link_msg null");
		return QDF_STATUS_E_INVAL;
	}

	recfg_vdev = add_link_msg->recfg_vdev;
	if (!recfg_vdev) {
		mlo_err("recfg_vdev null");
		goto end;
	}

	status = mlo_link_recfg_add_link_connect(recfg_vdev,
						 &add_link_msg->req);

end:
	if (recfg_vdev) {
		/* deliver conn completion message with error code */
		if (QDF_IS_STATUS_ERROR(status))
			mlo_link_recfg_add_connect_done_indication(
				recfg_vdev, status);

		wlan_objmgr_vdev_release_ref(recfg_vdev, WLAN_LINK_RECFG_ID);
	}

	qdf_mem_free(add_link_msg);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS mlo_link_recfg_add_link_flush_cb(
					struct scheduler_msg *msg)
{
	struct wlan_objmgr_vdev *recfg_vdev = NULL;
	struct add_link_msg *add_link_msg = msg->bodyptr;

	if (!add_link_msg) {
		mlo_err("add_link_msg null");
		return QDF_STATUS_E_INVAL;
	}

	recfg_vdev = add_link_msg->recfg_vdev;
	if (recfg_vdev) {
		/* deliver conn completion message with error code */
		mlo_link_recfg_add_connect_done_indication(
			recfg_vdev, QDF_STATUS_E_FAILURE);
		wlan_objmgr_vdev_release_ref(recfg_vdev, WLAN_LINK_RECFG_ID);
	} else {
		mlo_err("recfg_vdev null");
	}

	qdf_mem_free(add_link_msg);

	return QDF_STATUS_SUCCESS;
}

static inline QDF_STATUS
mlo_link_recfg_add_link_connect_async(struct mlo_link_recfg_context *recfg_ctx,
				      struct mlo_link_recfg_state_req *req)
{
	struct scheduler_msg msg = {0};
	struct wlan_objmgr_psoc *psoc;
	QDF_STATUS status;
	struct wlan_objmgr_vdev *recfg_vdev;
	struct add_link_msg *add_link_msg;

	psoc = mlo_link_recfg_get_psoc(recfg_ctx);
	if (!psoc) {
		mlo_err("psoc null");
		return QDF_STATUS_E_INVAL;
	}

	add_link_msg = qdf_mem_malloc(sizeof(*add_link_msg));
	if (!add_link_msg)
		return QDF_STATUS_E_NOMEM;

	recfg_vdev = wlan_objmgr_get_vdev_by_id_from_psoc(
				psoc, recfg_ctx->curr_recfg_req.vdev_id,
				WLAN_LINK_RECFG_ID);
	if (!recfg_vdev) {
		mlo_err("Invalid link recfg VDEV %d",
			recfg_ctx->curr_recfg_req.vdev_id);
		qdf_mem_free(add_link_msg);
		return QDF_STATUS_E_INVAL;
	}
	add_link_msg->recfg_vdev = recfg_vdev;
	add_link_msg->req = *req;
	msg.bodyptr = add_link_msg;
	msg.callback = mlo_link_recfg_add_link_req_cb;
	msg.flush_callback =
		mlo_link_recfg_add_link_flush_cb;
	status = scheduler_post_message(
			QDF_MODULE_ID_OS_IF,
			QDF_MODULE_ID_SCAN,
			QDF_MODULE_ID_OS_IF,
			&msg);

	if (QDF_IS_STATUS_ERROR(status)) {
		mlo_err("Failed to post scheduler msg");
		wlan_objmgr_vdev_release_ref(
				recfg_vdev,
				WLAN_LINK_RECFG_ID);
		qdf_mem_free(add_link_msg);
		return QDF_STATUS_E_FAULT;
	}
	mlo_debug("link add connect scheduler msg posted");

	return QDF_STATUS_SUCCESS;
}

struct link_switch_msg {
	struct wlan_objmgr_vdev *recfg_vdev;
	struct wlan_mlo_link_switch_req link_sw_req;
};

static QDF_STATUS mlo_link_recfg_link_sw_req_cb(struct scheduler_msg *msg)
{
	QDF_STATUS status = QDF_STATUS_E_INVAL;
	struct wlan_objmgr_vdev *recfg_vdev = NULL;
	struct link_switch_msg *link_switch_msg = msg->bodyptr;
	struct wlan_objmgr_vdev *vdev = NULL;
	struct wlan_objmgr_psoc *psoc;

	if (!link_switch_msg) {
		mlo_err("link_switch_msg null");
		return QDF_STATUS_E_INVAL;
	}

	recfg_vdev = link_switch_msg->recfg_vdev;
	if (!recfg_vdev) {
		mlo_err("recfg_vdev null");
		goto end;
	}

	psoc = wlan_vdev_get_psoc(recfg_vdev);
	if (!psoc) {
		mlme_err("psoc null");
		goto end;
	}

	vdev = wlan_objmgr_get_vdev_by_id_from_psoc(
				psoc, link_switch_msg->link_sw_req.vdev_id,
				WLAN_LINK_RECFG_ID);
	if (!vdev) {
		mlo_err("Invalid link_sw_req VDEV %d",
			link_switch_msg->link_sw_req.vdev_id);
		goto end;
	}

	status = mlo_mgr_link_switch_request_params(
				psoc,
				&link_switch_msg->link_sw_req);
end:
	if (recfg_vdev) {
		/* deliver conn completion message with error code */
		if (QDF_IS_STATUS_ERROR(status))
			mlo_link_recfg_linksw_completion_indication(
				recfg_vdev, status);

		wlan_objmgr_vdev_release_ref(recfg_vdev, WLAN_LINK_RECFG_ID);
	}

	if (vdev)
		wlan_objmgr_vdev_release_ref(vdev, WLAN_LINK_RECFG_ID);

	qdf_mem_free(link_switch_msg);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS mlo_link_recfg_link_sw_flush_cb(
					struct scheduler_msg *msg)
{
	struct wlan_objmgr_vdev *recfg_vdev = NULL;
	struct add_link_msg *link_switch_msg = msg->bodyptr;

	if (!link_switch_msg) {
		mlo_err("link_switch_msg null");
		return QDF_STATUS_E_INVAL;
	}

	recfg_vdev = link_switch_msg->recfg_vdev;
	if (recfg_vdev) {
		/* deliver conn completion message with error code */
		mlo_link_recfg_linksw_completion_indication(
			recfg_vdev, QDF_STATUS_E_FAILURE);
		wlan_objmgr_vdev_release_ref(recfg_vdev, WLAN_LINK_RECFG_ID);
	} else {
		mlo_err("recfg_vdev null");
	}

	qdf_mem_free(link_switch_msg);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS
mlo_link_recfg_host_trigger_link_switch(
			struct mlo_link_recfg_context *recfg_ctx,
			struct wlan_mlo_link_switch_req *link_sw_req)
{
	struct scheduler_msg msg = {0};
	struct wlan_objmgr_psoc *psoc;
	QDF_STATUS status;
	struct wlan_objmgr_vdev *recfg_vdev;
	struct link_switch_msg *link_switch_msg;

	psoc = mlo_link_recfg_get_psoc(recfg_ctx);
	if (!psoc) {
		mlo_err("psoc null");
		return QDF_STATUS_E_INVAL;
	}

	link_switch_msg = qdf_mem_malloc(sizeof(*link_switch_msg));
	if (!link_switch_msg)
		return QDF_STATUS_E_NOMEM;

	recfg_vdev = wlan_objmgr_get_vdev_by_id_from_psoc(
				psoc, recfg_ctx->curr_recfg_req.vdev_id,
				WLAN_LINK_RECFG_ID);
	if (!recfg_vdev) {
		mlo_err("Invalid link recfg VDEV %d",
			recfg_ctx->curr_recfg_req.vdev_id);
		qdf_mem_free(link_switch_msg);
		return QDF_STATUS_E_INVAL;
	}
	link_switch_msg->recfg_vdev = recfg_vdev;
	link_switch_msg->link_sw_req = *link_sw_req;
	msg.bodyptr = link_switch_msg;
	msg.callback = mlo_link_recfg_link_sw_req_cb;
	msg.flush_callback =
		mlo_link_recfg_link_sw_flush_cb;
	status = scheduler_post_message(
			QDF_MODULE_ID_OS_IF,
			QDF_MODULE_ID_SCAN,
			QDF_MODULE_ID_OS_IF,
			&msg);
	if (QDF_IS_STATUS_ERROR(status)) {
		mlo_err("Failed to post scheduler msg");
		wlan_objmgr_vdev_release_ref(
				recfg_vdev,
				WLAN_LINK_RECFG_ID);
		qdf_mem_free(link_switch_msg);
		return QDF_STATUS_E_FAULT;
	}
	mlo_debug("link switch scheduler msg posted");

	return QDF_STATUS_SUCCESS;

}

void mlo_link_recfg_set_link_resp(struct wlan_objmgr_vdev *vdev,
				  uint32_t result)
{
	QDF_STATUS status;
	struct set_link_resp set_link_rsp = {0};
	struct wlan_mlo_dev_context *mlo_dev_ctx;

	mlo_dev_ctx = vdev->mlo_dev_ctx;
	if (!mlo_dev_ctx) {
		mlo_err("invalid mlo_dev_ctx");
		return;
	}

	set_link_rsp.status = result;
	status = mlo_link_recfg_sm_deliver_event(
				mlo_dev_ctx,
				WLAN_LINK_RECFG_SM_EV_SET_LINK_RSP,
				sizeof(set_link_rsp), &set_link_rsp);
	if (QDF_IS_STATUS_ERROR(status))
		mlo_err("fail to deliver set link rsp result %d, vdev %d status %d",
			result, wlan_vdev_get_id(vdev), status);
}

QDF_STATUS mlo_link_recfg_link_add_join_req(struct wlan_objmgr_vdev *vdev)
{
	QDF_STATUS status;
	struct wlan_mlo_dev_context *mlo_dev_ctx;
	struct mlo_link_recfg_context *recfg_ctx;
	struct mlo_link_recfg_state_tran *tran;

	mlo_dev_ctx = vdev->mlo_dev_ctx;
	if (!mlo_dev_ctx) {
		mlo_err("invalid mlo_dev_ctx");
		return QDF_STATUS_E_INVAL;
	}
	recfg_ctx = mlo_dev_ctx->link_recfg_ctx;
	if (!recfg_ctx) {
		mlo_err("invalid recfg_ctx");
		return QDF_STATUS_E_INVAL;
	}

	ml_link_recfg_sm_lock_acquire(mlo_dev_ctx);
	mlo_debug("vdev %d recfg_type %d join_pending_vdev_id %d",
		  wlan_vdev_get_id(vdev),
		  recfg_ctx->curr_recfg_req.recfg_type,
		  recfg_ctx->curr_recfg_req.join_pending_vdev_id);
	/* for common link cases, we always send action frm before
	 * link add connecting. no needs pending the join request,
	 * just return success to process the cached link assoc
	 * response.
	 */
	if (recfg_ctx->curr_recfg_req.recfg_type !=
				link_recfg_del_add_no_common_link) {
		status = QDF_STATUS_SUCCESS;
		goto end;
	}
	/* for non-common link case, trigger link recfg action frame tx
	 * in the middle of link add connecting.
	 * Continue the peer assoc later after receive resp and generate
	 * link assoc response for added link, return
	 * QDF_STATUS_E_PENDING for such case.
	 */
	tran = mlo_link_recfg_get_curr_tran_req(recfg_ctx);
	if (!tran) {
		mlo_err("curr tran ctx null");
		status = QDF_STATUS_E_INVAL;
		goto end;
	}
	recfg_ctx->curr_recfg_req.join_pending_vdev_id =
			wlan_vdev_get_id(vdev);
	status = mlo_link_recfg_sm_deliver_event_sync(
					recfg_ctx->ml_dev,
					WLAN_LINK_RECFG_SM_EV_XMIT_REQ,
					sizeof(tran->req), &tran->req);
	if (QDF_IS_STATUS_ERROR(status)) {
		recfg_ctx->curr_recfg_req.join_pending_vdev_id =
			WLAN_INVALID_VDEV_ID;
		mlo_err("state %d event %d status %d",
			tran->state, tran->event, status);
	} else {
		status = QDF_STATUS_E_PENDING;
	}
end:
	ml_link_recfg_sm_lock_release(mlo_dev_ctx);

	return status;
}

static QDF_STATUS
mlo_link_recfg_link_add_join_continue(struct wlan_objmgr_vdev *vdev,
				      QDF_STATUS recfg_rsp_status)
{
	QDF_STATUS status;
	struct wlan_mlo_dev_context *mlo_dev_ctx;
	struct mlo_link_recfg_context *recfg_ctx;
	struct mlo_mgr_context *mlo_ctx = wlan_objmgr_get_mlo_ctx();
	struct wlan_objmgr_psoc *psoc;
	uint8_t join_pending_vdev_id;

	if (!mlo_ctx || !mlo_ctx->mlme_ops ||
	    !mlo_ctx->mlme_ops->mlo_mlme_ext_link_add_join_continue) {
		mlo_err("invalid link_add_join_continue cb");
		return QDF_STATUS_E_INVAL;
	}

	mlo_dev_ctx = vdev->mlo_dev_ctx;
	if (!mlo_dev_ctx) {
		mlo_err("invalid mlo_dev_ctx");
		return QDF_STATUS_E_INVAL;
	}
	recfg_ctx = mlo_dev_ctx->link_recfg_ctx;
	if (!recfg_ctx) {
		mlo_err("invalid recfg_ctx");
		return QDF_STATUS_E_INVAL;
	}
	psoc = mlo_link_recfg_get_psoc(recfg_ctx);
	if (!psoc) {
		mlo_err("invalid psoc");
		return QDF_STATUS_E_INVAL;
	}

	ml_link_recfg_sm_lock_acquire(mlo_dev_ctx);
	join_pending_vdev_id =
		recfg_ctx->curr_recfg_req.join_pending_vdev_id;
	mlo_debug("recfg_type %d join_pending_vdev_id %d",
		  recfg_ctx->curr_recfg_req.recfg_type,
		  join_pending_vdev_id);
	if (recfg_ctx->curr_recfg_req.recfg_type !=
				link_recfg_del_add_no_common_link ||
	    join_pending_vdev_id == WLAN_INVALID_VDEV_ID) {
		ml_link_recfg_sm_lock_release(mlo_dev_ctx);
		return QDF_STATUS_SUCCESS;
	}

	recfg_ctx->curr_recfg_req.join_pending_vdev_id = WLAN_INVALID_VDEV_ID;
	ml_link_recfg_sm_lock_release(mlo_dev_ctx);
	mlo_debug("continue vdev %d link add connecting, resp status %d",
		  join_pending_vdev_id, recfg_rsp_status);

	status = mlo_ctx->mlme_ops->mlo_mlme_ext_link_add_join_continue(
						psoc,
						join_pending_vdev_id,
						recfg_rsp_status);
	if (QDF_IS_STATUS_ERROR(status))
		mlo_err("link add no-common failed %d", status);

	return status;
}

static void
mlo_link_recfg_abort_link_add_no_comm(
	struct mlo_link_recfg_context *recfg_ctx)
{
	QDF_STATUS status;
	struct mlo_mgr_context *mlo_ctx = wlan_objmgr_get_mlo_ctx();
	struct wlan_objmgr_psoc *psoc;
	uint8_t join_pending_vdev_id;

	if (!mlo_ctx || !mlo_ctx->mlme_ops ||
	    !mlo_ctx->mlme_ops->mlo_mlme_ext_link_add_join_continue) {
		mlo_err("invalid link_add_join_continue cb");
		return;
	}

	psoc = mlo_link_recfg_get_psoc(recfg_ctx);
	if (!psoc) {
		mlo_err("invalid psoc");
		return;
	}

	join_pending_vdev_id =
		recfg_ctx->curr_recfg_req.join_pending_vdev_id;
	mlo_debug("recfg_type %d join_pending_vdev_id %d",
		  recfg_ctx->curr_recfg_req.recfg_type,
		  join_pending_vdev_id);
	/* for non-common link case, continue link add connecting
	 * after recevive the link recfg response.
	 */
	if (join_pending_vdev_id == WLAN_INVALID_VDEV_ID ||
	    recfg_ctx->curr_recfg_req.recfg_type !=
				link_recfg_del_add_no_common_link)
		return;

	recfg_ctx->curr_recfg_req.join_pending_vdev_id = WLAN_INVALID_VDEV_ID;
	status = mlo_ctx->mlme_ops->mlo_mlme_ext_link_add_join_continue(
						psoc,
						join_pending_vdev_id,
						QDF_STATUS_E_INVAL);
	if (QDF_IS_STATUS_ERROR(status))
		mlo_err("link add no-common failed %d", status);
}

bool mlo_link_recfg_is_start_as_active(struct wlan_objmgr_vdev *vdev)
{
	struct wlan_mlo_dev_context *mlo_dev_ctx;
	struct mlo_link_recfg_context *recfg_ctx;

	mlo_dev_ctx = vdev->mlo_dev_ctx;
	if (!mlo_dev_ctx) {
		mlo_err("invalid mlo_dev_ctx");
		return false;
	}
	recfg_ctx = mlo_dev_ctx->link_recfg_ctx;
	if (!recfg_ctx) {
		mlo_err("invalid recfg_ctx");
		return false;
	}

	ml_link_recfg_sm_lock_acquire(mlo_dev_ctx);
	mlo_debug("recfg_type %d", recfg_ctx->curr_recfg_req.recfg_type);
	if (recfg_ctx->curr_recfg_req.recfg_type !=
				link_recfg_del_add_no_common_link) {
		ml_link_recfg_sm_lock_release(mlo_dev_ctx);
		return false;
	}

	ml_link_recfg_sm_lock_release(mlo_dev_ctx);

	return true;
}

static QDF_STATUS
mlo_link_recfg_del_link_by_inact(
		struct mlo_link_recfg_context *recfg_ctx,
		struct mlo_link_recfg_state_req *req)
{
	struct wlan_objmgr_vdev *vdev;
	struct wlan_mlo_link_recfg_req *recfg_req;
	QDF_STATUS status;
	struct wlan_objmgr_psoc *psoc;
	uint16_t del_link_bitmap = 0;
	uint8_t i;
	struct wlan_mlo_dev_context *mlo_dev_ctx;

	psoc = mlo_link_recfg_get_psoc(recfg_ctx);
	if (!psoc) {
		mlo_err("invalid psoc");
		return QDF_STATUS_E_INVAL;
	}

	mlo_dev_ctx = mlo_link_recfg_get_mlo_ctx(recfg_ctx);
	if (!mlo_dev_ctx) {
		mlo_err("invalid mlo dev ctx");
		return QDF_STATUS_E_INVAL;
	}

	if (policy_mgr_is_set_link_in_progress(psoc)) {
		mlo_err("unexpected set link in progress");
		return QDF_STATUS_E_INVAL;
	}

	recfg_req = &recfg_ctx->curr_recfg_req;
	vdev = wlan_objmgr_get_vdev_by_id_from_psoc(psoc, recfg_req->vdev_id,
						    WLAN_MLO_MGR_ID);
	if (!vdev) {
		mlo_err("Invalid link recfg VDEV %d", recfg_req->vdev_id);
		return QDF_STATUS_E_INVAL;
	}

	for (i = 0; i < req->del_link_info.num_links; i++) {
		del_link_bitmap |= 1 << req->del_link_info.link[i].link_id;
		mlo_mgr_update_link_state_delete_flag(
					mlo_dev_ctx,
					req->del_link_info.link[i].link_id,
					true);
	}
	mlo_debug("vdev %d delete link bitmap 0x%x", recfg_req->vdev_id,
		  del_link_bitmap);

	status =
	policy_mgr_mlo_sta_set_nlink(psoc, wlan_vdev_get_id(vdev),
				     MLO_LINK_FORCE_REASON_LINK_DELETE,
				     MLO_LINK_FORCE_MODE_INACTIVE,
				     0,
				     del_link_bitmap,
				     0,
				     link_ctrl_f_dont_reschedule_workqueue |
				     link_ctrl_f_link_recfg);

	wlan_objmgr_vdev_release_ref(vdev, WLAN_MLO_MGR_ID);

	return status;
}

QDF_STATUS
mlo_link_recfg_send_request_frame(
		struct mlo_link_recfg_context *recfg_ctx,
		struct mlo_link_recfg_state_req *req)
{
	struct wlan_action_frame_args args;
	struct link_recfg_tx_result tx_result;
	struct wlan_objmgr_vdev *vdev = NULL;
	struct wlan_objmgr_peer *peer = NULL;
	QDF_STATUS status;
	uint8_t vdev_id;

	if (!req) {
		mlo_err("Link recfg req is null");
		return QDF_STATUS_E_NULL_VALUE;
	}

	if (!recfg_ctx) {
		mlo_err("Link recfg ctx is null");
		return QDF_STATUS_E_NULL_VALUE;
	}

	args.category = ACTION_CATEGORY_PROTECTED_EHT;
	args.action = EHT_LINK_RECONFIG_REQUEST;
	args.arg1 = mlo_link_recfg_dialog_token(req);

	peer = wlan_objmgr_get_peer_by_mac(recfg_ctx->psoc,
					   (uint8_t *)&req->peer_mac,
					   WLAN_MLO_MGR_ID);
	if (!peer) {
		mlo_err("Peer is null");
		return QDF_STATUS_E_NULL_VALUE;
	}

	vdev = wlan_peer_get_vdev(peer);
	if (!vdev) {
		mlo_err("Vdev is null");
		return QDF_STATUS_E_NULL_VALUE;
	}
	vdev_id = wlan_vdev_get_id(vdev);
	status = lim_send_link_recfg_action_req_frame(vdev_id,
						      (uint8_t *)&req->peer_mac,
						      &args, req);

	if (QDF_IS_STATUS_ERROR(status)) {
		mlo_err("Failed to send Link Reconfiguration action request frame");
		tx_result.status = status;
		mlo_link_recfg_sm_deliver_event(vdev->mlo_dev_ctx,
						WLAN_LINK_RECFG_SM_EV_XMIT_STATUS,
						sizeof(struct link_recfg_tx_result),
						&tx_result);
	}

	wlan_objmgr_peer_release_ref(peer, WLAN_MLO_MGR_ID);
	return status;
}

static QDF_STATUS
mlo_link_recfg_assign_self_link_addr(
			struct mlo_link_recfg_context *recfg_ctx,
			struct wlan_mlo_link_recfg_req *recfg_req,
			uint32_t del_link_set,
			uint32_t *first_del_link_set_no_common)
{
	struct wlan_mlo_dev_context *mlo_dev_ctx;
	struct mlo_link_info *link_info;
	uint8_t i;
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	struct wlan_objmgr_psoc *psoc;
	uint8_t idx;
	uint32_t allocated_bitmap;
	struct wlan_mlo_link_recfg_bss_info *link_add;
	struct wlan_objmgr_vdev *vdev;

	if (!recfg_req->add_link_info.num_links)
		return QDF_STATUS_SUCCESS;

	mlo_dev_ctx = mlo_link_recfg_get_mlo_ctx(recfg_ctx);
	if (!mlo_dev_ctx) {
		mlo_err("mlo_ctx null");
		return QDF_STATUS_E_INVAL;
	}

	psoc = mlo_link_recfg_get_psoc(recfg_ctx);
	if (!psoc) {
		mlo_err("psoc null");
		return QDF_STATUS_E_INVAL;
	}

	link_add = &recfg_req->add_link_info.link[0];
	allocated_bitmap = 0;
	idx = 0;

	/* 1. select the idle vdev's self mac as first choice
	 * for added link's self link addr.
	 * for example: L1 -> L1 L2
	 */
	for (i = 0; i < WLAN_MAX_ML_BSS_LINKS &&
	     idx < recfg_req->add_link_info.num_links; i++) {
		link_info = &mlo_dev_ctx->link_ctx->links_info[i];
		if (allocated_bitmap & (1 << i))
			continue;

		if (link_info->vdev_id == WLAN_INVALID_VDEV_ID)
			continue;

		vdev = wlan_objmgr_get_vdev_by_id_from_psoc(
					psoc, link_info->vdev_id,
					WLAN_LINK_RECFG_ID);
		if (!vdev) {
			mlo_err("Invalid VDEV id %d", link_info->vdev_id);
			continue;
		}

		if (!cm_is_vdev_disconnected(vdev)) {
			wlan_objmgr_vdev_release_ref(vdev, WLAN_LINK_RECFG_ID);
			continue;
		}
		/* todo: add validate vdev mac with link info link_add */
		wlan_objmgr_vdev_release_ref(vdev, WLAN_LINK_RECFG_ID);

		if (qdf_is_macaddr_zero(&link_info->ap_link_addr) ||
		    link_info->link_id == WLAN_INVALID_LINK_ID) {
			link_add[idx].self_link_addr = link_info->link_addr;
			link_add[idx].vdev_id = link_info->vdev_id;
			mlo_debug("assign idle self link addr: " QDF_MAC_ADDR_FMT " for add link %d freq %d vdev %d",
				  QDF_MAC_ADDR_REF(link_info->link_addr.bytes),
				  link_add[idx].link_id,
				  link_add[idx].freq,
				  link_info->vdev_id);
			mlo_debug("old link id %d flag 0x%x on vdev %d ",
				  link_info->link_id,
				  (uint32_t)link_info->link_status_flags,
				  link_info->vdev_id);
			idx++;
			allocated_bitmap |= 1 << i;
		}
	}

	/* 2. if FW indicate preferred vdev id for added link, validate it
	 * and use it, this happens in non-common link cases:
	 * for example:
	 * L1 L2 -> L3
	 * FW may indicate vdev id (from L1 or L2) for L3. here assign vdev
	 * to L3.
	 * The link will be deleted firstly. use the vdev on the deleted
	 * link to connect to new L3.
	 */
	for (i = 0; i < WLAN_MAX_ML_BSS_LINKS &&
	     idx < recfg_req->add_link_info.num_links; i++) {
		/* only checking the first one is enough */
		if (link_add[idx].vdev_id == WLAN_INVALID_VDEV_ID)
			break;
		link_info = &mlo_dev_ctx->link_ctx->links_info[i];
		if (allocated_bitmap & (1 << i))
			continue;
		if (link_info->vdev_id != link_add[idx].vdev_id)
			continue;

		vdev = wlan_objmgr_get_vdev_by_id_from_psoc(
					psoc, link_info->vdev_id,
					WLAN_MLO_MGR_ID);
		if (!vdev) {
			mlo_err("Invalid VDEV id %d", link_info->vdev_id);
			continue;
		}

		if (!cm_is_vdev_connected(vdev)) {
			wlan_objmgr_vdev_release_ref(vdev, WLAN_MLO_MGR_ID);
			continue;
		}

		wlan_objmgr_vdev_release_ref(vdev, WLAN_MLO_MGR_ID);

		if (qdf_atomic_test_bit(
				LS_F_AP_REMOVAL_BIT,
				&link_info->link_status_flags) ||
		    del_link_set & (1 << link_info->link_id)) {
			link_add[idx].self_link_addr = link_info->link_addr;
			mlo_debug("fw preferred vdev %d for add link %d",
				  link_add[idx].vdev_id,
				  link_add[idx].link_id);
			mlo_debug("assign active self link addr: " QDF_MAC_ADDR_FMT " for add link %d freq %d vdev %d",
				  QDF_MAC_ADDR_REF(link_info->link_addr.bytes),
				  link_add[idx].link_id,
				  link_add[idx].freq,
				  link_info->vdev_id);
			mlo_debug("old link id %d flag 0x%x on vdev %d ",
				  link_info->link_id,
				  (uint32_t)link_info->link_status_flags,
				  link_info->vdev_id);
			if (!*first_del_link_set_no_common) {
				*first_del_link_set_no_common |=
					1 << link_info->link_id;
				mlo_debug("select link %d to delete first if no common link",
					  link_info->link_id);
			}

			idx++;
			allocated_bitmap |= 1 << i;
			break;
		}
	}

	/* 3. select the link deleted active vdev's self mac as 3th choice
	 * for added link
	 * for example:
	 * L1 L2 -> L1 (L2 del, but vdev active) -> L1 L3.
	 * L1 L2 -> L1 (L2 del, but vdev active) -> L1 L2.
	 */
	for (i = 0; i < WLAN_MAX_ML_BSS_LINKS &&
	     idx < recfg_req->add_link_info.num_links; i++) {
		link_info = &mlo_dev_ctx->link_ctx->links_info[i];
		if (allocated_bitmap & (1 << i))
			continue;

		if (link_info->vdev_id == WLAN_INVALID_VDEV_ID)
			continue;

		vdev = wlan_objmgr_get_vdev_by_id_from_psoc(
					psoc, link_info->vdev_id,
					WLAN_LINK_RECFG_ID);
		if (!vdev) {
			mlo_err("Invalid VDEV id %d", link_info->vdev_id);
			continue;
		}

		if (!cm_is_vdev_connected(vdev)) {
			wlan_objmgr_vdev_release_ref(vdev, WLAN_LINK_RECFG_ID);
			continue;
		}
		/* todo: add validate vdev mac with link info link_add */
		wlan_objmgr_vdev_release_ref(vdev, WLAN_LINK_RECFG_ID);

		if (qdf_atomic_test_bit(
				LS_F_AP_REMOVAL_BIT,
				&link_info->link_status_flags) ||
		    del_link_set & (1 << link_info->link_id)) {
			link_add[idx].self_link_addr = link_info->link_addr;
			link_add[idx].vdev_id = link_info->vdev_id;
			mlo_debug("assign active self link addr: " QDF_MAC_ADDR_FMT " for add link %d freq %d vdev %d",
				  QDF_MAC_ADDR_REF(link_info->link_addr.bytes),
				  link_add[idx].link_id,
				  link_add[idx].freq,
				  link_info->vdev_id);
			mlo_debug("old link id %d flag 0x%x on vdev %d ",
				  link_info->link_id,
				  (uint32_t)link_info->link_status_flags,
				  link_info->vdev_id);
			/* no common link case:
			 * L1 L2 -> L3, to select the first deleted link.
			 * and use the vdev on the deleted link to connect
			 * to new L3.
			 */
			if (!qdf_atomic_test_bit(
					LS_F_AP_REMOVAL_BIT,
					&link_info->link_status_flags) &&
			    del_link_set & (1 << link_info->link_id)) {
				if (!*first_del_link_set_no_common) {
					*first_del_link_set_no_common |=
						1 << link_info->link_id;
					mlo_debug("select link %d to delete first if no common link",
						  link_info->link_id);
				}
			}

			idx++;
			allocated_bitmap |= 1 << i;
		}
	}

	/* 4. select the non-assoc idle or deleted standby link's self mac
	 * as 4th choice for added link
	 * for example:
	 * L1 L2 -> L1 L2 L3, use the idle non-assoc link's self mac
	 * L1 L2 L3 -> L1 L2 L4, use the deleted standby link's self mac
	 */
	for (i = 0; i < WLAN_MAX_ML_BSS_LINKS &&
	     idx < recfg_req->add_link_info.num_links; i++) {
		link_info = &mlo_dev_ctx->link_ctx->links_info[i];
		if (allocated_bitmap & (1 << i))
			continue;

		if (link_info->vdev_id != WLAN_INVALID_VDEV_ID)
			continue;

		if (qdf_atomic_test_bit(
				LS_F_AP_REMOVAL_BIT,
				&link_info->link_status_flags) ||
		    qdf_is_macaddr_zero(&link_info->ap_link_addr) ||
		    link_info->link_id == WLAN_INVALID_LINK_ID ||
		    del_link_set & (1 << link_info->link_id)) {
			link_add[idx].self_link_addr = link_info->link_addr;
			link_add[idx].vdev_id = link_info->vdev_id;
			mlo_debug("assign standby self link addr: " QDF_MAC_ADDR_FMT " for add link %d freq %d vdev %d",
				  QDF_MAC_ADDR_REF(link_info->link_addr.bytes),
				  link_add[idx].link_id,
				  link_add[idx].freq,
				  link_info->vdev_id);
			mlo_debug("old link id %d flag 0x%x on vdev %d ",
				  link_info->link_id,
				  (uint32_t)link_info->link_status_flags,
				  link_info->vdev_id);
			idx++;
			allocated_bitmap |= 1 << i;
		}
	}
	if (idx < recfg_req->add_link_info.num_links) {
		status = QDF_STATUS_E_INVAL;
		mlo_err("assign num %d, can't assign self mac for all added links, total %d",
			idx, recfg_req->add_link_info.num_links);
	}

	return status;
}

static QDF_STATUS
mlo_link_recfg_set_tx_link_addr(
			struct mlo_link_recfg_context *recfg_ctx,
			struct wlan_mlo_link_recfg_req *recfg_req,
			struct mlo_link_recfg_state_req *req,
			uint32_t candidate_link_set)
{
	struct wlan_mlo_dev_context *mlo_dev_ctx;
	struct mlo_link_info *link_info;
	uint8_t i;
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	struct wlan_objmgr_pdev *pdev;
	struct wlan_objmgr_psoc *psoc;

	/* decide which link will be used to send action frame */
	mlo_dev_ctx = mlo_link_recfg_get_mlo_ctx(recfg_ctx);
	if (!mlo_dev_ctx) {
		mlo_err("mlo_ctx null");
		return QDF_STATUS_E_INVAL;
	}

	psoc = mlo_link_recfg_get_psoc(recfg_ctx);
	if (!psoc) {
		mlo_err("psoc null");
		return QDF_STATUS_E_INVAL;
	}

	pdev = wlan_objmgr_get_pdev_by_id(psoc, 0, WLAN_LINK_RECFG_ID);
	if (!pdev) {
		mlo_err("Invalid pdev");
		return QDF_STATUS_E_INVAL;
	}

	for (i = 0; i < WLAN_MAX_ML_BSS_LINKS; i++) {
		link_info = &mlo_dev_ctx->link_ctx->links_info[i];
		if (qdf_is_macaddr_zero(&link_info->ap_link_addr))
			continue;

		if (link_info->link_id == WLAN_INVALID_LINK_ID)
			continue;

		if (qdf_atomic_test_bit(
				LS_F_AP_REMOVAL_BIT,
				&link_info->link_status_flags)) {
			mlo_debug("skip ap link addr: " QDF_MAC_ADDR_FMT " link flag 0x%x",
				  QDF_MAC_ADDR_REF(req->peer_mac.bytes),
				  (uint32_t)link_info->link_status_flags);
			continue;
		}

		if (link_info->vdev_id == WLAN_INVALID_VDEV_ID)
			continue;

		if (!cm_is_vdevid_connected(pdev, link_info->vdev_id))
			continue;

		if ((1 << link_info->link_id) & candidate_link_set) {
			req->peer_mac = link_info->ap_link_addr;
			mlo_debug("selected tx ap link addr: " QDF_MAC_ADDR_FMT "",
				  QDF_MAC_ADDR_REF(req->peer_mac.bytes));
			break;
		}
	}

	if (i == WLAN_MAX_ML_BSS_LINKS) {
		status = QDF_STATUS_E_INVAL;
		mlo_debug("no found valid peer mac");
	}

	if (pdev)
		wlan_objmgr_pdev_release_ref(pdev, WLAN_LINK_RECFG_ID);

	return status;
}

static QDF_STATUS
mlo_link_recfg_fill_del_link_no_common(
				struct mlo_link_recfg_context *recfg_ctx,
				struct wlan_mlo_link_recfg_req *recfg_req,
				uint32_t del_link_set_no_common,
				uint32_t curr_standby_set,
				struct wlan_mlo_link_recfg_info *del_link_info,
				uint32_t *final_del_link_set_no_common)
{
	uint8_t i, idx = 0;
	struct wlan_mlo_link_recfg_bss_info *link_del;
	QDF_STATUS status = QDF_STATUS_SUCCESS;

	*final_del_link_set_no_common = 0;
	for (i = 0; i < recfg_req->del_link_info.num_links; i++) {
		link_del = &recfg_req->del_link_info.link[i];

		if ((del_link_set_no_common | curr_standby_set) &
		    (1 << link_del->link_id)) {
			if (idx >= WLAN_MAX_ML_BSS_LINKS) {
				mlo_err("unexpected del link num");
				status = QDF_STATUS_E_INVAL;
				break;
			}
			del_link_info->link[idx++] = *link_del;
			*final_del_link_set_no_common |=
					1 << link_del->link_id;
			mlo_debug("del link %d in no-common",
				  link_del->link_id);
		}
	}
	del_link_info->num_links = idx;
	mlo_debug("del_link_set_no_common 0x%x standby 0x%x final_del_link_set_no_common 0x%x num %d",
		  del_link_set_no_common, curr_standby_set,
		  *final_del_link_set_no_common, idx);

	return status;
}

static QDF_STATUS mlo_link_pre_link_add_handler(
			struct mlo_link_recfg_context *recfg_ctx,
			struct mlo_link_recfg_state_req *req)
{
	QDF_STATUS status;

	status = mlo_link_recfg_update_added_link_in_mlo_mgr(
						recfg_ctx, req);
	if (QDF_IS_STATUS_ERROR(status)) {
		mlo_err("to update add link");
		return status;
	}

	return status;
}

static QDF_STATUS mlo_link_invoke_pre_link_add_handler(
			struct mlo_link_recfg_context *recfg_ctx,
			struct mlo_link_recfg_state_req *req)
{
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	struct mlo_link_recfg_state_tran *tran;

	tran = mlo_link_recfg_get_curr_tran_req(recfg_ctx);
	if (!tran) {
		mlo_err("curr tran ctx null");
		return QDF_STATUS_E_INVAL;
	}
	if (tran->pre_link_add_handler)
		status = tran->pre_link_add_handler(recfg_ctx, req);

	return status;
}

static QDF_STATUS
mlo_link_recfg_tranistion_to_next_state(
			struct mlo_link_recfg_context *recfg_ctx)
{
	QDF_STATUS status;
	struct mlo_link_recfg_state_tran *tran, *prev;

	if (recfg_ctx->sm.curr_state_idx != -1 &&
	    recfg_ctx->sm.curr_state_idx >=
		QDF_ARRAY_SIZE(recfg_ctx->sm.state_list)) {
		mlo_err("unexpected curr_state_idx %d",
			recfg_ctx->sm.curr_state_idx);
		return QDF_STATUS_E_FAILURE;
	}

	if (recfg_ctx->sm.curr_state_idx >= 0) {
		prev = &recfg_ctx->sm.state_list[recfg_ctx->sm.curr_state_idx];
		mlo_debug("prev idx %d st %d evt %d",
			  recfg_ctx->sm.curr_state_idx,
			  prev->state, prev->event);
	}
	recfg_ctx->sm.curr_state_idx++;
	tran = &recfg_ctx->sm.state_list[recfg_ctx->sm.curr_state_idx];
	mlo_debug("next idx %d st %d evt %d",
		  recfg_ctx->sm.curr_state_idx,
		  tran->state, tran->event);

	/* transition to next state */
	mlo_link_recfg_sm_transition_to(recfg_ctx, tran->state);
	status = mlo_link_recfg_sm_deliver_event_sync(
		recfg_ctx->ml_dev,
		tran->event, sizeof(tran->req), &tran->req);
	if (QDF_IS_STATUS_ERROR(status))
		mlo_err("state %d event %d status %d",
			tran->state, tran->event, status);

	return status;
}

QDF_STATUS
mlo_link_recfg_create_transition_list(
			struct mlo_link_recfg_context *recfg_ctx,
			struct wlan_mlo_link_recfg_req *recfg_req)
{
	uint32_t curr_link_set = 0, add_link_set = 0, del_link_set = 0;
	uint8_t curr_link_num = 0, add_link_num = 0, del_link_num = 0;
	uint8_t curr_standby_num = 0;
	uint32_t curr_standby_set = 0;
	uint32_t del_link_set_no_common = 0;
	struct mlo_link_recfg_state_tran *next = &recfg_ctx->sm.state_list[0];
	QDF_STATUS status;

	mlo_link_recfg_get_link_bitmap(recfg_ctx,
				       recfg_req,
				       &add_link_set,
				       &add_link_num,
				       &del_link_set,
				       &del_link_num,
				       &curr_link_set,
				       &curr_link_num,
				       &curr_standby_set,
				       &curr_standby_num);

	/* alloc self mac for link add */
	status = mlo_link_recfg_assign_self_link_addr(
					recfg_ctx, recfg_req,
					del_link_set,
					&del_link_set_no_common);
	if (QDF_IS_STATUS_ERROR(status)) {
		mlo_err("fail to ssign self link for added links status %d",
			status);
		return status;
	}

	/* create transition flow */
	recfg_ctx->sm.curr_state_idx = -1;
	qdf_mem_zero(&recfg_ctx->sm.state_list[0],
		     sizeof(recfg_ctx->sm.state_list[0]) *
		     MAX_RECFG_TRANSITION);
	recfg_req->recfg_type = link_recfg_undefined;
	recfg_req->join_pending_vdev_id = WLAN_INVALID_VDEV_ID;
	if (recfg_req->add_link_info.num_links &&
	    !recfg_req->del_link_info.num_links) {
		/* Add link only */
		mlo_debug("add link only");
		recfg_req->recfg_type = link_recfg_add_only;
		next->state = WLAN_LINK_RECFG_S_XMIT_REQ;
		next->event = WLAN_LINK_RECFG_SM_EV_XMIT_REQ;
		next->req.add_link_info = recfg_req->add_link_info;
		/* select ap link to be used to send action frame */
		mlo_link_recfg_set_tx_link_addr(recfg_ctx,
						recfg_req,
						&next->req,
						curr_link_set);
		next->abort_handler = NULL;
		next++;
		next->state = WLAN_LINK_RECFG_S_ADD_LINK;
		next->event = WLAN_LINK_RECFG_SM_EV_ADD_LINK;
		next->req.add_link_info = recfg_req->add_link_info;
		next->abort_handler = NULL;
		next++;
		next->state = WLAN_LINK_RECFG_S_COMPLETED;
		next->event = WLAN_LINK_RECFG_SM_EV_COMPLETED;
		next->req.add_link_info = recfg_req->add_link_info;
		next->abort_handler = NULL;
	} else if (!recfg_req->add_link_info.num_links &&
		   recfg_req->del_link_info.num_links) {
		/* Del link only */
		mlo_debug("del link only");
		recfg_req->recfg_type = link_recfg_del_only;
		next->state = WLAN_LINK_RECFG_S_DEL_LINK;
		next->event = WLAN_LINK_RECFG_SM_EV_DEL_LINK;
		next->req.del_link_info = recfg_req->del_link_info;
		next->abort_handler = NULL;
		next++;
		next->state = WLAN_LINK_RECFG_S_XMIT_REQ;
		next->event = WLAN_LINK_RECFG_SM_EV_XMIT_REQ;
		next->req.del_link_info = recfg_req->del_link_info;
		mlo_link_recfg_set_tx_link_addr(recfg_ctx,
						recfg_req,
						&next->req,
						curr_link_set &
						~del_link_set);
		next->abort_handler = NULL;
		next++;
		next->state = WLAN_LINK_RECFG_S_COMPLETED;
		next->event = WLAN_LINK_RECFG_SM_EV_COMPLETED;
		next->req.del_link_info = recfg_req->del_link_info;
		next->abort_handler = NULL;
	} else if ((curr_link_set & ~del_link_set) && add_link_set) {
		/* Add and Del link with common link */
		mlo_debug("del and add link");
		recfg_req->recfg_type =	link_recfg_del_add_common_link;
		next->state = WLAN_LINK_RECFG_S_DEL_LINK;
		next->event = WLAN_LINK_RECFG_SM_EV_DEL_LINK;
		next->req.del_link_info = recfg_req->del_link_info;
		next->abort_handler = NULL;
		next++;
		next->state = WLAN_LINK_RECFG_S_XMIT_REQ;
		next->event = WLAN_LINK_RECFG_SM_EV_XMIT_REQ;
		next->req.del_link_info = recfg_req->del_link_info;
		next->req.add_link_info = recfg_req->add_link_info;
		mlo_link_recfg_set_tx_link_addr(recfg_ctx,
						recfg_req,
						&next->req,
						curr_link_set &
						~del_link_set);
		next->abort_handler = NULL;
		next++;
		next->state = WLAN_LINK_RECFG_S_ADD_LINK;
		next->event = WLAN_LINK_RECFG_SM_EV_ADD_LINK;
		next->req.add_link_info = recfg_req->add_link_info;
		next++;
		next->state = WLAN_LINK_RECFG_S_COMPLETED;
		next->event = WLAN_LINK_RECFG_SM_EV_COMPLETED;
		next->req.del_link_info = recfg_req->del_link_info;
		next->req.add_link_info = recfg_req->add_link_info;
		next->abort_handler = NULL;
	} else if ((curr_link_set == del_link_set) && add_link_set) {
		/* Add and Del link with no common link */
		mlo_debug("del and add link - no common link");
		recfg_req->recfg_type = link_recfg_del_add_no_common_link;
		if (del_link_num > 1) {
			/* selete the first link to delete from
			 * del_link_set_no_common,
			 * L1 L2 - > L3, select one of del_link_info to
			 * del first.
			 */
			mlo_debug("del_link_set_no_common 0x%x",
				  del_link_set_no_common);
			next->state = WLAN_LINK_RECFG_S_DEL_LINK;
			next->event = WLAN_LINK_RECFG_SM_EV_DEL_LINK;
			next->abort_handler = NULL;
			status =
			mlo_link_recfg_fill_del_link_no_common(
						recfg_ctx,
						recfg_req,
						del_link_set_no_common,
						curr_standby_set,
						&next->req.del_link_info,
						&del_link_set_no_common);
			if (QDF_IS_STATUS_ERROR(status)) {
				mlo_err("fail to fill del link info status %d",
					status);
				return status;
			}
			next++;
		} else {
			/* L1 - > L2 */
		}
		next->state = WLAN_LINK_RECFG_S_ADD_LINK;
		next->event = WLAN_LINK_RECFG_SM_EV_ADD_LINK;
		next->abort_handler = NULL;
		next->pre_link_add_handler = mlo_link_pre_link_add_handler;
		next->req.add_link_info = recfg_req->add_link_info;
		/* to fill del link as well for action frame tx
		 * fill the tx link address for frame tx.
		 */
		next->req.del_link_info = recfg_req->del_link_info;
		status =
		mlo_link_recfg_set_tx_link_addr(recfg_ctx,
						recfg_req,
						&next->req,
						curr_link_set &
						~del_link_set_no_common);
		if (QDF_IS_STATUS_ERROR(status)) {
			mlo_err("fail to set tx frame link addr status %d",
				status);
			return status;
		}
		next++;

		next->state = WLAN_LINK_RECFG_S_DEL_LINK;
		next->event = WLAN_LINK_RECFG_SM_EV_DEL_LINK;
		next->abort_handler = NULL;
		/* To delete the left of links */
		del_link_set_no_common =
			del_link_set & ~del_link_set_no_common;
		status =
		mlo_link_recfg_fill_del_link_no_common(
					recfg_ctx,
					recfg_req,
					del_link_set_no_common,
					curr_standby_set,
					&next->req.del_link_info,
					&del_link_set_no_common);
		if (QDF_IS_STATUS_ERROR(status)) {
			mlo_err("fail to fill del link info status %d",
				status);
			return status;
		}
		next++;

		next->state = WLAN_LINK_RECFG_S_COMPLETED;
		next->event = WLAN_LINK_RECFG_SM_EV_COMPLETED;
		next->abort_handler = NULL;
		next->req.del_link_info = recfg_req->del_link_info;
		next->req.add_link_info = recfg_req->add_link_info;
	} else {
		/* not supported */
		mlo_err("unexpected request del set 0x%x add set 0x%x curr set 0x%x",
			del_link_set, add_link_set, curr_link_set);
		return QDF_STATUS_E_INVAL;
	}

	status = mlo_link_recfg_tranistion_to_next_state(recfg_ctx);
	if (QDF_IS_STATUS_ERROR(status))
		mlo_err("start trans failed status %d", status);

	return status;
}

static struct mlo_link_recfg_state_tran *
mlo_link_recfg_get_curr_tran_req(struct mlo_link_recfg_context *recfg_ctx)
{
	if (recfg_ctx->sm.curr_state_idx >= 0 &&
	    recfg_ctx->sm.curr_state_idx <
		QDF_ARRAY_SIZE(recfg_ctx->sm.state_list))
		return &recfg_ctx->sm.state_list[recfg_ctx->sm.curr_state_idx];

	mlo_err("unexpected curr_state_idx %d",
		recfg_ctx->sm.curr_state_idx);

	return NULL;
}

static void
mlo_link_recfg_add_link_completed(struct mlo_link_recfg_context *recfg_ctx)
{
	/* handle link add completed */

	/* if there is deleted standby link , remove link info from mlo mgr
	 * L1 L2 to L1 to L1 L3
	 */

	/* transition to next state */
	mlo_link_recfg_tranistion_to_next_state(recfg_ctx);
}

static void
mlo_link_recfg_del_link_completed(struct mlo_link_recfg_context *recfg_ctx)
{
	/* handle link del completed */

	/* transition to next state */
	mlo_link_recfg_tranistion_to_next_state(recfg_ctx);
}

static QDF_STATUS
mlo_link_recfg_response_handler(struct mlo_link_recfg_context *recfg_ctx,
				struct link_recfg_rx_rsp *recfg_resp_data,
				uint16_t event_data_len)
{
	struct mlo_link_recfg_state_tran *tran;
	QDF_STATUS status;


	if (!recfg_ctx || !recfg_resp_data || !event_data_len)
		return QDF_STATUS_E_INVAL;

	tran = mlo_link_recfg_get_curr_tran_req(recfg_ctx);
	if (!tran) {
		mlo_err("curr tran ctx null");
		return QDF_STATUS_E_INVAL;
	}

	if (QDF_IS_STATUS_ERROR(recfg_resp_data->status)) {
		mlo_err("RX response failure");
		return QDF_STATUS_E_INVAL;
	}

	/* notify kernel/supplicant for del only req/rsp
	 *	osif_notify_link_reconfig();
	 */
	mlo_err("RX response success");

	/* Handle link recfg link del.
	 * If deleted link is standby, remove link info from mlo mgr.
	 * For example:
	 * L1 L2 L3, del L2, link switch to L3. remove standby L2.
	 * or L1 L2 L3, del standby L3, then remove standby L3.
	 */
	mlo_link_recfg_remove_deleted_standby_in_mlo_mgr(recfg_ctx, &tran->req);

	status = mlo_link_recfg_update_added_link_in_mlo_mgr(
							recfg_ctx, &tran->req);
	if (QDF_IS_STATUS_ERROR(status)) {
		mlo_err("RX response failure");
		return status;
	}
	mlo_link_recfg_update_partner_info(recfg_ctx);
	mlo_link_recfg_store_key(recfg_ctx, &tran->req);
	/* handle link recfg link add rejected case */

	return status;
}

static QDF_STATUS
mlo_link_recfg_response_received(struct mlo_link_recfg_context *recfg_ctx,
				 struct link_recfg_rx_rsp *recfg_resp_data,
				 uint16_t event_data_len)
{
	QDF_STATUS status;

	status = mlo_link_recfg_response_handler(recfg_ctx,
						 recfg_resp_data,
						 event_data_len);
	if (QDF_IS_STATUS_ERROR(status)) {
		mlo_err("RX response handler failure status %d", status);
		return status;
	}

	status = mlo_link_recfg_tranistion_to_next_state(recfg_ctx);

	return status;
}

static void
mlo_link_recfg_del_link_aborted(struct mlo_link_recfg_context *recfg_ctx)
{
	/* handle link del aborted */

	/* move to abort state to complete link reconfig */
	mlo_link_recfg_sm_transition_to(recfg_ctx, WLAN_LINK_RECFG_S_ABORT);
	mlo_link_recfg_sm_deliver_event_sync(
			recfg_ctx->ml_dev, WLAN_LINK_RECFG_SM_EV_COMPLETED,
			0, NULL);
}

static void
mlo_link_recfg_add_link_aborted(struct mlo_link_recfg_context *recfg_ctx)
{
	/* handle link add aborted */

	/* move to abort state to complete link reconfig */
	mlo_link_recfg_sm_transition_to(recfg_ctx, WLAN_LINK_RECFG_S_ABORT);
	mlo_link_recfg_sm_deliver_event_sync(
			recfg_ctx->ml_dev, WLAN_LINK_RECFG_SM_EV_COMPLETED,
			0, NULL);
}

static void
mlo_link_recfg_complete(struct mlo_link_recfg_context *recfg_ctx,
			bool success)
{
	struct wlan_mlo_link_recfg_req *recfg_req;
	struct wlan_objmgr_psoc *psoc;
	struct wlan_lmac_if_mlo_tx_ops *mlo_tx_ops;
	struct wlan_mlo_link_recfg_complete_params complete_params = {0};
	struct wlan_objmgr_vdev *vdev;
	QDF_STATUS status;

	recfg_req = &recfg_ctx->curr_recfg_req;

	psoc = mlo_link_recfg_get_psoc(recfg_ctx);
	if (!psoc) {
		mlo_err("psoc is null");
		return;
	}

	mlo_tx_ops = target_if_mlo_get_tx_ops(psoc);
	if (!mlo_tx_ops) {
		mlo_err("tx_ops is null!");
		return;
	}

	if (!mlo_tx_ops->send_mlo_link_recfg_complete_cmd) {
		mlo_err("send_mlo_link_recfg_complete_cmd is null!");
		return;
	}

	vdev = wlan_objmgr_get_vdev_by_id_from_psoc(
				psoc, recfg_ctx->curr_recfg_req.vdev_id,
				WLAN_MLO_MGR_ID);
	if (!vdev) {
		mlo_err("link vdev is null");
		return;
	}

	if (recfg_req->is_fw_ind_received) {
		/* send wmi link config complete command to firmware
		 * only if the fw has indicated event to host.
		 */
		complete_params.ap_mld_addr =
				recfg_req->fw_ind_param.ap_mld_addr;
		complete_params.reassoc_if_failure = false;
		complete_params.status = success ? 0 : 1;
		complete_params.vdev_id = recfg_req->fw_ind_param.vdev_id;
		status = mlo_tx_ops->send_mlo_link_recfg_complete_cmd(
						psoc, &complete_params);
		if (QDF_IS_STATUS_ERROR(status))
			mlo_err("send_mlo_link_recfg_complete_cmd failed %d",
				status);
	}
	/* Send link reconfig status to userspace */
	mlme_cm_send_link_reconfig_status(vdev);
	wlan_objmgr_vdev_release_ref(vdev, WLAN_MLO_MGR_ID);
	/* reset state tran index and move to init state  */
	recfg_ctx->sm.curr_state_idx = -1;
	recfg_req->recfg_type = link_recfg_undefined;
	recfg_req->join_pending_vdev_id = WLAN_INVALID_VDEV_ID;

	mlo_link_recfg_sm_transition_to(recfg_ctx, WLAN_LINK_RECFG_S_INIT);

	/* remove reconfig ser command */
	mlo_remove_link_recfg_cmd(recfg_ctx);

	/* free link recfg ctx ies */
	mlo_link_recfg_ctx_free_ies(recfg_ctx);
}

static enum wlan_link_recfg_sm_state
mlo_link_recfg_sm_get_state(struct mlo_link_recfg_context *recfg_ctx)
{
	return recfg_ctx->sm.link_recfg_state;
}

static enum wlan_link_recfg_sm_state
mlo_link_recfg_sm_get_substate(struct mlo_link_recfg_context *recfg_ctx)
{
	return recfg_ctx->sm.link_recfg_substate;
}

static void
mlo_link_recfg_sm_set_state(struct mlo_link_recfg_context *recfg_ctx,
			    enum wlan_link_recfg_sm_state state)
{
	if (state < WLAN_LINK_RECFG_S_MAX)
		recfg_ctx->sm.link_recfg_state = state;
	else
		mlo_err("invalid state %d", state);
}

static void
mlo_link_recfg_sm_set_substate(struct mlo_link_recfg_context *recfg_ctx,
			       enum wlan_link_recfg_sm_state substate)
{
	if (substate > WLAN_LINK_RECFG_S_MAX &&
	    substate < WLAN_LINK_RECFG_SS_MAX)
		recfg_ctx->sm.link_recfg_substate = substate;
	else
		mlo_err("invalid state %d", substate);
}

static void
mlo_link_recfg_sm_state_update(struct mlo_link_recfg_context *recfg_ctx,
			       enum wlan_link_recfg_sm_state state,
			       enum wlan_link_recfg_sm_state substate)
{
	mlo_link_recfg_sm_set_state(recfg_ctx, state);
	mlo_link_recfg_sm_set_substate(recfg_ctx, substate);
}

static void
mlo_link_recfg_ser_timeout_sm_handler(
	struct mlo_link_recfg_context *recfg_ctx)
{
	enum wlan_link_recfg_sm_state state;
	enum wlan_link_recfg_sm_state substate;
	struct wlan_mlo_dev_context *mlo_dev_ctx;
	struct wlan_objmgr_psoc *psoc;

	psoc = mlo_link_recfg_get_psoc(recfg_ctx);
	if (!psoc) {
		mlo_err("invalid psoc");
		return;
	}

	mlo_dev_ctx = mlo_link_recfg_get_mlo_ctx(recfg_ctx);
	if (!mlo_dev_ctx) {
		mlo_err("invalid mlo dev ctx");
		return;
	}

	state = mlo_link_recfg_sm_get_state(recfg_ctx);
	substate = mlo_link_recfg_sm_get_substate(recfg_ctx);
	mlo_debug("curr st %d subst %d", state, substate);

	switch (state) {
	case WLAN_LINK_RECFG_S_START:
	case WLAN_LINK_RECFG_S_DEL_LINK:
	case WLAN_LINK_RECFG_S_ADD_LINK:
		break;
	case WLAN_LINK_RECFG_S_XMIT_REQ:
		goto abort;
	default:
		mlo_err("unexpected state %d when ser timeout vdev %d",
			state,
			recfg_ctx->curr_recfg_req.vdev_id);
		goto abort;
	}

	switch (substate) {
	case WLAN_LINK_RECFG_SS_START_PENDING:
	case WLAN_LINK_RECFG_SS_START_ACTIVE:
		break;
	case WLAN_LINK_RECFG_SS_DEL_LINK_WAIT_SET_LINK:
	case WLAN_LINK_RECFG_SS_DEL_LINK_ABORT_WAIT_SET_LINK:
		/* timeout set link req */
		mlo_link_recfg_set_link_resp_timeout(mlo_dev_ctx);
		break;
	case WLAN_LINK_RECFG_SS_DEL_LINK_WAIT_LINK_SW:
	case WLAN_LINK_RECFG_SS_DEL_LINK_ABORT_WAIT_LINK_SW:
		/* timeout link switch req */
		if (mlo_link_recfg_is_link_switch_in_progress(recfg_ctx)) {
			mlo_err("Link recfg Link switch timeout");
			mlo_link_recfg_link_switch_timeout(psoc, mlo_dev_ctx);
		} else {
			mlo_err("Link recfg Link switch not active");
		}
		break;
	case WLAN_LINK_RECFG_SS_ADD_LINK_WAIT_ADD_CONN:
		mlo_link_recfg_abort_link_add_no_comm(recfg_ctx);
		break;
	case WLAN_LINK_RECFG_SS_ADD_LINK_ABORT_WAIT_ADD_CONN:
		/* add partner link timeout */
		break;
	case WLAN_LINK_RECFG_SS_ADD_LINK_WAIT_LINK_SW:
		mlo_link_recfg_abort_link_add_no_comm(recfg_ctx);
		break;
	case WLAN_LINK_RECFG_SS_ADD_LINK_ABORT_WAIT_LINK_SW:
		/* timeout link switch req */
		break;
	default:
		mlo_err("unexpected substate %d when ser timeout vdev %d",
			state,
			recfg_ctx->curr_recfg_req.vdev_id);
		break;
	}

abort:
	mlo_link_recfg_sm_transition_to(recfg_ctx, WLAN_LINK_RECFG_S_ABORT);
	mlo_link_recfg_sm_deliver_event_sync(
			recfg_ctx->ml_dev, WLAN_LINK_RECFG_SM_EV_COMPLETED,
			0, NULL);
}

static QDF_STATUS
mlo_link_recfg_no_common_link_event(void *ctx,
				    uint16_t event,
				    uint16_t event_data_len,
				    void *event_data)
{
	struct mlo_link_recfg_context *recfg_ctx = ctx;
	struct mlo_link_recfg_state_req *req;
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	struct link_recfg_tx_result *tx_status;

	if (recfg_ctx->curr_recfg_req.recfg_type !=
				link_recfg_del_add_no_common_link) {
		mlo_debug("unexpected event %d in link recfg type %d",
			  event, recfg_ctx->curr_recfg_req.recfg_type);
		return QDF_STATUS_SUCCESS;
	}

	switch (event) {
	case WLAN_LINK_RECFG_SM_EV_XMIT_REQ:
		req = (struct mlo_link_recfg_state_req *)event_data;
		status =
		mlo_link_recfg_send_request_frame(recfg_ctx, req);
		break;
	case WLAN_LINK_RECFG_SM_EV_XMIT_STATUS:
		/* Handle tx failure */
		tx_status = (struct link_recfg_tx_result *)event_data;
		status = tx_status->status;
		break;
	case WLAN_LINK_RECFG_SM_EV_RX_RSP:
		status = mlo_link_recfg_response_handler(
			recfg_ctx, (struct link_recfg_rx_rsp *)event_data,
			event_data_len);
		if (QDF_IS_STATUS_ERROR(status))
			mlo_err("error to handle resp status %d", status);
		break;
	default:
		break;
	}

	return status;
}

/* WLAN_LINK_RECFG_S_INIT */
static void
mlo_link_recfg_state_init_entry(void *ctx)
{
	mlo_link_recfg_sm_state_update(ctx, WLAN_LINK_RECFG_S_INIT,
				       WLAN_LINK_RECFG_SS_IDLE);
}

static bool
mlo_link_recfg_state_init_event(void *ctx,
				uint16_t event,
				uint16_t event_data_len,
				void *event_data)
{
	struct mlo_link_recfg_context *recfg_ctx = ctx;
	bool event_handled = true;

	switch (event) {
	case WLAN_LINK_RECFG_SM_EV_FW_IND:
	case WLAN_LINK_RECFG_SM_EV_USER_REQ:
		/* validate request */

		/* transition to start */
		mlo_link_recfg_sm_transition_to(ctx,
						WLAN_LINK_RECFG_S_START);
		mlo_link_recfg_sm_deliver_event_sync(
					recfg_ctx->ml_dev, event,
					event_data_len, event_data);
		break;
	default:
		event_handled = false;
		break;
	}

	return event_handled;
}

static void
mlo_link_recfg_state_init_exit(void *ctx)
{
}

/* WLAN_LINK_RECFG_S_START */
static void
mlo_link_recfg_state_start_entry(void *ctx)
{
	mlo_link_recfg_sm_state_update(ctx, WLAN_LINK_RECFG_S_START,
				       WLAN_LINK_RECFG_SS_IDLE);
}

static bool
mlo_link_recfg_state_start_event(void *ctx,
				 uint16_t event,
				 uint16_t event_data_len,
				 void *event_data)
{
	struct mlo_link_recfg_context *recfg_ctx = ctx;
	bool event_handled = true;
	QDF_STATUS status;

	switch (event) {
	case WLAN_LINK_RECFG_SM_EV_FW_IND:
	case WLAN_LINK_RECFG_SM_EV_USER_REQ:
		mlo_link_recfg_sm_transition_to(
			ctx,
			WLAN_LINK_RECFG_SS_START_PENDING);

		status = mlo_link_recfg_sm_deliver_event_sync(
				recfg_ctx->ml_dev, WLAN_LINK_RECFG_SM_EV_START,
				event_data_len, event_data);
		break;
	default:
		event_handled = false;
		break;
	}

	return event_handled;
}

static void
mlo_link_recfg_state_start_exit(void *ctx)
{
}

/* WLAN_LINK_RECFG_SS_START_PENDING */
static void
mlo_link_recfg_subst_start_pending_entry(void *ctx)
{
	if (mlo_link_recfg_sm_get_state(ctx) != WLAN_LINK_RECFG_S_START)
		QDF_BUG(0);

	mlo_link_recfg_sm_set_substate(
			ctx, WLAN_LINK_RECFG_SS_START_PENDING);
}

static bool
mlo_link_recfg_subst_start_pending_event(void *ctx,
					 uint16_t event,
					 uint16_t event_data_len,
					 void *event_data)
{
	struct mlo_link_recfg_context *recfg_ctx = ctx;
	bool event_handled = true;
	struct wlan_mlo_link_recfg_req *recfg_req;
	QDF_STATUS status;

	switch (event) {
	case WLAN_LINK_RECFG_SM_EV_START:
		recfg_req = (struct wlan_mlo_link_recfg_req *)event_data;
		status = mlo_ser_link_recfg_cmd(recfg_ctx, recfg_req);
		if (QDF_IS_STATUS_ERROR(status)) {
			event_handled = false;
			/* todo: handle error if link recfg ser is failed */
			break;
		}
		break;
	case WLAN_LINK_RECFG_SM_EV_ACTIVE:
		recfg_req = &recfg_ctx->curr_recfg_req;
		if (recfg_req->is_user_req) {
			/* for user initiated request, we need to send
			 * wmi command to target to trigger recfg and
			 * wait for target event
			 */
			mlo_link_recfg_sm_transition_to(
				recfg_ctx, WLAN_LINK_RECFG_SS_START_ACTIVE);
			mlo_link_recfg_sm_deliver_event_sync(
					recfg_ctx->ml_dev,
					WLAN_LINK_RECFG_SM_EV_ACTIVE,
					0, NULL);
		} else {
			/* for target initiated request, we can start
			 * recfg here.
			 */
			status = mlo_link_recfg_create_transition_list(
				recfg_ctx,
				&recfg_ctx->curr_recfg_req);
		}
		break;
	case WLAN_LINK_RECFG_SM_EV_DISCONNECT_IND:
	case WLAN_LINK_RECFG_SM_EV_ROAM_START_IND:
		/* todo: handle disc or roam if link recfg ser not active */
		mlo_link_recfg_ser_timeout_sm_handler(recfg_ctx);
		break;
	default:
		event_handled = false;
		break;
	}

	return event_handled;
}

static void
mlo_link_recfg_subst_start_pending_exit(void *ctx)
{
}

/* WLAN_LINK_RECFG_SS_START_ACTIVE: to handle usr link recfg request */
static void
mlo_link_recfg_subst_start_active_entry(void *ctx)
{
	if (mlo_link_recfg_sm_get_state(ctx) != WLAN_LINK_RECFG_S_START)
		QDF_BUG(0);

	mlo_link_recfg_sm_set_substate(
			ctx, WLAN_LINK_RECFG_SS_START_ACTIVE);
}

static bool
mlo_link_recfg_subst_start_active_event(void *ctx,
					uint16_t event,
					uint16_t event_data_len,
					void *event_data)
{
	struct mlo_link_recfg_context *recfg_ctx = ctx;
	bool event_handled = true;
	struct wlan_mlo_link_recfg_req *recfg_req;
	struct wlan_mlo_link_recfg_req *fw_ind_recfg_req;
	QDF_STATUS status;

	switch (event) {
	case WLAN_LINK_RECFG_SM_EV_ACTIVE:
		recfg_req = &recfg_ctx->curr_recfg_req;
		if (recfg_req->is_user_req) {
			/* send link reconfig wmi WMI_MLO_LINK_RECONFIG_CMDID
			 * and wait for WLAN_LINK_RECFG_SM_EV_FW_IND
			 */
		} else {
			/* unexpected for ap initiated */
		}
		break;
	case WLAN_LINK_RECFG_SM_EV_FW_IND:
		fw_ind_recfg_req =
			(struct wlan_mlo_link_recfg_req *)event_data;
		/* validate the target link recfg reason is "host force
		 * reason" code. and check if indication param is same
		 * as user requested in recfg_ctx->curr_recfg_req,
		 * then start link recfg
		 */
		recfg_req = &recfg_ctx->curr_recfg_req;
		recfg_req->is_fw_ind_received = true;
		recfg_req->add_link_info = fw_ind_recfg_req->add_link_info;
		recfg_req->del_link_info = fw_ind_recfg_req->del_link_info;
		recfg_req->fw_ind_param = fw_ind_recfg_req->fw_ind_param;
		status = mlo_link_recfg_create_transition_list(
					recfg_ctx,
					&recfg_ctx->curr_recfg_req);
		break;
	case WLAN_LINK_RECFG_SM_EV_DISCONNECT_IND:
	case WLAN_LINK_RECFG_SM_EV_ROAM_START_IND:
		/* handle disc or roam if link recfg ser is active */
		mlo_link_recfg_ser_timeout_sm_handler(recfg_ctx);
		break;
	case WLAN_LINK_RECFG_SM_EV_SER_TIMEOUT:
		/* handle serialization timeout if no fw link reconfig event */
		mlo_link_recfg_ser_timeout_sm_handler(recfg_ctx);
		break;
	default:
		event_handled = false;
		break;
	}

	return event_handled;
}

static void
mlo_link_recfg_subst_start_active_exit(void *ctx)
{
}

/* WLAN_LINK_RECFG_S_DEL_LINK */
static void
mlo_link_recfg_state_del_link_entry(void *ctx)
{
	mlo_link_recfg_sm_state_update(ctx, WLAN_LINK_RECFG_S_DEL_LINK,
				       WLAN_LINK_RECFG_SS_IDLE);
}

static bool
mlo_link_recfg_state_del_link_event(void *ctx,
				    uint16_t event,
				    uint16_t event_data_len,
				    void *event_data)
{
	struct mlo_link_recfg_context *recfg_ctx = ctx;
	bool event_handled = true;
	struct mlo_link_recfg_state_req *req;

	switch (event) {
	case WLAN_LINK_RECFG_SM_EV_DEL_LINK:
		req = (struct mlo_link_recfg_state_req *)event_data;
		if (mlo_link_recfg_is_standby_link_del_only(recfg_ctx, req)) {
			/* If the link delete is only for standby link
			 * then send bss param update to target to delete
			 * link, and complete the link delete state.
			 * for example, ABC -> AB: delete sandby link C
			 */
			mlo_link_recfg_del_standby_link(recfg_ctx, req);
			mlo_link_recfg_del_link_completed(recfg_ctx);
		} else {
			/* If any non-standby link is included in delete
			 * request, then have to use set link inactive
			 * command to delete links.
			 */
			mlo_link_recfg_sm_transition_to(
				ctx,
				WLAN_LINK_RECFG_SS_DEL_LINK_WAIT_SET_LINK);
			mlo_link_recfg_sm_deliver_event_sync(
				recfg_ctx->ml_dev, event,
				event_data_len, event_data);
		}
		break;
	default:
		event_handled = false;
		break;
	}

	return event_handled;
}

static void
mlo_link_recfg_state_del_link_exit(void *ctx)
{
}

/* WLAN_LINK_RECFG_SS_DEL_LINK_WAIT_SET_LINK */
static void
mlo_link_recfg_subst_del_link_wait_set_link_entry(void *ctx)
{
	if (mlo_link_recfg_sm_get_state(ctx) != WLAN_LINK_RECFG_S_DEL_LINK)
		QDF_BUG(0);

	mlo_link_recfg_sm_set_substate(
			ctx, WLAN_LINK_RECFG_SS_DEL_LINK_WAIT_SET_LINK);
}

static bool
mlo_link_recfg_subst_del_link_wait_set_link_event(void *ctx,
						  uint16_t event,
						  uint16_t event_data_len,
						  void *event_data)
{
	struct mlo_link_recfg_context *recfg_ctx = ctx;
	bool event_handled = true;
	struct mlo_link_recfg_state_req *req;
	struct set_link_resp *set_link_resp;

	switch (event) {
	case WLAN_LINK_RECFG_SM_EV_DEL_LINK:
		req = (struct mlo_link_recfg_state_req *)event_data;
		mlo_link_recfg_del_link_by_inact(recfg_ctx, req);
		break;
	case WLAN_LINK_RECFG_SM_EV_SET_LINK_RSP:
		set_link_resp = (struct set_link_resp *)event_data;
		if (set_link_resp->status) {
			mlo_debug("set link inactive failed, abort del link");
			mlo_link_recfg_del_link_aborted(recfg_ctx);
			break;
		}

		if (mlo_link_recfg_is_standby_link_present_for_link_switch(
							recfg_ctx)) {
			/* ABC -> AC: B is set inactive for delete,
			 * link switch is expected. fw should link
			 * switch to C.
			 */
			mlo_link_recfg_sm_transition_to(
				ctx, WLAN_LINK_RECFG_SS_DEL_LINK_WAIT_LINK_SW);
		} else {
			/* AB -> A: B is set inactive for delete,
			 * no link switch event.
			 * or ABC -> A: C is standby but has been deleted
			 * by set link inactive
			 */
			mlo_link_recfg_del_link_completed(recfg_ctx);

		}
		break;
	case WLAN_LINK_RECFG_SM_EV_DISCONNECT_IND:
	case WLAN_LINK_RECFG_SM_EV_ROAM_START_IND:
		/* transition to abort state */
		mlo_link_recfg_sm_transition_to(
			ctx, WLAN_LINK_RECFG_SS_DEL_LINK_ABORT_WAIT_SET_LINK);
		break;
	case WLAN_LINK_RECFG_SM_EV_SER_TIMEOUT:
		/* handle serialization timeout */
		mlo_link_recfg_ser_timeout_sm_handler(recfg_ctx);
		break;
	default:
		event_handled = false;
		break;
	}

	return event_handled;
}

static void
mlo_link_recfg_subst_del_link_wait_set_link_exit(void *ctx)
{
}

/* WLAN_LINK_RECFG_SS_DEL_LINK_ABORT_WAIT_SET_LINK */
static void
mlo_link_recfg_subst_del_link_abort_wait_set_link_entry(void *ctx)
{
	if (mlo_link_recfg_sm_get_state(ctx) != WLAN_LINK_RECFG_S_DEL_LINK)
		QDF_BUG(0);

	mlo_link_recfg_sm_set_substate(
			ctx, WLAN_LINK_RECFG_SS_DEL_LINK_ABORT_WAIT_SET_LINK);
}

static bool
mlo_link_recfg_subst_del_link_abort_wait_set_link_event(void *ctx,
							uint16_t event,
							uint16_t event_data_len,
							void *event_data)
{
	struct mlo_link_recfg_context *recfg_ctx = ctx;
	bool event_handled = true;

	switch (event) {
	case WLAN_LINK_RECFG_SM_EV_SET_LINK_RSP:
		mlo_link_recfg_del_link_aborted(recfg_ctx);
		break;
	case WLAN_LINK_RECFG_SM_EV_SER_TIMEOUT:
		/* handle serialization timeout */
		mlo_link_recfg_ser_timeout_sm_handler(recfg_ctx);
		break;
	default:
		event_handled = false;
		break;
	}

	return event_handled;
}

static void
mlo_link_recfg_subst_del_link_abort_wait_set_link_exit(void *ctx)
{
}

/* WLAN_LINK_RECFG_SS_DEL_LINK_WAIT_LINK_SW */
static void
mlo_link_recfg_subst_del_link_wait_link_sw_entry(void *ctx)
{
	if (mlo_link_recfg_sm_get_state(ctx) != WLAN_LINK_RECFG_S_DEL_LINK)
		QDF_BUG(0);

	mlo_link_recfg_sm_set_substate(
			ctx, WLAN_LINK_RECFG_SS_DEL_LINK_WAIT_LINK_SW);
}

static bool
mlo_link_recfg_subst_del_link_wait_link_sw_event(void *ctx,
						 uint16_t event,
						 uint16_t event_data_len,
						 void *event_data)
{
	struct mlo_link_recfg_context *recfg_ctx = ctx;
	bool event_handled = true;
	struct link_switch_ind *link_switch_ind;
	struct link_switch_rsp *link_switch_rsp;

	switch (event) {
	case WLAN_LINK_RECFG_SM_EV_SET_LINK_RSP:
		/* start timer for fw link reconfig indication event.
		 * now using WLAN_LINK_RECFG_SM_EV_SER_TIMEOUT to abort link sw.
		 */
		break;
	case WLAN_LINK_RECFG_SM_EV_LINK_SWITCH_IND:
		/* cancel fw link reconfig indication timer.
		 */
		link_switch_ind = (struct link_switch_ind *)event_data;
		if (QDF_IS_STATUS_ERROR(link_switch_ind->status)) {
			/* unexpected link switch maybe rejected by other
			 * component. anyway link switch rsp will come with
			 * failure code.
			 */
			mlo_debug("link switch rejected status %d",
				  link_switch_ind->status);
		}
		break;
	case WLAN_LINK_RECFG_SM_EV_LINK_SWITCH_RSP:
		link_switch_rsp = (struct link_switch_rsp *)event_data;
		if (QDF_IS_STATUS_ERROR(link_switch_rsp->status)) {
			mlo_debug("link switch comp with failure status %d",
				  link_switch_rsp->status);
			mlo_link_recfg_del_link_aborted(recfg_ctx);
			break;
		}
		mlo_link_recfg_del_link_completed(recfg_ctx);
		break;
	case WLAN_LINK_RECFG_SM_EV_DISCONNECT_IND:
	case WLAN_LINK_RECFG_SM_EV_ROAM_START_IND:
		if (mlo_link_recfg_is_link_switch_in_progress(recfg_ctx)) {
			/* transition to abort state */
			mlo_link_recfg_sm_transition_to(
			ctx, WLAN_LINK_RECFG_SS_DEL_LINK_ABORT_WAIT_LINK_SW);
		} else {
			/* link switch haven't starting, just complete
			 * link recfg with abort
			 */
			mlo_link_recfg_del_link_aborted(recfg_ctx);
		}
		break;
	case WLAN_LINK_RECFG_SM_EV_SER_TIMEOUT:
		mlo_link_recfg_ser_timeout_sm_handler(recfg_ctx);
		break;
	default:
		event_handled = false;
		break;
	}

	return event_handled;
}

static void
mlo_link_recfg_subst_del_link_wait_link_sw_exit(void *ctx)
{
}

/* WLAN_LINK_RECFG_SS_DEL_LINK_ABORT_WAIT_LINK_SW */
static void
mlo_link_recfg_subst_del_link_abort_wait_link_sw_entry(void *ctx)
{
	if (mlo_link_recfg_sm_get_state(ctx) != WLAN_LINK_RECFG_S_DEL_LINK)
		QDF_BUG(0);

	mlo_link_recfg_sm_set_substate(
			ctx, WLAN_LINK_RECFG_SS_DEL_LINK_ABORT_WAIT_LINK_SW);
}

static bool
mlo_link_recfg_subst_del_link_abort_wait_link_sw_event(void *ctx,
						       uint16_t event,
						       uint16_t event_data_len,
						       void *event_data)
{
	struct mlo_link_recfg_context *recfg_ctx = ctx;
	bool event_handled = true;
	struct link_switch_rsp *link_switch_rsp;

	switch (event) {
	case WLAN_LINK_RECFG_SM_EV_LINK_SWITCH_RSP:
		link_switch_rsp = (struct link_switch_rsp *)event_data;
		if (QDF_IS_STATUS_ERROR(link_switch_rsp->status))
			mlo_debug("link switch comp with failure status %d",
				  link_switch_rsp->status);
		mlo_link_recfg_del_link_aborted(recfg_ctx);
		break;
	case WLAN_LINK_RECFG_SM_EV_SER_TIMEOUT:
		mlo_link_recfg_ser_timeout_sm_handler(recfg_ctx);
		break;
	default:
		event_handled = false;
		break;
	}

	return event_handled;
}

static void
mlo_link_recfg_subst_del_link_abort_wait_link_sw_exit(void *ctx)
{
}

/* WLAN_LINK_RECFG_S_ADD_LINK */
static void
mlo_link_recfg_state_add_link_entry(void *ctx)
{
	mlo_link_recfg_sm_state_update(ctx, WLAN_LINK_RECFG_S_ADD_LINK,
				       WLAN_LINK_RECFG_SS_IDLE);
}

static bool
mlo_link_recfg_state_add_link_event(void *ctx,
				    uint16_t event,
				    uint16_t event_data_len,
				    void *event_data)
{
	struct mlo_link_recfg_context *recfg_ctx = ctx;
	struct mlo_link_recfg_state_req *req;
	bool event_handled = true;
	struct wlan_mlo_link_switch_req link_sw_req = {0};
	QDF_STATUS status;

	switch (event) {
	case WLAN_LINK_RECFG_SM_EV_ADD_LINK:
		req = (struct mlo_link_recfg_state_req *)event_data;
		status = mlo_link_invoke_pre_link_add_handler(ctx, req);
		if (QDF_IS_STATUS_ERROR(status)) {
			mlo_link_recfg_add_link_aborted(recfg_ctx);
			break;
		}
		if (mlo_link_recfg_has_idle_vdev_for_add_link(
					recfg_ctx, req)) {
			/* A-> AB : use idle vdev to connect new add link */
			mlo_link_recfg_sm_transition_to(
			ctx, WLAN_LINK_RECFG_SS_ADD_LINK_WAIT_ADD_CONN);
			mlo_link_recfg_sm_deliver_event_sync(
				recfg_ctx->ml_dev, event,
				event_data_len, event_data);
		} else if (mlo_link_recfg_has_active_vdev_for_add_link(
					recfg_ctx, req, &link_sw_req)) {
			/* AB(B was deleted on vdev 1 by force inactive) -> AC:
			 * add standby C, trigger link switch from B -> C.
			 *
			 * AB(B was deleted on vdev 1 by force inactive) -> AB:
			 * trigger disconnect B and reconnect B(host initiated
			 * Link SW)
			 */
			mlo_link_recfg_sm_transition_to(
				ctx, WLAN_LINK_RECFG_SS_ADD_LINK_WAIT_LINK_SW);
			mlo_link_recfg_sm_deliver_event_sync(
				recfg_ctx->ml_dev, event,
				sizeof(link_sw_req), &link_sw_req);
		} else {
			/* AB -> ABC : add standby link C */
			mlo_link_recfg_add_standby_link(recfg_ctx, req);
			mlo_link_recfg_add_link_completed(recfg_ctx);
		}
		break;
	default:
		event_handled = false;
		break;
	}

	return event_handled;
}

static void
mlo_link_recfg_state_add_link_exit(void *ctx)
{
}

/* WLAN_LINK_RECFG_SS_ADD_LINK_WAIT_ADD_CONN */
static void
mlo_link_recfg_subst_add_link_wait_add_conn_entry(void *ctx)
{
	if (mlo_link_recfg_sm_get_state(ctx) != WLAN_LINK_RECFG_S_ADD_LINK)
		QDF_BUG(0);

	mlo_link_recfg_sm_set_substate(
			ctx, WLAN_LINK_RECFG_SS_ADD_LINK_WAIT_ADD_CONN);
}

static bool
mlo_link_recfg_subst_add_link_wait_add_conn_event(void *ctx,
						  uint16_t event,
						  uint16_t event_data_len,
						  void *event_data)
{
	struct mlo_link_recfg_context *recfg_ctx = ctx;
	struct mlo_link_recfg_state_req *req;
	bool event_handled = true;
	QDF_STATUS status;
	struct add_link_conn_rsp *add_link_conn_rsp;

	switch (event) {
	case WLAN_LINK_RECFG_SM_EV_ADD_LINK:
		req = (struct mlo_link_recfg_state_req *)event_data;
		status = mlo_link_recfg_add_link_connect_async(recfg_ctx, req);
		if (QDF_IS_STATUS_ERROR(status)) {
			mlo_link_recfg_add_link_aborted(recfg_ctx);
			break;
		}
		break;
	case WLAN_LINK_RECFG_SM_EV_ADD_CONN_RSP:
		add_link_conn_rsp = (struct add_link_conn_rsp *)event_data;
		if (QDF_IS_STATUS_ERROR(add_link_conn_rsp->status)) {
			mlo_link_recfg_add_link_aborted(recfg_ctx);
			break;
		}
		mlo_link_recfg_add_link_completed(recfg_ctx);
		break;
	case WLAN_LINK_RECFG_SM_EV_XMIT_REQ:
	case WLAN_LINK_RECFG_SM_EV_XMIT_STATUS:
	case WLAN_LINK_RECFG_SM_EV_RX_RSP:
		status =
		mlo_link_recfg_no_common_link_event(ctx, event,
						    event_data_len,
						    event_data);
		if (QDF_IS_STATUS_ERROR(status))
			event_handled = false;
		break;
	case WLAN_LINK_RECFG_SM_EV_DISCONNECT_IND:
	case WLAN_LINK_RECFG_SM_EV_ROAM_START_IND:
		/* transition to abort state */
		mlo_link_recfg_sm_transition_to(
			ctx, WLAN_LINK_RECFG_SS_ADD_LINK_ABORT_WAIT_ADD_CONN);
		break;
	case WLAN_LINK_RECFG_SM_EV_SER_TIMEOUT:
		mlo_link_recfg_ser_timeout_sm_handler(recfg_ctx);
		break;
	default:
		event_handled = false;
		break;
	}

	return event_handled;
}

static void
mlo_link_recfg_subst_add_link_wait_add_conn_exit(void *ctx)
{
}

/* WLAN_LINK_RECFG_SS_ADD_LINK_ABORT_WAIT_ADD_CONN */
static void
mlo_link_recfg_subst_add_link_abort_wait_add_conn_entry(void *ctx)
{
	if (mlo_link_recfg_sm_get_state(ctx) != WLAN_LINK_RECFG_S_ADD_LINK)
		QDF_BUG(0);

	mlo_link_recfg_sm_set_substate(
			ctx, WLAN_LINK_RECFG_SS_ADD_LINK_ABORT_WAIT_ADD_CONN);
}

static bool
mlo_link_recfg_subst_add_link_abort_wait_add_conn_event(
						void *ctx,
						uint16_t event,
						uint16_t event_data_len,
						void *event_data)
{
	struct mlo_link_recfg_context *recfg_ctx = ctx;
	bool event_handled = true;

	switch (event) {
	case WLAN_LINK_RECFG_SM_EV_ADD_CONN_RSP:
		mlo_link_recfg_add_link_aborted(recfg_ctx);
		break;
	case WLAN_LINK_RECFG_SM_EV_SER_TIMEOUT:
		mlo_link_recfg_ser_timeout_sm_handler(recfg_ctx);
		break;
	default:
		event_handled = false;
		break;
	}

	return event_handled;
}

static void
mlo_link_recfg_subst_add_link_abort_wait_add_conn_exit(void *ctx)
{
}

/* WLAN_LINK_RECFG_SS_ADD_LINK_WAIT_LINK_SW */
static void
mlo_link_recfg_subst_add_link_wait_link_sw_entry(void *ctx)
{
	if (mlo_link_recfg_sm_get_state(ctx) != WLAN_LINK_RECFG_S_ADD_LINK)
		QDF_BUG(0);

	mlo_link_recfg_sm_set_substate(
			ctx, WLAN_LINK_RECFG_SS_ADD_LINK_WAIT_LINK_SW);
}

static bool
mlo_link_recfg_subst_add_link_wait_link_sw_event(void *ctx,
						 uint16_t event,
						 uint16_t event_data_len,
						 void *event_data)
{
	struct mlo_link_recfg_context *recfg_ctx = ctx;
	struct wlan_mlo_link_switch_req *link_sw_req;
	struct link_switch_rsp *link_switch_rsp;
	QDF_STATUS status;
	bool event_handled = true;

	switch (event) {
	case WLAN_LINK_RECFG_SM_EV_ADD_LINK:
		link_sw_req = (struct wlan_mlo_link_switch_req *)event_data;
		status = mlo_link_recfg_host_trigger_link_switch(recfg_ctx,
								 link_sw_req);
		if (QDF_IS_STATUS_ERROR(status))
			mlo_link_recfg_add_link_aborted(recfg_ctx);

		/* start timer for fw link reconfig indication event? or
		 * using serialization timeout to abort the state.
		 */
		break;
	case WLAN_LINK_RECFG_SM_EV_XMIT_REQ:
	case WLAN_LINK_RECFG_SM_EV_XMIT_STATUS:
	case WLAN_LINK_RECFG_SM_EV_RX_RSP:
		status =
		mlo_link_recfg_no_common_link_event(ctx, event,
						    event_data_len,
						    event_data);
		if (QDF_IS_STATUS_ERROR(status))
			event_handled = false;
		break;
	case WLAN_LINK_RECFG_SM_EV_LINK_SWITCH_IND:
		// cancel fw link reconfig indication timer.
		break;
	case WLAN_LINK_RECFG_SM_EV_LINK_SWITCH_RSP:
		link_switch_rsp = (struct link_switch_rsp *)event_data;
		if (QDF_IS_STATUS_ERROR(link_switch_rsp->status)) {
			mlo_debug("link switch comp with failure status %d",
				  link_switch_rsp->status);
			mlo_link_recfg_add_link_aborted(recfg_ctx);
			break;
		}
		mlo_link_recfg_add_link_completed(recfg_ctx);
		break;
	case WLAN_LINK_RECFG_SM_EV_DISCONNECT_IND:
	case WLAN_LINK_RECFG_SM_EV_ROAM_START_IND:
		if (mlo_link_recfg_is_link_switch_in_progress(recfg_ctx)) {
			/* transition to abort state */
			mlo_link_recfg_sm_transition_to(
			ctx, WLAN_LINK_RECFG_SS_ADD_LINK_ABORT_WAIT_LINK_SW);
		} else {
			/* link switch haven't starting, just complete
			 * link recfg with abort
			 */
			mlo_link_recfg_add_link_aborted(recfg_ctx);
		}
		break;
	case WLAN_LINK_RECFG_SM_EV_SER_TIMEOUT:
		mlo_link_recfg_ser_timeout_sm_handler(recfg_ctx);
		break;
	default:
		event_handled = false;
		break;
	}

	return event_handled;
}

static void
mlo_link_recfg_subst_add_link_wait_link_sw_exit(void *ctx)
{
}

/* WLAN_LINK_RECFG_SS_ADD_LINK_ABORT_WAIT_LINK_SW */
static void
mlo_link_recfg_subst_add_link_abort_wait_link_sw_entry(void *ctx)
{
	if (mlo_link_recfg_sm_get_state(ctx) != WLAN_LINK_RECFG_S_ADD_LINK)
		QDF_BUG(0);

	mlo_link_recfg_sm_set_substate(
			ctx, WLAN_LINK_RECFG_SS_ADD_LINK_ABORT_WAIT_LINK_SW);
}

static bool
mlo_link_recfg_subst_add_link_abort_wait_link_sw_event(
						void *ctx,
						uint16_t event,
						uint16_t event_data_len,
						void *event_data)
{
	struct mlo_link_recfg_context *recfg_ctx = ctx;
	bool event_handled = true;

	switch (event) {
	case WLAN_LINK_RECFG_SM_EV_LINK_SWITCH_RSP:
		mlo_link_recfg_add_link_aborted(recfg_ctx);
		break;
	case WLAN_LINK_RECFG_SM_EV_SER_TIMEOUT:
		mlo_link_recfg_ser_timeout_sm_handler(recfg_ctx);
		break;
	default:
		event_handled = false;
		break;
	}

	return event_handled;
}

static void
mlo_link_recfg_subst_add_link_abort_wait_link_sw_exit(void *ctx)
{
}

/* WLAN_LINK_RECFG_S_XMIT_REQ */
static void
mlo_link_recfg_state_xmit_req_entry(void *ctx)
{
	mlo_link_recfg_sm_state_update(ctx, WLAN_LINK_RECFG_S_XMIT_REQ,
				       WLAN_LINK_RECFG_SS_IDLE);
}

static bool
mlo_link_recfg_state_xmit_req_event(void *ctx,
				    uint16_t event,
				    uint16_t event_data_len,
				    void *event_data)
{
	struct mlo_link_recfg_context *recfg_ctx = ctx;
	struct mlo_link_recfg_state_req *req;
	bool event_handled = true;
	QDF_STATUS status;

	switch (event) {
	case WLAN_LINK_RECFG_SM_EV_XMIT_REQ:
		req = (struct mlo_link_recfg_state_req *)event_data;
		mlo_link_recfg_send_request_frame(recfg_ctx, req);
		break;
	case WLAN_LINK_RECFG_SM_EV_XMIT_STATUS:
		/* Handle tx failure		*/
		mlo_link_recfg_sm_transition_to(ctx, WLAN_LINK_RECFG_S_ABORT);
		mlo_link_recfg_sm_deliver_event_sync(
					recfg_ctx->ml_dev,
					WLAN_LINK_RECFG_SM_EV_COMPLETED,
					0, NULL);
		break;
	case WLAN_LINK_RECFG_SM_EV_RX_RSP:
		status = mlo_link_recfg_response_received(
			recfg_ctx, (struct link_recfg_rx_rsp *)event_data,
			event_data_len);
		if (QDF_IS_STATUS_ERROR(status)) {
			mlo_err("error to handle resp status %d", status);
			mlo_link_recfg_sm_transition_to(ctx, WLAN_LINK_RECFG_S_ABORT);
			mlo_link_recfg_sm_deliver_event_sync(
						recfg_ctx->ml_dev,
						WLAN_LINK_RECFG_SM_EV_COMPLETED,
						0, NULL);
		}
		break;
	case WLAN_LINK_RECFG_SM_EV_DISCONNECT_IND:
	case WLAN_LINK_RECFG_SM_EV_ROAM_START_IND:
		mlo_link_recfg_sm_transition_to(ctx, WLAN_LINK_RECFG_S_ABORT);
		mlo_link_recfg_sm_deliver_event_sync(
					recfg_ctx->ml_dev,
					WLAN_LINK_RECFG_SM_EV_COMPLETED,
					0, NULL);
		break;
	case WLAN_LINK_RECFG_SM_EV_SER_TIMEOUT:
		mlo_link_recfg_ser_timeout_sm_handler(recfg_ctx);
		break;
	default:
		event_handled = false;
		break;
	}

	return event_handled;
}

static void
mlo_link_recfg_state_xmit_req_exit(void *ctx)
{
}

/* WLAN_LINK_RECFG_S_ABORT */
static void
mlo_link_recfg_state_abort_entry(void *ctx)
{
	mlo_link_recfg_sm_state_update(ctx, WLAN_LINK_RECFG_S_ABORT,
				       WLAN_LINK_RECFG_SS_IDLE);
}

static bool
mlo_link_recfg_state_abort_event(void *ctx,
				 uint16_t event,
				 uint16_t event_data_len,
				 void *event_data)
{
	struct mlo_link_recfg_context *recfg_ctx = ctx;
	bool event_handled = true;

	switch (event) {
	case WLAN_LINK_RECFG_SM_EV_COMPLETED:
		mlo_link_recfg_complete(recfg_ctx, false);
		break;
	default:
		event_handled = false;
		break;
	}

	return event_handled;
}

static void
mlo_link_recfg_state_abort_exit(void *ctx)
{
}

/* WLAN_LINK_RECFG_S_COMPLETED */
static void
mlo_link_recfg_state_completed_entry(void *ctx)
{
	mlo_link_recfg_sm_state_update(ctx, WLAN_LINK_RECFG_S_COMPLETED,
				       WLAN_LINK_RECFG_SS_IDLE);
}

static bool
mlo_link_recfg_state_completed_event(void *ctx,
				     uint16_t event,
				     uint16_t event_data_len,
				     void *event_data)
{
	struct mlo_link_recfg_context *recfg_ctx = ctx;
	bool event_handled = true;

	switch (event) {
	case WLAN_LINK_RECFG_SM_EV_COMPLETED:
		mlo_link_recfg_complete(recfg_ctx, true);
		break;
	default:
		event_handled = false;
		break;
	}

	return event_handled;
}

static void
mlo_link_recfg_state_completed_exit(void *ctx)
{
}

static struct wlan_sm_state_info mlo_link_recfg_sm_info[] = {
	{
		(uint8_t)WLAN_LINK_RECFG_S_INIT,
		(uint8_t)WLAN_SM_ENGINE_STATE_NONE,
		(uint8_t)WLAN_SM_ENGINE_STATE_NONE,
		false,
		"INIT",
		mlo_link_recfg_state_init_entry,
		mlo_link_recfg_state_init_exit,
		mlo_link_recfg_state_init_event,
	},
	{
		(uint8_t)WLAN_LINK_RECFG_S_START,
		(uint8_t)WLAN_SM_ENGINE_STATE_NONE,
		(uint8_t)WLAN_SM_ENGINE_STATE_NONE,
		true,
		"START",
		mlo_link_recfg_state_start_entry,
		mlo_link_recfg_state_start_exit,
		mlo_link_recfg_state_start_event,
	},
	{
		(uint8_t)WLAN_LINK_RECFG_S_DEL_LINK,
		(uint8_t)WLAN_SM_ENGINE_STATE_NONE,
		(uint8_t)WLAN_SM_ENGINE_STATE_NONE,
		true,
		"DEL_LINK",
		mlo_link_recfg_state_del_link_entry,
		mlo_link_recfg_state_del_link_exit,
		mlo_link_recfg_state_del_link_event,
	},
	{
		(uint8_t)WLAN_LINK_RECFG_S_XMIT_REQ,
		(uint8_t)WLAN_SM_ENGINE_STATE_NONE,
		(uint8_t)WLAN_SM_ENGINE_STATE_NONE,
		false,
		"XMIT_REQ",
		mlo_link_recfg_state_xmit_req_entry,
		mlo_link_recfg_state_xmit_req_exit,
		mlo_link_recfg_state_xmit_req_event,
	},
	{
		(uint8_t)WLAN_LINK_RECFG_S_ADD_LINK,
		(uint8_t)WLAN_SM_ENGINE_STATE_NONE,
		(uint8_t)WLAN_SM_ENGINE_STATE_NONE,
		true,
		"ADD_LINK",
		mlo_link_recfg_state_add_link_entry,
		mlo_link_recfg_state_add_link_exit,
		mlo_link_recfg_state_add_link_event,
	},
	{
		(uint8_t)WLAN_LINK_RECFG_S_COMPLETED,
		(uint8_t)WLAN_SM_ENGINE_STATE_NONE,
		(uint8_t)WLAN_SM_ENGINE_STATE_NONE,
		false,
		"COMPLETED",
		mlo_link_recfg_state_completed_entry,
		mlo_link_recfg_state_completed_exit,
		mlo_link_recfg_state_completed_event,
	},
	{
		(uint8_t)WLAN_LINK_RECFG_S_ABORT,
		(uint8_t)WLAN_SM_ENGINE_STATE_NONE,
		(uint8_t)WLAN_SM_ENGINE_STATE_NONE,
		false,
		"ABORT",
		mlo_link_recfg_state_abort_entry,
		mlo_link_recfg_state_abort_exit,
		mlo_link_recfg_state_abort_event,
	},
	{
		(uint8_t)WLAN_LINK_RECFG_S_MAX,
		(uint8_t)WLAN_SM_ENGINE_STATE_NONE,
		(uint8_t)WLAN_SM_ENGINE_STATE_NONE,
		false,
		"INVALID",
		NULL,
		NULL,
		NULL,
	},
	{
		(uint8_t)WLAN_LINK_RECFG_SS_IDLE,
		(uint8_t)WLAN_SM_ENGINE_STATE_NONE,
		(uint8_t)WLAN_SM_ENGINE_STATE_NONE,
		false,
		"IDLE",
		NULL,
		NULL,
		NULL,
	},
	{
		(uint8_t)WLAN_LINK_RECFG_SS_START_PENDING,
		(uint8_t)WLAN_LINK_RECFG_S_START,
		(uint8_t)WLAN_SM_ENGINE_STATE_NONE,
		false,
		"START_PENDING",
		mlo_link_recfg_subst_start_pending_entry,
		mlo_link_recfg_subst_start_pending_exit,
		mlo_link_recfg_subst_start_pending_event,
	},
	{
		(uint8_t)WLAN_LINK_RECFG_SS_START_ACTIVE,
		(uint8_t)WLAN_LINK_RECFG_S_START,
		(uint8_t)WLAN_SM_ENGINE_STATE_NONE,
		false,
		"START_ACTIVE",
		mlo_link_recfg_subst_start_active_entry,
		mlo_link_recfg_subst_start_active_exit,
		mlo_link_recfg_subst_start_active_event,
	},
	{
		(uint8_t)WLAN_LINK_RECFG_SS_DEL_LINK_WAIT_SET_LINK,
		(uint8_t)WLAN_LINK_RECFG_S_DEL_LINK,
		(uint8_t)WLAN_SM_ENGINE_STATE_NONE,
		false,
		"DEL_LINK_WAIT_SET_LINK",
		mlo_link_recfg_subst_del_link_wait_set_link_entry,
		mlo_link_recfg_subst_del_link_wait_set_link_exit,
		mlo_link_recfg_subst_del_link_wait_set_link_event,
	},
	{
		(uint8_t)WLAN_LINK_RECFG_SS_DEL_LINK_WAIT_LINK_SW,
		(uint8_t)WLAN_LINK_RECFG_S_DEL_LINK,
		(uint8_t)WLAN_SM_ENGINE_STATE_NONE,
		false,
		"DEL_LINK_WAIT_LINK_SW",
		mlo_link_recfg_subst_del_link_wait_link_sw_entry,
		mlo_link_recfg_subst_del_link_wait_link_sw_exit,
		mlo_link_recfg_subst_del_link_wait_link_sw_event,
	},
	{
		(uint8_t)WLAN_LINK_RECFG_SS_DEL_LINK_ABORT_WAIT_SET_LINK,
		(uint8_t)WLAN_LINK_RECFG_S_DEL_LINK,
		(uint8_t)WLAN_SM_ENGINE_STATE_NONE,
		false,
		"DEL_LINK_ABORT_WAIT_SET_LINK",
		mlo_link_recfg_subst_del_link_abort_wait_set_link_entry,
		mlo_link_recfg_subst_del_link_abort_wait_set_link_exit,
		mlo_link_recfg_subst_del_link_abort_wait_set_link_event,
	},
	{
		(uint8_t)WLAN_LINK_RECFG_SS_DEL_LINK_ABORT_WAIT_LINK_SW,
		(uint8_t)WLAN_LINK_RECFG_S_DEL_LINK,
		(uint8_t)WLAN_SM_ENGINE_STATE_NONE,
		false,
		"DEL_LINK_ABORT_WAIT_LINK_SW",
		mlo_link_recfg_subst_del_link_abort_wait_link_sw_entry,
		mlo_link_recfg_subst_del_link_abort_wait_link_sw_exit,
		mlo_link_recfg_subst_del_link_abort_wait_link_sw_event,
	},
	{
		(uint8_t)WLAN_LINK_RECFG_SS_ADD_LINK_WAIT_ADD_CONN,
		(uint8_t)WLAN_LINK_RECFG_S_ADD_LINK,
		(uint8_t)WLAN_SM_ENGINE_STATE_NONE,
		false,
		"ADD_LINK_WAIT_ADD_CONN",
		mlo_link_recfg_subst_add_link_wait_add_conn_entry,
		mlo_link_recfg_subst_add_link_wait_add_conn_exit,
		mlo_link_recfg_subst_add_link_wait_add_conn_event,
	},
	{
		(uint8_t)WLAN_LINK_RECFG_SS_ADD_LINK_WAIT_LINK_SW,
		(uint8_t)WLAN_LINK_RECFG_S_ADD_LINK,
		(uint8_t)WLAN_SM_ENGINE_STATE_NONE,
		false,
		"ADD_LINK_WAIT_LINK_SW",
		mlo_link_recfg_subst_add_link_wait_link_sw_entry,
		mlo_link_recfg_subst_add_link_wait_link_sw_exit,
		mlo_link_recfg_subst_add_link_wait_link_sw_event,
	},
	{
		(uint8_t)WLAN_LINK_RECFG_SS_ADD_LINK_ABORT_WAIT_ADD_CONN,
		(uint8_t)WLAN_LINK_RECFG_S_ADD_LINK,
		(uint8_t)WLAN_SM_ENGINE_STATE_NONE,
		false,
		"ADD_LINK_ABORT_WAIT_ADD_CONN",
		mlo_link_recfg_subst_add_link_abort_wait_add_conn_entry,
		mlo_link_recfg_subst_add_link_abort_wait_add_conn_exit,
		mlo_link_recfg_subst_add_link_abort_wait_add_conn_event,
	},
	{
		(uint8_t)WLAN_LINK_RECFG_SS_ADD_LINK_ABORT_WAIT_LINK_SW,
		(uint8_t)WLAN_LINK_RECFG_S_ADD_LINK,
		(uint8_t)WLAN_SM_ENGINE_STATE_NONE,
		false,
		"ADD_LINK_ABORT_WAIT_LINK_SW",
		mlo_link_recfg_subst_add_link_abort_wait_link_sw_entry,
		mlo_link_recfg_subst_add_link_abort_wait_link_sw_exit,
		mlo_link_recfg_subst_add_link_abort_wait_link_sw_event,
	},
	{
		(uint8_t)WLAN_LINK_RECFG_SS_MAX,
		(uint8_t)WLAN_SM_ENGINE_STATE_NONE,
		(uint8_t)WLAN_SM_ENGINE_STATE_NONE,
		false,
		"INVALID",
		NULL,
		NULL,
		NULL,
	},
};

static const char *mlo_link_recfg_sm_event_names[] = {
	"EV_USER_REQ",
	"EV_FW_IND",
	"EV_START",
	"EV_ACTIVE",
	"EV_DEL_LINK",
	"EV_ADD_LINK",
	"EV_XMIT_REQ",
	"EV_XMIT_STATUS",
	"EV_RX_RSP",
	"EV_SET_LINK_RSP",
	"EV_LINK_SWITCH_IND",
	"EV_LINK_SWITCH_RSP",
	"EV_ADD_CONN_RSP",
	"EV_DISCONNECT_IND",
	"EV_ROAM_START_IND",
	"EV_COMPLETED",
	"EV_SER_TIMEOUT",
};

static QDF_STATUS mlo_link_recfg_sm_create(struct mlo_link_recfg_context *ctx)
{
	struct wlan_sm *sm;
	uint8_t name[WLAN_SM_ENGINE_MAX_NAME];
	struct wlan_mlo_dev_context *ml_dev = ctx->ml_dev;
	uint8_t vdev_id;

	if (!ml_dev->wlan_vdev_list[0]) {
		mlo_err("no vdev in ml dev");
		return QDF_STATUS_E_INVAL;
	}
	vdev_id = wlan_vdev_get_id(ml_dev->wlan_vdev_list[0]);
	qdf_scnprintf(name, sizeof(name), "LNK_RCFG_%d", vdev_id);
	sm = wlan_sm_create(name, ctx,
			    WLAN_CM_S_INIT,
			    mlo_link_recfg_sm_info,
			    QDF_ARRAY_SIZE(mlo_link_recfg_sm_info),
			    mlo_link_recfg_sm_event_names,
			    QDF_ARRAY_SIZE(mlo_link_recfg_sm_event_names));
	if (!sm)
		return QDF_STATUS_E_NOMEM;

	ctx->sm.sm_hdl = sm;

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS mlo_link_recfg_sm_destroy(struct mlo_link_recfg_context *ctx)
{
	wlan_sm_delete(ctx->sm.sm_hdl);

	return QDF_STATUS_SUCCESS;
}

QDF_STATUS mlo_link_recfg_init(struct wlan_objmgr_psoc *psoc,
			       struct wlan_mlo_dev_context *ml_dev)
{
	QDF_STATUS status;
	struct mlo_link_recfg_context *recfg_ctx;

	if (!wlan_mlme_is_link_recfg_support(psoc)) {
		mlo_debug("link_recfg not supported");
		return QDF_STATUS_SUCCESS;
	}

	recfg_ctx = qdf_mem_malloc(sizeof(struct mlo_link_recfg_context));
	if (!recfg_ctx)
		return QDF_STATUS_E_NOMEM;

	recfg_ctx->psoc = psoc;
	recfg_ctx->ml_dev = ml_dev;
	status = mlo_link_recfg_sm_create(recfg_ctx);
	if (QDF_IS_STATUS_ERROR(status)) {
		qdf_mem_free(recfg_ctx);
		return status;
	}
	ml_dev->link_recfg_ctx = recfg_ctx;
	ml_link_recfg_sm_lock_create(ml_dev);
	recfg_ctx->sm.curr_state_idx = -1;

	return QDF_STATUS_SUCCESS;
}

QDF_STATUS mlo_link_recfg_deinit(struct wlan_mlo_dev_context *ml_dev)
{
	if (!ml_dev->link_recfg_ctx)
		return QDF_STATUS_SUCCESS;

	ml_link_recfg_sm_lock_destroy(ml_dev);
	mlo_link_recfg_sm_destroy(ml_dev->link_recfg_ctx);
	qdf_mem_free(ml_dev->link_recfg_ctx);
	ml_dev->link_recfg_ctx = NULL;

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS
mlo_link_recfg_if_add_link_accepted(
			struct wlan_mlo_link_recfg_req *link_recfg_req)
{
	uint8_t i;

	if (!link_recfg_req)
		return QDF_STATUS_E_NULL_VALUE;

	if (!link_recfg_req->add_link_info.num_links)
		return QDF_STATUS_SUCCESS;

	for (i = 0; i < link_recfg_req->add_link_info.num_links; i++) {
		if (link_recfg_req->add_link_info.link[i].status_code ==
			STATUS_SUCCESS)
			return QDF_STATUS_SUCCESS;
	}

	return QDF_STATUS_E_FAILURE;
}

static enum wlan_status_code
mlo_link_recfg_find_link_status(uint8_t link_id,
				struct wlan_mlo_link_recfg_rsp *link_recfg_rsp)
{
	uint8_t i;

	for (i = 0; i < link_recfg_rsp->count; i++) {
		if (link_id == link_recfg_rsp->recfg_status_list[i].link_id)
			return link_recfg_rsp->recfg_status_list[i].status_code;
	}

	if (i == link_recfg_rsp->count)
		mlo_err("link id not found");

	return STATUS_UNSPECIFIED_FAILURE;
}

static QDF_STATUS
mlo_link_recfg_update_result(struct mlo_link_recfg_context *ctx,
			     struct wlan_mlo_link_recfg_rsp *link_recfg_rsp)
{
	uint8_t i;
	uint8_t link_id;
	struct wlan_mlo_link_recfg_req *link_recfg_req;
	struct wlan_mlo_link_recfg_info *add_link_info;
	struct wlan_mlo_link_recfg_info *del_link_info;

	if (!ctx || !link_recfg_rsp)
		return QDF_STATUS_E_NULL_VALUE;

	link_recfg_req = &ctx->curr_recfg_req;
	if (!link_recfg_rsp->count) {
		mlo_debug("Link recfg rsp count is 0");
		return QDF_STATUS_E_NULL_VALUE;
	}

	add_link_info = &link_recfg_req->add_link_info;
	del_link_info = &link_recfg_req->del_link_info;

	for (i = 0; i < add_link_info->num_links; i++) {
		link_id = add_link_info->link[i].link_id;
		add_link_info->link[i].status_code =  mlo_link_recfg_find_link_status(link_id,
										      link_recfg_rsp);
	}

	for (i = 0; i < del_link_info->num_links; i++) {
		link_id = del_link_info->link[i].link_id;
		del_link_info->link[i].status_code =  mlo_link_recfg_find_link_status(link_id,
										      link_recfg_rsp);
	}

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS
mlo_link_recfg_gen_link_assoc_rsp(struct wlan_objmgr_vdev *vdev,
				  struct mlo_link_recfg_context *ctx,
				  struct wlan_mlo_link_recfg_req *link_recfg_req,
				  struct wlan_action_frame_args *action_frm,
				  uint16_t frm_len,
				  uint16_t *ie_offset)
{
	uint8_t i;
	uint8_t link_id;
	struct wlan_mlo_link_recfg_info *add_link_info;
	struct element_info org_assoc_rsp;
	QDF_STATUS status;

	if (!ctx || !link_recfg_req)
		return QDF_STATUS_E_NULL_VALUE;

	add_link_info = &link_recfg_req->add_link_info;
	if (!add_link_info->num_links)
		return QDF_STATUS_SUCCESS;

	mlo_get_assoc_rsp(vdev, &org_assoc_rsp);
	if (!org_assoc_rsp.len) {
		mlo_err("Org Assoc response frame len is 0");
		return QDF_STATUS_E_INVAL;
	}

	for (i = 0; i < add_link_info->num_links; i++) {
		struct element_info link_assoc_rsp;

		if (add_link_info->link[i].status_code == STATUS_SUCCESS) {
			link_id = add_link_info->link[i].link_id;

			add_link_info->link[i].link_assoc_rsp.ptr = qdf_mem_malloc(org_assoc_rsp.len +
										   frm_len);
			if (!add_link_info->link[i].link_assoc_rsp.ptr)
				return QDF_STATUS_E_NOMEM;

			link_assoc_rsp.ptr = add_link_info->link[i].link_assoc_rsp.ptr;
			link_assoc_rsp.len = org_assoc_rsp.len + frm_len;
			status = util_gen_link_recfg_assoc_rsp(
								&org_assoc_rsp,
								(uint8_t *)action_frm,
								frm_len,
								*ie_offset, link_id,
								add_link_info->link[i].self_link_addr,
								link_assoc_rsp.ptr,
								org_assoc_rsp.len + frm_len,
								(qdf_size_t *)&link_assoc_rsp.len);
			add_link_info->link[i].link_assoc_rsp.len = link_assoc_rsp.len;
			if (QDF_IS_STATUS_SUCCESS(status)) {
				mlo_debug("MLO: link recfg assoc rsp for link vdev");
				mlo_update_cache_link_assoc_rsp(vdev, link_id,
								&link_assoc_rsp);
				mgmt_txrx_frame_hex_dump(link_assoc_rsp.ptr,
							 link_assoc_rsp.len,
							 false);
				qdf_mem_free(add_link_info->link[i].link_assoc_rsp.ptr);
				add_link_info->link[i].link_assoc_rsp.ptr = NULL;
				add_link_info->link[i].link_assoc_rsp.len = 0;
			} else {
				mlo_err("Link recfg assoc resp generation failed");
				add_link_info->link[i].link_assoc_rsp.len = 0;
				qdf_mem_free(add_link_info->link[i].link_assoc_rsp.ptr);
				add_link_info->link[i].link_assoc_rsp.ptr = NULL;
				return status;
			}
		}
	}

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS
mlo_link_recfg_parse_action_rsp(struct mlo_link_recfg_context *ctx,
				struct wlan_mlo_link_recfg_rsp *link_recfg_rsp,
				struct wlan_action_frame *action_frm,
				uint32_t frame_len,
				uint16_t *ie_offset)
{
	uint8_t *link_recfg_action_frm = NULL, *frame = NULL;
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	QDF_STATUS link_status;
	uint16_t ie_len_parsed;
	uint8_t i;
	struct element_info oci_ie = {0};
	uint8_t *mlieseq;
	qdf_size_t mlieseqlen;
	struct wlan_mlo_link_recfg_req *link_recfg_req;

	if (!ctx)
		return QDF_STATUS_E_NULL_VALUE;

	if (!action_frm || !frame_len)
		return QDF_STATUS_E_NULL_VALUE;

	link_recfg_req = &ctx->curr_recfg_req;
	/*
	 * Link Reconfig response action frame
	 *
	 *   1-byte     1-byte     1-byte   1-byte   3- bytes
	 *----------------------------------------------------
	 * |         |           |        |        |         |
	 * | Category| Protected | Dialog | Count  | Recfg   |
	 * |         |    EHT    | token  |        | Status  |
	 * |         |  Action   |        |        | Duple   |
	 *----------------------------------------------------
	 */

	ie_len_parsed = sizeof(*action_frm) + sizeof(uint8_t) +
			sizeof(uint8_t);

	mlo_debug("Link Recfg rsp frame len %d ", frame_len);

	if (frame_len < ie_len_parsed) {
		mlo_err("Action frame length %d too short", frame_len);
		return QDF_STATUS_E_FAILURE;
	}

	frame = (uint8_t *)action_frm;
	link_recfg_action_frm = (uint8_t *)action_frm + sizeof(*action_frm);

	link_recfg_rsp->dialog_token = *link_recfg_action_frm;
	mlo_debug("Link Recfg rsp dialog_token %d ",
		  link_recfg_rsp->dialog_token);

	link_recfg_rsp->count = *(link_recfg_action_frm + sizeof(uint8_t));
	mlo_debug("Link Recfg rsp count %d ",
		  link_recfg_rsp->count);

	link_recfg_action_frm += sizeof(uint8_t) + sizeof(uint8_t);
	for (i = 0; i < link_recfg_rsp->count; i++) {
		link_recfg_rsp->recfg_status_list[i].link_id = *link_recfg_action_frm;
		link_recfg_rsp->recfg_status_list[i].status_code =
			qdf_le16_to_cpu(*(uint16_t *)(link_recfg_action_frm + sizeof(uint8_t)));
		mlo_debug("Link Recfg rsp link id %d status_code %d",
			  link_recfg_rsp->recfg_status_list[i].link_id,
			  link_recfg_rsp->recfg_status_list[i].status_code);
		link_recfg_action_frm += sizeof(uint8_t) + sizeof(uint16_t);
	}

	mlo_link_recfg_update_result(ctx, link_recfg_rsp);
	link_status = mlo_link_recfg_if_add_link_accepted(link_recfg_req);
	if (QDF_IS_STATUS_SUCCESS(link_status) &&
	    link_recfg_req->add_link_info.num_links) {
		// Group key data
		link_recfg_rsp->grp_key_data.len = *link_recfg_action_frm;
		link_recfg_rsp->grp_key_data.ptr = qdf_mem_malloc(link_recfg_rsp->grp_key_data.len);

		if (!link_recfg_rsp->grp_key_data.ptr) {
			mlo_err("Malloc failed");
			status = QDF_STATUS_E_NOMEM;
		}
		link_recfg_action_frm += sizeof(uint8_t);
		qdf_mem_copy(link_recfg_rsp->grp_key_data.ptr,
			     link_recfg_action_frm,
			     link_recfg_rsp->grp_key_data.len);
		link_recfg_action_frm += sizeof(uint8_t) * link_recfg_rsp->grp_key_data.len;

		// OCI IE
		if (*(link_recfg_action_frm) == WLAN_ELEMID_EXTN_ELEM &&
		    *(link_recfg_action_frm + sizeof(uint16_t)) == WLAN_EXTN_ELEMID_OCI) {
			oci_ie.len = *(link_recfg_action_frm + sizeof(uint8_t));
			mlo_debug("OCI IE len %d", oci_ie.len);
			oci_ie.ptr = link_recfg_action_frm + sizeof(uint8_t) +
				     sizeof(uint16_t);
			link_recfg_rsp->oci_ie.ptr = qdf_mem_malloc(oci_ie.len);
			if (!link_recfg_rsp->oci_ie.ptr) {
				mlo_err("Malloc failed");
				status = QDF_STATUS_E_NOMEM;
				goto end;
			}
			qdf_mem_copy(link_recfg_rsp->oci_ie.ptr, oci_ie.ptr,
				     oci_ie.len);
			link_recfg_rsp->oci_ie.len = oci_ie.len;
			oci_ie.len += MIN_IE_LEN;
		}
		link_recfg_action_frm += oci_ie.len;

		// Basic MLIE
		status = util_find_mlie(link_recfg_action_frm,
					(frame_len - (uint16_t)(link_recfg_action_frm - frame)),
					&mlieseq,
					&mlieseqlen);
		if (QDF_IS_STATUS_ERROR(status)) {
			mlo_err("ML IE parsing failed %d", status);
			return status;
		}

		if (!mlieseq) {
			*ie_offset = 0;
			mlo_err("ML IE not found %zu ie_offset %d", mlieseqlen,
				*ie_offset);
		} else {
			*ie_offset = (uint16_t)(link_recfg_action_frm - frame);
			mlo_err("ML IE found %zu, ie_offset %d", mlieseqlen,
				*ie_offset);
		}

		if (mlieseqlen) {
			link_recfg_rsp->mlo_ie.len = mlieseqlen;
			link_recfg_rsp->mlo_ie.ptr = qdf_mem_malloc(link_recfg_rsp->mlo_ie.len);
			if (!link_recfg_rsp->mlo_ie.ptr) {
				mlo_err("Malloc failed");
				status = QDF_STATUS_E_NOMEM;
				goto end;
			}
			qdf_mem_copy(link_recfg_rsp->mlo_ie.ptr, mlieseq,
				     link_recfg_rsp->mlo_ie.len);
			mlo_debug("ML IE len %d", link_recfg_rsp->mlo_ie.len);
		}
	}

	if (frame_len) {
		ctx->rsp_frame.ptr = qdf_mem_malloc(frame_len);
		if (!ctx->rsp_frame.ptr) {
			mlo_err("rsp frame malloc failed");
			status = QDF_STATUS_E_NOMEM;
			goto end;
		}

		// copy the Link Reconfiguration response frame
		qdf_mem_copy(ctx->rsp_frame.ptr,
			     frame,
			     frame_len);
		ctx->rsp_frame.len = frame_len;
		mlo_err("Link Reconfig rsp rx dump:");
		QDF_TRACE_HEX_DUMP(QDF_MODULE_ID_MLO,
				   QDF_TRACE_LEVEL_DEBUG,
				   frame,
				   frame_len);
	}
end:
	return status;
}

QDF_STATUS
mlo_link_recfg_rx_rsp(struct wlan_objmgr_vdev *vdev,
		      enum wlan_link_recfg_sm_evt event,
		      void *event_data,
		      uint32_t frame_len)
{
	struct mlo_link_recfg_context *ctx;
	struct wlan_action_frame_args *action_frm;
	struct link_recfg_rx_rsp rx_rsp = {0};
	QDF_STATUS status;
	uint16_t ie_offset = 0;

	if (!vdev || !vdev->mlo_dev_ctx)
		return QDF_STATUS_E_NULL_VALUE;

	if (!vdev->mlo_dev_ctx->link_recfg_ctx)
		return QDF_STATUS_E_NULL_VALUE;

	if (!event_data || !frame_len) {
		mlo_err("event data or frame_len is null");
		return QDF_STATUS_E_NULL_VALUE;
	}

	action_frm = (struct wlan_action_frame_args *)event_data;
	ctx = vdev->mlo_dev_ctx->link_recfg_ctx;
	status = mlo_link_recfg_parse_action_rsp(ctx,
						 &ctx->curr_recfg_rsp,
						 event_data,
						 frame_len,
						 &ie_offset);

	if (QDF_IS_STATUS_ERROR(status))
		mlo_err("Link Recfg Response Parsing failed");
	else
		mlo_link_recfg_gen_link_assoc_rsp(vdev, ctx,
						  &ctx->curr_recfg_req,
						  action_frm,
						  frame_len,
						  &ie_offset);

	rx_rsp.status = status;
	status = mlo_link_recfg_sm_deliver_event(vdev->mlo_dev_ctx, event,
						 sizeof(struct link_recfg_rx_rsp),
						 &rx_rsp);

	if (QDF_IS_STATUS_ERROR(status))
		mlo_err("Posting Link Recfg Response event failed");

	/* In case of common link case, no-op in below API.
	 * In no-common link case, it will continue link assoc rsp process
	 * and peer assoc for the pending vdev.
	 */
	mlo_link_recfg_link_add_join_continue(vdev, status);

	return status;
}

static void
mlo_link_recfg_zero_and_free_memory(uint8_t *ptr, uint32_t len)
{
	if (!ptr)
		return;

	qdf_mem_zero(ptr, len);
	qdf_mem_free(ptr);
}

void
mlo_link_recfg_ctx_free_ies(struct mlo_link_recfg_context *ctx)
{
	if (!ctx)
		return;

	mlo_link_recfg_zero_and_free_memory(ctx->req_frame.ptr,
					    ctx->req_frame.len);
	ctx->req_frame.len = 0;
	ctx->req_frame.ptr = NULL;

	mlo_link_recfg_zero_and_free_memory(ctx->rsp_frame.ptr,
					    ctx->rsp_frame.len);
	ctx->rsp_frame.len = 0;
	ctx->rsp_frame.ptr = NULL;

	mlo_link_recfg_zero_and_free_memory(ctx->curr_recfg_rsp.grp_key_data.ptr,
					    ctx->curr_recfg_rsp.grp_key_data.len);
	ctx->curr_recfg_rsp.grp_key_data.len = 0;
	ctx->curr_recfg_rsp.grp_key_data.ptr = NULL;

	mlo_link_recfg_zero_and_free_memory(ctx->curr_recfg_rsp.oci_ie.ptr,
					    ctx->curr_recfg_rsp.oci_ie.len);
	ctx->curr_recfg_rsp.oci_ie.len = 0;
	ctx->curr_recfg_rsp.oci_ie.ptr = NULL;

	mlo_link_recfg_zero_and_free_memory(ctx->curr_recfg_rsp.mlo_ie.ptr,
					    ctx->curr_recfg_rsp.mlo_ie.len);
	ctx->curr_recfg_rsp.mlo_ie.len = 0;
	ctx->curr_recfg_rsp.mlo_ie.ptr = NULL;
}

QDF_STATUS
mlo_link_recfg_store_key(struct mlo_link_recfg_context *ctx,
			 struct mlo_link_recfg_state_req *req)
{
	struct wlan_mlo_dev_context *mlo_dev_ctx;
	uint8_t i;
	struct mlo_link_info *link_info;
	struct wlan_objmgr_vdev *vdev = NULL;
	struct wlan_mlo_link_recfg_bss_info *add_link;
	struct wlan_objmgr_psoc *psoc = NULL;
	QDF_STATUS status = QDF_STATUS_SUCCESS;

	if (!req || !ctx)
		return QDF_STATUS_E_NULL_VALUE;

	if (!req->add_link_info.num_links)
		return QDF_STATUS_SUCCESS;

	mlo_dev_ctx = mlo_link_recfg_get_mlo_ctx(ctx);
	if (!mlo_dev_ctx) {
		mlo_err("mlo_ctx null");
		return QDF_STATUS_E_INVAL;
	}

	psoc = mlo_link_recfg_get_psoc(ctx);
	if (!psoc) {
		mlo_err("psoc is null");
		return QDF_STATUS_E_INVAL;
	}

	vdev = wlan_objmgr_get_vdev_by_id_from_psoc(
				psoc, ctx->curr_recfg_req.vdev_id,
				WLAN_LINK_RECFG_ID);
	if (!vdev) {
		mlo_err("Invalid link recfg VDEV %d",
			ctx->curr_recfg_req.vdev_id);
		return QDF_STATUS_E_INVAL;
	}

	for (i = 0; i < req->add_link_info.num_links; i++) {
		add_link = &req->add_link_info.link[i];

		link_info = mlo_mgr_get_ap_link_info(vdev,
						     &add_link->ap_link_addr);
		if (!link_info) {
			mlo_debug("unexpected link info null for link id %d ap link mac " QDF_MAC_ADDR_FMT "",
				  add_link->link_id,
				  QDF_MAC_ADDR_REF(add_link->ap_link_addr.bytes));
			wlan_objmgr_vdev_release_ref(vdev, WLAN_LINK_RECFG_ID);
			return QDF_STATUS_E_INVAL;
		}

		status = mlo_link_recfg_save_unicast_key(ctx, vdev,
							 &link_info->link_addr,
							 &link_info->ap_link_addr,
							 link_info->link_id);
		if (QDF_IS_STATUS_ERROR(status))
			mlo_err("link unicast key save failed");
		else
			mlo_debug("unicast key saved for link id %d ap link mac " QDF_MAC_ADDR_FMT "",
				  link_info->link_id,
				  QDF_MAC_ADDR_REF(link_info->link_addr.bytes));
	}

	wlan_objmgr_vdev_release_ref(vdev, WLAN_LINK_RECFG_ID);
	return status;
}

QDF_STATUS
mlo_link_recfg_save_unicast_key(struct mlo_link_recfg_context *ctx,
				struct wlan_objmgr_vdev *vdev,
				struct qdf_mac_addr *link_addr,
				struct qdf_mac_addr *ap_link_addr,
				uint8_t link_id)
{
	struct wlan_objmgr_vdev *assoc_vdev = NULL;
	struct wlan_crypto_key *crypto_key;
	struct wlan_crypto_key *new_crypto_key;
	struct wlan_crypto_params *crypto_params;
	enum QDF_OPMODE op_mode;
	uint16_t i;
	uint8_t vdev_id, assoc_link_id;
	bool key_present = false;
	bool pairwise;
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	uint16_t max_key_index = WLAN_CRYPTO_MAXKEYIDX +
				 WLAN_CRYPTO_MAXIGTKKEYIDX +
				 WLAN_CRYPTO_MAXBIGTKKEYIDX;

	if (!ctx) {
		mlo_err("Link recfg ctx is null");
		return QDF_STATUS_E_NULL_VALUE;
	}

	if (!vdev) {
		mlo_err("Vdev is null");
		return QDF_STATUS_E_NULL_VALUE;
	}

	assoc_vdev = wlan_mlo_get_assoc_link_vdev(vdev);
	if (!assoc_vdev) {
		mlo_err("assoc Vdev is null");
		return QDF_STATUS_E_NULL_VALUE;
	}

	op_mode = wlan_vdev_mlme_get_opmode(assoc_vdev);
	if (op_mode != QDF_STA_MODE)
		return QDF_STATUS_E_FAILURE;

	crypto_params = wlan_crypto_vdev_get_crypto_params(assoc_vdev);
	if (!crypto_params) {
		mlo_err("crypto params is null");
		return QDF_STATUS_E_NULL_VALUE;
	}

	if (!crypto_params->ucastcipherset ||
	    QDF_HAS_PARAM(crypto_params->ucastcipherset, WLAN_CRYPTO_CIPHER_NONE))
		return QDF_STATUS_E_FAILURE;

	vdev_id = wlan_vdev_get_id(assoc_vdev);
	assoc_link_id = wlan_vdev_get_link_id(assoc_vdev);

	for (i = 0; i < max_key_index; i++) {
		wlan_crypto_aquire_lock();
		crypto_key = wlan_crypto_get_key(assoc_vdev, NULL, i);
		if (!crypto_key) {
			wlan_crypto_release_lock();
			continue;
		}

		wlan_crypto_release_lock();
		pairwise = crypto_key->key_type ? WLAN_CRYPTO_KEY_TYPE_UNICAST : WLAN_CRYPTO_KEY_TYPE_GROUP;
		if (pairwise) {
			mlo_debug("MLO: found unicast key for vdev_id %d link_id %d , key_idx %d",
				  vdev_id, assoc_link_id, i);

			wlan_crypto_aquire_lock();
			new_crypto_key = wlan_crypto_get_ml_sta_link_key(ctx->psoc, i,
									 link_addr,
									 link_id);
			if (!new_crypto_key) {
				wlan_crypto_release_lock();
				new_crypto_key = qdf_mem_malloc(sizeof(*new_crypto_key));
				if (!new_crypto_key) {
					mlo_err("malloc failed");
					status = QDF_STATUS_E_NOMEM;
					goto end;
				}
				key_present = true;
				wlan_crypto_aquire_lock();
			}

			qdf_mem_copy(new_crypto_key, crypto_key,
				     sizeof(*new_crypto_key));
			qdf_mem_copy(&new_crypto_key->macaddr,
				     ap_link_addr->bytes,
				     QDF_MAC_ADDR_SIZE);

			status = wlan_crypto_save_ml_sta_key(ctx->psoc, i,
							     new_crypto_key,
							     link_addr,
							     link_id);
			wlan_crypto_release_lock();
			if (QDF_IS_STATUS_ERROR(status)) {
				mlo_err("Failed to save key");
				qdf_mem_free(new_crypto_key);
				goto end;
			}
		}
	}

	if (!key_present) {
		mlo_err("No key found for link_id %d", assoc_link_id);
		status = QDF_STATUS_E_FAILURE;
	}
end:
	return status;
}

void
mlo_link_recfg_install_unicast_keys(struct wlan_objmgr_vdev *vdev)
{
	struct wlan_crypto_key *crypto_key;
	uint16_t i;
	bool pairwise;
	uint8_t vdev_id, link_id;
	bool key_present = false;
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	uint16_t max_key_index = WLAN_CRYPTO_MAXKEYIDX +
				 WLAN_CRYPTO_MAXIGTKKEYIDX +
				 WLAN_CRYPTO_MAXBIGTKKEYIDX;

	vdev_id = wlan_vdev_get_id(vdev);
	link_id = wlan_vdev_get_link_id(vdev);

	wlan_crypto_aquire_lock();
	for (i = 0; i < max_key_index; i++) {
		crypto_key = wlan_crypto_get_key(vdev, NULL, i);
		if (!crypto_key) {
			mlo_err("crypto key not found");
			continue;
		}
		pairwise = crypto_key->key_type ? WLAN_CRYPTO_KEY_TYPE_UNICAST : WLAN_CRYPTO_KEY_TYPE_GROUP;
		if (pairwise) {
			key_present = true;
			status = mlme_cm_osif_send_keys(vdev, i, pairwise,
							crypto_key->cipher_type);
			if (QDF_IS_STATUS_ERROR(status)) {
				mlo_err("MLO: fail to send key for vdev_id %d link_id %d, key_idx %d, pairwise %d",
					vdev_id, link_id, i, pairwise);
				goto err;
			} else {
				mlo_debug("MLO: send keys for vdev_id %d link_id %d, key_idx %d, pairwise %d",
					  vdev_id, link_id, i, pairwise);
			}
			break;
		}
	}
err:
	wlan_crypto_release_lock();
	if (!key_present) {
		mlme_err("No unicast key found for link_id %d", link_id);
		mlo_disconnect(vdev, CM_OSIF_DISCONNECT,
			       REASON_KEY_FAIL_TO_INSTALL, NULL);
	}
}
