/*
 * Copyright (c) 2018, 2021 The Linux Foundation. All rights reserved.
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
 * DOC: target_if_cp_stats.c
 *
 * This file provide definition for APIs registered through lmac Tx Ops
 */

#include <qdf_mem.h>
#include <qdf_status.h>
#include <target_if_cp_stats.h>
#include <wmi_unified_priv.h>
#include <wmi_unified_param.h>
#include <target_if.h>
#include <wlan_tgt_def_config.h>
#include <wmi_unified_api.h>
#include <wmi_unified_cp_stats_api.h>
#include <wlan_osif_priv.h>
#include <wlan_cp_stats_utils_api.h>
#include <wlan_cp_stats_mc_ucfg_api.h>
#include <wlan_objmgr_peer_obj.h>
#ifdef WLAN_FEATURE_MIB_STATS
#include <wlan_cp_stats_mc_defs.h>
#endif
#include "cp_stats/core/src/wlan_cp_stats_defs.h"
#include "cdp_txrx_cmn_struct.h"
#include "cdp_txrx_ctrl.h"
#include "cp_stats/core/src/wlan_cp_stats_comp_handler.h"
#ifdef WLAN_FEATURE_LL_LT_SAP
#include <target_if_ll_sap.h>
#endif

#ifdef WLAN_SUPPORT_INFRA_CTRL_PATH_STATS
#ifdef WLAN_SUPPORT_TWT
/**
 * target_if_infra_cp_stats_twt_event_free() - Free event buffer
 * @ev: pointer to infra cp stats event structure
 *
 * Return: None
 */
static
void target_if_infra_cp_stats_twt_event_free(struct infra_cp_stats_event *ev)
{
	qdf_mem_free(ev->twt_infra_cp_stats);
	ev->twt_infra_cp_stats = NULL;
}

/**
 * target_if_infra_cp_stats_twt_event_alloc() - Allocate event buffer for TWT
 * parameters
 * @ev: pointer to infra cp stats event structure
 *
 * Return: QDF_STATUS_SUCCESS on Success, other QDF_STATUS error codes on
 * failure
 */
static QDF_STATUS
target_if_infra_cp_stats_twt_event_alloc(struct infra_cp_stats_event *ev)
{
	ev->twt_infra_cp_stats =
			qdf_mem_malloc(sizeof(*ev->twt_infra_cp_stats) *
			INFRA_CP_STATS_MAX_RESP_TWT_DIALOG_ID);
	if (!ev->twt_infra_cp_stats) {
		cp_stats_err("mem alloc failed for ev.twt_infra_cp_stats");
		return QDF_STATUS_E_NOMEM;
	}

	return QDF_STATUS_SUCCESS;
}

#else
static inline
void target_if_infra_cp_stats_twt_event_free(struct infra_cp_stats_event *ev)
{
}

static inline QDF_STATUS
target_if_infra_cp_stats_twt_event_alloc(struct infra_cp_stats_event *ev)
{
	return QDF_STATUS_SUCCESS;
}

static inline
void target_if_infra_cp_stats_free_stats_event(struct infra_cp_stats_event *ev)
{
}
#endif /* WLAN_SUPPORT_TWT */

static
void target_if_infra_cp_stats_rrm_sta_stats_event_free(
					struct infra_cp_stats_event *ev)
{
	qdf_mem_free(ev->sta_stats);
	ev->sta_stats = NULL;
}

static QDF_STATUS
target_if_infra_cp_stats_rrm_sta_stats_event_alloc(
			struct infra_cp_stats_event *ev)
{
	ev->sta_stats =
	qdf_mem_malloc(sizeof(*ev->sta_stats));
	if (!ev->sta_stats) {
		return QDF_STATUS_E_NOMEM;
	}

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS
target_if_infra_cp_stats_enchance_stats_event_alloc(
					struct infra_cp_stats_event *ev)
{
	ev->vdev_beacon_stats =
		qdf_mem_malloc(sizeof(*ev->vdev_beacon_stats) *
		CTRL_PATH_STATS_MAX_VDEV_ID);
	if (!ev->vdev_beacon_stats)
		return QDF_STATUS_E_NOMEM;

	ev->vdev_congestion_stats =
			qdf_mem_malloc(sizeof(*ev->vdev_congestion_stats) *
			CTRL_PATH_STATS_MAX_VDEV_ID);
	if (!ev->vdev_congestion_stats)
		return QDF_STATUS_E_NOMEM;

	ev->vdev_data_stats =
			qdf_mem_malloc(sizeof(*ev->vdev_data_stats) *
			CTRL_PATH_STATS_MAX_VDEV_ID);
	if (!ev->vdev_data_stats)
		return QDF_STATUS_E_NOMEM;

