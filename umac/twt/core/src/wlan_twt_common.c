/*
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
 *  DOC: wlan_twt_common.c
 */
#include "include/wlan_mlme_cmn.h"
#include "wlan_twt_common.h"
#include "wlan_twt_priv.h"
#include <wlan_twt_public_structs.h>
#include <wlan_objmgr_peer_obj.h>
#include <wlan_twt_tgt_if_tx_api.h>
#include "twt/core/src/wlan_twt_cfg.h"
#include "wlan_policy_mgr_public_struct.h"
#include "wlan_policy_mgr_api.h"

#define TWT_NUM_BIT 1
#define TWT_ALL_MACS_ID 255
#define MAX_MAC 2

QDF_STATUS
wlan_twt_tgt_caps_get_responder(struct wlan_objmgr_psoc *psoc, bool *val)
{
	struct twt_psoc_priv_obj *twt_psoc;

	twt_psoc = wlan_objmgr_psoc_get_comp_private_obj(psoc,
							 WLAN_UMAC_COMP_TWT);
	if (!twt_psoc) {
		twt_err("null twt psoc priv obj");
		return QDF_STATUS_E_FAILURE;
	}

	*val = twt_psoc->twt_caps.twt_responder;
	return QDF_STATUS_SUCCESS;
}

QDF_STATUS
wlan_twt_tgt_caps_get_nudge_enabled(struct wlan_objmgr_psoc *psoc,
				    bool *val)
{
	struct twt_psoc_priv_obj *twt_psoc;

	twt_psoc = wlan_objmgr_psoc_get_comp_private_obj(psoc,
							 WLAN_UMAC_COMP_TWT);
	if (!twt_psoc) {
		twt_err("null twt psoc priv obj");
		return QDF_STATUS_E_FAILURE;
	}

	*val = twt_psoc->twt_caps.twt_nudge_enabled;
	return QDF_STATUS_SUCCESS;
}

QDF_STATUS
wlan_twt_tgt_caps_get_all_twt_enabled(struct wlan_objmgr_psoc *psoc,
				      bool *val)
{
	struct twt_psoc_priv_obj *twt_psoc;

	twt_psoc = wlan_objmgr_psoc_get_comp_private_obj(psoc,
							 WLAN_UMAC_COMP_TWT);
	if (!twt_psoc) {
		twt_err("null twt psoc priv obj");
		return QDF_STATUS_E_FAILURE;
	}

	*val = twt_psoc->twt_caps.all_twt_enabled;
	return QDF_STATUS_SUCCESS;
}

QDF_STATUS
wlan_twt_tgt_caps_get_stats_enabled(struct wlan_objmgr_psoc *psoc,
					bool *val)
{
	struct twt_psoc_priv_obj *twt_psoc;

	twt_psoc = wlan_objmgr_psoc_get_comp_private_obj(psoc,
							 WLAN_UMAC_COMP_TWT);
	if (!twt_psoc) {
		twt_err("null twt psoc priv obj");
		return QDF_STATUS_E_FAILURE;
	}

	*val = twt_psoc->twt_caps.twt_stats_enabled;
	return QDF_STATUS_SUCCESS;
}

QDF_STATUS wlan_twt_check_all_twt_support(struct wlan_objmgr_psoc *psoc,
					  uint32_t dialog_id)
{
	bool is_all_twt_enabled = false;
	QDF_STATUS status;

	/* Cap check is check NOT required if id is for a single session */
	if (dialog_id != TWT_ALL_SESSIONS_DIALOG_ID)
		return QDF_STATUS_SUCCESS;

	status = wlan_twt_tgt_caps_get_all_twt_enabled(psoc,
						       &is_all_twt_enabled);
	if (QDF_IS_STATUS_ERROR(status))
		return QDF_STATUS_E_INVAL;

	if (!is_all_twt_enabled)
		return QDF_STATUS_E_NOSUPPORT;

	return QDF_STATUS_SUCCESS;
}

