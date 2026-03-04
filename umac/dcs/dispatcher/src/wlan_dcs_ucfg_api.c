/*
 * Copyright (c) 2020-2021 The Linux Foundation. All rights reserved.
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
 * DOC: This file has the dcs dispatcher API implementation which is exposed
 * to outside of dcs component.
 */

#include "wlan_dcs_ucfg_api.h"
#include "../../core/src/wlan_dcs.h"
#include "wlan_objmgr_vdev_obj.h"
#include <wlan_mlme_public_struct.h>
#include "wlan_dcs_init_deinit_api.h"
#include "cfg_dcs.h"

void ucfg_dcs_register_cb(
			struct wlan_objmgr_psoc *psoc,
			dcs_callback cbk,
			void *arg)
{
	struct dcs_psoc_priv_obj *dcs_psoc_priv;

	dcs_psoc_priv = wlan_objmgr_psoc_get_comp_private_obj(
							psoc,
							WLAN_UMAC_COMP_DCS);
	if (!dcs_psoc_priv) {
		dcs_err("dcs psoc private object is null");
		return;
	}

	dcs_psoc_priv->dcs_cbk.cbk = cbk;
	dcs_psoc_priv->dcs_cbk.arg = arg;
}

void
ucfg_dcs_register_user_cb(struct wlan_objmgr_psoc *psoc,
			  uint8_t vdev_id,
			  void (*cb)(uint8_t vdev_id,
				     struct wlan_host_dcs_im_user_stats *stats,
				     int status))
{
	struct dcs_core_priv_obj *dcs_core_priv;

	dcs_core_priv = wlan_dcs_get_core_private_obj(psoc, DCS_INVALID_PDEV_ID,
						      vdev_id);
	if (!dcs_core_priv) {
		dcs_err("dcs core private object is null");
		return;
	}

	dcs_core_priv->requestor_vdev_id = vdev_id;
	dcs_core_priv->user_cb = cb;
}

uint32_t
wlan_dcs_get_trnsprt_switch_rjt_th_cu(struct wlan_objmgr_psoc *psoc,
				      uint8_t vdev_id)
{
	return dcs_get_trnsprt_switch_rjt_th_cu(psoc, vdev_id);
}

QDF_STATUS ucfg_dcs_register_awgn_cb(struct wlan_objmgr_psoc *psoc,
				     dcs_switch_chan_cb cb)
{
	struct dcs_psoc_priv_obj *dcs_psoc_priv;

	dcs_psoc_priv =
		wlan_objmgr_psoc_get_comp_private_obj(psoc, WLAN_UMAC_COMP_DCS);
	if (!dcs_psoc_priv) {
		dcs_err("dcs psoc private object is null");
		return QDF_STATUS_E_INVAL;
	}

	dcs_psoc_priv->switch_chan_cb = cb;
	return QDF_STATUS_SUCCESS;
}

QDF_STATUS ucfg_dcs_register_afc_sel_chan_cb(struct wlan_objmgr_psoc *psoc,
					     dcs_afc_select_chan_cb cb,
					     void *arg)
{
	struct dcs_psoc_priv_obj *dcs_psoc_priv;

	dcs_psoc_priv =
		wlan_objmgr_psoc_get_comp_private_obj(psoc, WLAN_UMAC_COMP_DCS);
	if (!dcs_psoc_priv) {
		dcs_err("dcs psoc private object is null");
		return QDF_STATUS_E_INVAL;
	}

	dcs_psoc_priv->afc_sel_chan_cbk.cbk = cb;
	dcs_psoc_priv->afc_sel_chan_cbk.arg = arg;
	return QDF_STATUS_SUCCESS;
}

#ifdef WLAN_FEATURE_VDEV_DCS
QDF_STATUS
ucfg_wlan_dcs_cmd(struct wlan_objmgr_psoc *psoc, uint32_t mac_id,
		  uint8_t vdev_id)
{
	if (wlan_is_vdev_level_dcs_supported(psoc))
		return wlan_send_dcs_cmd_for_vdev(psoc, mac_id, vdev_id);
	else
		return wlan_dcs_cmd_send(psoc, mac_id, true);
}
#else
QDF_STATUS
ucfg_wlan_dcs_cmd(struct wlan_objmgr_psoc *psoc, uint32_t mac_id,
		  uint8_t vdev_id)
{
	return wlan_dcs_cmd_send(psoc, mac_id, true);
}
#endif