	return QDF_STATUS_SUCCESS;
}
#ifdef CONFIG_WLAN_BMISS

/**
 * target_if_infra_cp_stats_bmiss_event_free() - Free event buffer
 * @ev: pointer to infra cp stats event structure
 *
 * Return: None
 */
static
void target_if_infra_cp_stats_bmiss_event_free(struct infra_cp_stats_event *ev)
{
	qdf_mem_free(ev->bmiss_infra_cp_stats);
	ev->bmiss_infra_cp_stats = NULL;
}

/**
 * target_if_infra_cp_stats_bmiss_event_alloc() - Allocate buffer for bmiss
 * parameters
 * @ev: pointer to infra cp stats event structure
 *
 * Return: QDF_STATUS_SUCCESS on Success, other QDF_STATUS error codes on
 * failure
 */
static QDF_STATUS
target_if_infra_cp_stats_bmiss_event_alloc(struct infra_cp_stats_event *ev)
{
	ev->bmiss_infra_cp_stats =
	qdf_mem_malloc(sizeof(*ev->bmiss_infra_cp_stats));
	if (!ev->bmiss_infra_cp_stats) {
		cp_stats_err("mem alloc failed for ev.bmiss_infra_cp_stats");
		return QDF_STATUS_E_NOMEM;
	}

	return QDF_STATUS_SUCCESS;
}
#else

static inline
void target_if_infra_cp_stats_bmiss_event_free(struct infra_cp_stats_event *ev)
{
}

static inline QDF_STATUS
target_if_infra_cp_stats_bmiss_event_alloc(struct infra_cp_stats_event *ev)
{
	return QDF_STATUS_SUCCESS;
}
#endif /* CONFIG_WLAN_BMISS */

/* Free memory for vdev_beacon_stats_event */
static void
target_if_infra_cp_stats_enchance_event_free(struct infra_cp_stats_event *ev)
{
	if (ev->vdev_beacon_stats) {
		qdf_mem_free(ev->vdev_beacon_stats);
		ev->vdev_beacon_stats = NULL;
	}

	if (ev->vdev_congestion_stats) {
		qdf_mem_free(ev->vdev_congestion_stats);
		ev->vdev_congestion_stats = NULL;
	}

	if (ev->vdev_data_stats) {
		qdf_mem_free(ev->vdev_data_stats);
		ev->vdev_data_stats = NULL;
	}
}

/**
 * target_if_infra_cp_stats_event_free() - Free event buffer
 * @ev: pointer to infra cp stats event structure
 *
 * Return : None
 */
static
void target_if_infra_cp_stats_event_free(struct infra_cp_stats_event *ev)
{
	target_if_infra_cp_stats_twt_event_free(ev);
	target_if_infra_cp_stats_bmiss_event_free(ev);
	target_if_infra_cp_stats_rrm_sta_stats_event_free(ev);
	target_if_infra_cp_stats_enchance_event_free(ev);
}

/**
 * target_if_infra_cp_stats_event_alloc() - Allocate buffer for event
 * parameters
 * @ev: pointer to infra cp stats event structure
 *
 * Return: QDF_STATUS_SUCCESS on Success, other QDF_STATUS error codes on
 * failure
 */
static QDF_STATUS
target_if_infra_cp_stats_event_alloc(struct infra_cp_stats_event *ev)
{
	QDF_STATUS status;

	status = target_if_infra_cp_stats_twt_event_alloc(ev);
	if (QDF_IS_STATUS_ERROR(status))
		return QDF_STATUS_E_NOMEM;

	status = target_if_infra_cp_stats_bmiss_event_alloc(ev);
	if (QDF_IS_STATUS_ERROR(status))
		return QDF_STATUS_E_NOMEM;

	status = target_if_infra_cp_stats_rrm_sta_stats_event_alloc(ev);
	if (QDF_IS_STATUS_ERROR(status))
		return QDF_STATUS_E_NOMEM;

	status = target_if_infra_cp_stats_enchance_stats_event_alloc(ev);
	if (QDF_IS_STATUS_ERROR(status))
		return QDF_STATUS_E_NOMEM;

	return QDF_STATUS_SUCCESS;
}

/**
 * target_if_extract_infra_cp_stats_event() - Extract data from stats event
 * @wmi_hdl: WMI Handle
 * @data: pointer to event data buffer from firmware
 * @data_len: length of the data buffer
 * @ev: pointer of output structure to be filled with extracted values
 *
 * Return: QDF_STATUS_SUCCESS on Success, other QDF_STATUS error codes
 * on failure
 */
static QDF_STATUS
target_if_extract_infra_cp_stats_event(struct wmi_unified *wmi_hdl,
				       uint8_t *data, uint32_t data_len,
				       struct infra_cp_stats_event *ev)
{
	QDF_STATUS status;
	uint32_t more_flag = 0;

	status = wmi_unified_extract_cp_stats_more_pending(wmi_hdl, data,
							   &more_flag);

	status = wmi_unified_extract_infra_cp_stats(wmi_hdl, data,
						    data_len, ev);

	cp_stats_debug("request_id %d", ev->request_id);

	return QDF_STATUS_SUCCESS;
}

/**
 * target_if_infra_cp_stats_event_handler() - Handle
 * wmi_pdev_cp_fwstats_eventid
 * @scn: opaque scn handle
 * @data: event buffer received from fw
 * @datalen: length of event buffer
 *
 * Return: 0 for success or non zero error codes for failure
 */
static
int target_if_infra_cp_stats_event_handler(ol_scn_t scn, uint8_t *data,
					   uint32_t datalen)
{
	QDF_STATUS status;
	struct infra_cp_stats_event ev = {0};
	struct wlan_objmgr_psoc *psoc;
	struct wmi_unified *wmi_handle;
	struct wlan_lmac_if_cp_stats_rx_ops *rx_ops;

	cp_stats_debug("Enter");

	if (!scn || !data) {
		cp_stats_err("scn: 0x%pK, data: 0x%pK", scn, data);
		return -EINVAL;
	}

	psoc = target_if_get_psoc_from_scn_hdl(scn);
	if (!psoc) {
		cp_stats_err("null psoc");
		return -EINVAL;
	}

	rx_ops = target_if_cp_stats_get_rx_ops(psoc);
	if (!rx_ops || !rx_ops->process_stats_event) {
		cp_stats_err("callback not registered");
		return -EINVAL;
	}

	wmi_handle = get_wmi_unified_hdl_from_psoc(psoc);
	if (!wmi_handle) {
		cp_stats_err("wmi_handle is null");
		return -EINVAL;
	}
	status = target_if_infra_cp_stats_event_alloc(&ev);
	if (QDF_IS_STATUS_ERROR(status)) {
		cp_stats_err("Alloc event mem failed");
		goto end;
	}

	status = target_if_extract_infra_cp_stats_event(wmi_handle, data,
							datalen, &ev);
	if (QDF_IS_STATUS_ERROR(status)) {
		cp_stats_err("extract event failed");
		goto end;
	}

	status = rx_ops->process_infra_stats_event(psoc, &ev);

end:
	target_if_infra_cp_stats_event_free(&ev);
	return qdf_status_to_os_return(status);
}

QDF_STATUS
target_if_cp_stats_send_coex_stats_req(struct wlan_objmgr_psoc *psoc)
{
	struct wmi_unified *wmi_handle;

	wmi_handle = get_wmi_unified_hdl_from_psoc(psoc);
	if (!wmi_handle) {
		target_if_err("Invalid WMI handle");
		return QDF_STATUS_E_INVAL;
	}

	return wmi_unified_coex_get_policy_stats_cmd_send(wmi_handle);
}

/*
 * TAS GET_METRICS log buffer sizing:
 *
 * Header: "TAS GET_METRICS: num_chains=%u time_window=%u"
 *   - literal: 44 chars, num_chains: 2 digits, time_window: 10 digits -> 60
 *
 * Per-chain entry: " [%u: no=%u band=%u region=%u]"
 *   - index: 2 digits, chain_no: 10 digits, band: 1 digit, region: 1 digit,
 *     literal: 24 chars -> 40 per chain
 */
#define TAS_METRICS_LOG_HDR_LEN   60
#define TAS_METRICS_LOG_CHAIN_LEN 40
#define TAS_METRICS_LOG_BUF_LEN(n) \
	(TAS_METRICS_LOG_HDR_LEN + (n) * TAS_METRICS_LOG_CHAIN_LEN)

/*
 * TAS GET_PLIMIT log buffer sizing:
 *
 * Header: "TAS GET_PLIMIT: dsi_id=%u num_chains=%u"
 *   - literal: 40 chars, dsi_id: 10 digits, num_chains: 2 digits -> 52 -> 60
 *
 * Per-chain entry: " [%u: no=%u band=%u power=%d]"
 *   - index: 2 digits, chain_no: 2 digits, band: 1 digit, power: 5 digits,
 *     literal: 22 chars -> 32 per chain
 *
 * Total for max 24 chains: 60 + 24 * 32 = 828 -> capped at 512 via
 * qdf_scnprintf truncation for large chain counts.
 */
#define TAS_PLIMIT_LOG_HDR_LEN   60
#define TAS_PLIMIT_LOG_CHAIN_LEN 32
#define TAS_PLIMIT_LOG_BUF_LEN(n) \
	QDF_MIN(512, TAS_PLIMIT_LOG_HDR_LEN + (n) * TAS_PLIMIT_LOG_CHAIN_LEN)

QDF_STATUS
target_if_cp_stats_send_tas_mode(struct wlan_objmgr_psoc *psoc,
				 enum host_tas_direction direction)
{
	struct wmi_unified *wmi_handle;