QDF_STATUS
wlan_twt_tgt_caps_get_ack_supported(struct wlan_objmgr_psoc *psoc,
				    bool *val)
{
	struct twt_psoc_priv_obj *twt_psoc;

	twt_psoc = wlan_objmgr_psoc_get_comp_private_obj(psoc,
							 WLAN_UMAC_COMP_TWT);
	if (!twt_psoc) {
		twt_err("null twt psoc priv obj");
		*val = false;
		return QDF_STATUS_E_FAILURE;
	}

	*val = twt_psoc->twt_caps.twt_ack_supported;
	return QDF_STATUS_SUCCESS;
}

QDF_STATUS
wlan_twt_tgt_caps_get_restricted_support(struct wlan_objmgr_psoc *psoc,
					 bool *val)
{
	struct twt_psoc_priv_obj *twt_psoc;

	if (!psoc) {
		twt_err("null psoc");
		return QDF_STATUS_E_FAILURE;
	}

	twt_psoc = wlan_objmgr_psoc_get_comp_private_obj(psoc,
							 WLAN_UMAC_COMP_TWT);
	if (!twt_psoc) {
		twt_err("null twt psoc priv obj");
		return QDF_STATUS_E_FAILURE;
	}

	*val = twt_psoc->twt_caps.restricted_twt_support;
	return QDF_STATUS_SUCCESS;
}

QDF_STATUS
wlan_twt_requestor_disable(struct wlan_objmgr_psoc *psoc,
			   struct twt_disable_param *req,
			   void *context)
{
	struct twt_psoc_priv_obj *twt_psoc;
	bool twt_req_en_dis_vdev_support = false;

	twt_psoc = wlan_objmgr_psoc_get_comp_private_obj(psoc,
							 WLAN_UMAC_COMP_TWT);
	if (!twt_psoc) {
		twt_err("null twt psoc priv obj");
		return QDF_STATUS_E_FAILURE;
	}

	twt_psoc->disable_context.twt_role = TWT_ROLE_REQUESTOR;
	twt_psoc->disable_context.context = context;

	req->twt_role = TWT_ROLE_REQUESTOR;

	/*
	 * If FW supports per-vdev TWT en/dis for requestor role and a valid
	 * vdev_id was provided, keep it so the WMI layer can use vdev_id and
	 * set the VDEV_SUPPORT flag. Otherwise reset vdev_id to
	 * WLAN_INVALID_VDEV_ID to fall back to MAC-level TWT using pdev_id.
	 */
	wlan_twt_tgt_caps_get_req_en_dis_vdev_support(
						psoc,
						&twt_req_en_dis_vdev_support);
	if (!twt_req_en_dis_vdev_support)
		req->vdev_id = WLAN_INVALID_VDEV_ID;

	twt_debug("TWT req disable: pdev_id:%d vdev_id:%d role:%d ext:%d reason_code:%d vdev_support:%d",
		  req->pdev_id, req->vdev_id, req->twt_role,
		  req->ext_conf_present, req->dis_reason_code,
		  twt_req_en_dis_vdev_support);

	return tgt_twt_disable_req_send(psoc, req);
}

QDF_STATUS
wlan_twt_responder_disable(struct wlan_objmgr_psoc *psoc,
			   struct twt_disable_param *req,
			   void *context)
{
	struct twt_psoc_priv_obj *twt_psoc;

	twt_psoc = wlan_objmgr_psoc_get_comp_private_obj(psoc,
							 WLAN_UMAC_COMP_TWT);
	if (!twt_psoc) {
		twt_err("null twt psoc priv obj");
		return QDF_STATUS_E_FAILURE;
	}

	twt_psoc->disable_context.twt_role = TWT_ROLE_RESPONDER;
	twt_psoc->disable_context.context = context;

	req->twt_role = TWT_ROLE_RESPONDER;

	twt_debug("TWT res disable: mac_id:%d role:%d ext:%d reason_code:%d",
		  req->pdev_id, req->twt_role, req->ext_conf_present,
		  req->dis_reason_code);

	return tgt_twt_disable_req_send(psoc, req);
}

