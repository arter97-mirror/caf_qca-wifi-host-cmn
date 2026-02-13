/*
 * Copyright (c) 2020-2021, The Linux Foundation. All rights reserved.
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
 * DOC: This file has main dcs structures definition.
 */

#ifndef _WLAN_DCS_H_
#define _WLAN_DCS_H_

#include <wmi_unified_param.h>
#include "wlan_dcs_tgt_api.h"
#include "wlan_dcs_ucfg_api.h"

#define dcs_debug(args ...) \
		QDF_TRACE_DEBUG(QDF_MODULE_ID_DCS, ## args)
#define dcs_info(args ...) \
		QDF_TRACE_INFO(QDF_MODULE_ID_DCS, ## args)
#define dcs_err(args ...) \
		QDF_TRACE_ERROR(QDF_MODULE_ID_DCS, ## args)
#define dcs_debug_rl(args ...) \
		QDF_TRACE_DEBUG_RL(QDF_MODULE_ID_DCS, ## args)

#define WLAN_DCS_MAX_PDEVS 3

/* Invalid VDEV ID marker for DCS tracking */
#define DCS_INVALID_VDEV_ID 0xFF

#define DCS_TX_MAX_CU  30
#define MAX_DCS_TIME_RECORD 10
#define DCS_FREQ_CONTROL_TIME (5 * 60 * 1000)

/**
 * enum wlan_dcs_mode - vdev operating mode for DCS
 * @DCS_SAP: General SAP mode except XPAN/XR
 * @DCS_XPAN: XPAN SAP mode
 * @DCS_XR: XR SAP mode
 * @DCS_GO: P2P GO mode
 * @MAX_DCS_MODE_NUM: Max place holder
 *
 * These are generic IDs that identify the various modes
 * in Dynamic Channel Selection.
 */
enum wlan_dcs_mode {
	DCS_SAP,
	DCS_XPAN,
	DCS_XR,
	DCS_GO,
	MAX_DCS_MODE_NUM,
};

/**
 * dcs_sanitize_quotes() - Sanitize quoted configuration string
 * @raw: pointer to raw configuration string
 *
 * This helper function removes leading and trailing quotes from a
 * configuration string returned by cfg_get(). It also performs basic
 * validation to ensure the sanitized string is non-empty and not a
 * duplicate/invalid entry.
 *
 * Return: pointer to sanitized string if valid, NULL otherwise
 */
static inline char *dcs_sanitize_quotes(char *raw)
{
	size_t len;

	if (!raw)
		return NULL;

	len = strlen(raw);
	if (len == 0)
		return NULL;

	if (raw[0] == '"') {
		raw++;
		len--;
	}

	if (len > 0 && raw[len - 1] == '"') {
		raw[len - 1] = '\0';
		len--;
	}

	if (len == 0)
		return NULL;

	return raw;
}

#define DCS_CFG_GET(psoc, CFG, type, array)                             \
do {                                                                    \
	uint8_t i;                                                      \
	QDF_STATUS status;                                              \
	type *arr_ = (array);                                           \
	void *psoc_ = (psoc);                                           \
	qdf_size_t out_size = 0;                                        \
	uint32_t out[MAX_DCS_MODE_NUM];                                 \
	char *raw;                                                      \
									\
	if (!arr_) {                                                    \
		dcs_err("%s NULL arr_", #CFG);                          \
		break;                                                  \
	}                                                               \
									\
	/* Initialize defaults */                                       \
	for (i = 0; i < MAX_DCS_MODE_NUM; i++)                          \
		arr_[i] = (type)(CFG_DCS_##CFG##_DEFAULT);              \
									\
	if (!psoc_) {                                                   \
		dcs_err("NULL psoc");                                   \
		break;                                                  \
	}                                                               \
	raw  = (char *)cfg_get(psoc_, CFG_DCS_##CFG);                   \
	if (!raw) {                                                     \
		dcs_err("%s raw=NULL", #CFG);                           \
		break;                                                  \
	}                                                               \
									\
	char *str = dcs_sanitize_quotes(raw);                           \
	if (!str) {                                                     \
		dcs_debug("%s NULL", #CFG);                             \
		break;                                                  \
	}                                                               \
									\
	/* Parse sanitized string */                                    \
	status = qdf_uint32_array_parse(str, out, MAX_DCS_MODE_NUM,     \
					&out_size);                     \
	if (QDF_IS_STATUS_ERROR(status) || out_size == 0) {             \
		dcs_err("%s=%s parse failed, use default",              \
			#CFG, raw);                                     \
		break;                                                  \
	}                                                               \
									\
	dcs_debug("%s=%s", #CFG, str);                                  \
									\
	/* Range check */                                               \
	for (i = 0; i < out_size; i++) {                                \
		if (out[i] < (CFG_DCS_##CFG##_MIN))                     \
			out[i] = (CFG_DCS_##CFG##_MIN);                 \
		else if (out[i] > (CFG_DCS_##CFG##_MAX))                \
			out[i] = (CFG_DCS_##CFG##_MAX);                 \
		arr_[i] = (type)out[i];                                 \
	}                                                               \
} while (0)

/**
 * struct wlan_dcs_type_bitmap - types of DCS interference bitmap
 * @cwim: continuous wave interference
 * @wlanim: wlan interference stats
 * @reserved: to be used in future
 */
struct wlan_dcs_type_bitmap {
	uint8_t cwim:1;
	uint8_t wlanim:1;
	uint8_t reserved:6;
};

union wlan_dcs_cfg {
	uint8_t val;
	struct wlan_dcs_type_bitmap bitmap;
};

/**
 * enum wlan_dcs_debug_level - dcs debug trace level
 * @DCS_DEBUG_DISABLE: disable debug trace
 * @DCS_DEBUG_CRITICAL: critical debug trace level
 * @DCS_DEBUG_VERBOSE:  verbose debug trace level
 */
enum wlan_dcs_debug_level {
	DCS_DEBUG_DISABLE = 0,
	DCS_DEBUG_CRITICAL = 1,
	DCS_DEBUG_VERBOSE = 2
};

/**
 * struct dcs_im_stats - define dcs interference mitigation stats
 * @prev_dcs_im_stats: previous statistics at last time
 * @user_dcs_im_stats: statistics requested from userspace
 * @dcs_ch_util_im_stats: chan utilization statistics
 * @im_intfr_cnt: number of times the interference is
 *                detected within detection window
 * @im_samp_cnt: sample counter
 */
struct dcs_im_stats {
	struct wlan_host_dcs_im_tgt_stats prev_dcs_im_stats;
	struct wlan_host_dcs_im_user_stats user_dcs_im_stats;
	struct wlan_host_dcs_ch_util_stats dcs_ch_util_im_stats;
	uint8_t im_intfr_cnt;
	uint8_t im_samp_cnt;
};

/**
 * struct core_dcs_params - define dcs configuration parameters
 * @dcs_enable_cfg: dcs enable from ini config
 * @dcs_enable: dcs enable from ucfg config
 * @dcs_algorithm_process: do dcs algorithm process or not
 * @force_disable_algorithm: disable dcs algorithm forcely
 * @dcs_debug: dcs debug trace level
 * @phy_err_penalty: phy error penalty
 * @phy_err_threshold: phy error threshold
 * @radar_err_threshold: radar error threshold
 * @coch_intfr_threshold: co-channel interference threshold
 * @dcs_trnsprt_rjt_threshold_cu: transport reject threshold cu
 * @user_max_cu: tx channel utilization due to AP's tx and rx
 * @intfr_detection_threshold: interference detection threshold
 * @intfr_detection_window: interference sampling window
 * @tx_err_threshold: transmission failure rate threshold
 * @user_request_count: counter of stats requested from userspace
 * @notify_user: whether to notify userspace
 */
struct core_dcs_params {
	uint8_t dcs_enable_cfg;
	uint8_t dcs_enable;
	bool dcs_algorithm_process;
	bool force_disable_algorithm;
	enum wlan_dcs_debug_level dcs_debug;
	uint32_t phy_err_penalty;
	uint32_t phy_err_threshold;
	uint32_t radar_err_threshold;
	uint32_t coch_intfr_threshold;
	uint32_t dcs_trnsprt_rjt_threshold_cu;
	uint32_t user_max_cu;
	uint32_t intfr_detection_threshold;
	uint32_t intfr_detection_window;
	uint32_t tx_err_threshold;
	uint32_t user_request_count;
	uint8_t notify_user;
};

/**
 * struct dcs_freq_ctrl_params - define dcs frequency control parameters
 * @disable_threshold_per_5mins: in five minutes, if dcs happen more than
 *                               threshold, then disable dcs for some time
 * @restart_delay: when dcs happen more than threshold in five minutes,
 *                 then start to disable dcs for restart_delay minutes
 * @timestamp: record dcs happened timestamp
 * @dcs_happened_count: dcs happened count
 * @disable_delay_process: in dcs disable delay process or not
 */
struct dcs_freq_ctrl_params {
	uint8_t disable_threshold_per_5mins;
	uint32_t restart_delay;
	unsigned long timestamp[MAX_DCS_TIME_RECORD];
	unsigned long dcs_happened_count;
	bool disable_delay_process;
};

/**
 * struct core_dcs_timer_args - define dcs core timer args
 * @psoc: psoc pointer
 * @pdev_id: pdev id
 * @vdev_id: vdev id
 */
struct core_dcs_timer_args {
	struct wlan_objmgr_psoc *psoc;
	uint8_t pdev_id;
	uint8_t vdev_id;
};

/**
 * struct psoc_dcs_cbk - define dcs callback in psoc object
 * @cbk: callback
 * @arg: arguments
 */
struct psoc_dcs_cbk {
	dcs_callback cbk;
	void *arg;
};

#define DCS_INVALID_PDEV_ID 0xFF
#define WLAN_DCS_MAX_STA_NUM  1
#define WLAN_DCS_MAX_SAP_NUM  2
#define WLAN_DCS_AFC_PREFER_BW  CH_WIDTH_80MHZ

/**
 * struct connection_chan_info - define connection channel information
 * @freq: channel frequency
 * @bw: channel bandwidth
 * @vdev_id: connection vdev id
 */
struct connection_chan_info {
	qdf_freq_t freq;
	enum phy_ch_width bw;
	uint8_t vdev_id;
};

/**
 * struct wlan_dcs_conn_info - define arguments list for DCS when AFC updated
 * @sta_cnt: station count
 * @sap_5ghz_cnt: 5 GHz sap count
 * @sap_6ghz_cnt: 6 GHz sap count
 * @sta: connection info of station
 * @sap_5ghz: connection info of 5 GHz sap
 * @sap_6ghz: connection info of 6 GHz sap
 * @exit_condition: flag to exit iteration immediately
 */
struct wlan_dcs_conn_info {
	uint8_t sta_cnt;
	uint8_t sap_5ghz_cnt;
	uint8_t sap_6ghz_cnt;
	struct connection_chan_info sta[WLAN_DCS_MAX_STA_NUM];
	struct connection_chan_info sap_5ghz[WLAN_DCS_MAX_SAP_NUM];
	struct connection_chan_info sap_6ghz[WLAN_DCS_MAX_SAP_NUM];
	bool exit_condition;
};

/**
 * struct dcs_afc_select_chan_cbk - define sap afc select channel callback
 * @cbk: callback
 * @arg: argument supply by register
 */
struct dcs_afc_select_chan_cbk {
	dcs_afc_select_chan_cb cbk;
	void *arg;
};

/**
 * struct dcs_core_priv_obj  - define dcs core priv
 * @dcs_host_params: dcs host configuration parameter
 * @dcs_im_stats: dcs im statistics
 * @dcs_freq_ctrl_params: dcs frequency control parameter
 * @dcs_disable_timer: dcs disable timer
 * @dcs_timer_args: dcs disable timer args
 * @lock: lock to protect dcs pdev priv
 * @requestor_vdev_id: user request vdev id
 * @user_cb: user request callback
 */
struct dcs_core_priv_obj {
	struct core_dcs_params dcs_host_params;
	struct dcs_im_stats dcs_im_stats;
	struct dcs_freq_ctrl_params dcs_freq_ctrl_params;
	qdf_timer_t dcs_disable_timer;
	struct core_dcs_timer_args dcs_timer_args;
	qdf_spinlock_t lock;
	uint8_t requestor_vdev_id;
	void (*user_cb)(uint8_t vdev_id,
			struct wlan_host_dcs_im_user_stats *stats,
			int status);
};

/**
 * struct dcs_pdev_priv_obj  - define dcs pdev priv
 * @dcs_core: Pointer to DCS core private object
 * @pdev_id: PDEV ID
 */
struct dcs_pdev_priv_obj {
	struct dcs_core_priv_obj *dcs_core;
	uint8_t pdev_id;
};

/**
 * struct dcs_vdev_priv_obj  - define dcs vdev priv
 * @dcs_core: Pointer to DCS core private object
 * @vdev: Pointer to VDEV object
 */
struct dcs_vdev_priv_obj {
	struct dcs_core_priv_obj *dcs_core;
	struct wlan_objmgr_vdev *vdev;
};

/**
 * enum wlan_dcs_chan_seg - Different segments in the channel band.
 * @WLAN_DCS_SEG_INVALID: invalid segment
 * @WLAN_DCS_SEG_PRI20: primary 20MHz
 * @WLAN_DCS_SEG_SEC20: secondary 20MHz
 * @WLAN_DCS_SEG_SEC40: secondary 40MHz
 * @WLAN_DCS_SEG_SEC80: secondary 80MHz
 * @WLAN_DCS_SEG_SEC160: secondary 160MHz
 */
enum wlan_dcs_chan_seg {
	WLAN_DCS_SEG_INVALID,
	WLAN_DCS_SEG_PRI20,
	WLAN_DCS_SEG_SEC20,
	WLAN_DCS_SEG_SEC40,
	WLAN_DCS_SEG_SEC80,
	WLAN_DCS_SEG_SEC160,
};

/* masks for segments */
#define WLAN_DCS_SEG_PRI20_MASK BIT(0)
#define WLAN_DCS_SEG_SEC20_MASK BIT(1)
#define WLAN_DCS_SEG_SEC40_MASK (BIT(2) | BIT(3))
#define WLAN_DCS_SEG_SEC80_MASK (BIT(4) | BIT(5) | BIT(6) | BIT(7))
#define WLAN_DCS_SEG_SEC160_MASK (BIT(8) | BIT(9) | BIT(10) | BIT(11) | \
				  BIT(12) | BIT(13) | BIT(14) | BIT(15))

#define WLAN_DCS_CHAN_FREQ_OFFSET 5
#define WLAN_DCS_IS_FREQ_IN_WIDTH(__cfreq, __cfreq0, __cfreq1, __width, __freq)\
	((((__width) == CH_WIDTH_20MHZ) &&                                     \
	  ((__cfreq) == (__freq))) ||                                          \
	 (((__width) == CH_WIDTH_40MHZ) &&                                     \
	  (((__freq) >= ((__cfreq0) - (2 * WLAN_DCS_CHAN_FREQ_OFFSET))) &&     \
	   ((__freq) <= ((__cfreq0) + (2 * WLAN_DCS_CHAN_FREQ_OFFSET))))) ||   \
	 (((__width) == CH_WIDTH_80MHZ) &&                                     \
	  (((__freq) >= ((__cfreq0) - (6 * WLAN_DCS_CHAN_FREQ_OFFSET))) &&     \
	   ((__freq) <= ((__cfreq0) + (6 * WLAN_DCS_CHAN_FREQ_OFFSET))))) ||   \
	 (((__width) == CH_WIDTH_160MHZ) &&                                    \
	  (((__freq) >= ((__cfreq1) - (14 * WLAN_DCS_CHAN_FREQ_OFFSET))) &&    \
	   ((__freq) <= ((__cfreq1) + (14 * WLAN_DCS_CHAN_FREQ_OFFSET))))) ||  \
	 (((__width) == CH_WIDTH_80P80MHZ) &&                                  \
	  ((((__freq) >= ((__cfreq0) - (6 * WLAN_DCS_CHAN_FREQ_OFFSET))) &&    \
	   ((__freq) <= ((__cfreq0) + (6 * WLAN_DCS_CHAN_FREQ_OFFSET)))) ||    \
	   (((__freq) >= ((__cfreq1) - (6 * WLAN_DCS_CHAN_FREQ_OFFSET))) &&    \
	   ((__freq) <= ((__cfreq1) + (6 * WLAN_DCS_CHAN_FREQ_OFFSET)))))))

/**
 * struct psoc_dcs_params - define per-mode DCS configuration parameters
 * @dcs_debug: DCS debug trace level
 * @dcs_enable_cfg: DCS enable configuration per mode
 *                  Controls which interference types are enabled
 *                  for each mode (SAP, XPAN, XR, GO)
 * @intfr_detection_threshold: Interference detection threshold per mode
 *                             Number of times interference must be
 *                             detected within the detection window
 *                             to trigger DCS
 * @dcs_trnsprt_rjt_threshold_cu: Transport reject threshold per mode
 *                                Channel utilization threshold for
 *                                transport rejection
 * @dcs_coch_intfr_threshold: Co-channel interference threshold per mode
 *                            Threshold for detecting co-channel
 *                            interference
 * @dcs_tx_err_threshold: Transmission error threshold per mode
 *                        Threshold for transmission failure rate
 * @dcs_phy_err_penalty: PHY error penalty per mode
 *                       Penalty for each PHY error detected
 * @dcs_phy_err_threshold: PHY error threshold per mode
 *                         Threshold for PHY errors
 * @user_max_cu: Maximum channel utilization per mode
 *               Maximum allowed channel utilization due to AP's
 *               TX and RX
 * @dcs_radar_err_threshold: Radar error threshold per mode
 *                           Threshold for radar errors
 * @dcs_intfr_detection_window: Interference detection window per mode
 *                              Sampling window for interference
 *                              detection
 * @dcs_disable_thresh_per_5mins: DCS disable threshold per mode
 *                                If DCS happens more than this
 *                                threshold in 5 minutes, disable DCS
 * @dcs_restart_delay: DCS restart delay per mode
 *                     When DCS is disabled, wait this many minutes
 *                     before re-enabling
 * @dcs_disable_algorithm: DCS algorithm disable flag per mode
 *                         When true, disables DCS algorithm for
 *                         the specific mode
 */
struct psoc_dcs_params {
	enum wlan_dcs_debug_level dcs_debug;
	union wlan_dcs_cfg dcs_enable_cfg[MAX_DCS_MODE_NUM];
	uint8_t intfr_detection_threshold[MAX_DCS_MODE_NUM];
	uint32_t dcs_trnsprt_rjt_threshold_cu[MAX_DCS_MODE_NUM];
	uint32_t dcs_coch_intfr_threshold[MAX_DCS_MODE_NUM];
	uint32_t dcs_tx_err_threshold[MAX_DCS_MODE_NUM];
	uint32_t dcs_phy_err_penalty[MAX_DCS_MODE_NUM];
	uint32_t dcs_phy_err_threshold[MAX_DCS_MODE_NUM];
	uint32_t user_max_cu[MAX_DCS_MODE_NUM];
	uint32_t dcs_radar_err_threshold[MAX_DCS_MODE_NUM];
	uint32_t dcs_intfr_detection_window[MAX_DCS_MODE_NUM];
	uint8_t dcs_disable_thresh_per_5mins[MAX_DCS_MODE_NUM];
	uint32_t dcs_restart_delay[MAX_DCS_MODE_NUM];
	bool dcs_disable_algorithm[MAX_DCS_MODE_NUM];
};

/**
 * struct dcs_psoc_priv_obj - define dcs psoc priv
 * @dcs_pdev_priv: dcs pdev priv
 * @dcs_cbk: dcs callback
 * @switch_chan_cb: callback for switching channel
 * @afc_sel_chan_cbk: callback for afc channel selection
 * @dcs_per_mode_param: per-mode DCS configuration parameters
 * @dcs_enable_cfg: enable dcs for pdev mode DCS
 * @intfr_detection_threshold:Interference detection threshold
 *                            for pdev mode DCS
 * @dcs_enable_count: Number of VDEVs with DCS currently enabled
 * @enabled_vdev_ids: Array of VDEV IDs that have DCS enabled
 * @dcs_psoc_lock: Lock to protect VDEV tracking data
 */
struct dcs_psoc_priv_obj {
	struct dcs_pdev_priv_obj dcs_pdev_priv[WLAN_DCS_MAX_PDEVS];
	struct psoc_dcs_cbk dcs_cbk;
	dcs_switch_chan_cb switch_chan_cb;
	struct dcs_afc_select_chan_cbk afc_sel_chan_cbk;
	struct psoc_dcs_params dcs_per_mode_param;
	uint8_t dcs_enable_cfg;
	uint8_t intfr_detection_threshold;
	uint8_t dcs_enable_count;
	uint8_t enabled_vdev_ids[WLAN_DCS_MAX_VDEVS];
	qdf_spinlock_t dcs_psoc_lock;
};

/**
 * dcs_init_params_by_mode() - Init DCS params for given mode
 * @psoc: psoc object
 * @dcs_core: DCS core private object
 * @dcs_mode: DCS mode (SAP, XPAN, XR, GO)
 *
 * Copies per-mode config from psoc into core object.
 * Defaults to SAP if mode is invalid.
 */
void
dcs_init_params_by_mode(struct wlan_objmgr_psoc *psoc,
			struct dcs_core_priv_obj *dcs_core,
			enum wlan_dcs_mode dcs_mode);

/**
 * wlan_dcs_get_pdev_private_obj() - get dcs pdev private object
 * @psoc: psoc pointer
 * @pdev_id: pdev_id
 *
 * API to retrieve the pdev private object from the psoc context
 *
 * Return: pdev private object pointer on success, NULL on error
 */
struct dcs_pdev_priv_obj *
wlan_dcs_get_pdev_private_obj(struct wlan_objmgr_psoc *psoc, uint32_t pdev_id);

/**
 * wlan_dcs_get_vdev_private_obj() - get dcs vdev private object
 * @psoc: psoc pointer
 * @vdev_id: vdev_id
 *
 * API to retrieve the vdev private object from the psoc context
 *
 * Return: vdev private object pointer on success, NULL on error
 */
struct dcs_vdev_priv_obj *
wlan_dcs_get_vdev_private_obj(struct wlan_objmgr_psoc *psoc, uint32_t vdev_id);

/**
 * wlan_dcs_get_core_private_obj() - get dcs core object
 * @psoc: psoc pointer
 * @pdev_id: pdev_id
 * @vdev_id: vdev_id
 *
 * API to retrieve the core object based on vdev or pdev
 *
 * Return: core object pointer on success, NULL on error
 */
struct dcs_core_priv_obj *
wlan_dcs_get_core_private_obj(struct wlan_objmgr_psoc *psoc, uint8_t pdev_id,
			      uint8_t vdev_id);

/**
 * dcs_get_trnsprt_switch_rjt_th_cu() - get unused cu threshold
 * @psoc: psoc pointer
 * @pdev_id: pdev_id
 *
 * Return: cu threshold
 */
uint32_t
dcs_get_trnsprt_switch_rjt_th_cu(struct wlan_objmgr_psoc *psoc,
				 uint8_t pdev_id);

/**
 * wlan_dcs_attach() - Attach dcs handler
 * @psoc: psoc pointer
 *
 * This function gets called to register dcs FW events handler
 *
 * Return: QDF_STATUS
 */
QDF_STATUS wlan_dcs_attach(struct wlan_objmgr_psoc *psoc);

/**
 * wlan_dcs_detach() - Detach dcs handler
 * @psoc: psoc pointer
 *
 * This function gets called to unregister dcs FW events handler
 *
 * Return: QDF_STATUS
 */
QDF_STATUS wlan_dcs_detach(struct wlan_objmgr_psoc *psoc);

/**
 * wlan_dcs_inc_enable_count() - Increment DCS enable count and add VDEV to list
 * @psoc: psoc pointer
 * @vdev_id: VDEV ID to add to enabled list
 *
 * This function increments the DCS enable count and adds the VDEV ID to the
 * enabled VDEV list. It is called when DCS is enabled on a VDEV.
 *
 * Return: void
 */
void wlan_dcs_inc_enable_count(struct wlan_objmgr_psoc *psoc, uint8_t vdev_id);

/**
 * wlan_dcs_dec_enable_count() - Decrement DCS enable count and remove VDEV
 * from list
 * @psoc: psoc pointer
 * @vdev_id: VDEV ID to remove from enabled list
 *
 * This function decrements the DCS enable count and removes the VDEV ID from
 * the enabled VDEV list. It is called when DCS is disabled on a VDEV.
 *
 * Return: void
 */
void wlan_dcs_dec_enable_count(struct wlan_objmgr_psoc *psoc, uint8_t vdev_id);

/**
 * wlan_dcs_cmd_send() - Send dcs command to target_if layer
 * @psoc: psoc pointer
 * @pdev_id: pdev_id
 * @is_host_pdev_id: pdev_id is host id or not
 *
 * The function gets called to send dcs command to FW
 *
 * return: QDF_STATUS_SUCCESS for success or error code
 */
QDF_STATUS wlan_dcs_cmd_send(struct wlan_objmgr_psoc *psoc,
			     uint8_t pdev_id,
			     bool is_host_pdev_id);

#ifdef WLAN_FEATURE_VDEV_DCS
/**
 * wlan_send_dcs_cmd_for_vdev() - Send dcs command to target_if layer in
 * vdev level
 * @psoc: psoc pointer
 * @mac_id: mac_id
 * @vdev_id: vdev_id
 *
 * The function gets called to send dcs command in vdev level to FW
 *
 * return: QDF_STATUS_SUCCESS for success or error code
 */
QDF_STATUS wlan_send_dcs_cmd_for_vdev(struct wlan_objmgr_psoc *psoc,
				      uint32_t mac_id,
				      uint8_t vdev_id);
#endif

/**
 * wlan_is_vdev_level_dcs_supported() -API to check whether vdev level
 * DCS is supported or not
 * @psoc: pointer to psoc object
 *
 * Return: True/False
 */
bool wlan_is_vdev_level_dcs_supported(struct wlan_objmgr_psoc *psoc);

/**
 * wlan_is_two_vdev_dcs_supported() - API to check whether concurrent 2 VDEV
 * DCS is supported or not
 * @psoc: pointer to psoc object
 *
 * Return: True/False
 */
bool wlan_is_two_vdev_dcs_supported(struct wlan_objmgr_psoc *psoc);

/**
 * wlan_dcs_process() - dcs process main entry
 * @psoc: psoc pointer
 * @event: dcs event pointer
 *
 * This function is the main entry to do dcs related operation
 * such as algorithm handling and dcs frequency control.
 *
 * Return: QDF_STATUS
 */
QDF_STATUS wlan_dcs_process(struct wlan_objmgr_psoc *psoc,
			    struct wlan_host_dcs_event *event);

/**
 * wlan_dcs_disable_timer_fn() - dcs disable timer callback
 * @dcs_timer_args: dcs timer argument pointer
 *
 * This function gets called when dcs disable timer timeout
 *
 * Return: None
 */
void wlan_dcs_disable_timer_fn(void *dcs_timer_args);

/**
 * wlan_dcs_clear() - clear dcs information
 * @psoc: psoc pointer
 * @vdev_id: vdev_id
 *
 * The function gets called to clear dcs information such as dcs
 * frequency control parameters and stop dcs disable timer
 *
 * Return: None
 */
void wlan_dcs_clear(struct wlan_objmgr_psoc *psoc, uint8_t vdev_id);

/**
 * wlan_dcs_set_algorithm_process() - config dcs event data to do algorithm
 * process or not
 * @psoc: psoc pointer
 * @pdev_id: pdev id
 * @vdev_id: vdev id
 * @dcs_algorithm_process: dcs algorithm process
 *
 * The function gets called to config dcs event data to do algorithm
 * process or not
 *
 * Return: None
 */
void wlan_dcs_set_algorithm_process(struct wlan_objmgr_psoc *psoc,
				    uint8_t pdev_id, uint8_t vdev_id,
				    bool dcs_algorithm_process);

/**
 * wlan_dcs_core_obj_lock() - private API to acquire spinlock for core object
 * @dcs_core: pointer to dcs core object
 *
 * Return: void
 */
static inline void wlan_dcs_core_obj_lock(struct dcs_core_priv_obj *dcs_core)
{
	qdf_spin_lock_bh(&dcs_core->lock);
}

/**
 * wlan_dcs_core_obj_unlock() - private api to release spinlock for core object
 * @dcs_core: pointer to dcs core object
 *
 * Return: void
 */
static inline void wlan_dcs_core_obj_unlock(struct dcs_core_priv_obj *dcs_core)
{
	qdf_spin_unlock_bh(&dcs_core->lock);
}

/**
 * wlan_dcs_trigger_dcs() - wrapper to trigger DCS
 * @psoc: pointer to dcs psoc object
 * @pdev_id: pdev id
 * @vdev_id: vdev id
 * @dcs_type: DCS type
 *
 * Return: void
 */
void
wlan_dcs_trigger_dcs(struct wlan_objmgr_psoc *psoc, uint8_t pdev_id,
		     uint8_t vdev_id, enum wlan_host_dcs_type dcs_type);

/**
 * wlan_dcs_switch_chan() - switch channel for vdev
 * @vdev: vdev ptr
 * @tgt_freq: target frequency
 * @tgt_width: target channel width
 *
 * Return: QDF_STATUS
 */
QDF_STATUS
wlan_dcs_switch_chan(struct wlan_objmgr_vdev *vdev, qdf_freq_t tgt_freq,
		     enum phy_ch_width tgt_width);
#endif  /* _WLAN_DCS_H_ */
