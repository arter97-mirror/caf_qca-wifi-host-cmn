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
 * DOC: target_if_ipa_ring_stats.c
 *
 * This file provides definition for APIs registered through lmac Tx Ops
 * and WMI event handlers for IPA ring stats.
 */

#include <qdf_status.h>
#include <qdf_mem.h>
#include <target_if.h>
#include <wlan_ipa_priv.h>
#include <target_if_ipa_ring_stats.h>
#include <wmi_unified_ipa_ring_stats_api.h>
#include <wlan_cfg80211.h>
#include <wmi_unified_api.h>
#include <wmi_unified_priv.h>
#include <wlan_lmac_if_def.h>
#include <hal_internal.h>
#include <wlan_ipa_ring_stats_api.h>
#include <wlan_ipa_ring_stats_priv_api.h>
#include <wlan_ipa_ring_stats_ucfg_api.h>

/**
 * ipa_convert_ring_to_srng() - Convert IPA stats ring enum to HAL SRNG ring id
 * @ring: IPA stats ring (REO/TCL)
 *
 * This helper converts the high-level IPA ring stats ring id to the underlying
 * HAL SRNG ring id that firmware uses in the WMI command.
 *
 * Return: HAL SRNG ring id; caller must validate input enum range.
 */
static enum hal_srng_ring_id
ipa_convert_ring_to_srng(uint32_t ring)
{
	switch (ring) {
	case QCA_WLAN_VENDOR_IPA_RING_STATS_ID_REO:
		/* REO path: e.g. REO2SW4 */
		return HAL_SRNG_REO2SW4;
	case QCA_WLAN_VENDOR_IPA_RING_STATS_ID_TCL:
		/* TCL path: e.g. SW2TCL3 */
		return HAL_SRNG_SW2TCL3;
	default:
		target_if_err("Invalid ring_id %d, defaulting to REO2SW4", ring);
		return HAL_SRNG_REO2SW4;
	}
}

/**
 * target_if_ipa_ring_stats_event_handler() - WMI IPA ring stats event handler
 * @handle: WMI handle (wmi_unified_t)
 * @evt_buf: pointer to WMI event buffer
 * @len: length of @evt_buf
 *
 * This handler is invoked by WMI when firmware sends IPA ring stats event.
 * It parses TLV using wmi_extract_ipa_stats_event() and stores the result
 * into psoc-level IPA private object (wlan_ipa_priv), then signals
 * completion to wake up the waiting thread in ucfg/tgt layer.
 *
 * Return: 0 on success; negative errno on failure
 */
static int
target_if_ipa_ring_stats_event_handler(void *handle, uint8_t *evt_buf, uint32_t evt_len)
{
	ol_scn_t scn = handle;
	struct wlan_objmgr_psoc *psoc;
	struct wmi_unified *wmi_handle;
	QDF_STATUS status;
	struct osif_request *request;
	struct ipa_ring_stats_req_priv *priv;
	struct ipa_ring_stats_psoc_priv_obj *ipa_obj;

	if (!scn || !evt_buf) {
		target_if_err("IPA stats event: invalid scn or evt_buf");
		return -EINVAL;
	}

	psoc = target_if_get_psoc_from_scn_hdl(scn);
	if (!psoc) {
		target_if_err("IPA stats event: null psoc");
		return -EINVAL;
	}

	wmi_handle = get_wmi_unified_hdl_from_psoc(psoc);
	if (!wmi_handle) {
		target_if_err("IPA stats event: null wmi_handle");
		return -EINVAL;
	}

	ipa_obj = ipa_ring_stats_get_psoc_priv_obj(psoc);
	if (!ipa_obj)
		return -EINVAL;

	qdf_spin_lock_bh(&ipa_obj->lock);
	request = ipa_obj->active_request;
	qdf_spin_unlock_bh(&ipa_obj->lock);

	if (!request) {
		target_if_err("IPA stats event: no pending request");
		return -EINVAL;
	}

	priv = osif_request_priv(request);
	if (!priv) {
		target_if_err("IPA stats event: priv is NULL");
		osif_request_put(request);
		return -EINVAL;
	}

	priv->evt.version   = 0;
	priv->evt.num_rings = 0;

	status = wmi_extract_ipa_ring_stats_event(wmi_handle,
                                         	  evt_buf, evt_len,
                                         	  &priv->evt);

	if (QDF_IS_STATUS_ERROR(status)) {
		target_if_err("IPA stats event: extract failed, status=%d",
			      status);
	}

	osif_request_complete(request);

	return 0;
}

