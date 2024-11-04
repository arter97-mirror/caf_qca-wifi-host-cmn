/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
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
#ifdef WLAN_FEATURE_11BE_MLO_ADV_FEATURE
#include "wlan_cm_roam_api.h"
#include <wlan_mlo_mgr_roam.h>
#include "wlan_dlm_api.h"
#include "wlan_dp_ucfg_api.h"
#include "target_if_mlo_mgr.h"
#endif
#include "host_diag_core_event.h"

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

	if (!vdev || !vdev->mlo_dev_ctx)
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
	recfg_req = &recfg_ctx->last_recfg_req;
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
	qdf_mem_copy(&recfg_ctx->last_recfg_req, recfg_req,
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
		wlan_objmgr_vdev_release_ref(vdev, WLAN_LINK_RECFG_ID);
		return QDF_STATUS_E_INVAL;
	}

	recfg_req.vdev_id = evt_params->vdev_id;
	recfg_req.is_user_req = evt_params->trigger_reason ==
				ROAM_TRIGGER_REASON_FORCED;
	recfg_req.is_fw_ind_received = true;
	recfg_req.add_link_info = evt_params->add_link;
	recfg_req.del_link_info = evt_params->del_link;
	recfg_req.fw_ind_param = *evt_params;
	status = mlo_link_recfg_sm_deliver_event(
				mlo_dev_ctx,
				WLAN_LINK_RECFG_SM_EV_FW_IND,
				sizeof(recfg_req), &recfg_req);

	wlan_objmgr_vdev_release_ref(vdev, WLAN_LINK_RECFG_ID);

	return QDF_STATUS_SUCCESS;
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

static void
mlo_link_recfg_get_link_bitmap(struct mlo_link_recfg_context *recfg_ctx,
			       struct wlan_mlo_link_recfg_req *recfg_req,
			       uint32_t *add_link_set,
			       uint8_t *add_link_num,
			       uint32_t *del_link_set,
			       uint8_t *del_link_num,
			       uint32_t *curr_link_set,
			       uint8_t *curr_link_num)
{
}

static bool
mlo_link_recfg_assign_idle_vdev_for_add_link(
				struct mlo_link_recfg_context *recfg_ctx,
				struct mlo_link_recfg_state_req *req)
{
	return true;
}

static bool
mlo_link_recfg_assign_active_vdev_for_add_link(
				struct mlo_link_recfg_context *recfg_ctx,
				struct mlo_link_recfg_state_req *req)
{
	return true;
}

static bool
mlo_link_recfg_is_link_add_back_on_active_vdev(
				     struct mlo_link_recfg_context *recfg_ctx,
				     struct mlo_link_recfg_state_req *req)
{
	return true;
}

static bool
mlo_link_recfg_is_standby_link_del_only(
				struct mlo_link_recfg_context *recfg_ctx,
				struct mlo_link_recfg_state_req *req)
{
	return true;
}

static bool
mlo_link_recfg_is_standby_link_present_for_link_sw(
				struct mlo_link_recfg_context *recfg_ctx)
{
	return true;
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
mlo_link_recfg_del_standby_link(struct mlo_link_recfg_context *recfg_ctx,
				struct mlo_link_recfg_state_req *req)
{
	/* Send bss params wmi to del standby link */
	/* remove link info from mlo mgr */
}

static void
mlo_link_recfg_add_standby_link(struct mlo_link_recfg_context *recfg_ctx,
				struct mlo_link_recfg_state_req *req)
{
	/* Send bss params wmi to add standby link */
	/* add link info to mlo mgr */
}

static QDF_STATUS
mlo_link_recfg_add_link_connect(struct mlo_link_recfg_context *recfg_ctx,
				struct mlo_link_recfg_state_req *req)
{
	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS
mlo_link_recfg_host_trigger_link_switch(
			struct mlo_link_recfg_context *recfg_ctx,
			struct mlo_link_recfg_state_req *req)
{
	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS
mlo_link_recfg_del_link_by_inact(
		struct mlo_link_recfg_context *recfg_ctx,
		struct mlo_link_recfg_state_req *req)
{
	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS
mlo_link_recfg_send_request_frame(
		struct mlo_link_recfg_context *recfg_ctx,
		struct mlo_link_recfg_state_req *req)
{
	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS
mlo_link_recfg_assign_self_link_addr_for_link_add(
			struct wlan_mlo_link_recfg_req *recfg_req)
{
	/* Assign self link address for added link */

