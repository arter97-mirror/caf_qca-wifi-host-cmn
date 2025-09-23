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
 * DOC: This file contains centralized definitions of DCS component
 */
#ifndef __CONFIG_DCS_H
#define __CONFIG_DCS_H

#include "cfg_define.h"

#define DCS_MAX_STR_LEN 256

/*
 * <ini>
 * gEnableDcs - Enable/Disable DCS
 * @Min: 0
 * @Max: 3
 * @Default: 0
 *
 * This ini is used to enable/disable DCS. Configurations are as follows:
 * 0 - Disable DCS.
 * 1 - Enable DCS for CW interference mitigation(CW_IM).
 * 2 - Enable DCS for WLAN interference mitigation(WLAN_IM).
 * 3 - Enable both DCS for CW_IM and DCS for WLAN_IM.
 *
 * Related: None
 *
 * Supported Feature: DCS
 *
 * Usage: External
 *
 * </ini>
 */
#define CFG_DCS_ENABLE CFG_INI_UINT(\
		"gEnableDcs",\
		0, 3, 0,\
		CFG_VALUE_OR_DEFAULT, "Enable DCS")

/*
 * <ini>
 * dcs_debug - Configure dcs debug trace level for debug purpose
 * @Min: 0
 * @Max: 2
 * @Default: 1
 *
 * This ini is used to configure dcs debug trace level for debug purpose
 *
 * Related: None
 *
 * Usage: External
 *
 * </ini>
 */
#define CFG_DCS_DEBUG CFG_INI_UINT(\
		"dcs_debug",\
		0, 2, 1,\
		CFG_VALUE_OR_DEFAULT,\
		"dcs debug trace level")

/*
 * <ini>
 * dcs_coch_intfr_threshold - Configure co-channel interference threshold
 * @Default: "30"
 *
 * This ini is used to configure co-channel interference threshold.
 *
 * Supports per-mode configuration using comma-separated values, and each value
 * must be within the range 0-0xFFFFFFFF. For example: "30,40,50,60" sets
 * thresholds for SAP, XPAN, XR, and GO modes respectively. If fewer values are
 * specified, the default value is applied to the remaining modes.
 *
 * Related: None
 *
 * Usage: External
 *
 * </ini>
 */
#define CFG_DCS_COCH_INTFR_THRESHOLD CFG_INI_STRING(\
		"dcs_coch_intfr_threshold",\
		0, DCS_MAX_STR_LEN, "30",\
		"dcs co-channel interference threshold level")

/*
 * <ini>
 * dcs_trnsprt_rjt_threshold_cu - Configure Transport reject interference
 * threshold
 * @Default: "30"
 *
 * This ini is used to configure Transport reject interference threshold.
 *
 * Supports per-mode configuration using comma-separated values, and each value
 * must be within the range 0-0xFFFFFFFF. For example: "30,40,50,60" sets
 * thresholds for SAP, XPAN, XR, and GO modes respectively. If fewer values are
 * provided, the default value is applied to the remaining modes.
 *
 * Related: None
 *
 * Usage: External
 *
 * </ini>
 */
#define CFG_DCS_TRANSPORT_SWITCH_RJT_TH_CU CFG_INI_STRING(\
		"dcs_trnsprt_rjt_threshold_cu",\
		0, DCS_MAX_STR_LEN, "30",\
		"dcs ll sap Transport reject interference threshold level")

/*
 * <ini>
 * dcs_tx_err_threshold - Configure transmission failure rate threshold
 * @Default: "30"
 *
 * This ini is used to configure transmission failure rate threshold.
 *
 * Supports per-mode configuration using comma-separated values, and each value
 * must be within the range 0-0xFFFFFFFF. For example: "30,40,50,60" sets
 * thresholds for SAP, XPAN, XR, and GO modes respectively. If fewer values are
 * provided, the default value is applied to the remaining modes.
 *
 * Related: None
 *
 * Usage: External
 *
 * </ini>
 */
#define CFG_DCS_TX_ERR_THRESHOLD CFG_INI_STRING(\
		"dcs_tx_err_threshold",\
		0, DCS_MAX_STR_LEN, "30",\
		"dcs transmission failure rate threshold")