bool ucfg_is_two_vdev_dcs_supported(struct wlan_objmgr_psoc *psoc)
{
	return wlan_is_two_vdev_dcs_supported(psoc);
}

void ucfg_config_dcs_enable(struct wlan_objmgr_psoc *psoc,
			uint32_t mac_id,
			uint8_t vdev_id,
			uint8_t interference_type)
{
	struct dcs_core_priv_obj *dcs_core_priv;

	dcs_core_priv = wlan_dcs_get_core_private_obj(psoc, mac_id,
						      vdev_id);
	if (!dcs_core_priv) {
		dcs_err("dcs core private object is null");
		return;
	}

	dcs_core_priv->dcs_host_params.dcs_enable |= interference_type;
}

void ucfg_config_dcs_disable(struct wlan_objmgr_psoc *psoc,
			uint8_t vdev_id,
			uint8_t interference_type)
{
	struct dcs_core_priv_obj *dcs_core_priv;

	dcs_core_priv = wlan_dcs_get_core_private_obj(psoc, DCS_INVALID_PDEV_ID,
						      vdev_id);
	if (!dcs_core_priv) {
		dcs_err("dcs core private object is null");
		return;
	}

	dcs_core_priv->dcs_host_params.dcs_enable &= (~interference_type);
}

uint8_t ucfg_get_dcs_enable(struct wlan_objmgr_psoc *psoc, uint8_t vdev_id)
{
	struct dcs_core_priv_obj *dcs_core_priv;
	uint8_t enable = 0;

	dcs_core_priv = wlan_dcs_get_core_private_obj(psoc, DCS_INVALID_PDEV_ID,
						      vdev_id);
	if (!dcs_core_priv) {
		dcs_err("dcs core private object is null");
		return 0;
	}

	if (dcs_core_priv->dcs_host_params.dcs_enable_cfg)
		enable = dcs_core_priv->dcs_host_params.dcs_enable;

	return enable;
}

void ucfg_dcs_clear(struct wlan_objmgr_psoc *psoc, uint8_t vdev_id)
{
	wlan_dcs_clear(psoc, vdev_id);
}

void ucfg_config_dcs_event_data(struct wlan_objmgr_psoc *psoc, uint8_t pdev_id,
				uint8_t vdev_id, bool dcs_algorithm_process)
{
	wlan_dcs_set_algorithm_process(psoc, pdev_id, vdev_id,
				       dcs_algorithm_process);
}

void ucfg_dcs_reset_user_stats(struct wlan_objmgr_psoc *psoc, uint8_t vdev_id)
{
	struct dcs_core_priv_obj *dcs_core_priv;
	struct wlan_host_dcs_im_user_stats *user_stats;

	dcs_core_priv = wlan_dcs_get_core_private_obj(psoc, DCS_INVALID_PDEV_ID,
						      vdev_id);
	if (!dcs_core_priv) {
		dcs_err("dcs core private object is null");
		return;
	}

	wlan_dcs_core_obj_lock(dcs_core_priv);
	dcs_core_priv->dcs_host_params.user_request_count = 0;
	dcs_core_priv->dcs_host_params.notify_user = 0;
	user_stats = &dcs_core_priv->dcs_im_stats.user_dcs_im_stats;
	user_stats->cycle_count = 0;
	user_stats->rxclr_count = 0;
	user_stats->rx_frame_count = 0;
	user_stats->my_bss_rx_cycle_count = 0;
	user_stats->max_rssi = 0;
	user_stats->min_rssi = 0;
	wlan_dcs_core_obj_unlock(dcs_core_priv);
}

void ucfg_dcs_set_user_request(struct wlan_objmgr_psoc *psoc, uint8_t vdev_id,
			       uint32_t user_request_count)
{
	struct dcs_core_priv_obj *dcs_core_priv;

	dcs_core_priv = wlan_dcs_get_core_private_obj(psoc, DCS_INVALID_PDEV_ID,
						      vdev_id);
	if (!dcs_core_priv) {
		dcs_err("dcs core private object is null");
		return;
	}

	wlan_dcs_core_obj_lock(dcs_core_priv);
	dcs_core_priv->dcs_host_params.user_request_count = user_request_count;
	wlan_dcs_core_obj_unlock(dcs_core_priv);
}