	return QDF_STATUS_SUCCESS;
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
	uint32_t curr_link_set, add_link_set, del_link_set;
	uint8_t curr_link_num, add_link_num, del_link_num;
	struct mlo_link_recfg_state_tran *next = &recfg_ctx->sm.state_list[0];
	QDF_STATUS status;

	mlo_link_recfg_get_link_bitmap(recfg_ctx,
				       recfg_req,
				       &add_link_set,
				       &add_link_num,
				       &del_link_set,
				       &del_link_num,
				       &curr_link_set,
				       &curr_link_num);

	/* alloc self mac for link add */
	mlo_link_recfg_assign_self_link_addr_for_link_add(recfg_req);

	/* create transition flow */
	recfg_ctx->sm.curr_state_idx = -1;
	if (recfg_req->add_link_info.num_links &&
	    !recfg_req->del_link_info.num_links) {
		/* Add link only */
		next->state = WLAN_LINK_RECFG_S_XMIT_REQ;
		next->event = WLAN_LINK_RECFG_SM_EV_XMIT_REQ;
		next->req.add_link_info = recfg_req->add_link_info;
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
		next->state = WLAN_LINK_RECFG_S_DEL_LINK;
		next->event = WLAN_LINK_RECFG_SM_EV_DEL_LINK;
		next->req.del_link_info = recfg_req->del_link_info;
		next->abort_handler = NULL;
		next++;
		next->state = WLAN_LINK_RECFG_S_XMIT_REQ;
		next->event = WLAN_LINK_RECFG_SM_EV_XMIT_REQ;
		next->req.del_link_info = recfg_req->del_link_info;
		next->abort_handler = NULL;
		next++;
		next->state = WLAN_LINK_RECFG_S_COMPLETED;
		next->event = WLAN_LINK_RECFG_SM_EV_COMPLETED;
		next->req.del_link_info = recfg_req->del_link_info;
		next->abort_handler = NULL;
	} else if ((curr_link_set & ~del_link_set) && add_link_set) {
		/* Add and Del link with common link */
		next->state = WLAN_LINK_RECFG_S_DEL_LINK;
		next->event = WLAN_LINK_RECFG_SM_EV_DEL_LINK;
		next->req.del_link_info = recfg_req->del_link_info;
		next->abort_handler = NULL;
		next++;
		next->state = WLAN_LINK_RECFG_S_XMIT_REQ;
		next->event = WLAN_LINK_RECFG_SM_EV_XMIT_REQ;
		next->req.del_link_info = recfg_req->del_link_info;
		next->req.add_link_info = recfg_req->add_link_info;
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
		next->state = WLAN_LINK_RECFG_S_DEL_LINK;
		next->event = WLAN_LINK_RECFG_SM_EV_DEL_LINK;
		/* todo: select one of del_link_info to del first */
		next->req.del_link_info = recfg_req->del_link_info;
		next->abort_handler = NULL;
		next++;
		next->state = WLAN_LINK_RECFG_S_ADD_LINK;
		next->event = WLAN_LINK_RECFG_SM_EV_ADD_LINK;
		next->req.add_link_info = recfg_req->add_link_info;
		next->abort_handler = NULL;
		next++;
		if (del_link_num > 1) {
			next->state = WLAN_LINK_RECFG_S_DEL_LINK;
			next->event = WLAN_LINK_RECFG_SM_EV_DEL_LINK;
			// the others of del_link_set.
			next->req.del_link_info = recfg_req->del_link_info;
			next->abort_handler = NULL;
			next++;
		}
		next->state = WLAN_LINK_RECFG_S_COMPLETED;
		next->event = WLAN_LINK_RECFG_SM_EV_COMPLETED;
		next->req.del_link_info = recfg_req->del_link_info;
		next->req.add_link_info = recfg_req->add_link_info;
		next->abort_handler = NULL;
	} else {
		/* not supported */
		return QDF_STATUS_E_INVAL;
	}

	status = mlo_link_recfg_tranistion_to_next_state(recfg_ctx);
	if (QDF_IS_STATUS_ERROR(status))
		mlo_err("start trans failed status %d", status);