/*
 * <ini>
 * dcs_phy_err_penalty - Configure channel time wasted due to each Phy
 * error(phy error penalty)
 * @Default: "500"
 *
 * This ini is used to configure phy error penalty.
 *
 * Supports per-mode configuration using comma-separated values, and each value
 * must be within the range 0-0xFFFFFFFF. For example: "500,600,700,800" assigns
 * different penalties for SAP, XPAN, XR, and GO modes respectively. If fewer
 * values are specified, the default value is applied to the remaining modes.
 *
 * Related: None
 *
 * Usage: External
 *
 * </ini>
 */
#define CFG_DCS_PHY_ERR_PENALTY CFG_INI_STRING(\
		"dcs_phy_err_penalty",\
		0, DCS_MAX_STR_LEN, "500",\
		"dcs phy error penalty")

/*
 * <ini>
 * dcs_phy_err_threshold - Configure phy err threshold
 * @Default: "300"
 *
 * This ini is used to configure phy error threshold.
 *
 * Supports per-mode configuration using comma-separated values. Each value must
 * be within the range 0-0xFFFFFFFF. For example: "300,400,500,600" sets
 * thresholds for SAP, XPAN, XR, and GO modes respectively. If fewer values are
 * provided, the default value is applied to the remaining modes.
 *
 * Related: None
 *
 * Usage: External
 *
 * </ini>
 */
#define CFG_DCS_PHY_ERR_THRESHOLD CFG_INI_STRING(\
		"dcs_phy_err_threshold",\
		0, DCS_MAX_STR_LEN, "300",\
		"dcs phy error threshold")

/*
 * <ini>
 * dcs_user_max_cu - Configure tx channel utilization due to AP's tx
 * and rx
 * @Default: "50"
 *
 * This ini is uised to configure tx channel utilization due to AP's tx
 * and rx.
 *
 * Supports per-mode configuration using comma-separated values, with each value
 * must be within the range 0-0xFFFFFFFF. For example: "50,60,70,80" sets
 * utilization values for SAP, XPAN, XR, and GO modes respectively. If fewer
 * values are provided, the default value is applied to the remaining modes.
 *
 * Related: None
 *
 * Usage: External
 *
 * </ini>
 */
#define CFG_DCS_USER_MAX_CU CFG_INI_STRING(\
		"user_max_cu",\
		0, DCS_MAX_STR_LEN, "50",\
		"dcs tx channel utilization")

/*
 * <ini>
 * dcs_radar_err_threshold - Configure radar error threshold
 * @Default: "1000"
 *
 * This ini is used to configure radar error threshold.
 *
 * Supports per-mode configuration using comma-separated values, with each value
 * must be within the range 0-0xFFFFFFFF. For example: "1000,1100,1200,1300"
 * sets utilization values for SAP, XPAN, XR, and GO modes respectively. If
 * fewer values are provided, the default value is applied to the remaining
 * modes.
 *
 * Related: None
 *
 * Usage: External
 *
 * </ini>
 */
#define CFG_DCS_RADAR_ERR_THRESHOLD CFG_INI_STRING(\
		"dcs_radar_err_threshold",\
		0, DCS_MAX_STR_LEN, "1000",\
		"dcs radar error threshold")

/*
 * <ini>
 * dcs_intfr_detection_threshold - Configure interference detection threshold
 * @Min: 0
 * @Max: 0xFFFFFFFF
 * @Default: 6
 *
 * This ini is used to configure interference detection threshold
 *
 *
 * Related: None
 *
 * Usage: External
 *
 * </ini>
 */
#define CFG_DCS_INTFR_DETECTION_THRESHOLD CFG_INI_UINT(\
		"dcs_intfr_detection_threshold",\
		0, 0xFFFFFFFF, 6,\
		CFG_VALUE_OR_DEFAULT,\
		"dcs interference detection threshold")

/*
 * <ini>
 * dcs_intfr_detection_window - Configure interference sampling window
 * @Default: "10"
 *
 * This ini is used to configure interference sampling window.
 *
 * Supports per-mode configuration using comma-separated values, with each value
 * must be within the range 0-0xFFFFFFFF. For example: "10,15,20,25" sets
 * utilization values for SAP, XPAN, XR, and GO modes respectively. If fewer
 * values are provided, the default value is applied to the remaining modes.
 *
 * Related: None
 *
 * Usage: External
 *
 * </ini>
 */