	wmi_handle = get_wmi_unified_hdl_from_psoc(psoc);
	if (!wmi_handle) {
		cp_stats_err("Invalid WMI handle");
		return QDF_STATUS_E_INVAL;
	}

	return wmi_unified_send_modify_tx_plim_cmd(wmi_handle, direction);
}

QDF_STATUS
target_if_cp_stats_send_get_avg_tx_power(struct wlan_objmgr_psoc *psoc,
					 uint32_t dsi_id)
{
	struct wmi_unified *wmi_handle;

	wmi_handle = get_wmi_unified_hdl_from_psoc(psoc);
	if (!wmi_handle) {
		cp_stats_err("Invalid WMI handle");
		return QDF_STATUS_E_INVAL;
	}

	return wmi_unified_send_get_avg_tx_power_cmd(wmi_handle, dsi_id);
}

QDF_STATUS
target_if_cp_stats_send_get_tx_power_calling(struct wlan_objmgr_psoc *psoc,
					     uint32_t dsi_id)
{
	struct wmi_unified *wmi_handle;

	wmi_handle = get_wmi_unified_hdl_from_psoc(psoc);
	if (!wmi_handle) {
		cp_stats_err("Invalid WMI handle");
		return QDF_STATUS_E_INVAL;
	}

	return wmi_unified_send_get_tx_power_calling_cmd(wmi_handle, dsi_id);
}

static int
target_if_cp_stats_plimit_table_event_handler(ol_scn_t scn, uint8_t *data,
					      uint32_t datalen)
{
	QDF_STATUS status;
	struct wmi_unified *wmi_handle;
	struct wlan_objmgr_psoc *psoc;
	struct request_info last_req = {0};
	uint32_t fw_status, dsi_id, num_chains;
	wmi_tx_power_per_antenna_chain *chain_data = NULL;
	bool pending = false;
	struct wlan_tas_plimit_event ev = {0};
	char *buf;
	uint32_t i;

	if (!scn || !data) {
		cp_stats_err("scn: 0x%pK, data: 0x%pK", scn, data);
		return -EINVAL;
	}

	psoc = target_if_get_psoc_from_scn_hdl(scn);
	if (!psoc) {
		cp_stats_err("psoc is NULL");
		return -EINVAL;
	}

	wmi_handle = get_wmi_unified_hdl_from_psoc(psoc);
	if (!wmi_handle) {
		cp_stats_err("wmi_handle is NULL");
		return -EINVAL;
	}

	status = wmi_unified_extract_plimit_table_event(wmi_handle, data,
							&fw_status, &dsi_id,
							&chain_data,
							&num_chains);

	if (QDF_IS_STATUS_ERROR(status)) {
		cp_stats_err("Failed to extract plimit table event");
		return -EINVAL;
	}

	status = ucfg_mc_cp_stats_get_pending_req(psoc,
						  TYPE_TAS_CURRENT_PLIMIT,
						  &last_req);
	if (QDF_IS_STATUS_ERROR(status)) {
		cp_stats_err("Failed to get pending TAS plimit request");
		return -EINVAL;
	}

	ucfg_mc_cp_stats_reset_pending_req(psoc, TYPE_TAS_CURRENT_PLIMIT,
					   &last_req, &pending);

	if (pending && last_req.u.get_tas_current_plimit_cb) {
		ev.fw_status = fw_status;
		ev.dsi_id = dsi_id;
		if (chain_data) {
			ev.num_chains = QDF_MIN(num_chains,
						WLAN_TAS_MAX_CHAINS);
			buf = qdf_mem_malloc(TAS_PLIMIT_LOG_BUF_LEN(
							ev.num_chains));
			if (buf) {
				int buf_pos;
				int buf_len = TAS_PLIMIT_LOG_BUF_LEN(
							ev.num_chains);

				buf_pos = qdf_scnprintf(
						buf, buf_len,
						"TAS GET_PLIMIT: dsi_id=%u num_chains=%u",
						ev.dsi_id, ev.num_chains);
				for (i = 0; i < ev.num_chains; i++) {
					ev.chains[i].chain_no =
						chain_data[i].chain_no;
					ev.chains[i].chain_operating_band =
						chain_data[i].chain_operating_band;
					ev.chains[i].power_limit_dbm =
						chain_data[i].power;
					if (buf_pos < buf_len)
						buf_pos += qdf_scnprintf(
							buf + buf_pos,
							buf_len - buf_pos,
							" [%u: no=%u band=%u power=%d]",
							i,
							ev.chains[i].chain_no,
							ev.chains[i].chain_operating_band,
							ev.chains[i].power_limit_dbm);
				}
				cp_stats_debug("%s", buf);
				qdf_mem_free(buf);
			} else {
				for (i = 0; i < ev.num_chains; i++) {
					ev.chains[i].chain_no =
						chain_data[i].chain_no;
					ev.chains[i].chain_operating_band =
						chain_data[i].chain_operating_band;
					ev.chains[i].power_limit_dbm =
						chain_data[i].power;
				}
			}
		}
		last_req.u.get_tas_current_plimit_cb(&ev, last_req.cookie);
	}

	return 0;
}

static int
target_if_cp_stats_avg_tx_power_event_handler(ol_scn_t scn, uint8_t *data,
					      uint32_t datalen)
{
	QDF_STATUS status;
	struct wmi_unified *wmi_handle;
	struct wlan_objmgr_psoc *psoc;
	struct request_info last_req = {0};
	uint32_t fw_status, time_window_in_sec, num_chains;
	wmi_avg_tx_power_region_per_antenna_chain *chain_data = NULL;
	bool pending = false;
	struct wlan_tas_metrics_event ev = {0};
	char *buf;
	uint32_t i;

	if (!scn || !data) {
		cp_stats_err("scn: 0x%pK, data: 0x%pK", scn, data);
		return -EINVAL;
	}

	psoc = target_if_get_psoc_from_scn_hdl(scn);
	if (!psoc) {
		cp_stats_err("psoc is NULL");
		return -EINVAL;
	}

	wmi_handle = get_wmi_unified_hdl_from_psoc(psoc);
	if (!wmi_handle) {
		cp_stats_err("wmi_handle is NULL");
		return -EINVAL;
	}

	status = wmi_unified_extract_avg_tx_power_event(
						wmi_handle, data,
						&fw_status,
						&time_window_in_sec,
						&chain_data, &num_chains);

	if (QDF_IS_STATUS_ERROR(status)) {
		cp_stats_err("Failed to extract avg tx power event");
		return -EINVAL;
	}

	status = ucfg_mc_cp_stats_get_pending_req(psoc, TYPE_TAS_METRICS,
						  &last_req);
	if (QDF_IS_STATUS_ERROR(status)) {
		cp_stats_err("Failed to get pending TAS metrics request");
		return -EINVAL;
	}

	ucfg_mc_cp_stats_reset_pending_req(psoc, TYPE_TAS_METRICS,
					   &last_req, &pending);

	if (pending && last_req.u.get_tas_metrics_cb) {
		ev.fw_status = fw_status;
		ev.time_window_in_sec = time_window_in_sec;
		if (chain_data) {
			ev.num_chains =
				QDF_MIN(num_chains, WLAN_TAS_MAX_CHAINS);
			buf = qdf_mem_malloc(TAS_METRICS_LOG_BUF_LEN(
								ev.num_chains));
			if (buf) {
				int buf_pos;
				int buf_len = TAS_METRICS_LOG_BUF_LEN(
								ev.num_chains);

				buf_pos = qdf_scnprintf(buf, buf_len,
							"TAS GET_METRICS: num_chains=%u time_window=%u",
							ev.num_chains,
							ev.time_window_in_sec);
				for (i = 0; i < ev.num_chains; i++) {
					ev.chains[i].chain_no =
						chain_data[i].chain_no;
					ev.chains[i].chain_operating_band =
						chain_data[i].chain_operating_band;
					ev.chains[i].chain_power_region =
						chain_data[i].chain_power_region;
					if (buf_pos < buf_len)
						buf_pos += qdf_scnprintf(
						buf + buf_pos,
						buf_len - buf_pos,
						" [%u: no=%u band=%u region=%u]",
						i,
						ev.chains[i].chain_no,
						ev.chains[i].chain_operating_band,
						ev.chains[i].chain_power_region);
				}
				cp_stats_debug("%s", buf);
				qdf_mem_free(buf);
			} else {
				for (i = 0; i < ev.num_chains; i++) {
					ev.chains[i].chain_no = chain_data[i].chain_no;
					ev.chains[i].chain_operating_band =
						chain_data[i].chain_operating_band;
					ev.chains[i].chain_power_region =
						chain_data[i].chain_power_region;
				}
			}
		}
		last_req.u.get_tas_metrics_cb(&ev, last_req.cookie);
	}

	return 0;
}

static int
target_if_cp_stats_tas_event_handler(ol_scn_t scn, uint8_t *data,
				     uint32_t datalen)
{
	QDF_STATUS status;
	struct wmi_unified *wmi_handle;
	struct wlan_objmgr_psoc *psoc;
	uint32_t fw_status;

	if (!scn || !data) {
		cp_stats_err("scn: 0x%pK, data: 0x%pK", scn, data);
		return -EINVAL;
	}

	psoc = target_if_get_psoc_from_scn_hdl(scn);
	if (!psoc) {
		cp_stats_err("psoc is NULL");
		return -EINVAL;
	}

	wmi_handle = get_wmi_unified_hdl_from_psoc(psoc);
	if (!wmi_handle) {
		cp_stats_err("wmi_handle is NULL");
		return -EINVAL;
	}

	status = wmi_unified_extract_modify_tx_plim_event(wmi_handle,
							  data, &fw_status);
	if (QDF_IS_STATUS_ERROR(status)) {
		cp_stats_err("Failed to extract TAS mode event");
		return -EINVAL;
	}

	cp_stats_debug("TAS mode set status from FW: %u", fw_status);

	return 0;
}

QDF_STATUS
target_if_cp_stats_get_coex_stats(struct wlan_objmgr_vdev *vdev)
{
	struct wlan_objmgr_psoc *psoc;

	if (!vdev)
		return QDF_STATUS_E_INVAL;

	psoc = wlan_vdev_get_psoc(vdev);
	if (!psoc)
		return QDF_STATUS_E_INVAL;

	return target_if_cp_stats_send_coex_stats_req(psoc);
}
#else
static
int target_if_infra_cp_stats_event_handler(ol_scn_t scn, uint8_t *data,
					   uint32_t datalen)
{
	return 0;
}
#endif /* WLAN_SUPPORT_INFRA_CTRL_PATH_STATS */

#ifdef WLAN_FEATURE_QSH_SCAN
/**
 * target_if_scan_stats_event_handler() - Handle WMI_GET_SCAN_STATS_RESP_EVENTID
 * @scn: opaque scn handle
 * @data: event buffer received from fw
 * @datalen: length of event buffer
 *
 * Return: 0 for success or non zero error codes for failure
 */
static int target_if_scan_stats_event_handler(ol_scn_t scn, uint8_t *data,
					      uint32_t datalen)
{
	QDF_STATUS status;
	struct wlan_objmgr_psoc *psoc;
	struct wmi_unified *wmi_handle;
	struct wmi_scan_stats_event event = {0};
	struct qsh_stats_event qsh_ev = {0};
	struct wlan_lmac_if_cp_stats_rx_ops *rx_ops;

	cp_stats_debug("Scan stats event received");

	if (!scn || !data) {
		cp_stats_err("scn: 0x%pK, data: 0x%pK", scn, data);
		return -EINVAL;
	}

	psoc = target_if_get_psoc_from_scn_hdl(scn);
	if (!psoc) {
		cp_stats_err("null psoc");
		return -EINVAL;
	}

	wmi_handle = get_wmi_unified_hdl_from_psoc(psoc);
	if (!wmi_handle) {
		cp_stats_err("wmi_handle is null");
		return -EINVAL;
	}

	/* Extract event data */
	status =
		wmi_unified_extract_scan_stats_event(wmi_handle, data, &event);
	if (QDF_IS_STATUS_ERROR(status)) {
		cp_stats_err("Failed to extract scan stats event");
		return qdf_status_to_os_return(status);
	}

	qsh_ev.scan_req_id = event.scan_req_id;
	qsh_ev.scan_count = event.scan_count;

	cp_stats_debug("Scan stats: scan_req_id=%u scan_count=%u",
		       event.scan_req_id, event.scan_count);

	/* Get RX ops and call process function */
	rx_ops = target_if_cp_stats_get_rx_ops(psoc);
	if (!rx_ops || !rx_ops->process_scan_stats_event) {
		cp_stats_err("QSH scan stats callback not registered");
		return -EINVAL;
	}

	status = rx_ops->process_scan_stats_event(psoc, &qsh_ev);

	return qdf_status_to_os_return(status);
}
#endif

#ifdef WLAN_FEATURE_LL_LT_SAP
/**
 * target_if_ll_sap_twt_session_params() - TWT session params for LL_SAP
 * @psoc: psoc object
 * @evt_buf: event buf
 * @params: pointer to twt_session_stats_event_param structure
 * @twt_params: pointer to twt_session_stats_info structure
 *
 * Return: QDF_STATUS
 */
static QDF_STATUS
target_if_ll_sap_twt_session_params(
			struct wlan_objmgr_psoc *psoc,
			uint8_t *evt_buf,
			struct twt_session_stats_event_param *params,
			struct twt_session_stats_info *twt_params)
{
	if (target_if_ll_sap_is_twt_event_type_query_rsp(
						psoc, evt_buf,
						params, twt_params)) {
		target_if_ll_sap_continue_csa_after_tsf_rsp(
						psoc, twt_params);
		return QDF_STATUS_SUCCESS;
	}