/**
 * wlan_twt_req_en_set_vdev_cong_timeout() - Set vdev_id and congestion
 * timeout for TWT requestor enable based on FW capability.
 * @psoc: Pointer to global psoc
 * @req: Pointer to TWT enable request parameters
 *
 * If FW supports per-vdev TWT en/dis for requestor role and a valid vdev_id
 * was provided, keep it so the WMI layer can use vdev_id and set the
 * VDEV_SUPPORT flag, and fetch the per-vdev congestion timeout.
 * Otherwise reset vdev_id to WLAN_INVALID_VDEV_ID to fall back to MAC-level
 * TWT using pdev_id, and fetch the per-MAC congestion timeout.
 */
static void
wlan_twt_req_en_set_vdev_cong_timeout(struct wlan_objmgr_psoc *psoc,
				      struct twt_enable_param *req)
{
	bool twt_req_en_dis_vdev_support = false;

	wlan_twt_tgt_caps_get_req_en_dis_vdev_support(
					psoc,
					&twt_req_en_dis_vdev_support);
	if (!twt_req_en_dis_vdev_support)
		req->vdev_id = WLAN_INVALID_VDEV_ID;

	if (twt_req_en_dis_vdev_support &&
	    req->vdev_id != WLAN_INVALID_VDEV_ID)
		wlan_twt_cfg_get_vdev_congestion_timeout(
						psoc, req->vdev_id,
						&req->sta_cong_timer_ms);
	else
		wlan_twt_cfg_get_congestion_timeout_per_mac(
						psoc, req->pdev_id,
						&req->sta_cong_timer_ms);
}

QDF_STATUS
wlan_twt_requestor_enable(struct wlan_objmgr_psoc *psoc,
			  struct twt_enable_param *req,
			  void *context)
{
	struct twt_psoc_priv_obj *twt_psoc;
	bool requestor_en = false, twt_bcast_requestor = false;
	bool rtwt_requestor = false, restricted_support = false;

	twt_psoc = wlan_objmgr_psoc_get_comp_private_obj(psoc,
							 WLAN_UMAC_COMP_TWT);
	if (!twt_psoc) {
		twt_err("null twt psoc priv obj");
		return QDF_STATUS_E_FAILURE;
	}

	wlan_twt_cfg_get_requestor(psoc, &requestor_en);
	if (!requestor_en) {
		twt_warn("twt requestor ini is not enabled");
		return QDF_STATUS_E_FAILURE;
	}

	twt_psoc->enable_context.twt_role = TWT_ROLE_REQUESTOR;
	twt_psoc->enable_context.context = context;

	wlan_twt_req_en_set_vdev_cong_timeout(psoc, req);

	wlan_twt_cfg_get_bcast_requestor(psoc, &twt_bcast_requestor);
	req->b_twt_enable = twt_bcast_requestor;
	req->twt_role = TWT_ROLE_REQUESTOR;
	if (twt_bcast_requestor)
		req->twt_oper = TWT_OPERATION_BROADCAST;
	else
		req->twt_oper = TWT_OPERATION_INDIVIDUAL;

	wlan_twt_cfg_get_rtwt_requestor(psoc, &rtwt_requestor);
	wlan_twt_tgt_caps_get_restricted_support(psoc, &restricted_support);

	req->r_twt_enable = QDF_MIN(restricted_support, rtwt_requestor);

	twt_debug("TWT req enable: pdev_id:%d vdev_id:%d cong:%d bcast:%d rtwt:%d",
		  req->pdev_id, req->vdev_id, req->sta_cong_timer_ms,
		  req->b_twt_enable, req->r_twt_enable);
	twt_debug("TWT req enable: role:%d ext:%d oper:%d",
		  req->twt_role, req->ext_conf_present, req->twt_oper);

	return tgt_twt_enable_req_send(psoc, req);
}