QDF_STATUS ucfg_dcs_get_ch_util(struct wlan_objmgr_psoc *psoc, uint8_t vdev_id,
				struct wlan_host_dcs_ch_util_stats *dcs_stats)
{
	struct dcs_core_priv_obj *dcs_core_priv;

	dcs_core_priv = wlan_dcs_get_core_private_obj(psoc, DCS_INVALID_PDEV_ID,
						      vdev_id);
	if (!dcs_core_priv) {
		dcs_err("dcs core private object is null");
		return QDF_STATUS_E_INVAL;
	}

	wlan_dcs_core_obj_lock(dcs_core_priv);
	qdf_mem_copy(dcs_stats,
		     &dcs_core_priv->dcs_im_stats.dcs_ch_util_im_stats,
		     sizeof(*dcs_stats));
	wlan_dcs_core_obj_unlock(dcs_core_priv);

	return QDF_STATUS_SUCCESS;
}

QDF_STATUS ucfg_dcs_set_mode_config(struct wlan_objmgr_psoc *psoc,
				    uint8_t vdev_id, enum QDF_OPMODE mode,
				    uint8_t ap_policy)
{
	struct dcs_core_priv_obj *dcs_core_priv;
	enum wlan_dcs_mode dcs_mode;
	uint8_t dcs_enable, dcs_enable_cfg;

	if (!psoc) {
		dcs_err("NULL psoc");
		return QDF_STATUS_E_INVAL;
	}

	switch (mode) {
	case QDF_P2P_GO_MODE:
		dcs_mode = DCS_GO;
		break;
	case QDF_SAP_MODE:
		if (ap_policy == HOST_CONCURRENT_AP_POLICY_XR)
			dcs_mode = DCS_XR;
		else if (ap_policy == HOST_CONCURRENT_AP_POLICY_GAMING_AUDIO ||
			 ap_policy ==
			 HOST_CONCURRENT_AP_POLICY_LOSSLESS_AUDIO_STREAMING)
			dcs_mode = DCS_XPAN;
		else
			dcs_mode = DCS_SAP;
		break;
	default:
		dcs_err("Unsupported opmode %d", mode);
		return QDF_STATUS_E_INVAL;
	}

	dcs_debug("vdev %d mode %d", vdev_id, dcs_mode);

	dcs_core_priv = wlan_dcs_get_core_private_obj(psoc, DCS_INVALID_PDEV_ID,
						      vdev_id);
	if (!dcs_core_priv) {
		dcs_err("NULL core obj vdev %d", vdev_id);
		return QDF_STATUS_E_INVAL;
	}

	dcs_enable = dcs_core_priv->dcs_host_params.dcs_enable;
	dcs_enable_cfg = dcs_core_priv->dcs_host_params.dcs_enable_cfg;

	dcs_init_params_by_mode(psoc, dcs_core_priv, dcs_mode);

	if ((dcs_enable & dcs_enable_cfg) !=
	    (dcs_core_priv->dcs_host_params.dcs_enable &
	     dcs_core_priv->dcs_host_params.dcs_enable_cfg)) {
		dcs_debug("Send DCS cmd vdev %d, ori %d cfg %d new %d cfg %d",
			  vdev_id, dcs_enable, dcs_enable_cfg,
			  dcs_core_priv->dcs_host_params.dcs_enable,
			  dcs_core_priv->dcs_host_params.dcs_enable_cfg);
		wlan_send_dcs_cmd_for_vdev(psoc, DCS_INVALID_PDEV_ID, vdev_id);
	}

	return QDF_STATUS_SUCCESS;
}

#ifdef DCS_INTERFERENCE_DETECTION
void
ucfg_dcs_trigger_dcs(struct wlan_objmgr_psoc *psoc, uint8_t pdev_id,
		     uint8_t vdev_id, enum wlan_host_dcs_type dcs_type)
{
	wlan_dcs_trigger_dcs(psoc, pdev_id, vdev_id, dcs_type);
}