	return QDF_STATUS_E_INVAL;
}
#else
static inline QDF_STATUS
target_if_ll_sap_twt_session_params(
			struct wlan_objmgr_psoc *psoc,
			uint8_t *evt_buf,
			struct twt_session_stats_event_param *params,
			struct twt_session_stats_info *twt_params)
{
	return QDF_STATUS_E_NOSUPPORT;
}
#endif

/**
 * target_if_coex_policy_stats_event_handler() - Coex policy stats event handler
 * @scn: scn handle
 * @evt_buf: event buffer
 * @evt_data_len: event data length
 *
 * This function handles the coexistence policy statistics event from firmware
 * and forwards it to the CP stats layer for further processing.
 *
 * Return: 0 on success, error code otherwise
 */
static int
target_if_coex_policy_stats_event_handler(ol_scn_t scn,
					  uint8_t *evt_buf,
					  uint32_t evt_data_len)
{
	struct wlan_objmgr_psoc *psoc;
	struct wmi_unified *wmi_hdl;
	struct wmi_coex_policy_stats_event_param event_param = {0};
	struct request_info last_req = {0};
	bool pending = false;
	struct wlan_coex_policy_stats stats = {0};
	QDF_STATUS status;

	TARGET_IF_ENTER();

	if (!scn || !evt_buf) {
		target_if_err("scn: 0x%pK, evt_buf: 0x%pK", scn, evt_buf);
		return -EINVAL;
	}

	psoc = target_if_get_psoc_from_scn_hdl(scn);
	if (!psoc) {
		target_if_err("psoc object is null!");
		return -EINVAL;
	}

	wmi_hdl = get_wmi_unified_hdl_from_psoc(psoc);
	if (!wmi_hdl) {
		target_if_err("wmi_handle is null!");
		return -EINVAL;
	}

	status = wmi_extract_coex_policy_stats_event(wmi_hdl, evt_buf,
						     &event_param);
	if (QDF_IS_STATUS_ERROR(status)) {
		target_if_err("Failed to extract coex policy stats event. status = %d",
			      status);
		return qdf_status_to_os_return(status);
	}

	/* Get pending request info */
	status = ucfg_mc_cp_stats_get_pending_req(psoc, TYPE_COEX_STATS,
						  &last_req);
	if (QDF_IS_STATUS_ERROR(status)) {
		target_if_err("Failed to get pending coex stats request. status = %d",
			      status);
		return qdf_status_to_os_return(status);
	}

	/* Copy WMI event params into the public coex stats struct */
	stats.btc_policy           = event_param.btc_policy;
	stats.mws_policy           = event_param.mws_policy;
	stats.uwb_policy           = event_param.uwb_policy;
	stats.monitoring_period    = event_param.monitoring_period;
	stats.ocs_active_percent   = event_param.ocs_active_percent;
	stats.ocs_non_wlan_percent = event_param.ocs_non_wlan_percent;

	/* Reset pending request (coex stats is single-event) */
	ucfg_mc_cp_stats_reset_pending_req(psoc, TYPE_COEX_STATS,
					   &last_req, &pending);
	/* Invoke callback if pending */
	if (pending && last_req.u.get_coex_stats_cb)
		last_req.u.get_coex_stats_cb(&stats, last_req.cookie);

	TARGET_IF_EXIT();
	return 0;
}
#if defined(WLAN_SUPPORT_TWT) && defined(WLAN_TWT_CONV_SUPPORTED)
static int
target_if_twt_session_params_event_handler(ol_scn_t scn,
					   uint8_t *evt_buf,
					   uint32_t evt_data_len)
{
	struct wlan_objmgr_psoc *psoc;
	struct wlan_objmgr_peer *peer_obj;
	struct wmi_unified *wmi_hdl;
	struct twt_session_stats_info twt_params;
	struct twt_session_stats_event_param params = {0};
	struct peer_cp_stats *peer_cp_stats;
	int i;
	QDF_STATUS status;
	uint32_t ev;
	cdp_config_param_type val = {0};
	ol_txrx_soc_handle soc_txrx_handle;
	struct wlan_lmac_if_rx_ops *rx_ops;

	TARGET_IF_ENTER();

	if (!scn || !evt_buf) {
		target_if_err("scn: 0x%pK, evt_buf: 0x%pK", scn, evt_buf);
		return -EINVAL;
	}

	psoc = target_if_get_psoc_from_scn_hdl(scn);
	if (!psoc) {
		target_if_err("psoc object is null!");
		return -EINVAL;
	}

	soc_txrx_handle = wlan_psoc_get_dp_handle(psoc);
	if (!soc_txrx_handle)
		return -EINVAL;

	wmi_hdl = get_wmi_unified_hdl_from_psoc(psoc);
	if (!wmi_hdl) {
		target_if_err("wmi_handle is null!");
		return -EINVAL;
	}

	rx_ops = wlan_psoc_get_lmac_if_rxops(psoc);
	if (!rx_ops) {
		target_if_err("No valid twt session stats rx ops");
		return -EINVAL;
	}

	status = wmi_extract_twt_session_stats_event(wmi_hdl, evt_buf, &params);
	if (QDF_IS_STATUS_ERROR(status)) {
		target_if_err("Could not extract twt session stats event");
		return qdf_status_to_os_return(status);
	}

	if (params.num_sessions > WLAN_MAX_TWT_SESSIONS_PER_PEER) {
		target_if_err("Number of twt sessions exceeded, num:%d max:%d",
			      params.num_sessions, WLAN_MAX_TWT_SESSIONS_PER_PEER);
		return -EINVAL;
	}

	status = target_if_ll_sap_twt_session_params(
						psoc, evt_buf,
						&params, &twt_params);
	if (QDF_IS_STATUS_SUCCESS(status))
		return 0;

	for (i = 0; i < params.num_sessions; i++) {
		status = wmi_extract_twt_session_stats_data(wmi_hdl, evt_buf,
							    &params,
							    &twt_params, i);

		if (QDF_IS_STATUS_ERROR(status)) {
			target_if_err("Unable to extract twt params for idx %d",
				      i);
			return -EINVAL;
		}
		peer_obj = wlan_objmgr_get_peer_by_mac(psoc,
						twt_params.peer_mac.bytes,
						WLAN_CP_STATS_ID);
		if (!peer_obj) {
			target_if_err("peer obj not found for "QDF_MAC_ADDR_FMT,
				      QDF_MAC_ADDR_REF(twt_params.peer_mac.bytes));
			continue;
		}

		ev = twt_params.event_type;
		if (ev == HOST_TWT_SESSION_SETUP)
			val.cdp_peer_param_in_twt = 1;
		else if (ev == HOST_TWT_SESSION_TEARDOWN)
			val.cdp_peer_param_in_twt = 0;

		cdp_txrx_set_peer_param(soc_txrx_handle, twt_params.vdev_id,
					twt_params.peer_mac.bytes,
					CDP_CONFIG_IN_TWT, val);

		peer_cp_stats = wlan_cp_stats_get_peer_stats_obj(peer_obj);
		if (!peer_cp_stats) {
			target_if_err("peer_cp_stats is null");
			continue;
		}

		wlan_cp_stats_peer_obj_lock(peer_cp_stats);

		rx_ops->cp_stats_rx_ops.twt_get_session_param_resp(psoc,
								 &twt_params);

		wlan_cp_stats_peer_obj_unlock(peer_cp_stats);
		wlan_objmgr_peer_release_ref(peer_obj, WLAN_CP_STATS_ID);
	}
	return 0;
}

static QDF_STATUS
target_if_cp_stats_register_twt_session_event(struct wmi_unified *wmi_handle)
{
	QDF_STATUS ret_val;

	ret_val = wmi_unified_register_event_handler(wmi_handle,
				wmi_twt_session_stats_event_id,
				target_if_twt_session_params_event_handler,
				WMI_RX_WORK_CTX);

	return ret_val;
}

static void
target_if_cp_stats_unregister_twt_session_event(struct wmi_unified *wmi_handle)
{
	wmi_unified_unregister_event_handler(wmi_handle,
					     wmi_twt_session_stats_event_id);
}
#else
static QDF_STATUS
target_if_cp_stats_register_twt_session_event(struct wmi_unified *wmi_handle)
{
	return QDF_STATUS_SUCCESS;
}

static void
target_if_cp_stats_unregister_twt_session_event(struct wmi_unified *wmi_handle)
{
}
#endif /*  WLAN_SUPPORT_TWT && WLAN_TWT_CONV_SUPPORTED*/

#ifdef WLAN_SUPPORT_INFRA_CTRL_PATH_STATS
static QDF_STATUS
target_if_cp_stats_infra_register_event_handler(struct wlan_objmgr_psoc *psoc,
						struct wmi_unified *wmi_handle)
{
	QDF_STATUS ret_val;