QDF_STATUS
wlan_twt_responder_enable(struct wlan_objmgr_psoc *psoc,
			  struct twt_enable_param *req,
			  void *context)
{
	struct twt_psoc_priv_obj *twt_psoc;
	uint8_t responder_en;
	bool twt_bcast_responder = false;

	twt_psoc = wlan_objmgr_psoc_get_comp_private_obj(psoc,
							 WLAN_UMAC_COMP_TWT);
	if (!twt_psoc) {
		twt_err("null twt psoc priv obj");
		return QDF_STATUS_E_FAILURE;
	}

	wlan_twt_cfg_get_responder(psoc, &responder_en);
	if (!responder_en) {
		twt_warn("twt responder ini is not enabled");
		return QDF_STATUS_E_FAILURE;
	}

	twt_psoc->enable_context.twt_role = TWT_ROLE_RESPONDER;
	twt_psoc->enable_context.context = context;

	wlan_twt_cfg_get_bcast_responder(psoc, &twt_bcast_responder);
	req->b_twt_enable = twt_bcast_responder;
	req->twt_role = TWT_ROLE_RESPONDER;
	if (twt_bcast_responder)
		req->twt_oper = TWT_OPERATION_BROADCAST;
	else
		req->twt_oper = TWT_OPERATION_INDIVIDUAL;

	twt_debug("TWT res enable: mac_id:%d bcast:%d",
		  req->pdev_id, req->b_twt_enable);
	twt_debug("TWT res enable: role:%d ext:%d oper:%d",
		  req->twt_role, req->ext_conf_present, req->twt_oper);

	return tgt_twt_enable_req_send(psoc, req);
}

QDF_STATUS
wlan_twt_set_peer_capabilities(struct wlan_objmgr_psoc *psoc,
			       struct qdf_mac_addr *peer_mac,
			       uint8_t peer_cap)
{
	struct twt_peer_priv_obj *peer_priv;
	struct wlan_objmgr_peer *peer;

	peer = wlan_objmgr_get_peer_by_mac(psoc, peer_mac->bytes,
					   WLAN_TWT_ID);
	if (!peer) {
		twt_debug("Peer object not found " QDF_MAC_ADDR_FMT,
			  QDF_MAC_ADDR_REF(peer_mac->bytes));
		return QDF_STATUS_E_FAILURE;
	}

	peer_priv = wlan_objmgr_peer_get_comp_private_obj(peer,
							  WLAN_UMAC_COMP_TWT);
	if (!peer_priv) {
		wlan_objmgr_peer_release_ref(peer, WLAN_TWT_ID);
		twt_err("peer twt component object is NULL");
		return QDF_STATUS_E_FAILURE;
	}

	twt_lock_acquire(&peer_priv->twt_peer_lock);
	peer_priv->peer_capability = peer_cap;
	twt_lock_release(&peer_priv->twt_peer_lock);

	twt_debug("set peer cap: 0x%x", peer_cap);
	wlan_objmgr_peer_release_ref(peer, WLAN_TWT_ID);
	return QDF_STATUS_SUCCESS;
}

