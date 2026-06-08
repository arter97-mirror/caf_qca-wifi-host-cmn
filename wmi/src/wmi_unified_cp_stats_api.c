/*
 * Copyright (c) 2016-2021, The Linux Foundation. All rights reserved.
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

#include "wmi_unified_priv.h"
#include "wmi_unified_param.h"
#include "wmi_unified_cp_stats_api.h"

#if defined(WLAN_SUPPORT_INFRA_CTRL_PATH_STATS) || \
	defined(WLAN_CONFIG_TELEMETRY_AGENT)
QDF_STATUS
wmi_unified_infra_cp_stats_request_send(wmi_unified_t wmi_handle,
					struct infra_cp_stats_cmd_info *param)
{
	if (wmi_handle->ops->send_infra_cp_stats_request_cmd)
		return wmi_handle->ops->send_infra_cp_stats_request_cmd(
								wmi_handle,
								param);

	return QDF_STATUS_E_FAILURE;
}

QDF_STATUS
wmi_unified_extract_infra_cp_stats(wmi_unified_t wmi_handle,
				   void *evt_buf, uint32_t evt_buf_len,
				   struct infra_cp_stats_event *params)
{
	if (wmi_handle->ops->extract_infra_cp_stats)
		return wmi_handle->ops->extract_infra_cp_stats(wmi_handle,
								   evt_buf,
								   evt_buf_len,
								   params);

	return QDF_STATUS_E_FAILURE;
}

#endif /* WLAN_SUPPORT_INFRA_CTRL_PATH_STATS */

QDF_STATUS wmi_unified_stats_request_send(wmi_unified_t wmi_handle,
					  uint8_t macaddr[QDF_MAC_ADDR_SIZE],
					  struct stats_request_params *param)
{
	if (wmi_handle->ops->send_stats_request_cmd)
		return wmi_handle->ops->send_stats_request_cmd(wmi_handle,
				   macaddr, param);

	return QDF_STATUS_E_FAILURE;
}

#ifdef WLAN_FEATURE_BIG_DATA_STATS
QDF_STATUS
wmi_unified_big_data_stats_request_send(wmi_unified_t wmi_handle,
					struct stats_request_params *param)
{
	if (wmi_handle->ops->send_big_data_stats_request_cmd)
		return wmi_handle->ops->send_big_data_stats_request_cmd(
								wmi_handle,
								param);

	return QDF_STATUS_E_FAILURE;
}
#endif

#ifdef WLAN_FEATURE_POWER_STATISTICS
QDF_STATUS
wmi_unified_power_datapath_stats_request_send(
			wmi_unified_t wmi_handle,
			struct wmi_power_datapath_stats_cmd_param *param)
{
	if (wmi_handle->ops->send_pdev_power_datapath_stats_cmd)
		return wmi_handle->ops->send_pdev_power_datapath_stats_cmd(
							wmi_handle,
							param);

	return QDF_STATUS_E_FAILURE;
}
#endif

QDF_STATUS
wmi_extract_stats_param(wmi_unified_t wmi_handle, void *evt_buf,
			wmi_host_stats_event *stats_param)
{
	if (wmi_handle->ops->extract_all_stats_count)
		return wmi_handle->ops->extract_all_stats_count(wmi_handle,
			evt_buf, stats_param);

	return QDF_STATUS_E_FAILURE;
}

QDF_STATUS
wmi_extract_pdev_stats(wmi_unified_t wmi_handle, void *evt_buf,
		       uint32_t index, wmi_host_pdev_stats *pdev_stats)
{
	if (wmi_handle->ops->extract_pdev_stats)
		return wmi_handle->ops->extract_pdev_stats(wmi_handle,
			evt_buf, index, pdev_stats);

	return QDF_STATUS_E_FAILURE;
}

QDF_STATUS
wmi_extract_vdev_stats(wmi_unified_t wmi_handle, void *evt_buf,
		       uint32_t index, wmi_host_vdev_stats *vdev_stats)
{
	if (wmi_handle->ops->extract_vdev_stats)
		return wmi_handle->ops->extract_vdev_stats(wmi_handle,
			evt_buf, index, vdev_stats);

	return QDF_STATUS_E_FAILURE;
}

