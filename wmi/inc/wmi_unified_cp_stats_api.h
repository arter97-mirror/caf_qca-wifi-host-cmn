/*
 * Copyright (c) 2013-2021, The Linux Foundation. All rights reserved.
 * Copyright (c) 2021,2023 Qualcomm Innovation Center, Inc. All rights reserved.
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
 * DOC: Implement API's specific to cp stats component.
 */

#ifndef _WMI_UNIFIED_CP_STATS_API_H_
#define _WMI_UNIFIED_CP_STATS_API_H_

#ifdef QCA_SUPPORT_MC_CP_STATS
#include <wmi_unified_mc_cp_stats_api.h>
#endif
#include <wlan_cp_stats_public_structs.h>

#if defined(WLAN_SUPPORT_INFRA_CTRL_PATH_STATS) || \
	defined(WLAN_CONFIG_TELEMETRY_AGENT)
/**
 * wmi_unified_infra_cp_stats_request_send() - WMI request infra_cp_stats
 * function
 * @wmi_handle: wmi handle
 * @param: pointer to hold infra_cp_stats request parameters
 *
 * Return: QDF_STATUS_SUCCESS on Success, other QDF_STATUS error
 *         codes on failure
 */
QDF_STATUS
wmi_unified_infra_cp_stats_request_send(
				wmi_unified_t wmi_handle,
				struct infra_cp_stats_cmd_info *param);

/**
 * wmi_unified_extract_infra_cp_stats() - extract various infra cp statistics
 * @wmi_handle: wmi handle
 * @evt_buf: event buffer
 * @evt_buf_len: length of event buffer
 * @params: pointer to store the extracted event info
 *
 * This function extracts the infra cp statistics from the event
 *
 * Return: QDF_STATUS_SUCCESS on success and QDF_STATUS_E_FAILURE for failure
 */
QDF_STATUS
wmi_unified_extract_infra_cp_stats(wmi_unified_t wmi_handle,
				   void *evt_buf, uint32_t evt_buf_len,
				   struct infra_cp_stats_event *params);

/**
 * wmi_stats_handler() - parse the wmi event and fill the stats values
 * @wmi_handle: wmi handle
 * @buff: Buffer containing wmi event
 * @len: length of event buffer
 * @params: buffer to hold parameters extracted from response event
 *
 * Return: QDF_STATUS_SUCCESS on success, else other qdf error values
 */
QDF_STATUS wmi_stats_handler(wmi_unified_t wmi_handle, void *buff, int32_t len,
			     struct infra_cp_stats_event *params);

QDF_STATUS
extract_infra_cp_stats_tlv(wmi_unified_t wmi_handle, void *evt_buf,
			   uint32_t evt_buf_len,
			   struct infra_cp_stats_event *params);
#endif /* WLAN_SUPPORT_INFRA_CTRL_PATH_STATS */

/**
 * wmi_unified_stats_request_send() - WMI request stats function
 * @wmi_handle: handle to WMI
 * @macaddr: MAC address
 * @param: pointer to hold stats request parameter
 *
 * Return: QDF_STATUS_SUCCESS on success and QDF_STATUS_E_FAILURE for failure
 */
QDF_STATUS wmi_unified_stats_request_send(wmi_unified_t wmi_handle,
					  uint8_t macaddr[QDF_MAC_ADDR_SIZE],
					  struct stats_request_params *param);

#ifdef WLAN_FEATURE_BIG_DATA_STATS
/**
 * wmi_unified_big_data_stats_request_send() - WMI request big data stats
 * function
 * @wmi_handle: handle to WMI
 * @param: pointer to hold stats request parameter
 *
 * Return: QDF_STATUS_SUCCESS on success and QDF_STATUS_E_FAILURE for failure
 */
QDF_STATUS
wmi_unified_big_data_stats_request_send(wmi_unified_t wmi_handle,
					struct stats_request_params *param);
#endif

/**
 * wmi_extract_stats_param() - extract all stats count from event
 * @wmi_handle: wmi handle
 * @evt_buf: pointer to event buffer
 * @stats_param: Pointer to hold stats count
 *
 * Return: QDF_STATUS_SUCCESS on success and QDF_STATUS_E_FAILURE for failure
 */
QDF_STATUS
wmi_extract_stats_param(wmi_unified_t wmi_handle, void *evt_buf,
			wmi_host_stats_event *stats_param);

/**
 * wmi_extract_pdev_stats() - extract pdev stats from event
 * @wmi_handle: wmi handle
 * @evt_buf: pointer to event buffer
 * @index: Index into pdev stats
 * @pdev_stats: Pointer to hold pdev stats
 *
 * Return: QDF_STATUS_SUCCESS on success and QDF_STATUS_E_FAILURE for failure
 */
QDF_STATUS
wmi_extract_pdev_stats(wmi_unified_t wmi_handle, void *evt_buf,
		       uint32_t index, wmi_host_pdev_stats *pdev_stats);

/**
 * wmi_extract_vdev_stats() - extract vdev stats from event
 * @wmi_handle: wmi handle
 * @evt_buf: pointer to event buffer
 * @index: Index into vdev stats
 * @vdev_stats: Pointer to hold vdev stats
 *
 * Return: QDF_STATUS_SUCCESS on success and QDF_STATUS_E_FAILURE for failure
 */