QDF_STATUS
ucfg_dcs_switch_chan(struct wlan_objmgr_vdev *vdev, qdf_freq_t tgt_freq,
		     enum phy_ch_width tgt_width)
{
	return wlan_dcs_switch_chan(vdev, tgt_freq, tgt_width);
}
#endif

QDF_STATUS ucfg_dcs_get_config(struct wlan_objmgr_vdev *vdev,
			       struct wlan_dcs_user_config *config)
{
	struct wlan_objmgr_psoc *psoc;
	struct dcs_core_priv_obj *dcs_core_priv;
	uint8_t vdev_id;

	if (!vdev || !config) {
		dcs_err("vdev or config is null");
		return QDF_STATUS_E_INVAL;
	}

	psoc = wlan_vdev_get_psoc(vdev);
	if (!psoc) {
		dcs_err("psoc is null");
		return QDF_STATUS_E_INVAL;
	}

	vdev_id = wlan_vdev_get_id(vdev);
	dcs_core_priv = wlan_dcs_get_core_private_obj(psoc, DCS_INVALID_PDEV_ID,
						      vdev_id);
	if (!dcs_core_priv) {
		dcs_err("dcs core private object is null");
		return QDF_STATUS_E_INVAL;
	}

	wlan_dcs_core_obj_lock(dcs_core_priv);
	config->dcs_enable = dcs_core_priv->dcs_host_params.dcs_enable &
			     dcs_core_priv->dcs_host_params.dcs_enable_cfg;
	config->intfr_detection_threshold =
		dcs_core_priv->dcs_host_params.intfr_detection_threshold;
	config->phy_err_penalty =
		dcs_core_priv->dcs_host_params.phy_err_penalty;
	config->phy_err_threshold =
		dcs_core_priv->dcs_host_params.phy_err_threshold;
	config->radar_err_threshold =
		dcs_core_priv->dcs_host_params.radar_err_threshold;
	config->tx_err_threshold =
		dcs_core_priv->dcs_host_params.tx_err_threshold;
	config->intfr_detection_window =
		dcs_core_priv->dcs_host_params.intfr_detection_window;
	config->coch_intfr_threshold =
		dcs_core_priv->dcs_host_params.coch_intfr_threshold;
	config->max_cu = dcs_core_priv->dcs_host_params.user_max_cu;
	wlan_dcs_core_obj_unlock(dcs_core_priv);

	return QDF_STATUS_SUCCESS;
}

QDF_STATUS ucfg_dcs_set_config(struct wlan_objmgr_vdev *vdev,
			       struct wlan_dcs_user_config *config)
{
	struct wlan_objmgr_psoc *psoc;
	struct dcs_core_priv_obj *dcs_core_priv;
	struct core_dcs_params *dcs_param;
	uint8_t vdev_id, dcs_enable;
	uint32_t current_time, dcs_enable_interval;
	bool bchanged = false;

	if (!vdev || !config) {
		dcs_err("vdev or config is null");
		return QDF_STATUS_E_INVAL;
	}

	psoc = wlan_vdev_get_psoc(vdev);
	if (!psoc) {
		dcs_err("psoc is null");
		return QDF_STATUS_E_INVAL;
	}

	vdev_id = wlan_vdev_get_id(vdev);
	dcs_core_priv = wlan_dcs_get_core_private_obj(psoc, DCS_INVALID_PDEV_ID,
						      vdev_id);
	if (!dcs_core_priv) {
		dcs_err("dcs core private object is null");
		return QDF_STATUS_E_INVAL;
	}

	dcs_param = &dcs_core_priv->dcs_host_params;

	/* Validate configuration parameters */
	if (config->intfr_detection_threshold <
	    CFG_DCS_INTFR_DETECTION_THRESHOLD_MIN ||
	    config->intfr_detection_threshold >
	    CFG_DCS_INTFR_DETECTION_THRESHOLD_MAX) {
		dcs_err_rl("Inval intfr_detection_thre: %u (min: %u, max: %u)",
			   config->intfr_detection_threshold,
			   CFG_DCS_INTFR_DETECTION_THRESHOLD_MIN,
			   CFG_DCS_INTFR_DETECTION_THRESHOLD_MAX);
		return QDF_STATUS_E_INVAL;
	}

