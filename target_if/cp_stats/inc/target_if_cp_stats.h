/*
 * Copyright (c) 2018, 2020-2021 The Linux Foundation. All rights reserved.
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
 * DOC: target_if_cp_stats.h
 *
 * This header file provide declarations required for Rx and Tx events from
 * firmware
 */

#ifndef __TARGET_IF_CP_STATS_H__
#define __TARGET_IF_CP_STATS_H__

#include <target_if.h>
#include <wlan_lmac_if_def.h>
#include <wlan_cp_stats_utils_api.h>

#ifdef QCA_SUPPORT_CP_STATS

/**
 * target_if_cp_stats_get_rx_ops() - get rx ops
 * @psoc: pointer to psoc object
 *
 * Return: pointer to rx ops
 */
static inline struct wlan_lmac_if_cp_stats_rx_ops *
target_if_cp_stats_get_rx_ops(struct wlan_objmgr_psoc *psoc)
{
	struct wlan_lmac_if_rx_ops *rx_ops;

	rx_ops = wlan_psoc_get_lmac_if_rxops(psoc);
	if (!rx_ops) {
		cp_stats_err("rx_ops is NULL");
		return NULL;
	}

	return &rx_ops->cp_stats_rx_ops;
}

/**
 * target_if_cp_stats_get_tx_ops() - get tx ops
 * @psoc: pointer to psoc object
 *
 * Return: pointer to tx ops
 */
static inline struct wlan_lmac_if_cp_stats_tx_ops *
target_if_cp_stats_get_tx_ops(struct wlan_objmgr_psoc *psoc)
{
	struct wlan_lmac_if_tx_ops *tx_ops;

	tx_ops = wlan_psoc_get_lmac_if_txops(psoc);
	if (!tx_ops) {
		cp_stats_err("tx_ops is NULL");
		return NULL;
	}

	return &tx_ops->cp_stats_tx_ops;
}

/**
 * target_if_cp_stats_send_coex_stats_req() - Send coex stats request to
 *                                            firmware
 * @psoc: PSOC object
 *
 * This function sends a request to firmware to retrieve coexistence policy
 * statistics using WMI_COEX_GET_POLICY_STATS_CMDID.
 *
 * Return: QDF_STATUS_SUCCESS on success, QDF_STATUS_E_** on error
 */
QDF_STATUS
target_if_cp_stats_send_coex_stats_req(struct wlan_objmgr_psoc *psoc);

/**
 * target_if_cp_stats_get_coex_stats() - Get coexistence statistics
 * @vdev: VDEV object
 *
 * This function sends a request to firmware to retrieve coexistence policy
 * statistics using WMI_COEX_GET_POLICY_STATS_CMDID.
 *
 * Return: QDF_STATUS_SUCCESS on success, QDF_STATUS_E_** on error
 */
QDF_STATUS
target_if_cp_stats_get_coex_stats(struct wlan_objmgr_vdev *vdev);

/**
 * target_if_cp_stats_send_tas_mode() - Send TAS mode to firmware
 * @psoc: PSOC object
 * @direction: TAS direction as enum host_tas_direction; WMI layer converts
 *             to wmi_plim_direction_type before sending to firmware
 *
 * Sends WMI_SET_MODIFY_TX_PLIM_CMDID to firmware. Called via
 * send_tas_mode tx_ops from ucfg_cp_stats_send_tas_mode().
 *
 * Return: QDF_STATUS_SUCCESS on success, QDF_STATUS_E_** on error
 */
QDF_STATUS
target_if_cp_stats_send_tas_mode(struct wlan_objmgr_psoc *psoc,
				 enum host_tas_direction direction);

/**
 * target_if_cp_stats_register_tx_ops() - define cp_stats lmac tx ops functions
 * @tx_ops: pointer to lmac tx ops
 *
 * Return: QDF_STATUS_SUCCESS on success, QDF_STATUS_E_** on error
 */
QDF_STATUS
target_if_cp_stats_register_tx_ops(struct wlan_lmac_if_tx_ops *tx_ops);

#ifdef WLAN_SUPPORT_LEGACY_CP_STATS_HANDLERS
/**
 * target_if_cp_stats_register_legacy_event_handler() - Register handler
 * specific to legacy components
 * @psoc: pointer to psoc object
 *
 * Return: QDF_STATUS_SUCCESS on Success, other QDF_STATUS error codes on
 * failure
 */
QDF_STATUS
target_if_cp_stats_register_legacy_event_handler(struct wlan_objmgr_psoc *psoc);

/**
 * target_if_cp_stats_unregister_legacy_event_handler() - Unregister handler
 * specific to legacy components
 * @psoc: pointer to psoc object
 *
 * Return: QDF_STATUS_SUCCESS on Success, other QDF_STATUS error codes on
 * failure
 */
QDF_STATUS
target_if_cp_stats_unregister_legacy_event_handler(
						struct wlan_objmgr_psoc *psoc);
#else

static inline QDF_STATUS
target_if_cp_stats_register_legacy_event_handler(struct wlan_objmgr_psoc *psoc)
{
	return QDF_STATUS_SUCCESS;
}

static inline QDF_STATUS
target_if_cp_stats_unregister_legacy_event_handler(
						struct wlan_objmgr_psoc *psoc)
{
	return QDF_STATUS_SUCCESS;
}
#endif  /* WLAN_SUPPORT_LEGACY_CP_STATS_HANDLERS */

#ifdef WLAN_CHIPSET_STATS

/**
 * target_if_cp_stats_is_service_cstats_enabled() - Interface to check and
 * return whether FW support Chipset Stats logging or not
 * @psoc: pointer to psoc object
 * @is_fw_support_cstats: True if feature is supported by FW else False
 *
 * Return: QDF_STATUS_SUCCESS on Success, other QDF_STATUS error codes on
 * failure
 */
QDF_STATUS
target_if_cp_stats_is_service_cstats_enabled(struct wlan_objmgr_psoc *psoc,
					     bool *is_fw_support_cstats);
#else
static inline QDF_STATUS
target_if_cp_stats_is_service_cstats_enabled(struct wlan_objmgr_psoc *psoc,
					     bool *is_fw_support_cstats)
{
	return QDF_STATUS_E_NOSUPPORT;
}
#endif

#else
static inline QDF_STATUS
target_if_cp_stats_register_tx_ops(struct wlan_lmac_if_tx_ops *tx_ops)
{
	return QDF_STATUS_SUCCESS;
}
#endif /* QCA_SUPPORT_CP_STATS */

/**
 * target_if_is_ctas_plim_indication_supported() - Check whether FW supports
 * C-TAS power indication and power limit enquiring
 * @psoc: pointer to psoc object
 *
 * API to check WMI_SERVICE_CTAS_PLIM_INDICATION_SUPPORT capability advertised
 * by firmware via the service ready extended2 bitmap.
 *
 * Return: true if FW supports the capability, false otherwise
 */
bool
target_if_is_ctas_plim_indication_supported(struct wlan_objmgr_psoc *psoc);

#endif /* __TARGET_IF_CP_STATS_H__ */