QDF_STATUS
wmi_extract_peer_stats(wmi_unified_t wmi_handle, void *evt_buf,
		       uint32_t index, wmi_host_peer_stats *peer_stats)
{
	if (wmi_handle->ops->extract_peer_stats)
		return wmi_handle->ops->extract_peer_stats(wmi_handle,
			evt_buf, index, peer_stats);

	return QDF_STATUS_E_FAILURE;
}

QDF_STATUS
wmi_extract_peer_extd_stats(wmi_unified_t wmi_handle, void *evt_buf,
			    uint32_t index,
			    wmi_host_peer_extd_stats *peer_extd_stats)
{
	if (wmi_handle->ops->extract_peer_extd_stats)
		return wmi_handle->ops->extract_peer_extd_stats(wmi_handle,
			evt_buf, index, peer_extd_stats);

	return QDF_STATUS_E_FAILURE;
}

#ifdef WLAN_FEATURE_SON
QDF_STATUS
wmi_extract_inst_rssi_stats_resp(wmi_unified_t wmi_handle, void *evt_buf,
			struct wmi_host_inst_rssi_stats_resp *inst_rssi_resp)
{
	if (wmi_handle->ops->extract_inst_rssi_stats_resp)
		return wmi_handle->ops->extract_inst_rssi_stats_resp(
				wmi_handle, evt_buf, inst_rssi_resp);

	return QDF_STATUS_E_FAILURE;
}
#endif

QDF_STATUS
wmi_unified_send_modify_tx_plim_cmd(wmi_unified_t wmi_handle,
				    enum host_tas_direction direction)
{
	if (wmi_handle->ops->send_modify_tx_plim_cmd)
		return wmi_handle->ops->send_modify_tx_plim_cmd(wmi_handle,
								direction);

	return QDF_STATUS_E_FAILURE;
}

QDF_STATUS
wmi_unified_extract_modify_tx_plim_event(wmi_unified_t wmi_handle,
					 void *evt_buf,
					 uint32_t *status)
{
	if (wmi_handle->ops->extract_modify_tx_plim_event)
		return wmi_handle->ops->extract_modify_tx_plim_event(wmi_handle,
								      evt_buf,
								      status);

	return QDF_STATUS_E_FAILURE;
}

QDF_STATUS
wmi_unified_send_get_avg_tx_power_cmd(wmi_unified_t wmi_handle,
				      uint32_t dsi_id)
{
	if (wmi_handle->ops->send_get_avg_tx_power_cmd)
		return wmi_handle->ops->send_get_avg_tx_power_cmd(wmi_handle,
								  dsi_id);

	return QDF_STATUS_E_FAILURE;
}

QDF_STATUS
wmi_unified_extract_avg_tx_power_event(
		wmi_unified_t wmi_handle,
		void *evt_buf,
		uint32_t *fw_status,
		uint32_t *time_window_in_sec,
		wmi_avg_tx_power_region_per_antenna_chain **chain_data,
		uint32_t *num_chains)
{
	if (wmi_handle->ops->extract_avg_tx_power_event)
		return wmi_handle->ops->extract_avg_tx_power_event(
					wmi_handle, evt_buf,
					fw_status, time_window_in_sec,
					chain_data, num_chains);
	return QDF_STATUS_E_FAILURE;
}

QDF_STATUS
wmi_unified_send_get_tx_power_calling_cmd(wmi_unified_t wmi_handle,
					  uint32_t dsi_id)
{
	if (wmi_handle->ops->send_get_tx_power_calling_cmd)
		return wmi_handle->ops->send_get_tx_power_calling_cmd(
								wmi_handle,
								dsi_id);

	return QDF_STATUS_E_FAILURE;
}

QDF_STATUS
wmi_unified_extract_plimit_table_event(
				wmi_unified_t wmi_handle,
				void *evt_buf,
				uint32_t *fw_status,
				uint32_t *dsi_id,
				wmi_tx_power_per_antenna_chain **chain_data,
				uint32_t *num_chains)
{
	if (wmi_handle->ops->extract_plimit_table_event)
		return wmi_handle->ops->extract_plimit_table_event(
						wmi_handle, evt_buf,
						fw_status, dsi_id,
						chain_data, num_chains);

	return QDF_STATUS_E_FAILURE;
}