	if (config->phy_err_penalty < CFG_DCS_PHY_ERR_PENALTY_MIN ||
	    config->phy_err_penalty > CFG_DCS_PHY_ERR_PENALTY_MAX) {
		dcs_err_rl("Inval phy_err_penalty: %u (min: %u, max: %u)",
			   config->phy_err_penalty,
			   CFG_DCS_PHY_ERR_PENALTY_MIN,
			   CFG_DCS_PHY_ERR_PENALTY_MAX);
		return QDF_STATUS_E_INVAL;
	}

	if (config->phy_err_threshold < CFG_DCS_PHY_ERR_THRESHOLD_MIN ||
	    config->phy_err_threshold > CFG_DCS_PHY_ERR_THRESHOLD_MAX) {
		dcs_err_rl("Inval phy_err_threshold: %u (min: %u, max: %u)",
			   config->phy_err_threshold,
			   CFG_DCS_PHY_ERR_THRESHOLD_MIN,
			   CFG_DCS_PHY_ERR_THRESHOLD_MAX);
		return QDF_STATUS_E_INVAL;
	}

	if (config->radar_err_threshold < CFG_DCS_RADAR_ERR_THRESHOLD_MIN ||
	    config->radar_err_threshold > CFG_DCS_RADAR_ERR_THRESHOLD_MAX) {
		dcs_err_rl("Inval radar_err_threshold: %u (min: %u, max: %u)",
			   config->radar_err_threshold,
			   CFG_DCS_RADAR_ERR_THRESHOLD_MIN,
			   CFG_DCS_RADAR_ERR_THRESHOLD_MAX);
		return QDF_STATUS_E_INVAL;
	}

	if (config->tx_err_threshold < CFG_DCS_TX_ERR_THRESHOLD_MIN ||
	    config->tx_err_threshold > CFG_DCS_TX_ERR_THRESHOLD_MAX) {
		dcs_err_rl("Inval tx_err_threshold: %u (min: %u, max: %u)",
			   config->tx_err_threshold,
			   CFG_DCS_TX_ERR_THRESHOLD_MIN,
			   CFG_DCS_TX_ERR_THRESHOLD_MAX);
		return QDF_STATUS_E_INVAL;
	}

	if (config->intfr_detection_window <
	    CFG_DCS_INTFR_DETECTION_WINDOW_MIN ||
	    config->intfr_detection_window >
	    CFG_DCS_INTFR_DETECTION_WINDOW_MAX) {
		dcs_err_rl("Inval intfr_detection_win: %u (min: %u, max: %u)",
			   config->intfr_detection_window,
			   CFG_DCS_INTFR_DETECTION_WINDOW_MIN,
			   CFG_DCS_INTFR_DETECTION_WINDOW_MAX);
		return QDF_STATUS_E_INVAL;
	}

	if (config->coch_intfr_threshold < CFG_DCS_COCH_INTFR_THRESHOLD_MIN ||
	    config->coch_intfr_threshold > CFG_DCS_COCH_INTFR_THRESHOLD_MAX) {
		dcs_err_rl("Inval coch_intfr_threshold: %u (min: %u, max: %u)",
			   config->coch_intfr_threshold,
			   CFG_DCS_COCH_INTFR_THRESHOLD_MIN,
			   CFG_DCS_COCH_INTFR_THRESHOLD_MAX);
		return QDF_STATUS_E_INVAL;
	}

	if (config->max_cu < CFG_DCS_USER_MAX_CU_MIN ||
	    config->max_cu > CFG_DCS_USER_MAX_CU_MAX) {
		dcs_err_rl("Inval max_cu: %u (min: %u, max: %u)",
			   config->max_cu,
			   CFG_DCS_USER_MAX_CU_MIN,
			   CFG_DCS_USER_MAX_CU_MAX);
		return QDF_STATUS_E_INVAL;
	}

	if (config->dcs_enable != 0 &&
	    config->dcs_enable != WLAN_HOST_DCS_WLANIM) {
		dcs_err_rl("Unsupported DCS enable: 0x%x", config->dcs_enable);
		return QDF_STATUS_E_INVAL;
	}