QDF_STATUS
wlan_twt_get_peer_capabilities(struct wlan_objmgr_psoc *psoc,
			       struct qdf_mac_addr *peer_mac,
			       uint8_t *peer_cap)
{
	struct twt_peer_priv_obj *peer_priv;
	struct wlan_objmgr_peer *peer;

	peer = wlan_objmgr_get_peer_by_mac(psoc, peer_mac->bytes,
					   WLAN_TWT_ID);
	if (!peer) {
		twt_err("Peer object not found "QDF_MAC_ADDR_FMT,
			QDF_MAC_ADDR_REF(peer_mac->bytes));
		*peer_cap = 0;
		return QDF_STATUS_E_FAILURE;
	}

	peer_priv = wlan_objmgr_peer_get_comp_private_obj(peer,
							  WLAN_UMAC_COMP_TWT);
	if (!peer_priv) {
		wlan_objmgr_peer_release_ref(peer, WLAN_TWT_ID);
		*peer_cap = 0;
		twt_err("peer twt component object is NULL");
		return QDF_STATUS_E_FAILURE;
	}

	twt_lock_acquire(&peer_priv->twt_peer_lock);
	*peer_cap = peer_priv->peer_capability;
	twt_lock_release(&peer_priv->twt_peer_lock);

	twt_debug("get peer cap: 0x%x", *peer_cap);
	wlan_objmgr_peer_release_ref(peer, WLAN_TWT_ID);
	return QDF_STATUS_SUCCESS;
}

QDF_STATUS
wlan_twt_cfg_get_mac_responder_flag(struct wlan_objmgr_psoc *psoc,
				    uint8_t mac_id, bool *val)
{
	struct twt_psoc_priv_obj *twt_psoc_obj;
	uint8_t i;

	twt_psoc_obj = wlan_objmgr_psoc_get_comp_private_obj(
							psoc,
							WLAN_UMAC_COMP_TWT);
	if (!twt_psoc_obj) {
		twt_err("twt psoc priv obj is null");
		*val = false;
		return QDF_STATUS_E_NULL_VALUE;
	}

	/* Handle mac_id 255 which represents all MACs */
	if (mac_id == TWT_ALL_MACS_ID) {
		*val = true;
		for (i = 0; i < MAX_MAC; i++) {
			if (!QDF_GET_BITS(twt_psoc_obj->twt_resp_flag, i,
					  TWT_NUM_BIT)) {
				*val = false;
				break;
			}
		}
		return QDF_STATUS_SUCCESS;
	}

	if (mac_id >= MAX_MAC) {
		twt_err("mac id %d greater than or equal to MAX value", mac_id);
		return QDF_STATUS_E_FAILURE;
	}

	*val = QDF_GET_BITS(twt_psoc_obj->twt_resp_flag, mac_id, TWT_NUM_BIT);
	return QDF_STATUS_SUCCESS;
}

QDF_STATUS
wlan_twt_cfg_set_mac_responder_flag(struct wlan_objmgr_psoc *psoc,
				    uint8_t mac_id, bool val)
{
	struct twt_psoc_priv_obj *twt_psoc_obj;
	uint8_t i;

	twt_psoc_obj = wlan_objmgr_psoc_get_comp_private_obj(
							psoc,
							WLAN_UMAC_COMP_TWT);
	if (!twt_psoc_obj) {
		twt_err("twt psoc priv obj is null");
		return QDF_STATUS_E_NULL_VALUE;
	}

	/* Handle mac_id 255 which represents all MACs */
	if (mac_id == TWT_ALL_MACS_ID) {
		for (i = 0; i < MAX_MAC; i++)
			QDF_SET_BITS(twt_psoc_obj->twt_resp_flag, i,
				     TWT_NUM_BIT, val);
		return QDF_STATUS_SUCCESS;
	}

	if (mac_id >= MAX_MAC) {
		twt_err("mac id %d greater MAX MAC allowed", mac_id);
		return QDF_STATUS_E_FAILURE;
	}

	QDF_SET_BITS(twt_psoc_obj->twt_resp_flag, mac_id, TWT_NUM_BIT, val);
	return QDF_STATUS_SUCCESS;
}

QDF_STATUS
wlan_twt_enable_event_handler(struct wlan_objmgr_psoc *psoc,
			      struct twt_enable_complete_event_param *event)
{
	struct twt_psoc_priv_obj *twt_psoc;
	struct twt_en_dis_context *twt_context;