/**
 * target_if_send_ipa_ring_stats_req() - Send IPA ring stats request to FW via WMI
 * @psoc: pointer to psoc object
 * @ring_id: IPA ring stats ring (REO/TCL)
 *
 * This function is registered in lmac_if_ipa_ring_stats_tx_ops and invoked
 * by upper layers (tgt/ucfg) to request IPA ring stats from firmware.
 *
 * Return: QDF_STATUS
 */
static QDF_STATUS
target_if_send_ipa_ring_stats_req(struct wlan_objmgr_psoc *psoc,
			     uint32_t ring_id)
{
	struct wmi_unified *wmi_handle;
	enum hal_srng_ring_id srng_id;

	if (!psoc) {
		target_if_err("IPA stats req: null psoc");
		return QDF_STATUS_E_NULL_VALUE;
	}

	wmi_handle = get_wmi_unified_hdl_from_psoc(psoc);
	if (!wmi_handle) {
		target_if_err("IPA stats req: null wmi_handle");
		return QDF_STATUS_E_NULL_VALUE;
	}

	if (ring_id >= QCA_WLAN_VENDOR_IPA_RING_STATS_ID_MAX) {
		target_if_err("IPA stats req: invalid ring_id %d", ring_id);
		return QDF_STATUS_E_INVAL;
	}

	srng_id = ipa_convert_ring_to_srng(ring_id);

	return wmi_unified_ipa_ring_stats_req_cmd_send(wmi_handle, srng_id);
}

/**
 * target_if_ipa_ring_stats_register_event_handler() - Register WMI event handler
 * @psoc: pointer to psoc object
 *
 * This function registers the IPA ring stats WMI event handler with the
 * wmi_unified layer. It is typically invoked from lmac tx_ops init flow.
 *
 * Return: QDF_STATUS
 */
static QDF_STATUS
target_if_ipa_ring_stats_register_event_handler(struct wlan_objmgr_psoc *psoc)
{
	wmi_unified_t wmi_handle;
	QDF_STATUS status;

	if (!psoc) {
		target_if_err("IPA ring stats unreg_event: null psoc");
		return QDF_STATUS_E_FAILURE;
	}

	wmi_handle = get_wmi_unified_hdl_from_psoc(psoc);
	if (!wmi_handle) {
		target_if_err("IPA ring stats unreg_event: null wmi_handle");
		return QDF_STATUS_E_FAILURE;
	}

	status = wmi_unified_register_event(wmi_handle,
					    wmi_vendor_get_ipa_ring_stats_id,
					    target_if_ipa_ring_stats_event_handler);

	if (QDF_IS_STATUS_ERROR(status)) {
		target_if_err("Failed to register IPA stats event: status=%d",
			      status);
		return status;
	}

	return QDF_STATUS_SUCCESS;
}

/**
 * target_if_ipa_ring_stats_unregister_event_handler() - unregister WMI event handler
 * @psoc: pointer to psoc object
 *
 * Unregister the IPA ring stats WMI event handler for the given psoc.
 *
 * Return: QDF_STATUS_SUCCESS on success, QDF_STATUS_E_* on error
 */
static QDF_STATUS
target_if_ipa_ring_stats_unregister_event_handler(
		struct wlan_objmgr_psoc *psoc)
{
	wmi_unified_t wmi_handle;

	wmi_handle = get_wmi_unified_hdl_from_psoc(psoc);
	if (!wmi_handle)
		return QDF_STATUS_E_NULL_VALUE;

	return wmi_unified_unregister_event_handler(wmi_handle,
						    wmi_vendor_get_ipa_ring_stats_id);
}

/**
 * target_if_ipa_ring_stats_register_tx_ops() - register IPA ring stats tx ops
 * @tx_ops: pointer to lmac if tx ops
 *
 * This function is called from global lmac tx_ops init code to
 * hook IPA ring stats TX ops and event registration callbacks.
 *
 * Return: QDF_STATUS_SUCCESS on success, QDF_STATUS_E_INVAL on error
 */
QDF_STATUS
target_if_ipa_ring_stats_register_tx_ops(struct wlan_lmac_if_tx_ops *tx_ops)
{
	struct wlan_lmac_if_ipa_ring_stats_tx_ops *ipa_ring_stats_ops;

	if (!tx_ops) {
		target_if_err("IPA stats tx_ops is NULL!");
		return QDF_STATUS_E_INVAL;
	}

	ipa_ring_stats_ops = &tx_ops->ipa_ring_stats_tx_ops;

	ipa_ring_stats_ops->send_ipa_ring_stats_req = target_if_send_ipa_ring_stats_req;

	ipa_ring_stats_ops->register_ipa_ring_stats_event_handler =
		target_if_ipa_ring_stats_register_event_handler;
	ipa_ring_stats_ops->unregister_ipa_ring_stats_event_handler =
    		target_if_ipa_ring_stats_unregister_event_handler;

	return QDF_STATUS_SUCCESS;
}
