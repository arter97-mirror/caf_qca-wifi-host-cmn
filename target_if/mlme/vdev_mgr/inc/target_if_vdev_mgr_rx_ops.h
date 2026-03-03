/*
 * Copyright (c) 2019-2020 The Linux Foundation. All rights reserved.
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
 * DOC: target_if_vdev_mgr_rx_ops.h
 *
 * This file provides declarations for APIs registered for wmi events
 */

#ifndef __TARGET_IF_VDEV_MGR_RX_OPS_H__
#define __TARGET_IF_VDEV_MGR_RX_OPS_H__

#include <wmi_unified_param.h>
#include <wlan_objmgr_psoc_obj.h>
#include <wlan_lmac_if_def.h>
#include <qdf_platform.h>

/**
 * target_if_vdev_mgr_is_panic_allowed: API to get if panic is allowed on
 * timeout
 *
 * Return: TRUE or FALSE when VDEV_ASSERT_MANAGEMENT is disabled else FALSE
 */
#ifdef VDEV_ASSERT_MANAGEMENT
static inline bool target_if_vdev_mgr_is_panic_allowed(void)
{
	return false;
}
#else
static inline bool target_if_vdev_mgr_is_panic_allowed(void)
{
	if (qdf_is_recovering() || qdf_is_fw_down())
		return false;

	return true;
}
#endif


/**
 * target_if_vdev_mgr_offload_bcn_tx_status_handler() - API to handle beacon
 * tx status event
 * @scn: pointer to scan object
 * @data: pointer to data
 * @datalen: length of data
 *
 * Return: QDF_STATUS_SUCCESS on success, QDF_STATUS_E_** on error
 */
int target_if_vdev_mgr_offload_bcn_tx_status_handler(ol_scn_t scn,
						     uint8_t *data,
						     uint32_t datalen);

/**
 * target_if_vdev_mgr_tbttoffset_update_handler() - API to handle tbtt offset
 * update event
 * @scn: pointer to scan object
 * @data: pointer to data
 * @datalen: length of data
 *
 * Return: QDF_STATUS_SUCCESS on success, QDF_STATUS_E_** on error
 */
int target_if_vdev_mgr_tbttoffset_update_handler(ol_scn_t scn,
						 uint8_t *data,
						 uint32_t datalen);

/**
 * target_if_vdev_mgr_ext_tbttoffset_update_handler() - API to handle ext tbtt
 * offset update event
 * @scn: pointer to scan object
 * @data: pointer to data
 * @datalen: length of data
 *
 * Return: QDF_STATUS_SUCCESS on success, QDF_STATUS_E_** on error
 */
int target_if_vdev_mgr_ext_tbttoffset_update_handler(ol_scn_t scn,
						     uint8_t *data,
						     uint32_t datalen);

/**
 * target_if_vdev_mgr_is_panic_on_bug: API to get panic on bug
 *
 * Return: TRUE or FALSE
 */
static inline bool target_if_vdev_mgr_is_panic_on_bug(void)
{
#ifdef PANIC_ON_BUG
	return true;
#else
	return false;
#endif
}

/**
 * target_if_vdev_mgr_get_rx_ops() - get rx ops
 * @psoc: pointer to psoc object
 *
 * Return: pointer to rx ops
 */
static inline struct wlan_lmac_if_mlme_rx_ops *
target_if_vdev_mgr_get_rx_ops(struct wlan_objmgr_psoc *psoc)
{
	struct wlan_lmac_if_rx_ops *rx_ops;

	rx_ops = wlan_psoc_get_lmac_if_rxops(psoc);
	if (!rx_ops) {
		qdf_err("rx_ops is NULL");
		return NULL;
	}

	return &rx_ops->mops;
}

/**
 * target_if_vdev_mgr_rsp_timer_mgmt_cb() - function to handle response timer
 * @arg: pointer to argument
 *
 * Callback timer triggered when response timer expires which pass
 * vdev as argument
 *
 * Return: status of operation.
 */
void target_if_vdev_mgr_rsp_timer_mgmt_cb(void *arg);

/**
 * target_if_vdev_mgr_wmi_event_register() - function to handle register
 * events from WMI
 * @psoc: pointer to psoc object
 *
 * Return: QDF_STATUS_SUCCESS on success, QDF_STATUS_E_** on error
 */
QDF_STATUS target_if_vdev_mgr_wmi_event_register(
					struct wlan_objmgr_psoc *psoc);

/**
 * target_if_vdev_mgr_wmi_event_unregister() - function to handle unregister
 * events from WMI
 * @psoc: pointer to psoc object
 *
 * Return: QDF_STATUS_SUCCESS on success, QDF_STATUS_E_** on error
 */
QDF_STATUS target_if_vdev_mgr_wmi_event_unregister(
					struct wlan_objmgr_psoc *psoc);

/**
 * target_if_vdev_mgr_rsp_timer_cb() - function to handle vdev related timeouts
 * @arg: pointer to argument
 *
 * Return: none
 */
void target_if_vdev_mgr_rsp_timer_cb(void *arg);

/**
 * target_if_vdev_mgr_start_response_common() - Common vdev start response
 * processing api
 * @psoc: psoc object
 * @vdev_start_resp: vdev start response structure
 *
 * This function handles the common processing for vdev start responses
 * that doesn't depend on firmware events. It can be called both from
 * firmware response handlers and when operations are skipped due to
 * conditions like link switch in progress.
 *
 * Return: QDF_STATUS
 */
QDF_STATUS target_if_vdev_mgr_start_response_common(
				struct wlan_objmgr_psoc *psoc,
				struct vdev_start_response *vdev_start_resp);

/**
 * target_if_vdev_mgr_stop_response_process() - Process vdev stop response
 * @psoc: Pointer to psoc object
 * @vdev_id: vdev id
 *
 * This function handles the non-firmware-dependent processing of vdev stop
 * response, including stopping the response timer and calling the response
 * callback. This API can be called independently without WMI event data.
 *
 * Return: QDF_STATUS_SUCCESS on success, QDF_STATUS_E_** on error
 */
QDF_STATUS
target_if_vdev_mgr_stop_response_process(struct wlan_objmgr_psoc *psoc,
					 uint32_t vdev_id);
#endif /* __TARGET_IF_VDEV_MGR_RX_OPS_H__ */