	twt_psoc = wlan_objmgr_psoc_get_comp_private_obj(psoc,
							 WLAN_UMAC_COMP_TWT);
	if (!twt_psoc) {
		twt_err("null twt psoc priv obj");
		return QDF_STATUS_E_FAILURE;
	}

	twt_context = &twt_psoc->enable_context;

	twt_debug("mac_id:%d vdev_id:%d status:%d twt_role:%d flags:0x%x",
		  event->mac_id, event->vdev_id, event->status,
		  twt_context->twt_role, event->flags);
	switch (event->status) {
	case HOST_TWT_ENABLE_STATUS_OK:
	case HOST_TWT_ENABLE_STATUS_ALREADY_ENABLED:
		if (twt_context->twt_role == TWT_ROLE_REQUESTOR) {
			if (TWT_EN_DIS_EVENT_IS_VDEV_LEVEL(event->flags)) {
				/*
				 * Vdev-level TWT requestor: set the per-vdev
				 * requestor flag. Congestion timeout is set
				 * to 0 before the enable command is sent
				 * (in osif_twt_setup_req), not here.
				 */
				wlan_twt_cfg_set_vdev_requestor_flag(
						psoc,
						event->vdev_id,
						true);
			} else {
				/*
				 * Pdev-level TWT requestor (legacy): set the
				 * psoc-level requestor flag.
				 */
				wlan_twt_cfg_set_requestor_flag(psoc, true);
			}
		} else if (twt_context->twt_role == TWT_ROLE_RESPONDER) {
			wlan_twt_cfg_set_mac_responder_flag(psoc,
							    event->mac_id,
							    true);
		} else {
			twt_err("Invalid role:%d", twt_context->twt_role);
		}
		break;

	default:
		twt_err("twt enable status:%d", event->status);
		break;
	}

	return mlme_twt_osif_enable_complete_ind(psoc, event,
						 twt_context->context);
}

QDF_STATUS
wlan_twt_disable_event_handler(struct wlan_objmgr_psoc *psoc,
			       struct twt_disable_complete_event_param *event)
{
	struct twt_psoc_priv_obj *twt_psoc;
	struct twt_en_dis_context *twt_context;
	QDF_STATUS status;

	twt_psoc = wlan_objmgr_psoc_get_comp_private_obj(psoc,
							 WLAN_UMAC_COMP_TWT);
	if (!twt_psoc) {
		twt_err("null twt psoc priv obj");
		return QDF_STATUS_E_FAILURE;
	}

	twt_context = &twt_psoc->disable_context;

	twt_debug("mac_id:%d vdev_id:%d status:%d twt_role:%d flags:0x%x",
		  event->mac_id, event->vdev_id, event->status,
		  twt_context->twt_role, event->flags);
	switch (event->status) {
	case HOST_TWT_DISABLE_STATUS_OK:
		if (twt_context->twt_role == TWT_ROLE_REQUESTOR) {
			if (TWT_EN_DIS_EVENT_IS_VDEV_LEVEL(event->flags)) {
				/*
				 * Vdev-level TWT requestor: clear the per-vdev
				 * requestor flag and reset per-vdev congestion
				 * timeout to INI value.
				 */
				wlan_twt_cfg_set_vdev_requestor_flag(
						psoc,
						event->vdev_id,
						false);
				status = wlan_twt_cfg_reset_vdev_congestion_timeout_to_ini(
						psoc,
						event->vdev_id);
				if (QDF_IS_STATUS_ERROR(status))
					twt_err("Failed reset vdev congestion_timeout vdev_id:%d",
						event->vdev_id);
			} else {
				/*
				 * Pdev-level TWT requestor (legacy): clear the
				 * psoc-level requestor flag and reset
				 * congestion timeout to INI for the MAC.
				 */
				wlan_twt_cfg_set_requestor_flag(psoc, false);
				status = wlan_twt_cfg_reset_congestion_timeout_per_mac_to_ini(
						psoc, event->mac_id);
				if (QDF_IS_STATUS_ERROR(status))
					twt_err("Failed reset congestion_timeout MAC%d",
						event->mac_id);
			}
		} else if (twt_context->twt_role == TWT_ROLE_RESPONDER) {
			wlan_twt_cfg_set_mac_responder_flag(psoc,
							    event->mac_id,
							    false);
		} else {
			twt_err("Invalid role:%d", twt_context->twt_role);
		}

		break;

	default:
		twt_err("twt disable status:%d", event->status);
		break;
	}