	return status;
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

	/* if deleted link is standby , remove link info from mlo mgr
	 * L1 L2 L3, del L2, link switch to L3. remove standby link 2
	 */

	/* transition to next state */
	mlo_link_recfg_tranistion_to_next_state(recfg_ctx);
}

static void
mlo_link_recfg_response_received(struct mlo_link_recfg_context *recfg_ctx,
				 struct recfg_rsp *recfg_resp_data)
{
	/* handle link recfg response frame */

	/* notify kernel/supplicant for del only req/rsp
	 *	osif_notify_link_reconfig();
	 */

	/* handle link recfg link add rejected case */

	mlo_link_recfg_tranistion_to_next_state(recfg_ctx);
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
	QDF_STATUS status;

	recfg_req = &recfg_ctx->last_recfg_req;

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
		if (QDF_IS_STATUS_ERROR(status)) {
			mlo_err("send_mlo_link_recfg_complete_cmd failed %d",
				status);
		}
	}

	/* reset state tran index and move to init state  */
	recfg_ctx->sm.curr_state_idx = -1;

	mlo_link_recfg_sm_transition_to(recfg_ctx, WLAN_LINK_RECFG_S_INIT);

	/* remove reconfig ser command */
	mlo_remove_link_recfg_cmd(recfg_ctx);
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
		mlme_err("invalid state %d", state);
}

static void
mlo_link_recfg_sm_set_substate(struct mlo_link_recfg_context *recfg_ctx,
			       enum wlan_link_recfg_sm_state substate)
{
	if (substate > WLAN_LINK_RECFG_S_MAX &&
	    substate < WLAN_LINK_RECFG_SS_MAX)
		recfg_ctx->sm.link_recfg_substate = substate;
	else
		mlme_err("invalid state %d", substate);
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
			recfg_ctx->last_recfg_req.vdev_id);
		goto abort;
	}