	dcs_enable = dcs_param->dcs_enable_cfg & dcs_param->dcs_enable;
	if (config->dcs_enable != dcs_enable) {
		current_time = qdf_system_ticks_to_msecs(qdf_system_ticks());
		if (current_time < dcs_param->dcs_enable_timestamp)
			dcs_enable_interval = 0;
		else
			dcs_enable_interval = current_time -
					dcs_param->dcs_enable_timestamp;
		if (dcs_param->dcs_enable_timestamp &&
		    dcs_enable_interval < WLAN_DCS_ENABLE_DISABLE_EXPIRY_TIME) {
			dcs_err_rl("DCS enable/disable too frequent");
			return QDF_STATUS_E_INVAL;
		}
		dcs_param->dcs_enable_timestamp = current_time;

		dcs_debug("set dcs_enable from %d to %d",
			  dcs_enable, config->dcs_enable);
		wlan_dcs_core_obj_lock(dcs_core_priv);
		if (config->dcs_enable) {
			dcs_param->dcs_enable |= WLAN_HOST_DCS_WLANIM;
			dcs_param->dcs_enable_cfg |= WLAN_HOST_DCS_WLANIM;
		} else {
			dcs_param->dcs_enable &= (~WLAN_HOST_DCS_WLANIM);
			dcs_param->dcs_enable_cfg &= (~WLAN_HOST_DCS_WLANIM);
		}
		wlan_dcs_core_obj_unlock(dcs_core_priv);
		wlan_send_dcs_cmd_for_vdev(psoc, DCS_INVALID_PDEV_ID, vdev_id);
	}

	if (dcs_param->intfr_detection_threshold !=
	    config->intfr_detection_threshold) {
		dcs_debug("set intfr_detection_threshold from %d to %d",
			  dcs_param->intfr_detection_threshold,
			  config->intfr_detection_threshold);
		bchanged = true;
	}

	if (dcs_param->phy_err_penalty != config->phy_err_penalty) {
		dcs_debug("set phy_err_penalty from %d to %d",
			  dcs_param->phy_err_penalty, config->phy_err_penalty);
		bchanged = true;
	}

	if (dcs_param->phy_err_threshold != config->phy_err_threshold) {
		dcs_debug("set phy_err_threshold from %d to %d",
			  dcs_param->phy_err_threshold,
			  config->phy_err_threshold);
		bchanged = true;
	}

	if (dcs_param->radar_err_threshold != config->radar_err_threshold) {
		dcs_debug("set radar_err_threshold from %d to %d",
			  dcs_param->radar_err_threshold,
			  config->radar_err_threshold);
		bchanged = true;
	}

	if (dcs_param->tx_err_threshold != config->tx_err_threshold) {
		dcs_debug("set tx_err_threshold from %d to %d",
			  dcs_param->tx_err_threshold,
			  config->tx_err_threshold);
		bchanged = true;
	}

	if (dcs_param->intfr_detection_window !=
	    config->intfr_detection_window) {
		dcs_debug("set intfr_detection_window from %d to %d",
			  dcs_param->intfr_detection_window,
			  config->intfr_detection_window);
		bchanged = true;
	}

	if (dcs_param->coch_intfr_threshold != config->coch_intfr_threshold) {
		dcs_debug("set coch_intfr_threshold from %d to %d",
			  dcs_param->coch_intfr_threshold,
			  config->coch_intfr_threshold);
		bchanged = true;
	}

	if (dcs_param->user_max_cu != config->max_cu) {
		dcs_debug("set max_cu from %d to %d",
			  dcs_param->user_max_cu, config->max_cu);
		bchanged = true;
	}

	if (bchanged) {
		dcs_param->intfr_detection_threshold =
			config->intfr_detection_threshold;
		dcs_param->phy_err_penalty = config->phy_err_penalty;
		dcs_param->phy_err_threshold = config->phy_err_threshold;
		dcs_param->radar_err_threshold = config->radar_err_threshold;
		dcs_param->tx_err_threshold = config->tx_err_threshold;
		dcs_param->intfr_detection_window =
			config->intfr_detection_window;
		dcs_param->coch_intfr_threshold = config->coch_intfr_threshold;
		dcs_param->user_max_cu = config->max_cu;
	}

	return QDF_STATUS_SUCCESS;
}