#define CFG_DCS_INTFR_DETECTION_WINDOW CFG_INI_STRING(\
		"dcs_intfr_detection_window",\
		0, DCS_MAX_STR_LEN, "10",\
		"dcs interference sampling window")

/*
 * <ini>
 * dcs_disable_threshold_per_5mins - In five minutes, if dcs happen
 * more than threshold, then disable dcs for some time
 * @Default: "3"
 *
 * This ini is used to set the threshold for DCS events in five minutes.
 *
 * Supports per-mode configuration using comma-separated values, with each value
 * must be within the range 0-10. For example: "3,4,5,6" sets utilization values
 * for SAP, XPAN, XR, and GO modes respectively. If fewer values are provided,
 * the default value is applied to the remaining modes.
 *
 * Related: None
 *
 * Usage: External
 *
 * </ini>
 */
#define CFG_DCS_DISABLE_THRESHOLD_PER_5MINS CFG_INI_STRING(\
		"dcs_disable_thresh_per_5mins",\
		0, DCS_MAX_STR_LEN, "3",\
		"dcs happen times threshold in five minutes")

/*
 * <ini>
 * dcs_restart_delay - When dcs happen more than threshold in five
 * minutes, then start to disable dcs for some minutes, then enable dcs
 * again.
 * @Default: "30"
 *
 * This ini is used to configure dcs disable time length in minute unit.
 *
 * Supports per-mode configuration using comma-separated values, with each value
 * must be within the range 0-0xFFFFFFFF. For example: "30,40,50,60" sets
 * utilization values for SAP, XPAN, XR, and GO modes respectively. If fewer
 * values are provided, the default value is applied to the remaining modes.
 *
 * Related: None
 *
 * Usage: External
 *
 * </ini>
 */
#define CFG_DCS_RESTART_DELAY CFG_INI_STRING(\
		"dcs_restart_delay",\
		0, DCS_MAX_STR_LEN, "30",\
		"dcs restart delay")

/*
 * <ini>
 * dcs_disable_algorithm - Some feature like SON only need dcs stats,
 * then disable dcs algorithm for SON.
 * @Default: "0"
 *
 * This ini is used to disable dcs algorithm.
 *
 * Supports per-mode configuration using comma-separated values, with each value
 * must be within the range 0-1. For example: "1,1,0,0" sets utilization values
 * for SAP, XPAN, XR, and GO modes respectively. If fewer values are provided,
 * the default value is applied to the remaining modes.
 *
 * Related: None
 *
 * Usage: External
 *
 * </ini>
 */
#define CFG_DCS_DISABLE_ALGORITHM CFG_INI_STRING(\
		"dcs_disable_algorithm",\
		0, DCS_MAX_STR_LEN, "0",\
		"dcs disable algorithm")

#ifdef WLAN_FEATURE_VDEV_DCS
/*
 * <ini>
 * gEnableDcsPerMode - Enable/Disable DCS per mode
 * @Min: 0x0
 * @Max: 0x0f0f0f0f
 * @Default: 0x0200
 *
 * This ini is used to enable or disable DCS and DCS type per mode.
 *
 * BYTE        Role
 * 1st BYTE    DCS_SAP (lowest byte)
 * 2nd BYTE    DCS_XPAN
 * 3rd BYTE    DCS_XR
 * 4th BYTE    DCS_GO (highest byte)
 *
 * Configure each byte for each mode as follows:
 * 0 - Disable DCS.
 * 1 - Enable DCS for CW interference mitigation (CW_IM).
 * 2 - Enable DCS for WLAN interference mitigation (WLAN_IM).
 * 3 - Enable both DCS for CW_IM and DCS for WLAN_IM.
 *
 * Example: 0x0202 enables DCS for WLAN interference mitigation (WLAN_IM) for
 * both general SAP and XPAN.
 *
 * Usage: External
 *
 * </ini>
 */
#define CFG_DCS_ENABLE_PER_MODE CFG_INI_UINT( \
		"gEnableDcsPerMode", \
		0x0, \
		0x0f0f0f0f, \
		0x0200, \
		CFG_VALUE_OR_DEFAULT, \
		"Enable DCS per mode")