	if (!psoc) {
		cp_stats_err("PSOC is NULL!");
		return QDF_STATUS_E_INVAL;
	}

	if (!wmi_handle) {
		cp_stats_err("wmi_handle is null");
		return QDF_STATUS_E_INVAL;
	}

	ret_val = wmi_unified_register_event_handler(wmi_handle,
						     wmi_pdev_cp_fwstats_eventid,
						     target_if_infra_cp_stats_event_handler,
						     WMI_RX_WORK_CTX);
	if (QDF_IS_STATUS_ERROR(ret_val)) {
		cp_stats_err("Failed to register for pdev_cp_fwstats_event");
		return ret_val;
	}

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS
target_if_cp_stats_register_coex_stats_event(struct wmi_unified *wmi_handle)
{
	QDF_STATUS ret_val;

	ret_val = wmi_unified_register_event_handler(
				wmi_handle,
				wmi_coex_get_policy_stats_event_id,
				target_if_coex_policy_stats_event_handler,
				WMI_RX_WORK_CTX);

	return ret_val;
}

static void
target_if_cp_stats_unregister_coex_stats_event(struct wmi_unified *wmi_handle)
{
	wmi_unified_unregister_event_handler(
					wmi_handle,
					wmi_coex_get_policy_stats_event_id);
}
#else
static QDF_STATUS
target_if_cp_stats_infra_register_event_handler(struct wlan_objmgr_psoc *psoc,
						struct wmi_unified *wmi_handle)
{
	return QDF_STATUS_SUCCESS;
}

static inline QDF_STATUS
target_if_cp_stats_register_coex_stats_event(struct wmi_unified *wmi_handle)
{
	return QDF_STATUS_SUCCESS;
}

static inline void
target_if_cp_stats_unregister_coex_stats_event(struct wmi_unified *wmi_handle)
{
}
#endif /* WLAN_SUPPORT_INFRA_CTRL_PATH_STATS */

#ifdef WLAN_FEATURE_QSH_SCAN
static QDF_STATUS
target_if_cp_stats_register_qsh_event_handler(struct wmi_unified *wmi_handle)
{

	return wmi_unified_register_event_handler(wmi_handle,
						  wmi_get_scan_stats_resp_event_id,
						  target_if_scan_stats_event_handler,
						  WMI_RX_WORK_CTX);
}

static void
target_if_cp_stats_unregister_qsh_event_handler(struct wmi_unified *wmi_handle)
{
	wmi_unified_unregister_event_handler(wmi_handle,
					     wmi_get_scan_stats_resp_event_id);
}
#else
static QDF_STATUS
target_if_cp_stats_register_qsh_event_handler(struct wmi_unified *wmi_handle)
{
	return QDF_STATUS_SUCCESS;
}

static void
target_if_cp_stats_unregister_qsh_event_handler(struct wmi_unified *wmi_handle)
{
}
#endif /* WLAN_FEATURE_QSH_SCAN */

static QDF_STATUS
target_if_cp_stats_register_event_handler(struct wlan_objmgr_psoc *psoc)
{
	struct wmi_unified *wmi_handle;
	QDF_STATUS ret_val;

	if (!psoc) {
		cp_stats_err("PSOC is NULL!");
		return QDF_STATUS_E_INVAL;
	}

	wmi_handle = get_wmi_unified_hdl_from_psoc(psoc);
	if (!wmi_handle) {
		cp_stats_err("wmi_handle is null");
		return QDF_STATUS_E_INVAL;
	}

	ret_val = target_if_cp_stats_infra_register_event_handler(psoc,
								  wmi_handle);
	if (QDF_IS_STATUS_ERROR(ret_val)) {
		cp_stats_err("Failed to register for pdev_cp_fwstats_event");
		return ret_val;
	}

	ret_val = target_if_cp_stats_register_coex_stats_event(wmi_handle);
	if (QDF_IS_STATUS_ERROR(ret_val)) {
		cp_stats_err("Failed to register coex stats event");
		return ret_val;
	}

	ret_val = target_if_cp_stats_register_twt_session_event(wmi_handle);
	if (QDF_IS_STATUS_ERROR(ret_val)) {
		cp_stats_err("Failed to register twt session stats event");
		return ret_val;
	}

	ret_val = target_if_cp_stats_register_qsh_event_handler(wmi_handle);
	if (QDF_IS_STATUS_ERROR(ret_val)) {
		cp_stats_err("Failed to register scan stats event handler");
		return ret_val;
	}

	ret_val = wmi_unified_register_event_handler(
				wmi_handle,
				wmi_modify_tx_plim_event_id,
				target_if_cp_stats_tas_event_handler,
				WMI_RX_WORK_CTX);
	if (QDF_IS_STATUS_ERROR(ret_val)) {
		cp_stats_err("Failed to register TAS mode event handler");
		return ret_val;
	}

	ret_val = wmi_unified_register_event_handler(
				wmi_handle,
				wmi_avg_tx_power_event_id,
				target_if_cp_stats_avg_tx_power_event_handler,
				WMI_RX_WORK_CTX);
	if (QDF_IS_STATUS_ERROR(ret_val)) {
		cp_stats_err("Failed to register avg tx power event handler");
		return ret_val;
	}

	ret_val = wmi_unified_register_event_handler(
				wmi_handle,
				wmi_plimit_table_event_id,
				target_if_cp_stats_plimit_table_event_handler,
				WMI_RX_WORK_CTX);
	if (QDF_IS_STATUS_ERROR(ret_val)) {
		cp_stats_err("Failed to register plimit table event handler");
		return ret_val;
	}

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS
target_if_cp_stats_unregister_event_handler(struct wlan_objmgr_psoc *psoc)
{
	struct wmi_unified *wmi_handle;

	if (!psoc) {
		cp_stats_err("PSOC is NULL!");
		return QDF_STATUS_E_INVAL;
	}

	wmi_handle = get_wmi_unified_hdl_from_psoc(psoc);
	if (!wmi_handle) {
		cp_stats_err("wmi_handle is null");
		return QDF_STATUS_E_INVAL;
	}

	wmi_unified_unregister_event_handler(wmi_handle,
					     wmi_plimit_table_event_id);
	wmi_unified_unregister_event_handler(wmi_handle,
					     wmi_avg_tx_power_event_id);
	wmi_unified_unregister_event_handler(wmi_handle,
					     wmi_modify_tx_plim_event_id);
	wmi_unified_unregister_event_handler(wmi_handle,
					     wmi_pdev_cp_fwstats_eventid);
	target_if_cp_stats_unregister_coex_stats_event(wmi_handle);
	target_if_cp_stats_unregister_twt_session_event(wmi_handle);

	target_if_cp_stats_unregister_qsh_event_handler(wmi_handle);

	return QDF_STATUS_SUCCESS;
}

#ifdef WLAN_SUPPORT_INFRA_CTRL_PATH_STATS
/**
 * target_if_infra_cp_stats_req() - API to send stats request to wmi
 * @psoc: pointer to psoc object
 * @req: pointer to object containing stats request parameters
 *
 * Return: QDF_STATUS_SUCCESS on success, else other qdf error values
 */
static
QDF_STATUS target_if_infra_cp_stats_req(struct wlan_objmgr_psoc *psoc,
					struct infra_cp_stats_cmd_info *req)

{
	struct wmi_unified *wmi_handle;

	wmi_handle = get_wmi_unified_hdl_from_psoc(psoc);
	if (!wmi_handle) {
		cp_stats_err("wmi_handle is null.");
		return QDF_STATUS_E_NULL_VALUE;
	}

	return wmi_unified_infra_cp_stats_request_send(wmi_handle, req);
}

static void target_if_register_infra_cp_stats_txops(
				struct wlan_lmac_if_cp_stats_tx_ops *tx_ops)
{
	tx_ops->send_req_infra_cp_stats = target_if_infra_cp_stats_req;
}
#else
static void target_if_register_infra_cp_stats_txops(
				struct wlan_lmac_if_cp_stats_tx_ops *tx_ops)
{
}
#endif /* WLAN_SUPPORT_INFRA_CTRL_PATH_STATS */

#ifdef WLAN_CONFIG_TELEMETRY_AGENT
/**
 * target_if_telemetry_cp_stats_req() - API to send stats request to wmi
 * @pdev: pointer to pdev object
 * @req: pointer to object containing stats request parameters
 *
 * Return: QDF_STATUS_SUCCESS on success, else other qdf error values
 */
static
QDF_STATUS target_if_telemetry_cp_stats_req(struct wlan_objmgr_pdev *pdev,
					    struct infra_cp_stats_cmd_info *req)
{
	struct wmi_unified *wmi_handle;

	wmi_handle = get_wmi_unified_hdl_from_pdev(pdev);
	if (!wmi_handle) {
		cp_stats_err("wmi_handle is null.");
		return QDF_STATUS_E_NULL_VALUE;
	}
	return wmi_unified_infra_cp_stats_request_send(wmi_handle, req);
}

static void target_if_register_telemetry_cp_stats_txops(
				struct wlan_lmac_if_cp_stats_tx_ops *tx_ops)
{
	tx_ops->send_req_telemetry_cp_stats = target_if_telemetry_cp_stats_req;
}
#else
static void target_if_register_telemetry_cp_stats_txops(
				struct wlan_lmac_if_cp_stats_tx_ops *tx_ops)
{ }
#endif
#ifdef WLAN_CHIPSET_STATS
QDF_STATUS
target_if_cp_stats_is_service_cstats_enabled(struct wlan_objmgr_psoc *psoc,
					     bool *is_fw_support_cstats)
{
	struct wmi_unified *wmi_handle;

	if (!psoc) {
		cp_stats_err("psoc is NULL!");
		return QDF_STATUS_E_INVAL;
	}

	wmi_handle = get_wmi_unified_hdl_from_psoc(psoc);
	if (!wmi_handle) {
		cp_stats_err("wmi_handle is null");
		return QDF_STATUS_E_NULL_VALUE;
	}

	*is_fw_support_cstats =
		wmi_service_enabled(wmi_handle,
				    wmi_service_chipset_logging_support);
	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS
target_if_cp_stats_enable_cstats(struct wlan_objmgr_psoc *psoc,
				 uint32_t param_val, uint8_t mac_id)
{
	struct wmi_unified *wmi_handle;
	struct pdev_params params = {0};

	wmi_handle = get_wmi_unified_hdl_from_psoc(psoc);
	if (!wmi_handle) {
		cp_stats_err("wmi_handle is null");
		return QDF_STATUS_E_NULL_VALUE;
	}

	params.param_id = WMI_PDEV_PARAM_ENABLE_CHIPSET_LOGGING;
	params.param_value = param_val;

	return wmi_unified_pdev_param_send(wmi_handle, &params, mac_id);
}

/**
 * target_if_register_cstats_enable_txops() - Register cstats enable in txops
 *
 * @ops: pointer to wlan_lmac_if_cp_stats_tx_ops
 *
 * Return: void
 */
static void
target_if_register_cstats_enable_txops(struct wlan_lmac_if_cp_stats_tx_ops *ops)
{
	ops->send_cstats_enable = target_if_cp_stats_enable_cstats;
}
#else
static void
target_if_register_cstats_enable_txops(struct wlan_lmac_if_cp_stats_tx_ops *ops)
{
}
#endif

QDF_STATUS
target_if_cp_stats_register_tx_ops(struct wlan_lmac_if_tx_ops *tx_ops)
{
	struct wlan_lmac_if_cp_stats_tx_ops *cp_stats_tx_ops;

	if (!tx_ops) {
		cp_stats_err("lmac tx ops is NULL!");
		return QDF_STATUS_E_INVAL;
	}

	cp_stats_tx_ops = &tx_ops->cp_stats_tx_ops;
	if (!cp_stats_tx_ops) {
		cp_stats_err("lmac tx ops is NULL!");
		return QDF_STATUS_E_FAILURE;
	}

	target_if_register_cstats_enable_txops(cp_stats_tx_ops);

	target_if_register_infra_cp_stats_txops(cp_stats_tx_ops);
	target_if_register_telemetry_cp_stats_txops(cp_stats_tx_ops);

	cp_stats_tx_ops->cp_stats_attach =
		target_if_cp_stats_register_event_handler;
	cp_stats_tx_ops->cp_stats_detach =
		target_if_cp_stats_unregister_event_handler;
	cp_stats_tx_ops->cp_stats_legacy_attach =
		target_if_cp_stats_register_legacy_event_handler;
	cp_stats_tx_ops->cp_stats_legacy_detach =
		target_if_cp_stats_unregister_legacy_event_handler;
	cp_stats_tx_ops->send_req_coex_stats =
		target_if_cp_stats_send_coex_stats_req;
	cp_stats_tx_ops->is_ctas_plim_indication_supported =
		target_if_is_ctas_plim_indication_supported;
	cp_stats_tx_ops->send_tas_mode =
		target_if_cp_stats_send_tas_mode;
	cp_stats_tx_ops->send_get_avg_tx_power =
		target_if_cp_stats_send_get_avg_tx_power;
	cp_stats_tx_ops->send_get_tx_power_calling =
		target_if_cp_stats_send_get_tx_power_calling;
	return QDF_STATUS_SUCCESS;
}

bool
target_if_is_ctas_plim_indication_supported(struct wlan_objmgr_psoc *psoc)
{
	struct wmi_unified *wmi_handle;

	wmi_handle = get_wmi_unified_hdl_from_psoc(psoc);
	if (!wmi_handle) {
		cp_stats_err("wmi_handle is null");
		return false;
	}

	return wmi_service_enabled(wmi_handle,
				   wmi_service_ctas_plim_indication_support);
}