	switch (substate) {
	case WLAN_LINK_RECFG_SS_START_PENDING:
	case WLAN_LINK_RECFG_SS_START_ACTIVE:
		break;
	case WLAN_LINK_RECFG_SS_DEL_LINK_WAIT_SET_LINK:
	case WLAN_LINK_RECFG_SS_DEL_LINK_ABORT_WAIT_SET_LINK:
		/* timeout set link req */

		break;
	case WLAN_LINK_RECFG_SS_DEL_LINK_WAIT_LINK_SW:
	case WLAN_LINK_RECFG_SS_DEL_LINK_ABORT_WAIT_LINK_SW:
		/* timeout link switch req */

		break;
	case WLAN_LINK_RECFG_SS_ADD_LINK_WAIT_ADD_CONN:
	case WLAN_LINK_RECFG_SS_ADD_LINK_ABORT_WAIT_ADD_CONN:
		/* add partner link timeout */

		break;
	case WLAN_LINK_RECFG_SS_ADD_LINK_WAIT_LINK_SW:
	case WLAN_LINK_RECFG_SS_ADD_LINK_ABORT_WAIT_LINK_SW:
		/* timeout link switch req */

		break;
	default:
		mlo_err("unexpected substate %d when ser timeout vdev %d",
			state,
			recfg_ctx->last_recfg_req.vdev_id);
		break;
	}

abort:
	mlo_link_recfg_sm_transition_to(recfg_ctx, WLAN_LINK_RECFG_S_ABORT);
	mlo_link_recfg_sm_deliver_event_sync(
			recfg_ctx->ml_dev, WLAN_LINK_RECFG_SM_EV_COMPLETED,
			0, NULL);
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
		recfg_req = &recfg_ctx->last_recfg_req;
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
				&recfg_ctx->last_recfg_req);
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
		recfg_req = &recfg_ctx->last_recfg_req;
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
		 * as user requested in recfg_ctx->last_recfg_req,
		 * then start link recfg
		 */
		recfg_req = &recfg_ctx->last_recfg_req;
		recfg_req->is_fw_ind_received = true;
		recfg_req->add_link_info = fw_ind_recfg_req->add_link_info;
		recfg_req->del_link_info = fw_ind_recfg_req->del_link_info;
		recfg_req->fw_ind_param = fw_ind_recfg_req->fw_ind_param;
		status = mlo_link_recfg_create_transition_list(
					recfg_ctx,
					&recfg_ctx->last_recfg_req);
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
			/* ABC -> AB: delete sandby link C */
			mlo_link_recfg_del_standby_link(recfg_ctx, req);
			mlo_link_recfg_del_link_completed(recfg_ctx);
		} else {
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

		if (!mlo_link_recfg_is_standby_link_present_for_link_sw(
							recfg_ctx)) {
			/* AB -> A: B is set inactive,
			 * no link switch event.
			 */
			mlo_link_recfg_del_link_completed(recfg_ctx);
		} else {
			/* ABC -> AC: B is set inactive, link switch is
			 * expected. fw should link switch to C.
			 */
			mlo_link_recfg_sm_transition_to(
				ctx, WLAN_LINK_RECFG_SS_DEL_LINK_WAIT_LINK_SW);
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

	switch (event) {
	case WLAN_LINK_RECFG_SM_EV_SET_LINK_RSP:
		/* start timer for fw link reconfig indication event?
		 * or using WLAN_LINK_RECFG_SM_EV_SER_TIMEOUT to abort link sw.
		 */
		break;
	case WLAN_LINK_RECFG_SM_EV_LINK_SWITCH_IND:
		// cancel fw link reconfig indication timer.
		break;
	case WLAN_LINK_RECFG_SM_EV_LINK_SWITCH_RSP:
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

	switch (event) {
	case WLAN_LINK_RECFG_SM_EV_LINK_SWITCH_RSP:
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

	switch (event) {
	case WLAN_LINK_RECFG_SM_EV_ADD_LINK:
		req = (struct mlo_link_recfg_state_req *)event_data;
		if (mlo_link_recfg_assign_idle_vdev_for_add_link(
					recfg_ctx, req)) {
			/* A-> AB : use idle vdev to connect new add link */
			mlo_link_recfg_sm_transition_to(
			ctx, WLAN_LINK_RECFG_SS_ADD_LINK_WAIT_ADD_CONN);
			mlo_link_recfg_sm_deliver_event_sync(
				recfg_ctx->ml_dev, event,
				event_data_len, event_data);
		} else if (mlo_link_recfg_assign_active_vdev_for_add_link(
					recfg_ctx, req)) {
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
				event_data_len, event_data);
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

	switch (event) {
	case WLAN_LINK_RECFG_SM_EV_ADD_LINK:
		req = (struct mlo_link_recfg_state_req *)event_data;
		mlo_link_recfg_add_link_connect(recfg_ctx, req);
		break;
	case WLAN_LINK_RECFG_SM_EV_ADD_CONN_RSP:
		mlo_link_recfg_add_link_completed(recfg_ctx);
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
	struct mlo_link_recfg_state_req *req;
	bool event_handled = true;

	switch (event) {
	case WLAN_LINK_RECFG_SM_EV_ADD_LINK:
		req = (struct mlo_link_recfg_state_req *)event_data;
		if (mlo_link_recfg_is_link_add_back_on_active_vdev(
						recfg_ctx, req)) {
			/* AB->AB(B deleted)->AB*/
			mlo_link_recfg_host_trigger_link_switch(
						recfg_ctx, req);
		} else {
			/* AB->AB(B deleted)->AC*/
			/* add C as standby link,
			 * fw should indicate link switch event
			 */
			mlo_link_recfg_add_standby_link(recfg_ctx, req);
		}
		/* start timer for fw link reconfig indication event? or
		 * using serialization timeout to abort the state.
		 */
		break;
	case WLAN_LINK_RECFG_SM_EV_LINK_SWITCH_IND:
		// cancel fw link reconfig indication timer.
		break;
	case WLAN_LINK_RECFG_SM_EV_LINK_SWITCH_RSP:
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

	switch (event) {
	case WLAN_LINK_RECFG_SM_EV_XMIT_REQ:
		req = (struct mlo_link_recfg_state_req *)event_data;
		mlo_link_recfg_send_request_frame(recfg_ctx, req);
		break;
	case WLAN_LINK_RECFG_SM_EV_XMIT_STATUS:
		/* Handle tx failure		*/
		break;
	case WLAN_LINK_RECFG_SM_EV_RX_RSP:
		mlo_link_recfg_response_received(
			recfg_ctx, (struct recfg_rsp *)event_data);
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