/*
 * <ini>
 * dcs_intfr_detection_threshold_per_mode - Configure interference detection
 * threshold per mode
 * @Min: 0
 * @Max: 0xFFFFFFFF
 * @Default: 0x0606
 *
 * This ini configures the interference detection threshold per mode.
 *
 * BYTE        Meaning
 * 1st BYTE    Threshold for DCS_SAP (lowest byte)
 * 2nd BYTE    Threshold for DCS_XPAN
 * 3rd BYTE    Threshold for DCS_XR
 * 4th BYTE    Threshold for DCS_GO (highest byte)
 *
 * Example: 0x0606 sets the interference detection threshold to 6 for both
 * general SAP and XPAN.
 *
 * Related: None
 *
 * Usage: External
 *
 * </ini>
 */
#define CFG_DCS_INTFR_DETECTION_THRESHOLD_PER_MODE CFG_INI_UINT(\
		"dcs_intfr_detection_threshold_per_mode",\
		0, 0xFFFFFFFF, 0x0606,\
		CFG_VALUE_OR_DEFAULT,\
		"DCS interference detection threshold per mode")

#define CFG_DCS_PER_MODE \
	CFG(CFG_DCS_ENABLE_PER_MODE) \
	CFG(CFG_DCS_INTFR_DETECTION_THRESHOLD_PER_MODE)

#else
#define CFG_DCS_PER_MODE
#endif

#define CFG_DCS_ALL \
	CFG(CFG_DCS_ENABLE) \
	CFG(CFG_DCS_DEBUG) \
	CFG(CFG_DCS_COCH_INTFR_THRESHOLD) \
	CFG(CFG_DCS_TRANSPORT_SWITCH_RJT_TH_CU) \
	CFG(CFG_DCS_TX_ERR_THRESHOLD) \
	CFG(CFG_DCS_PHY_ERR_PENALTY) \
	CFG(CFG_DCS_PHY_ERR_THRESHOLD) \
	CFG(CFG_DCS_USER_MAX_CU) \
	CFG(CFG_DCS_RADAR_ERR_THRESHOLD) \
	CFG(CFG_DCS_INTFR_DETECTION_THRESHOLD) \
	CFG(CFG_DCS_INTFR_DETECTION_WINDOW) \
	CFG(CFG_DCS_DISABLE_THRESHOLD_PER_5MINS) \
	CFG(CFG_DCS_RESTART_DELAY) \
	CFG(CFG_DCS_DISABLE_ALGORITHM) \
	CFG_DCS_PER_MODE


#define DEFINE_DCS_CFG(name, min_val, max_val, default_val) \
enum { \
	name##_MIN     = (min_val), \
	name##_MAX     = (max_val), \
	name##_DEFAULT = (default_val) \
}

DEFINE_DCS_CFG(CFG_DCS_COCH_INTFR_THRESHOLD, 0, 0xFFFFFFFF, 30);
DEFINE_DCS_CFG(CFG_DCS_TRANSPORT_SWITCH_RJT_TH_CU, 0, 0xFFFFFFFF, 30);
DEFINE_DCS_CFG(CFG_DCS_TX_ERR_THRESHOLD, 0, 0xFFFFFFFF, 30);
DEFINE_DCS_CFG(CFG_DCS_PHY_ERR_PENALTY, 0, 0xFFFFFFFF, 500);
DEFINE_DCS_CFG(CFG_DCS_PHY_ERR_THRESHOLD, 0, 0xFFFFFFFF, 300);
DEFINE_DCS_CFG(CFG_DCS_USER_MAX_CU, 0, 0xFFFFFFFF, 50);
DEFINE_DCS_CFG(CFG_DCS_RADAR_ERR_THRESHOLD, 0, 0xFFFFFFFF, 1000);
DEFINE_DCS_CFG(CFG_DCS_INTFR_DETECTION_THRESHOLD, 0, 0xFFFFFFFF, 6);
DEFINE_DCS_CFG(CFG_DCS_INTFR_DETECTION_WINDOW, 0, 0xFFFFFFFF, 10);
DEFINE_DCS_CFG(CFG_DCS_DISABLE_THRESHOLD_PER_5MINS, 0, 10, 3);
DEFINE_DCS_CFG(CFG_DCS_RESTART_DELAY, 0, 0xFFFFFFFF, 30);
DEFINE_DCS_CFG(CFG_DCS_DISABLE_ALGORITHM, 0, 1, 0);

#endif /* __CONFIG_DCS_H */