	return mlme_twt_osif_disable_complete_ind(psoc, event,
						  twt_context->context);
}

QDF_STATUS
wlan_twt_tgt_caps_get_req_en_dis_vdev_support(struct wlan_objmgr_psoc *psoc,
					      bool *val)
{
	struct twt_psoc_priv_obj *twt_psoc;

	if (!psoc) {
		twt_err("psoc is null");
		*val = false;
		return QDF_STATUS_E_FAILURE;
	}

	twt_psoc = wlan_objmgr_psoc_get_comp_private_obj(psoc,
							 WLAN_UMAC_COMP_TWT);
	if (!twt_psoc) {
		twt_err("twt psoc priv obj is null");
		*val = false;
		return QDF_STATUS_E_FAILURE;
	}

	*val = twt_psoc->twt_caps.twt_req_en_dis_vdev_support;

	return QDF_STATUS_SUCCESS;
}

QDF_STATUS
wlan_twt_tgt_caps_get_p2p_go_concurrency_support(struct wlan_objmgr_psoc *psoc,
						 bool *val)
{
	struct twt_psoc_priv_obj *twt_psoc;

	if (!psoc) {
		twt_err("psoc is null");
		return QDF_STATUS_E_FAILURE;
	}

	twt_psoc = wlan_objmgr_psoc_get_comp_private_obj(psoc,
							 WLAN_UMAC_COMP_TWT);
	if (!twt_psoc) {
		twt_err("twt psoc priv obj is null");
		return QDF_STATUS_E_FAILURE;
	}

	*val = twt_psoc->twt_caps.p2p_go_concurrency_support;

	return QDF_STATUS_SUCCESS;
}

QDF_STATUS
wlan_twt_tgt_caps_get_resp_disable_per_vdev(struct wlan_objmgr_psoc *psoc,
					    bool *val)
{
	struct twt_psoc_priv_obj *twt_psoc;

	if (!psoc) {
		twt_err("psoc is null");
		return QDF_STATUS_E_FAILURE;
	}

	twt_psoc = wlan_objmgr_psoc_get_comp_private_obj(psoc,
							 WLAN_UMAC_COMP_TWT);
	if (!twt_psoc) {
		twt_err("twt psoc priv obj is null");
		return QDF_STATUS_E_FAILURE;
	}

	*val = twt_psoc->twt_caps.twt_resp_disable_per_vdev;

	return QDF_STATUS_SUCCESS;
}

QDF_STATUS
wlan_twt_handle_p2p_chan_usage_unavail(
			struct wlan_objmgr_psoc *psoc,
			struct twt_p2p_chan_usage_unavail_params *params)
{
	struct wlan_lmac_if_twt_tx_ops *tx_ops;

	if (!psoc || !params) {
		twt_err("Invalid params");
		return QDF_STATUS_E_INVAL;
	}

	tx_ops = wlan_twt_get_tx_ops(psoc);
	if (!tx_ops) {
		twt_err("twt tx_ops is NULL");
		return QDF_STATUS_E_NULL_VALUE;
	}

	if (!tx_ops->p2p_chan_usage_unavail) {
		twt_err("p2p_chan_usage_unavail tx op is NULL");
		return QDF_STATUS_E_NOSUPPORT;
	}

	return tx_ops->p2p_chan_usage_unavail(psoc, params);
}