QDF_STATUS
wmi_extract_vdev_stats(wmi_unified_t wmi_handle, void *evt_buf,
		       uint32_t index, wmi_host_vdev_stats *vdev_stats);

/**
 * wmi_extract_peer_stats() - extract peer stats from event
 * @wmi_handle: wmi handle
 * @evt_buf: pointer to event buffer
 * @index: Index into peer stats
 * @peer_stats: Pointer to hold peer stats
 *
 * Return: QDF_STATUS_SUCCESS on success and QDF_STATUS_E_FAILURE for failure
 */
QDF_STATUS
wmi_extract_peer_stats(wmi_unified_t wmi_handle, void *evt_buf,
		       uint32_t index, wmi_host_peer_stats *peer_stats);

/**
 * wmi_extract_peer_extd_stats() - extract extended peer stats from event
 * @wmi_handle: wmi handle
 * @evt_buf: pointer to event buffer
 * @index: Index into extended peer stats
 * @peer_extd_stats: Pointer to hold extended peer stats
 *
 * Return: QDF_STATUS_SUCCESS on success and QDF_STATUS_E_FAILURE for failure
 */
QDF_STATUS
wmi_extract_peer_extd_stats(wmi_unified_t wmi_handle, void *evt_buf,
			    uint32_t index,
			    wmi_host_peer_extd_stats *peer_extd_stats);

#ifdef WLAN_FEATURE_SON
/**
 * wmi_extract_inst_rssi_stats_resp() - extract inst rssi stats from event
 * @wmi_handle: wmi handle
 * @evt_buf: pointer to event buffer
 * @inst_rssi_resp: pointer to hold inst rssi stats
 *
 * Return: QDF_STATUS_SUCCESS on success and QDF_STATUS_E_FAILURE for failure
 */
QDF_STATUS
wmi_extract_inst_rssi_stats_resp(wmi_unified_t wmi_handle, void *evt_buf,
			struct wmi_host_inst_rssi_stats_resp *inst_rssi_resp);
#endif

#ifdef WLAN_FEATURE_QSH_SCAN
/**
 * wmi_unified_extract_scan_stats_event() - extract qsh scan stats from event
 * @wmi_handle: wmi handle
 * @evt_buf: pointer to event buffer
 * @scan_stats: Pointer to hold qsh scan stats
 *
 * Return: QDF_STATUS_SUCCESS on success and QDF_STATUS_E_FAILURE for failure
 */
QDF_STATUS
wmi_unified_extract_scan_stats_event(wmi_unified_t wmi_handle, void *evt_buf,
				     struct wmi_scan_stats_event *scan_stats);
#endif /* WLAN_FEATURE_QSH_SCAN*/

/**
 * wmi_unified_send_modify_tx_plim_cmd() - Send WMI_SET_MODIFY_TX_PLIM_CMDID
 * @wmi_handle: WMI handle
 * @direction: TAS direction as enum host_tas_direction (defined in
 *             wlan_cp_stats_public_structs.h); WMI layer converts to
 *             wmi_plim_direction_type before sending to firmware
 *
 * Return: QDF_STATUS_SUCCESS on success, QDF_STATUS_E_** on error
 */
QDF_STATUS
wmi_unified_send_modify_tx_plim_cmd(wmi_unified_t wmi_handle,
				    enum host_tas_direction direction);

/**
 * wmi_unified_extract_modify_tx_plim_event() - Extract
 * WMI_MODIFY_TX_PLIM_EVENTID
 * @wmi_handle: WMI handle
 * @evt_buf: pointer to event buffer
 * @status: output, firmware status (0 = success, 1 = failure)
 *
 * Return: QDF_STATUS_SUCCESS on success, QDF_STATUS_E_** on error
 */
QDF_STATUS
wmi_unified_extract_modify_tx_plim_event(wmi_unified_t wmi_handle,
					 void *evt_buf,
					 uint32_t *status);

/**
 * wmi_unified_send_get_avg_tx_power_cmd() - Send WMI_GET_AVG_TX_POWER_CMDID
 * @wmi_handle: WMI handle
 * @dsi_id: active Device State Index for which avg TX power is requested
 *
 * Return: QDF_STATUS_SUCCESS on success, QDF_STATUS_E_** on error
 */
QDF_STATUS
wmi_unified_send_get_avg_tx_power_cmd(wmi_unified_t wmi_handle,
				      uint32_t dsi_id);

/**
 * wmi_unified_extract_avg_tx_power_event() - Extract WMI_AVG_TX_POWER_EVENTID
 * @wmi_handle: WMI handle
 * @evt_buf: event data buffer
 * @fw_status: firmware status output (0=success, 1=failure)
 * @time_window_in_sec: time window output
 * @chain_data: per-chain data output pointer
 * @num_chains: number of chains output
 *
 * Return: QDF_STATUS_SUCCESS on success, QDF_STATUS_E_** on error
 */
QDF_STATUS
wmi_unified_extract_avg_tx_power_event(
			wmi_unified_t wmi_handle,
			void *evt_buf,
			uint32_t *fw_status,
			uint32_t *time_window_in_sec,
			wmi_avg_tx_power_region_per_antenna_chain **chain_data,
			uint32_t *num_chains);

#endif /* _WMI_UNIFIED_CP_STATS_API_H_ */
