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
#endif
#include "host_diag_core_event.h"

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

QDF_STATUS mlo_link_recfg_init(struct wlan_objmgr_psoc *psoc,
			       struct wlan_mlo_dev_context *ml_dev)
{
	return QDF_STATUS_SUCCESS;
}

QDF_STATUS mlo_link_recfg_deinit(struct wlan_mlo_dev_context *ml_dev)
{
	return QDF_STATUS_SUCCESS;
}

void mlo_remove_link_recfg_cmd(struct wlan_objmgr_vdev *vdev)
{
}

QDF_STATUS mlo_ser_link_recfg_cmd(struct wlan_objmgr_vdev *vdev,
				  struct wlan_mlo_link_recfg_req *req)
{
	return QDF_STATUS_SUCCESS;
}

QDF_STATUS mlo_link_recfg_notify(struct wlan_objmgr_vdev *vdev,
				 struct wlan_mlo_link_recfg_req *req)
{
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

QDF_STATUS mlo_link_recfg_complete(struct wlan_objmgr_vdev *vdev)
{
	return QDF_STATUS_SUCCESS;
}
