/*
 * Copyright (c) 2013-2021 The Linux Foundation. All rights reserved.
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

#include <osdep.h>
#include <wmi.h>
#include <wmi_unified_priv.h>
#include <nan_public_structs.h>
#include <wmi_unified_nan_api.h>
#include <wlan_nan_msg_common_v2.h>
#include <wlan_nan_msg.h>

uint8_t cluster_id[NAN_CLUSTER_MATCH_SIZE] = {0x50, 0x6F, 0x9A, 0x01};

/**
 * wmi_nan_get_tlv_type() - get TLV type from NAN DE event
 * @ptlv: pointer to TLV header
 *
 * Return: 16 bit TLV type
 */
static inline uint16_t wmi_nan_get_tlv_type(uint8_t *ptlv)
{
	return (uint16_t)((*ptlv & 0xFF) | ((*(ptlv + 1) & 0xFF) << 8));
}

/**
 * wmi_nan_get_tlv_len() - get TLV length from NAN DE event
 * @ptlv: pointer to TLV header
 *
 * Return: 16 bit TLV length
 */
static inline uint16_t wmi_nan_get_tlv_len(uint8_t *ptlv)
{
	return (uint16_t)((*(ptlv + 2) & 0xFF) | ((*(ptlv + 3) & 0xFF) << 8));
}

#if defined(WLAN_FEATURE_NAN) && defined(FEATURE_WLAN_SUPPORT_NAN_STANDARD_MODE)
static QDF_STATUS
extract_nan_disable_rsp_event_tlv(wmi_unified_t wmi_handle, void *evt_buf,
				  struct nan_event_params *temp_evt_params)
{
	WMI_NAN_DISABLE_CNF_EVENTID_param_tlvs *param_tlvs;
	wmi_nan_disable_cnf_event_fixed_param *fixed_param;

	param_tlvs = (WMI_NAN_DISABLE_CNF_EVENTID_param_tlvs *)evt_buf;
	if (!param_tlvs) {
		wmi_err("Invalid param_tlvs for NAN disable rsp event");
		return QDF_STATUS_E_INVAL;
	}

	fixed_param = param_tlvs->fixed_param;
	if (!fixed_param) {
		wmi_err("Invalid fixed_param for NAN disable rsp event");
		return QDF_STATUS_E_INVAL;
	}

	temp_evt_params->evt_type = nan_event_id_disable_rsp;

	wmi_debug("WMI_NAN_DISABLE_CNF_EVENTID, status %d",
		  fixed_param->status);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS
extract_nan_disable_ind_event_tlv(wmi_unified_t wmi_handle, void *evt_buf,
				  struct nan_event_params *temp_evt_params)
{
	WMI_NAN_DISABLE_IND_EVENTID_param_tlvs *param_tlvs;
	wmi_nan_disable_ind_event_fixed_param *fixed_param;

	param_tlvs = (WMI_NAN_DISABLE_IND_EVENTID_param_tlvs *)evt_buf;
	if (!param_tlvs) {
		wmi_err("Invalid param_tlvs for NAN disable ind event");
		return QDF_STATUS_E_INVAL;
	}

	fixed_param = param_tlvs->fixed_param;
	if (!fixed_param) {
		wmi_err("Invalid fixed_param for NAN disable ind event");
		return QDF_STATUS_E_INVAL;
	}

	temp_evt_params->evt_type = nan_event_id_disable_ind;

	wmi_debug("WMI_NAN_DISABLE_IND_EVENTID");

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS
extract_nan_enable_rsp_event_tlv(wmi_unified_t wmi_handle, void *evt_buf,
				 struct nan_enable_rsp_params *evt_params)
{
	WMI_NAN_ENABLE_RSP_EVENTID_param_tlvs *event;
	wmi_nan_enable_rsp_event_fixed_param *nan_rsp_event;

	event = (WMI_NAN_ENABLE_RSP_EVENTID_param_tlvs *)evt_buf;
	nan_rsp_event = event->fixed_param;

	if (!nan_rsp_event) {
		wmi_err("Invalid nan_enable_rsp evt");
		return QDF_STATUS_E_INVAL;
	}

	evt_params->vdev_id = nan_rsp_event->vdev_id;
	evt_params->status = nan_rsp_event->status;
	evt_params->mac_id = nan_rsp_event->mac_id;

	return QDF_STATUS_SUCCESS;
}
#endif

static QDF_STATUS
extract_nan_event_rsp_tlv(wmi_unified_t wmi_handle, void *evt_buf,
			  struct nan_event_params *evt_params,
			  uint8_t **msg_buf, uint32_t nan_config)
{
	WMI_NAN_EVENTID_param_tlvs *event;
	wmi_nan_event_hdr *nan_rsp_event_hdr;
	nan_msg_header_t *nan_msg_hdr;
	wmi_nan_event_info *nan_evt_info;
	uint8_t *ptlv;
	tNanEventIndMsg *nan_evt_msg;
	uint16_t tlv_type;
	uint16_t tlv_len;
	tpNanCapabilitiesRspMsg capabilities_rsp_msg;
	tpNanCapabilitiesRspParams capabilities_rsp;

	/*
	 * This is how received evt looks like
	 *
	 * <-------------------- evt_buf ----------------------------------->
	 *
	 * <--wmi_nan_event_hdr--><---WMI_TLV_HDR_SIZE---><----- data -------->
	 *
	 * +-----------+---------+-----------------------+--------------------+-
	 * | tlv_header| data_len| WMITLV_TAG_ARRAY_BYTE | nan_rsp_event_data |
	 * +-----------+---------+-----------------------+--------------------+-
	 *
	 * (Only for NAN Enable Resp)
	 * <--wmi_nan_event_info-->
	 * +-----------+-----------+
	 * | tlv_header| event_info|
	 * +-----------+-----------+
	 *
	 */

	event = (WMI_NAN_EVENTID_param_tlvs *)evt_buf;
	nan_rsp_event_hdr = event->fixed_param;

	/* Actual data may include some padding, so data_len <= num_data */
	if (nan_rsp_event_hdr->data_len > event->num_data) {
		wmi_err("Provided NAN event length(%d) exceeding actual length(%d)!",
			 nan_rsp_event_hdr->data_len,
			 event->num_data);
		return QDF_STATUS_E_INVAL;
	}
	evt_params->buf_len = nan_rsp_event_hdr->data_len;
	*msg_buf = event->data;

	if (nan_rsp_event_hdr->data_len < sizeof(nan_msg_header_t) ||
	    nan_rsp_event_hdr->data_len > (WMI_SVC_MSG_MAX_SIZE -
							    WMI_TLV_HDR_SIZE)) {
		wmi_err("Invalid NAN event data length(%d)!",
			 nan_rsp_event_hdr->data_len);
		return QDF_STATUS_E_INVAL;
	}

	if (!event->data) {
		wmi_err("event data is null");
		return QDF_STATUS_E_NULL_VALUE;
	}

	nan_msg_hdr = (nan_msg_header_t *)event->data;

	switch (nan_msg_hdr->msg_id) {
	case NAN_MSG_ID_ENABLE_RSP:
		nan_evt_info = event->event_info;
		if (!nan_evt_info) {
			if (!wmi_service_enabled(wmi_handle,
						 wmi_service_nan_dbs_support) &&
			    !wmi_service_enabled(wmi_handle,
						 wmi_service_nan_disable_support
						 )) {
				evt_params->evt_type = nan_event_id_generic_rsp;
				break;
			} else {
				wmi_err("Fail: NAN enable rsp event info Null");
				return QDF_STATUS_E_INVAL;
			}
		}
		evt_params->evt_type = nan_event_id_enable_rsp;
		evt_params->mac_id = nan_evt_info->mac_id;
		evt_params->is_nan_enable_success = (nan_evt_info->status == 0);
		evt_params->vdev_id = nan_evt_info->vdev_id;
		break;
	case NAN_MSG_ID_DISABLE_RSP:
		evt_params->evt_type = nan_event_id_disable_rsp;
		break;
	case NAN_MSG_ID_DISABLE_IND:
		evt_params->evt_type = nan_event_id_disable_ind;
		break;
	case NAN_MSG_ID_ERROR_RSP:
		evt_params->evt_type = nan_event_id_error_rsp;
		break;
	case NAN_MSG_ID_DE_EVENT_IND:
		evt_params->evt_type = nan_event_id_de_ind;
		nan_evt_msg = (tNanEventIndMsg *)event->data;
		ptlv = (uint8_t *)nan_evt_msg + sizeof(nan_msg_header_t);
		if (!ptlv) {
			wmi_err("DE event TLV is null");
			return QDF_STATUS_E_NULL_VALUE;
		}

		tlv_type = wmi_nan_get_tlv_type(ptlv);
		tlv_len = wmi_nan_get_tlv_len(ptlv);

		if (tlv_len < QDF_MAC_ADDR_SIZE) {
			wmi_err("TLV len %d is less than MAC addr size",
				tlv_len);
			return QDF_STATUS_E_INVAL;
		}

		if (evt_params->buf_len <
		    sizeof(nan_msg_header_t) + WMI_TLV_HDR_SIZE + tlv_len) {
			wmi_err("buf len %d is invalid", evt_params->buf_len);
			return QDF_STATUS_E_INVAL;
		}

		switch (tlv_type) {
		case NAN_TLV_TYPE_SELF_STA_MAC_ADDR:
			ptlv += WMI_TLV_HDR_SIZE;
			qdf_mem_copy(evt_params->nan_mac_addr.bytes, ptlv,
				     QDF_MAC_ADDR_SIZE);
			break;
		default:
			wmi_debug("not parsed tlv_type %d", tlv_type);
			evt_params->evt_type = nan_event_id_generic_rsp;
			break;
		}
		break;
	case NAN_MSG_ID_CAPABILITIES_RSP:
		evt_params->evt_type = nan_event_id_generic_rsp;
		capabilities_rsp_msg = (tNanCapabilitiesRspMsg *)event->data;
		capabilities_rsp = &capabilities_rsp_msg->capabilitiesRspParams;
		wmi_debug("NAN pairing support: %d", nan_config);
		if (!(nan_config & NAN_PARING_BIT)) {
			capabilities_rsp->nanPairingSupported = 0;
			wmi_err("NAN pairing support disabled");
		}
		break;
	default:
		evt_params->evt_type = nan_event_id_generic_rsp;
		break;
	}

	wmi_debug("msg_id %d, evt_type %d", nan_msg_hdr->msg_id,
		  evt_params->evt_type);

	return QDF_STATUS_SUCCESS;
}

/**
 * send_nan_disable_req_cmd_tlv() - to send nan disable request to target
 * @wmi_handle: wmi handle
 * @nan_msg: request data which will be non-null
 *
 * Return: QDF status
 */
static QDF_STATUS send_nan_disable_req_cmd_tlv(wmi_unified_t wmi_handle,
					       struct nan_disable_req *nan_msg)
{
	QDF_STATUS ret;
	wmi_nan_cmd_param *cmd;
	wmi_nan_host_config_param *cfg;
	wmi_buf_t buf;
	/* Initialize with minimum length required, which is Scenario 2*/
	uint16_t len = sizeof(*cmd) + sizeof(*cfg) + 2 * WMI_TLV_HDR_SIZE;
	uint16_t nan_data_len, nan_data_len_aligned = 0;
	uint8_t *buf_ptr;

	/*
	 *  Scenario 1: NAN Disable with NAN msg data from upper layers
	 *
	 *    <-----nan cmd param-----><-- WMI_TLV_HDR_SIZE --><--- data ---->
	 *    +------------+----------+-----------------------+--------------+
	 *    | tlv_header | data_len | WMITLV_TAG_ARRAY_BYTE | nan_msg_data |
	 *    +------------+----------+-----------------------+--------------+
	 *
	 *    <-- WMI_TLV_HDR_SIZE --><------nan host config params----->
	 *   -+-----------------------+---------------------------------+
	 *    | WMITLV_TAG_ARRAY_STRUC| tlv_header | 2g/5g disable flags|
	 *   -+-----------------------+---------------------------------+
	 *
	 * Scenario 2: NAN Disable without any NAN msg data from upper layers
	 *
	 *    <------nan cmd param------><--WMI_TLV_HDR_SIZE--><--WMI_TLV_HDR_SI
	 *    +------------+------------+----------------------+----------------
	 *    | tlv_header | data_len=0 | WMITLV_TAG_ARRAY_BYTE| WMITLV_TAG_ARRA
	 *    +------------+------------+----------------------+----------------
	 *
	 *    ZE----><------nan host config params----->
	 *    -------+---------------------------------+
	 *    Y_STRUC| tlv_header | 2g/5g disable flags|
	 *    -------+---------------------------------+
	 */

	if (!nan_msg) {
		wmi_err("nan req is not valid");
		return QDF_STATUS_E_FAILURE;
	}

	nan_data_len = nan_msg->params.request_data_len;

	if (nan_data_len) {
		nan_data_len_aligned = roundup(nan_data_len, sizeof(uint32_t));
		if (nan_data_len_aligned < nan_data_len) {
			wmi_err("Int overflow while rounding up data_len");
			return QDF_STATUS_E_FAILURE;
		}

		if (nan_data_len_aligned > WMI_SVC_MSG_MAX_SIZE
							- WMI_TLV_HDR_SIZE) {
			wmi_err("nan_data_len exceeding wmi_max_msg_size");
			return QDF_STATUS_E_FAILURE;
		}

		len += nan_data_len_aligned;
	}

	buf = wmi_buf_alloc(wmi_handle, len);
	if (!buf)
		return QDF_STATUS_E_NOMEM;

	buf_ptr = (uint8_t *)wmi_buf_data(buf);
	cmd = (wmi_nan_cmd_param *)buf_ptr;
	WMITLV_SET_HDR(&cmd->tlv_header,
		       WMITLV_TAG_STRUC_wmi_nan_cmd_param,
		       WMITLV_GET_STRUCT_TLVLEN(wmi_nan_cmd_param));

	cmd->data_len = nan_data_len;
	wmi_debug("nan data len value is %u", nan_data_len);
	buf_ptr += sizeof(wmi_nan_cmd_param);

	WMITLV_SET_HDR(buf_ptr, WMITLV_TAG_ARRAY_BYTE, nan_data_len_aligned);
	buf_ptr += WMI_TLV_HDR_SIZE;

	if (nan_data_len) {
		qdf_mem_copy(buf_ptr, nan_msg->params.request_data,
			     cmd->data_len);
		buf_ptr += nan_data_len_aligned;
	}

	WMITLV_SET_HDR(buf_ptr, WMITLV_TAG_ARRAY_STRUC,
		       sizeof(wmi_nan_host_config_param));
	buf_ptr += WMI_TLV_HDR_SIZE;

	cfg = (wmi_nan_host_config_param *)buf_ptr;
	WMITLV_SET_HDR(&cfg->tlv_header,
		       WMITLV_TAG_STRUC_wmi_nan_host_config_param,
		       WMITLV_GET_STRUCT_TLVLEN(wmi_nan_host_config_param));
	cfg->nan_2g_disc_disable = nan_msg->disable_2g_discovery;
	cfg->nan_5g_disc_disable = nan_msg->disable_5g_discovery;

	wmi_mtrace(WMI_NAN_CMDID, NO_SESSION, 0);
	ret = wmi_unified_cmd_send(wmi_handle, buf, len,
				   WMI_NAN_CMDID);
	if (QDF_IS_STATUS_ERROR(ret)) {
		wmi_err("Failed to send set param command ret = %d", ret);
		wmi_buf_free(buf);
	}

	return ret;
}

#if defined(WLAN_FEATURE_NAN) && defined(FEATURE_WLAN_SUPPORT_NAN_STANDARD_MODE)
/**
 * wmi_nan_populate_band_cfg() - Populate NAN band configuration parameters
 * @buf_ptr: Pointer to pointer to buffer where band configs will be written
 * @nan_conf: Pointer to NAN configuration
 *
 * This function iterates through all bands and populates the band configuration
 * parameters for each enabled band in the NAN configuration.
 *
 * Return: None (buffer pointer is updated through the parameter)
 */
static inline void
wmi_nan_populate_band_cfg(uint8_t **buf_ptr, struct nan_conf *nan_conf)
{
	uint32_t i;
	wmi_nan_disc_band_config_param *band_cfg;

	for (i = 0; i < NUM_NL80211_BANDS; i++) {
		if (nan_conf->bands & BIT(i)) {
			band_cfg = (wmi_nan_disc_band_config_param *)(*buf_ptr);
			WMITLV_SET_HDR(
				&band_cfg->tlv_header,
				WMITLV_TAG_STRUC_wmi_nan_disc_band_config_param,
				WMITLV_GET_STRUCT_TLVLEN(
				wmi_nan_disc_band_config_param));
			if (nan_conf->band_cfgs[i].freq)
				band_cfg->chan_freq =
					nan_conf->band_cfgs[i].freq;

			band_cfg->rssi_close_val =
				nan_conf->band_cfgs[i].rssi_close;
			band_cfg->rssi_middle_val =
				nan_conf->band_cfgs[i].rssi_middle;
			band_cfg->awake_dw_intval =
				nan_conf->band_cfgs[i].awake_dw_interval;
			WMI_NAN_DISC_SCAN_PARAMS_SET_DWELL_MS(
					band_cfg->dwell_ms,
					nan_conf->scan_dwell_time);
			WMI_NAN_DISC_SCAN_PARAMS_SET_INTERVAL_SEC(
					band_cfg->scan_params_word,
					nan_conf->scan_period);

			*buf_ptr += sizeof(*band_cfg);
		}
	}
}

/**
 * send_nan_stop_req_cmd_tlv() - to send nan stop request to target
 * @wmi_handle: wmi handle
 * @nan_req: request data which will be non-null
 *
 * Return: QDF status
 */
static QDF_STATUS send_nan_stop_req_cmd_tlv(wmi_unified_t wmi_handle,
					    struct nan_disable_req *nan_req)
{
	wmi_nan_disable_cmd_fixed_param *cmd;
	wmi_buf_t buf;
	uint16_t len;
	QDF_STATUS ret;

	len = sizeof(*cmd);
	buf = wmi_buf_alloc(wmi_handle, len);
	if (!buf)
		return QDF_STATUS_E_NOMEM;

	cmd = (wmi_nan_disable_cmd_fixed_param *)wmi_buf_data(buf);
	qdf_mem_zero(cmd, sizeof(*cmd));
	WMITLV_SET_HDR(&cmd->tlv_header,
		       WMITLV_TAG_STRUC_wmi_nan_disable_cmd_fixed_param,
		       WMITLV_GET_STRUCT_TLVLEN(
				wmi_nan_disable_cmd_fixed_param));
	cmd->vdev_id = nan_req->vdev_id;

	wmi_mtrace(WMI_NAN_DISABLE_CMDID, NO_SESSION, 0);
	ret = wmi_unified_cmd_send(wmi_handle, buf, len,
				   WMI_NAN_DISABLE_CMDID);
	if (QDF_IS_STATUS_ERROR(ret)) {
		wmi_err("Failed to send NAN stop/disable command ret = %d",
			ret);
		wmi_buf_free(buf);
	}

	return ret;
}

/**
 * send_nan_start_req_cmd_tlv() - to send nan start request to target
 * @wmi_handle: wmi handle
 * @nan_req: request data which will be non-null
 *
 * Return: QDF status
 */
static QDF_STATUS send_nan_start_req_cmd_tlv(wmi_unified_t wmi_handle,
					     struct nan_enable_req *nan_req)
{
	wmi_nan_enable_cmd_fixed_param *cmd;
	wmi_buf_t buf;
	uint32_t len;
	uint16_t i = 0;
	QDF_STATUS ret;
	uint8_t *buf_ptr;
	wmi_nan_ctrl_config_param *ctrl_cfg;
	uint32_t vdev_id;
	uint32_t band_len = 0;

	vdev_id = nan_req->vdev_id;
	for (i = 0; i < NUM_NL80211_BANDS; i++) {
		if (nan_req->nan_conf.bands & BIT(i))
			band_len++;
	}
	len = sizeof(wmi_nan_enable_cmd_fixed_param) +
		sizeof(wmi_nan_ctrl_config_param) + WMI_TLV_HDR_SIZE +
		+ WMI_TLV_HDR_SIZE
		+ (sizeof(wmi_nan_disc_band_config_param) * band_len);

	buf = wmi_buf_alloc(wmi_handle, len);
	if (!buf)
		return QDF_STATUS_E_NOMEM;

	buf_ptr = (uint8_t *)wmi_buf_data(buf);
	cmd = (wmi_nan_enable_cmd_fixed_param *)buf_ptr;
	WMITLV_SET_HDR(
		&cmd->tlv_header,
		WMITLV_TAG_STRUC_wmi_nan_enable_cmd_fixed_param,
		WMITLV_GET_STRUCT_TLVLEN(wmi_nan_enable_cmd_fixed_param));
	cmd->vdev_id = vdev_id;
	buf_ptr += sizeof(wmi_nan_enable_cmd_fixed_param);

	WMITLV_SET_HDR(buf_ptr, WMITLV_TAG_ARRAY_STRUC,
		       sizeof(wmi_nan_ctrl_config_param));
	buf_ptr += WMI_TLV_HDR_SIZE;
	ctrl_cfg = (wmi_nan_ctrl_config_param *)buf_ptr;
	WMITLV_SET_HDR(&ctrl_cfg->tlv_header,
		       WMITLV_TAG_STRUC_wmi_nan_ctrl_config_param,
		       WMITLV_GET_STRUCT_TLVLEN(wmi_nan_ctrl_config_param));
	ctrl_cfg->master_pref = nan_req->nan_conf.master_pref;
	if (nan_req->nan_conf.cluster_id)
		ctrl_cfg->cluster_id = (nan_req->nan_conf.cluster_id[4] << 8) |
				       nan_req->nan_conf.cluster_id[5];
	else
		ctrl_cfg->cluster_id = 0;
	ctrl_cfg->supp_disc_bands = nan_req->nan_conf.bands & 0x3;
	buf_ptr += sizeof(wmi_nan_ctrl_config_param);

	WMITLV_SET_HDR(buf_ptr, WMITLV_TAG_ARRAY_STRUC,
		       (sizeof(wmi_nan_disc_band_config_param) * band_len));
	buf_ptr += WMI_TLV_HDR_SIZE;

	wmi_nan_populate_band_cfg(&buf_ptr, &nan_req->nan_conf);

	wmi_mtrace(WMI_NAN_ENABLE_CMDID, NO_SESSION, 0);
	ret = wmi_unified_cmd_send(wmi_handle, buf, len,
				   WMI_NAN_ENABLE_CMDID);
	if (QDF_IS_STATUS_ERROR(ret)) {
		wmi_err("Failed to send NAN enable command ret = %d", ret);
		wmi_buf_free(buf);
	}

	return ret;
}

/**
 * send_nan_change_conf_req_cmd_tlv() - to send nan config change to target
 * @wmi_handle: wmi handle
 * @nan_req: request data which will be non-null
 *
 * Return: QDF status
 */
static QDF_STATUS
send_nan_change_conf_req_cmd_tlv(wmi_unified_t wmi_handle,
				 struct nan_change_conf_req *nan_req)
{
	wmi_nan_config_cmd_fixed_param *cmd;
	wmi_buf_t buf;
	uint16_t len, i = 0;
	QDF_STATUS ret;
	uint8_t *buf_ptr;
	wmi_nan_ctrl_config_param *ctrl_cfg;
	uint32_t vdev_id;
	uint32_t band_len = 0;
	uint16_t nan_avail_attr_len = 0;

	vdev_id = nan_req->vdev_id;
	for (i = 0; i < NUM_NL80211_BANDS; i++) {
		if (nan_req->nan_conf.bands & (1 << i))
			band_len++;
	}
	len = sizeof(wmi_nan_config_cmd_fixed_param) +
		sizeof(wmi_nan_ctrl_config_param) + WMI_TLV_HDR_SIZE +
		WMI_TLV_HDR_SIZE +
		(sizeof(wmi_nan_disc_band_config_param) * band_len);

	if (nan_req->nan_conf.extra_nan_attrs_len) {
		nan_avail_attr_len =
			roundup(nan_req->nan_conf.extra_nan_attrs_len,
				sizeof(uint32_t));
		len += WMI_TLV_HDR_SIZE + nan_avail_attr_len;
	}

	buf = wmi_buf_alloc(wmi_handle, len);
	if (!buf)
		return QDF_STATUS_E_NOMEM;

	buf_ptr = (uint8_t *)wmi_buf_data(buf);
	cmd = (wmi_nan_config_cmd_fixed_param *)buf_ptr;
	WMITLV_SET_HDR(&cmd->tlv_header,
		       WMITLV_TAG_STRUC_wmi_nan_config_cmd_fixed_param,
		       WMITLV_GET_STRUCT_TLVLEN(
					wmi_nan_config_cmd_fixed_param));
	cmd->vdev_id = vdev_id;
	cmd->nan_conf_change_bitmap = nan_req->param_bit_map;
	cmd->nan_availability_attributes_len =
					nan_req->nan_conf.extra_nan_attrs_len;

	buf_ptr += sizeof(wmi_nan_config_cmd_fixed_param);

	WMITLV_SET_HDR(buf_ptr, WMITLV_TAG_ARRAY_STRUC,
		       sizeof(wmi_nan_ctrl_config_param));
	buf_ptr += WMI_TLV_HDR_SIZE;
	ctrl_cfg = (wmi_nan_ctrl_config_param *)buf_ptr;
	WMITLV_SET_HDR(&ctrl_cfg->tlv_header,
		       WMITLV_TAG_STRUC_wmi_nan_ctrl_config_param,
		       WMITLV_GET_STRUCT_TLVLEN(wmi_nan_ctrl_config_param));
	ctrl_cfg->master_pref = nan_req->nan_conf.master_pref;
	if (nan_req->nan_conf.cluster_id)
		ctrl_cfg->cluster_id = (nan_req->nan_conf.cluster_id[4] << 8) |
				       nan_req->nan_conf.cluster_id[5];
	else
		ctrl_cfg->cluster_id = 0;
	ctrl_cfg->supp_disc_bands = nan_req->nan_conf.bands & 0x3;
	buf_ptr += sizeof(wmi_nan_ctrl_config_param);

	WMITLV_SET_HDR(buf_ptr, WMITLV_TAG_ARRAY_STRUC,
		       (sizeof(wmi_nan_disc_band_config_param) * band_len));
	buf_ptr += WMI_TLV_HDR_SIZE;

	wmi_nan_populate_band_cfg(&buf_ptr, &nan_req->nan_conf);

	if (cmd->nan_availability_attributes_len) {
		wmi_debug("NAN availability attribute");
		WMITLV_SET_HDR(buf_ptr, WMITLV_TAG_ARRAY_BYTE,
			       nan_avail_attr_len);
		buf_ptr += WMI_TLV_HDR_SIZE;
		qdf_mem_copy(buf_ptr, nan_req->nan_conf.extra_nan_attrs,
			     cmd->nan_availability_attributes_len);
		buf_ptr += nan_avail_attr_len;
	}

	wmi_mtrace(WMI_NAN_CONFIG_CMDID, NO_SESSION, 0);
	ret = wmi_unified_cmd_send(wmi_handle, buf, len,
				   WMI_NAN_CONFIG_CMDID);
	if (QDF_IS_STATUS_ERROR(ret)) {
		wmi_err("Failed to send NAN config command ret = %d", ret);
		wmi_buf_free(buf);
	}

	return ret;
}

#endif

/**
 * send_nan_req_cmd_tlv() - to send nan request to target
 * @wmi_handle: wmi handle
 * @nan_msg: request data which will be non-null
 *
 * Return: QDF status
 */
static QDF_STATUS send_nan_req_cmd_tlv(wmi_unified_t wmi_handle,
				       struct nan_msg_params *nan_msg)
{
	QDF_STATUS ret;
	wmi_nan_cmd_param *cmd;
	wmi_buf_t buf;
	wmi_nan_host_config_param *cfg;
	uint16_t len = sizeof(*cmd) + sizeof(*cfg) + 2 * WMI_TLV_HDR_SIZE;
	uint16_t nan_data_len, nan_data_len_aligned;
	uint8_t *buf_ptr;

	/*
	 *    <----- cmd ------------><-- WMI_TLV_HDR_SIZE --><--- data ---->
	 *    +------------+----------+-----------------------+--------------+
	 *    | tlv_header | data_len | WMITLV_TAG_ARRAY_BYTE | nan_msg_data |
	 *    +------------+----------+-----------------------+--------------+
	 *
	 *    <-- WMI_TLV_HDR_SIZE --><------nan host config params-------->
	 *    +-----------------------+------------------------------------+
	 *    | WMITLV_TAG_ARRAY_STRUC| tlv_header | disable flags | flags |
	 *    +-----------------------+------------------------------------+
	 */
	if (!nan_msg) {
		wmi_err("nan req is not valid");
		return QDF_STATUS_E_FAILURE;
	}
	nan_data_len = nan_msg->request_data_len;
	nan_data_len_aligned = roundup(nan_msg->request_data_len,
				       sizeof(uint32_t));
	if (nan_data_len_aligned < nan_msg->request_data_len) {
		wmi_err("integer overflow while rounding up data_len");
		return QDF_STATUS_E_FAILURE;
	}

	if (nan_data_len_aligned > WMI_SVC_MSG_MAX_SIZE - WMI_TLV_HDR_SIZE) {
		wmi_err("wmi_max_msg_size overflow for given datalen");
		return QDF_STATUS_E_FAILURE;
	}

	len += nan_data_len_aligned;
	buf = wmi_buf_alloc(wmi_handle, len);
	if (!buf)
		return QDF_STATUS_E_NOMEM;

	buf_ptr = (uint8_t *)wmi_buf_data(buf);
	cmd = (wmi_nan_cmd_param *)buf_ptr;
	WMITLV_SET_HDR(&cmd->tlv_header,
		       WMITLV_TAG_STRUC_wmi_nan_cmd_param,
		       WMITLV_GET_STRUCT_TLVLEN(wmi_nan_cmd_param));
	cmd->data_len = nan_msg->request_data_len;
	buf_ptr += sizeof(wmi_nan_cmd_param);
	WMITLV_SET_HDR(buf_ptr, WMITLV_TAG_ARRAY_BYTE, nan_data_len_aligned);
	buf_ptr += WMI_TLV_HDR_SIZE;
	qdf_mem_copy(buf_ptr, nan_msg->request_data, cmd->data_len);
	buf_ptr += nan_data_len_aligned;

	WMITLV_SET_HDR(buf_ptr, WMITLV_TAG_ARRAY_STRUC,
		       sizeof(wmi_nan_host_config_param));
	buf_ptr += WMI_TLV_HDR_SIZE;

	cfg = (wmi_nan_host_config_param *)buf_ptr;
	WMITLV_SET_HDR(&cfg->tlv_header,
		       WMITLV_TAG_STRUC_wmi_nan_host_config_param,
		       WMITLV_GET_STRUCT_TLVLEN(wmi_nan_host_config_param));

	WMI_NAN_SET_RANGING_INITIATOR_ROLE(cfg->flags, !!(nan_msg->rtt_cap &
					   WMI_FW_NAN_RTT_INITR));
	WMI_NAN_SET_RANGING_RESPONDER_ROLE(cfg->flags, !!(nan_msg->rtt_cap &
					   WMI_FW_NAN_RTT_RESPR));
	WMI_NAN_SET_NAN_6G_DISABLE(cfg->flags, nan_msg->disable_6g_nan);

	wmi_mtrace(WMI_NAN_CMDID, NO_SESSION, 0);
	ret = wmi_unified_cmd_send(wmi_handle, buf, len, WMI_NAN_CMDID);
	if (QDF_IS_STATUS_ERROR(ret)) {
		wmi_err("Failed to send NAN req command ret = %d", ret);
		wmi_buf_free(buf);
	}

	return ret;
}

/**
 * send_terminate_all_ndps_cmd_tlv() - send NDP Terminate for all NDP's
 * associated with the given vdev id
 * @wmi_handle: wmi handle
 * @vdev_id: vdev id
 *
 * Return: QDF status
 */
static QDF_STATUS send_terminate_all_ndps_cmd_tlv(wmi_unified_t wmi_handle,
						  uint32_t vdev_id)
{
	wmi_ndp_cmd_param *cmd;
	wmi_buf_t wmi_buf;
	uint32_t len;
	QDF_STATUS status;

	wmi_debug("Enter");

	len = sizeof(*cmd);
	wmi_buf = wmi_buf_alloc(wmi_handle, len);
	if (!wmi_buf)
		return QDF_STATUS_E_NOMEM;

	cmd = (wmi_ndp_cmd_param *)wmi_buf_data(wmi_buf);

	WMITLV_SET_HDR(&cmd->tlv_header, WMITLV_TAG_STRUC_wmi_ndp_cmd_param,
		       WMITLV_GET_STRUCT_TLVLEN(wmi_ndp_cmd_param));

	cmd->vdev_id = vdev_id;
	cmd->ndp_disable = 1;

	wmi_mtrace(WMI_NDP_CMDID, NO_SESSION, 0);
	status = wmi_unified_cmd_send(wmi_handle, wmi_buf, len, WMI_NDP_CMDID);
	if (QDF_IS_STATUS_ERROR(status)) {
		wmi_err("Failed to send NDP Terminate cmd: %d", status);
		wmi_buf_free(wmi_buf);
	}

	return status;
}

static QDF_STATUS nan_ndp_initiator_req_tlv(wmi_unified_t wmi_handle,
				struct nan_datapath_initiator_req *ndp_req)
{
	uint16_t len;
	wmi_buf_t buf;
	uint8_t *tlv_ptr;
	QDF_STATUS status;
	wmi_channel *ch_tlv;
	wmi_ndp_initiator_req_fixed_param *cmd;
	uint32_t passphrase_len, service_name_len;
	uint32_t ndp_cfg_len, ndp_app_info_len, pmk_len;
	wmi_ndp_transport_ip_param *tcp_ip_param;

	/*
	 * WMI command expects 4 byte aligned len:
	 * round up ndp_cfg_len and ndp_app_info_len to 4 bytes
	 */
	ndp_cfg_len = qdf_roundup(ndp_req->ndp_config.ndp_cfg_len, 4);
	ndp_app_info_len = qdf_roundup(ndp_req->ndp_info.ndp_app_info_len, 4);
	pmk_len = qdf_roundup(ndp_req->pmk.pmk_len, 4);
	passphrase_len = qdf_roundup(ndp_req->passphrase.passphrase_len, 4);
	service_name_len =
		   qdf_roundup(ndp_req->service_name.service_name_len, 4);
	/* allocated memory for fixed params as well as variable size data */
	len = sizeof(*cmd) + sizeof(*ch_tlv) + (5 * WMI_TLV_HDR_SIZE)
		+ ndp_cfg_len + ndp_app_info_len + pmk_len
		+ passphrase_len + service_name_len;

	if (ndp_req->is_ipv6_addr_present)
		len += sizeof(*tcp_ip_param);

	buf = wmi_buf_alloc(wmi_handle, len);
	if (!buf) {
		return QDF_STATUS_E_NOMEM;
	}

	cmd = (wmi_ndp_initiator_req_fixed_param *) wmi_buf_data(buf);
	WMITLV_SET_HDR(&cmd->tlv_header,
		       WMITLV_TAG_STRUC_wmi_ndp_initiator_req_fixed_param,
		       WMITLV_GET_STRUCT_TLVLEN(
				wmi_ndp_initiator_req_fixed_param));
	cmd->vdev_id = wlan_vdev_get_id(ndp_req->vdev);
	cmd->transaction_id = ndp_req->transaction_id;
	cmd->service_instance_id = ndp_req->service_instance_id;
	WMI_CHAR_ARRAY_TO_MAC_ADDR(ndp_req->peer_discovery_mac_addr.bytes,
				   &cmd->peer_discovery_mac_addr);

	cmd->ndp_cfg_len = ndp_req->ndp_config.ndp_cfg_len;
	cmd->ndp_app_info_len = ndp_req->ndp_info.ndp_app_info_len;
	cmd->ndp_channel_cfg = ndp_req->channel_cfg;
	cmd->nan_pmk_len = ndp_req->pmk.pmk_len;
	cmd->nan_csid = ndp_req->ncs_sk_type;
	cmd->nan_passphrase_len = ndp_req->passphrase.passphrase_len;
	cmd->nan_servicename_len = ndp_req->service_name.service_name_len;
	cmd->nan_csid_cap = ndp_req->ndp_add_params.csid_cap;
	cmd->nan_gtk_required = ndp_req->ndp_add_params.gtk;

	ch_tlv = (wmi_channel *)&cmd[1];
	WMITLV_SET_HDR(ch_tlv, WMITLV_TAG_STRUC_wmi_channel,
		       WMITLV_GET_STRUCT_TLVLEN(wmi_channel));
	ch_tlv->mhz = ndp_req->channel;
	tlv_ptr = (uint8_t *)&ch_tlv[1];

	WMITLV_SET_HDR(tlv_ptr, WMITLV_TAG_ARRAY_BYTE, ndp_cfg_len);
	qdf_mem_copy(&tlv_ptr[WMI_TLV_HDR_SIZE],
		     ndp_req->ndp_config.ndp_cfg, cmd->ndp_cfg_len);
	tlv_ptr = tlv_ptr + WMI_TLV_HDR_SIZE + ndp_cfg_len;

	WMITLV_SET_HDR(tlv_ptr, WMITLV_TAG_ARRAY_BYTE, ndp_app_info_len);
	qdf_mem_copy(&tlv_ptr[WMI_TLV_HDR_SIZE],
		     ndp_req->ndp_info.ndp_app_info, cmd->ndp_app_info_len);
	tlv_ptr = tlv_ptr + WMI_TLV_HDR_SIZE + ndp_app_info_len;

	WMITLV_SET_HDR(tlv_ptr, WMITLV_TAG_ARRAY_BYTE, pmk_len);
	qdf_mem_copy(&tlv_ptr[WMI_TLV_HDR_SIZE], ndp_req->pmk.pmk,
		     cmd->nan_pmk_len);
	tlv_ptr = tlv_ptr + WMI_TLV_HDR_SIZE + pmk_len;

	WMITLV_SET_HDR(tlv_ptr, WMITLV_TAG_ARRAY_BYTE, passphrase_len);
	qdf_mem_copy(&tlv_ptr[WMI_TLV_HDR_SIZE], ndp_req->passphrase.passphrase,
		     cmd->nan_passphrase_len);
	tlv_ptr = tlv_ptr + WMI_TLV_HDR_SIZE + passphrase_len;

	WMITLV_SET_HDR(tlv_ptr, WMITLV_TAG_ARRAY_BYTE, service_name_len);
	qdf_mem_copy(&tlv_ptr[WMI_TLV_HDR_SIZE],
		     ndp_req->service_name.service_name,
		     cmd->nan_servicename_len);
	tlv_ptr = tlv_ptr + WMI_TLV_HDR_SIZE + service_name_len;

	if (ndp_req->is_ipv6_addr_present) {
		tcp_ip_param = (wmi_ndp_transport_ip_param *)tlv_ptr;
		WMITLV_SET_HDR(tcp_ip_param,
			       WMITLV_TAG_STRUC_wmi_ndp_transport_ip_param,
			       WMITLV_GET_STRUCT_TLVLEN(
						wmi_ndp_transport_ip_param));
		tcp_ip_param->ipv6_addr_present = true;
		qdf_mem_copy(tcp_ip_param->ipv6_intf_addr,
			     ndp_req->ipv6_addr, WMI_NDP_IPV6_INTF_ADDR_LEN);
	}
	wmi_debug("IPv6 addr present: %d, addr: %pI6",
		 ndp_req->is_ipv6_addr_present, ndp_req->ipv6_addr);

	wmi_debug("vdev_id = %d, transaction_id: %d, service_instance_id: %d, ch: %d, ch_cfg: %d, csid: %d peer mac addr: mac_addr31to0: 0x%x, mac_addr47to32: 0x%x",
		 cmd->vdev_id, cmd->transaction_id, cmd->service_instance_id,
		 ch_tlv->mhz, cmd->ndp_channel_cfg, cmd->nan_csid,
		 cmd->peer_discovery_mac_addr.mac_addr31to0,
		 cmd->peer_discovery_mac_addr.mac_addr47to32);

	wmi_debug("ndp_config len: %d ndp_app_info len: %d pmk len: %d pass phrase len: %d service name len: %d",
		 cmd->ndp_cfg_len, cmd->ndp_app_info_len, cmd->nan_pmk_len,
		 cmd->nan_passphrase_len, cmd->nan_servicename_len);

	wmi_debug("ndp_csid_cap %d, ndp_gtk_required %d", cmd->nan_csid_cap,
		  cmd->nan_gtk_required);

	wmi_mtrace(WMI_NDP_INITIATOR_REQ_CMDID, cmd->vdev_id, 0);
	status = wmi_unified_cmd_send(wmi_handle, buf, len,
				      WMI_NDP_INITIATOR_REQ_CMDID);
	if (QDF_IS_STATUS_ERROR(status)) {
		wmi_err("WMI_NDP_INITIATOR_REQ_CMDID failed, ret: %d", status);
		wmi_buf_free(buf);
	}

	return status;
}

static QDF_STATUS nan_ndp_responder_req_tlv(wmi_unified_t wmi_handle,
					struct nan_datapath_responder_req *req)
{
	uint16_t len;
	wmi_buf_t buf;
	uint8_t *tlv_ptr;
	QDF_STATUS status;
	wmi_ndp_responder_req_fixed_param *cmd;
	wmi_ndp_transport_ip_param *tcp_ip_param;
	uint32_t passphrase_len, service_name_len;
	uint32_t vdev_id = 0, ndp_cfg_len, ndp_app_info_len, pmk_len;

	vdev_id = wlan_vdev_get_id(req->vdev);
	wmi_debug("vdev_id: %d, transaction_id: %d, ndp_rsp %d, ndp_instance_id: %d, ndp_app_info_len: %d",
		 vdev_id, req->transaction_id,
		 req->ndp_rsp,
		 req->ndp_instance_id,
		 req->ndp_info.ndp_app_info_len);

	/*
	 * WMI command expects 4 byte aligned len:
	 * round up ndp_cfg_len and ndp_app_info_len to 4 bytes
	 */
	ndp_cfg_len = qdf_roundup(req->ndp_config.ndp_cfg_len, 4);
	ndp_app_info_len = qdf_roundup(req->ndp_info.ndp_app_info_len, 4);
	pmk_len = qdf_roundup(req->pmk.pmk_len, 4);
	passphrase_len = qdf_roundup(req->passphrase.passphrase_len, 4);
	service_name_len =
		qdf_roundup(req->service_name.service_name_len, 4);

	/* allocated memory for fixed params as well as variable size data */
	len = sizeof(*cmd) + 5*WMI_TLV_HDR_SIZE + ndp_cfg_len + ndp_app_info_len
		+ pmk_len + passphrase_len + service_name_len;

	if (req->is_ipv6_addr_present || req->is_port_present ||
	    req->is_protocol_present)
		len += sizeof(*tcp_ip_param);

	buf = wmi_buf_alloc(wmi_handle, len);
	if (!buf) {
		return QDF_STATUS_E_NOMEM;
	}
	cmd = (wmi_ndp_responder_req_fixed_param *) wmi_buf_data(buf);
	WMITLV_SET_HDR(&cmd->tlv_header,
		       WMITLV_TAG_STRUC_wmi_ndp_responder_req_fixed_param,
		       WMITLV_GET_STRUCT_TLVLEN(
				wmi_ndp_responder_req_fixed_param));
	cmd->vdev_id = vdev_id;
	cmd->transaction_id = req->transaction_id;
	cmd->ndp_instance_id = req->ndp_instance_id;
	cmd->rsp_code = req->ndp_rsp;
	cmd->ndp_cfg_len = req->ndp_config.ndp_cfg_len;
	cmd->ndp_app_info_len = req->ndp_info.ndp_app_info_len;
	cmd->nan_pmk_len = req->pmk.pmk_len;
	cmd->nan_csid = req->ncs_sk_type;
	cmd->nan_passphrase_len = req->passphrase.passphrase_len;
	cmd->nan_servicename_len = req->service_name.service_name_len;
	cmd->nan_csid_cap = req->ndp_add_params.csid_cap;
	cmd->nan_gtk_required = req->ndp_add_params.gtk;

	tlv_ptr = (uint8_t *)&cmd[1];
	WMITLV_SET_HDR(tlv_ptr, WMITLV_TAG_ARRAY_BYTE, ndp_cfg_len);
	qdf_mem_copy(&tlv_ptr[WMI_TLV_HDR_SIZE],
		     req->ndp_config.ndp_cfg, cmd->ndp_cfg_len);

	tlv_ptr = tlv_ptr + WMI_TLV_HDR_SIZE + ndp_cfg_len;
	WMITLV_SET_HDR(tlv_ptr, WMITLV_TAG_ARRAY_BYTE, ndp_app_info_len);
	qdf_mem_copy(&tlv_ptr[WMI_TLV_HDR_SIZE],
		     req->ndp_info.ndp_app_info,
		     req->ndp_info.ndp_app_info_len);

	tlv_ptr = tlv_ptr + WMI_TLV_HDR_SIZE + ndp_app_info_len;
	WMITLV_SET_HDR(tlv_ptr, WMITLV_TAG_ARRAY_BYTE, pmk_len);
	qdf_mem_copy(&tlv_ptr[WMI_TLV_HDR_SIZE], req->pmk.pmk,
		     cmd->nan_pmk_len);

	tlv_ptr = tlv_ptr + WMI_TLV_HDR_SIZE + pmk_len;
	WMITLV_SET_HDR(tlv_ptr, WMITLV_TAG_ARRAY_BYTE, passphrase_len);
	qdf_mem_copy(&tlv_ptr[WMI_TLV_HDR_SIZE],
		     req->passphrase.passphrase,
		     cmd->nan_passphrase_len);
	tlv_ptr = tlv_ptr + WMI_TLV_HDR_SIZE + passphrase_len;

	WMITLV_SET_HDR(tlv_ptr, WMITLV_TAG_ARRAY_BYTE, service_name_len);
	qdf_mem_copy(&tlv_ptr[WMI_TLV_HDR_SIZE],
		     req->service_name.service_name,
		     cmd->nan_servicename_len);

	tlv_ptr = tlv_ptr + WMI_TLV_HDR_SIZE + service_name_len;

	if (req->is_ipv6_addr_present || req->is_port_present ||
	    req->is_protocol_present) {
		tcp_ip_param = (wmi_ndp_transport_ip_param *)tlv_ptr;
		WMITLV_SET_HDR(tcp_ip_param,
			       WMITLV_TAG_STRUC_wmi_ndp_transport_ip_param,
			       WMITLV_GET_STRUCT_TLVLEN(
						wmi_ndp_transport_ip_param));
		tcp_ip_param->ipv6_addr_present = req->is_ipv6_addr_present;
		qdf_mem_copy(tcp_ip_param->ipv6_intf_addr,
			     req->ipv6_addr, WMI_NDP_IPV6_INTF_ADDR_LEN);

		tcp_ip_param->trans_port_present = req->is_port_present;
		tcp_ip_param->transport_port = req->port;

		tcp_ip_param->trans_proto_present = req->is_protocol_present;
		tcp_ip_param->transport_protocol = req->protocol;
	}

	wmi_debug("ndp_config len: %d ndp_app_info len: %d pmk len: %d pass phrase len: %d service name len: %d",
		 req->ndp_config.ndp_cfg_len, req->ndp_info.ndp_app_info_len,
		 cmd->nan_pmk_len, cmd->nan_passphrase_len,
		 cmd->nan_servicename_len);

	wmi_debug("ndp_csid_cap %d, ndp_gtk_required %d", cmd->nan_csid_cap,
		  cmd->nan_gtk_required);

	wmi_mtrace(WMI_NDP_RESPONDER_REQ_CMDID, cmd->vdev_id, 0);
	status = wmi_unified_cmd_send(wmi_handle, buf, len,
				      WMI_NDP_RESPONDER_REQ_CMDID);
	if (QDF_IS_STATUS_ERROR(status)) {
		wmi_err("WMI_NDP_RESPONDER_REQ_CMDID failed, ret: %d", status);
		wmi_buf_free(buf);
	}
	return status;
}

static QDF_STATUS nan_ndp_end_req_tlv(wmi_unified_t wmi_handle,
				      struct nan_datapath_end_req *req)
{
	uint16_t len;
	wmi_buf_t buf;
	QDF_STATUS status;
	uint32_t ndp_end_req_len, i;
	wmi_ndp_end_req *ndp_end_req_lst;
	wmi_ndp_end_req_fixed_param *cmd;

	/* len of tlv following fixed param  */
	ndp_end_req_len = sizeof(wmi_ndp_end_req) * req->num_ndp_instances;
	/* above comes out to 4 byte aligned already, no need of padding */
	len = sizeof(*cmd) + ndp_end_req_len + WMI_TLV_HDR_SIZE;
	buf = wmi_buf_alloc(wmi_handle, len);
	if (!buf) {
		return QDF_STATUS_E_NOMEM;
	}

	cmd = (wmi_ndp_end_req_fixed_param *) wmi_buf_data(buf);
	WMITLV_SET_HDR(&cmd->tlv_header,
		       WMITLV_TAG_STRUC_wmi_ndp_end_req_fixed_param,
		       WMITLV_GET_STRUCT_TLVLEN(wmi_ndp_end_req_fixed_param));

	cmd->transaction_id = req->transaction_id;

	/* set tlv pointer to end of fixed param */
	WMITLV_SET_HDR((uint8_t *)&cmd[1], WMITLV_TAG_ARRAY_STRUC,
			ndp_end_req_len);

	ndp_end_req_lst = (wmi_ndp_end_req *)((uint8_t *)&cmd[1] +
						WMI_TLV_HDR_SIZE);
	for (i = 0; i < req->num_ndp_instances; i++) {
		WMITLV_SET_HDR(&ndp_end_req_lst[i],
			       WMITLV_TAG_ARRAY_FIXED_STRUC,
			       (sizeof(*ndp_end_req_lst) - WMI_TLV_HDR_SIZE));

		ndp_end_req_lst[i].ndp_instance_id = req->ndp_ids[i];

		/*
		 * vdev_id is added in NDP END TLV to facilitate fw to give it
		 * back in the NDP END indication.
		 */
		if (req->vdev) {
			ndp_end_req_lst[i].vdev_id =
						wlan_vdev_get_id(req->vdev);
			ndp_end_req_lst[i].vdev_id_valid = 1;
		}
	}

	wmi_mtrace(WMI_NDP_END_REQ_CMDID, NO_SESSION, 0);
	status = wmi_unified_cmd_send(wmi_handle, buf, len,
				      WMI_NDP_END_REQ_CMDID);
	if (QDF_IS_STATUS_ERROR(status)) {
		wmi_err("WMI_NDP_END_REQ_CMDID failed, ret: %d", status);
		wmi_buf_free(buf);
	}

	return status;
}

static QDF_STATUS
nan_ndp_update_config_tlv(wmi_unified_t wmi_handle,
			  struct nan_datapath_update_config *req)
{
	uint16_t len;
	wmi_buf_t buf;
	QDF_STATUS status;
	wmi_ndp_set_latency_tput_fixed_param *cmd;
	uint32_t vdev_id = 0;

	vdev_id = wlan_vdev_get_id(req->vdev);
	wmi_debug("vdev_id: %d, ndp_instance_id: %d, latency_ms: %d, tput_mbps: %d",
		  vdev_id,
		  req->ndp_instance_id,
		  req->latency_ms,
		  req->tput_mbps);

	/* allocated memory for fixed params as well as variable size data  */
	len = sizeof(*cmd);

	buf = wmi_buf_alloc(wmi_handle, len);
	if (!buf)
		return QDF_STATUS_E_NOMEM;

	cmd = (wmi_ndp_set_latency_tput_fixed_param *)wmi_buf_data(buf);
	WMITLV_SET_HDR(&cmd->tlv_header,
		       WMITLV_TAG_STRUC_wmi_ndp_set_latency_tput_fixed_param,
		       WMITLV_GET_STRUCT_TLVLEN(
			       wmi_ndp_set_latency_tput_fixed_param));
	cmd->vdev_id = vdev_id;
	cmd->ndp_instance_id = req->ndp_instance_id;
	cmd->latency_ms = req->latency_ms;
	cmd->tput_mbps = req->tput_mbps;

	wmi_debug("latency_ms: %d, tput_mbps: %d",
		  cmd->latency_ms, cmd->tput_mbps);

	wmi_mtrace(WMI_NDP_SET_LATENCY_TPUT_CMDID, cmd->vdev_id, 0);
	status = wmi_unified_cmd_send(wmi_handle, buf, len,
				      WMI_NDP_SET_LATENCY_TPUT_CMDID);
	if (QDF_IS_STATUS_ERROR(status)) {
		wmi_err("WMI_NDP_SET_LATENCY_TPUT_CMDID failed, ret: %d",
			status);
		wmi_buf_free(buf);
	}
	return status;
}

#if defined(WLAN_FEATURE_NAN) && defined(FEATURE_WLAN_SUPPORT_NAN_OFFLOAD_MODE)
/**
 * wmi_nan_add_func_calculate_len() - Calculate total buffer length needed
 * @params: Internal NAN function parameters
 *
 * Return: Total length in bytes, or 0 if any field exceeds its maximum.
 */
static
uint32_t wmi_nan_add_func_calculate_len(struct nan_add_func_params *params)
{
	uint32_t len = sizeof(wmi_nan_disc_service_req_cmd_fixed_param);
	uint32_t tlv_len = 0;

	tlv_len += WMI_TLV_HDR_SIZE + qdf_roundup(NDP_SERVICE_ID_LEN,
						  sizeof(uint32_t));
	tlv_len += WMI_TLV_HDR_SIZE + qdf_roundup(params->serv_spec_info_len,
						  sizeof(uint32_t));
	tlv_len += WMI_TLV_HDR_SIZE + qdf_roundup(params->srf_bf_len,
						  sizeof(uint32_t));
	tlv_len += WMI_TLV_HDR_SIZE +
		   (params->srf_num_macs * sizeof(wmi_mac_addr));
	tlv_len += WMI_TLV_HDR_SIZE + qdf_roundup(params->rx_filters_len,
						  sizeof(uint32_t));
	tlv_len += WMI_TLV_HDR_SIZE + qdf_roundup(params->tx_filters_len,
						  sizeof(uint32_t));
	tlv_len += WMI_TLV_HDR_SIZE +
		   (params->num_cipher_suites * sizeof(uint32_t));
	tlv_len += WMI_TLV_HDR_SIZE +
			(params->nd_pmk_set ?
			 qdf_roundup(NDP_PMK_LEN, sizeof(uint32_t)) : 0);
	tlv_len += WMI_TLV_HDR_SIZE +
		   qdf_roundup(params->extra_nan_attrs_len, sizeof(uint32_t));

	return len + tlv_len;
}

/**
 * wmi_nan_add_func_populate_tlv() - Populate TLVs for NAN add function command
 * @params: Internal NAN function parameters
 * @cmd: WMI command fixed param structure (start of allocated WMI buffer)
 * @buf_ptr: Same pointer as cmd, cast to uint8_t for byte arithmetic
 *
 * Return: QDF_STATUS_SUCCESS, or QDF_STATUS_E_INVAL for an unrecognised type.
 */
static QDF_STATUS
wmi_nan_add_func_populate_tlv(struct nan_add_func_params *params,
			      wmi_nan_disc_service_req_cmd_fixed_param *cmd,
			      uint8_t *buf_ptr)
{
	uint32_t aligned_len;

	if (!buf_ptr) {
		wmi_err("Invalid buf_ptr");
		return QDF_STATUS_E_INVAL;
	}

	qdf_mem_zero(cmd, sizeof(*cmd));
	WMITLV_SET_HDR(&cmd->tlv_header,
		       WMITLV_TAG_STRUC_wmi_nan_disc_service_req_cmd_fixed_param,
		       WMITLV_GET_STRUCT_TLVLEN(
				wmi_nan_disc_service_req_cmd_fixed_param));

	cmd->vdev_id = params->vdev_id;
	WMI_NAN_DISC_COOKIE_SET(cmd->cookie_low32, cmd->cookie_high32,
				params->cookie);
	cmd->ttl = params->ttl;

	switch (params->type) {
	case NAN_FUNC_TYPE_PUBLISH:
		cmd->service_req_type = WMI_NAN_DISC_SERVICE_REQ_PUBLISH;
		switch (params->publish_type) {
		case NAN_FUNC_PUBLISH_TYPE_UNSOLICITED:
			cmd->publish_type = WMI_NAN_DISC_PUBLISH_UNSOLICITED;
			break;
		case NAN_FUNC_PUBLISH_TYPE_SOLICITED:
			cmd->publish_type = WMI_NAN_DISC_PUBLISH_SOLICITED;
			break;
		case NAN_FUNC_PUBLISH_TYPE_UNSOLICITED_SOLICITED:
			cmd->publish_type =
				WMI_NAN_DISC_PUBLISH_UNSOLICITED_SOLICITED;
			break;
		default:
			cmd->publish_type = 0;
			break;
		}
		break;
	case NAN_FUNC_TYPE_SUBSCRIBE:
		cmd->service_req_type = WMI_NAN_DISC_SERVICE_REQ_SUBSCRIBE;
		cmd->publish_type = 0;
		break;
	case NAN_FUNC_TYPE_FOLLOW_UP:
		cmd->service_req_type = WMI_NAN_DISC_SERVICE_REQ_FOLLOW_UP;
		cmd->followup_instance_id = params->followup_id;
		cmd->followup_requestor_id = params->followup_reqid;
		WMI_CHAR_ARRAY_TO_MAC_ADDR(params->followup_dest.bytes,
					   &cmd->followup_dest);
		cmd->publish_type = 0;
		break;
	default:
		wmi_err("Invalid NAN function type: %d", params->type);
		return QDF_STATUS_E_INVAL;
	}

	cmd->sdea_ctrl = params->sdea_ctrl;
	cmd->pairing_bootstrap_methods = params->pairing_bootstrap_methods;

	cmd->service_req_flags = 0;
	WMI_NAN_DISC_SERVICE_REQ_FLAGS_SET_CLOSE_RANGE
			(cmd->service_req_flags, params->close_range ? 1 : 0);
	if (params->type == NAN_FUNC_TYPE_PUBLISH)
		WMI_NAN_DISC_SERVICE_REQ_FLAGS_SET_PUBLISH_BCAST(
				cmd->service_req_flags,
				params->publish_bcast ? 1 : 0);
	if (params->type == NAN_FUNC_TYPE_SUBSCRIBE)
		WMI_NAN_DISC_SERVICE_REQ_FLAGS_SET_SUBSCRIBE_ACTIVE(
				cmd->service_req_flags,
				params->subscribe_active ? 1 : 0);
	WMI_NAN_DISC_SERVICE_REQ_FLAGS_SET_GTK_REQUIRED
			(cmd->service_req_flags, params->gtk_required ? 1 : 0);
	WMI_NAN_DISC_SERVICE_REQ_FLAGS_SET_PAIRING_SETUP
			(cmd->service_req_flags, params->pairing_setup ? 1 : 0);
	WMI_NAN_DISC_SERVICE_REQ_FLAGS_SET_PAIRING_CACHE
			(cmd->service_req_flags, params->pairing_cache ? 1 : 0);
	WMI_NAN_DISC_SERVICE_REQ_FLAGS_SET_PAIRING_VERIFY
					(cmd->service_req_flags,
					 params->pairing_verify ? 1 : 0);
	WMI_NAN_DISC_SERVICE_REQ_FLAGS_SET_ND_PMK_VALID
			(cmd->service_req_flags, params->nd_pmk_set ? 1 : 0);

	cmd->srf_include = params->srf_include;
	cmd->srf_bf_idx = params->srf_bf_idx;

	cmd->serv_spec_info_len = params->serv_spec_info_len;
	cmd->srf_bf_len = params->srf_bf_len;
	cmd->rx_match_filter_len = params->rx_filters_len;
	cmd->tx_match_filter_len = params->tx_filters_len;
	cmd->extra_nan_attrs_len = params->extra_nan_attrs_len;

	buf_ptr += sizeof(wmi_nan_disc_service_req_cmd_fixed_param);

	WMITLV_SET_HDR(buf_ptr, WMITLV_TAG_ARRAY_BYTE,
		       qdf_roundup(NDP_SERVICE_ID_LEN, sizeof(uint32_t)));
	buf_ptr += WMI_TLV_HDR_SIZE;
	qdf_mem_copy(buf_ptr, params->service_id, NDP_SERVICE_ID_LEN);
	buf_ptr += qdf_roundup(NDP_SERVICE_ID_LEN, sizeof(uint32_t));

	aligned_len = qdf_roundup(params->serv_spec_info_len, sizeof(uint32_t));
	WMITLV_SET_HDR(buf_ptr, WMITLV_TAG_ARRAY_BYTE, aligned_len);
	buf_ptr += WMI_TLV_HDR_SIZE;
	if (params->serv_spec_info_len > 0)
		qdf_mem_copy(buf_ptr, params->serv_spec_info,
			     params->serv_spec_info_len);
	buf_ptr += aligned_len;

	aligned_len = qdf_roundup(params->srf_bf_len, sizeof(uint32_t));
	WMITLV_SET_HDR(buf_ptr, WMITLV_TAG_ARRAY_BYTE, aligned_len);
	buf_ptr += WMI_TLV_HDR_SIZE;
	if (params->srf_bf_len > 0)
		qdf_mem_copy(buf_ptr, params->srf_bf, params->srf_bf_len);
	buf_ptr += aligned_len;

	WMITLV_SET_HDR(buf_ptr, WMITLV_TAG_ARRAY_FIXED_STRUC,
		       params->srf_num_macs * sizeof(wmi_mac_addr));
	buf_ptr += WMI_TLV_HDR_SIZE;
	if (params->srf_num_macs > 0) {
		wmi_mac_addr *wmi_macs = (wmi_mac_addr *)buf_ptr;
		uint32_t i;

		for (i = 0; i < params->srf_num_macs; i++)
			WMI_CHAR_ARRAY_TO_MAC_ADDR(
				params->srf_macs + i * ETH_ALEN, &wmi_macs[i]);
	}
	buf_ptr += params->srf_num_macs * sizeof(wmi_mac_addr);

	aligned_len = qdf_roundup(params->rx_filters_len, sizeof(uint32_t));
	WMITLV_SET_HDR(buf_ptr, WMITLV_TAG_ARRAY_BYTE, aligned_len);
	buf_ptr += WMI_TLV_HDR_SIZE;
	if (params->rx_filters_len > 0)
		qdf_mem_copy(buf_ptr, params->rx_filters,
			     params->rx_filters_len);
	buf_ptr += aligned_len;

	aligned_len = qdf_roundup(params->tx_filters_len, sizeof(uint32_t));
	WMITLV_SET_HDR(buf_ptr, WMITLV_TAG_ARRAY_BYTE, aligned_len);
	buf_ptr += WMI_TLV_HDR_SIZE;
	if (params->tx_filters_len > 0)
		qdf_mem_copy(buf_ptr, params->tx_filters,
			     params->tx_filters_len);
	buf_ptr += aligned_len;

	WMITLV_SET_HDR(buf_ptr, WMITLV_TAG_ARRAY_UINT32,
		       params->num_cipher_suites * sizeof(uint32_t));
	buf_ptr += WMI_TLV_HDR_SIZE;
	if (params->num_cipher_suites > 0)
		qdf_mem_copy(buf_ptr, params->cipher_suites,
			     params->num_cipher_suites * sizeof(uint32_t));
	buf_ptr += params->num_cipher_suites * sizeof(uint32_t);

	aligned_len = params->nd_pmk_set ?
		      qdf_roundup(NDP_PMK_LEN, sizeof(uint32_t)) : 0;
	WMITLV_SET_HDR(buf_ptr, WMITLV_TAG_ARRAY_BYTE, aligned_len);
	buf_ptr += WMI_TLV_HDR_SIZE;
	if (params->nd_pmk_set)
		qdf_mem_copy(buf_ptr, params->nd_pmk, NDP_PMK_LEN);
	buf_ptr += aligned_len;

	aligned_len = qdf_roundup(params->extra_nan_attrs_len,
				  sizeof(uint32_t));
	WMITLV_SET_HDR(buf_ptr, WMITLV_TAG_ARRAY_BYTE, aligned_len);
	buf_ptr += WMI_TLV_HDR_SIZE;
	if (params->extra_nan_attrs_len > 0)
		qdf_mem_copy(buf_ptr, params->extra_nan_attrs,
			     params->extra_nan_attrs_len);
	buf_ptr += aligned_len;

	return QDF_STATUS_SUCCESS;
}

/**
 * send_add_nan_func_cmd_tlv() - Send NAN add function command (TLV format)
 * @wmi_handle: WMI handle
 * @params: Internal NAN function parameters
 *
 * Return: QDF_STATUS
 */
static QDF_STATUS
send_add_nan_func_cmd_tlv(wmi_unified_t wmi_handle,
			  struct nan_add_func_params *params)
{
	wmi_nan_disc_service_req_cmd_fixed_param *cmd;
	wmi_buf_t buf;
	uint8_t *buf_ptr;
	uint32_t len;
	QDF_STATUS status;

	if (!wmi_handle || !params) {
		wmi_err("Invalid parameters");
		return QDF_STATUS_E_INVAL;
	}

	len = wmi_nan_add_func_calculate_len(params);

	buf = wmi_buf_alloc(wmi_handle, len);
	if (!buf) {
		wmi_err("Failed to allocate WMI buffer");
		return QDF_STATUS_E_NOMEM;
	}

	buf_ptr = (uint8_t *)wmi_buf_data(buf);
	cmd = (wmi_nan_disc_service_req_cmd_fixed_param *)buf_ptr;

	status = wmi_nan_add_func_populate_tlv(params, cmd, buf_ptr);
	if (QDF_IS_STATUS_ERROR(status)) {
		wmi_buf_free(buf);
		return status;
	}

	wmi_mtrace(WMI_NAN_DISC_SERVICE_REQ_CMDID, cmd->vdev_id, 0);
	status = wmi_unified_cmd_send(wmi_handle, buf, len,
				      WMI_NAN_DISC_SERVICE_REQ_CMDID);
	if (QDF_IS_STATUS_ERROR(status)) {
		wmi_err("Failed to send NAN disc service request: %d", status);
		wmi_buf_free(buf);
		return status;
	}

	wmi_debug("NAN disc request sent: vdev_id=%d, type=%d",
		  cmd->vdev_id, cmd->service_req_type);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS
send_nan_del_func_cmd_tlv(wmi_unified_t wmi_handle,
			  struct nan_del_func_params *params)
{
	wmi_nan_disc_cancel_service_req_cmd_fixed_param *cmd;
	wmi_buf_t buf;
	QDF_STATUS status;
	uint32_t len;

	if (!wmi_handle || !params) {
		wmi_err("Invalid parameters");
		return QDF_STATUS_E_INVAL;
	}

	len = sizeof(*cmd);
	buf = wmi_buf_alloc(wmi_handle, len);
	if (!buf) {
		wmi_err("Failed to allocate NAN del func buf");
		return QDF_STATUS_E_NOMEM;
	}

	cmd = (wmi_nan_disc_cancel_service_req_cmd_fixed_param *)
		wmi_buf_data(buf);
	qdf_mem_zero(cmd, sizeof(*cmd));
	WMITLV_SET_HDR(&cmd->tlv_header,
		       WMITLV_TAG_STRUC_wmi_nan_disc_cancel_service_req_cmd_fixed_param,
		       WMITLV_GET_STRUCT_TLVLEN
		       (wmi_nan_disc_cancel_service_req_cmd_fixed_param));

	cmd->vdev_id = params->vdev_id;
	cmd->instance_id = params->instance_id;
	WMI_NAN_DISC_COOKIE_SET(cmd->cookie_low32, cmd->cookie_high32,
				params->cookie);

	wmi_mtrace(WMI_NAN_DISC_CANCEL_SERVICE_REQ_CMDID, cmd->vdev_id, 0);
	status = wmi_unified_cmd_send(wmi_handle, buf, len,
				      WMI_NAN_DISC_CANCEL_SERVICE_REQ_CMDID);
	if (QDF_IS_STATUS_ERROR(status)) {
		wmi_err("Failed to send NAN del func cmd: %d", status);
		wmi_buf_free(buf);
		return status;
	}

	wmi_debug("NAN disc cancel sent: vdev_id=%d, instance_id=%d",
		  cmd->vdev_id, cmd->instance_id);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS
extract_nan_disc_service_rsp_event_tlv(wmi_unified_t wmi_handle, void *evt_buf,
				       struct nan_disc_service_rsp_event *event)
{
	WMI_NAN_DISC_SERVICE_RSP_EVENTID_param_tlvs *param_buf;
	wmi_nan_disc_service_rsp_event_fixed_param *wmi_event;

	if (!evt_buf) {
		wmi_err("Invalid event buffer");
		return QDF_STATUS_E_INVAL;
	}

	if (!event) {
		wmi_err("Invalid output event buffer");
		return QDF_STATUS_E_INVAL;
	}

	param_buf = (WMI_NAN_DISC_SERVICE_RSP_EVENTID_param_tlvs *)evt_buf;

	wmi_event = param_buf->fixed_param;
	if (!wmi_event) {
		wmi_err("Invalid fixed param");
		return QDF_STATUS_E_INVAL;
	}

	event->vdev_id = wmi_event->vdev_id;
	event->instance_id = wmi_event->instance_id;
	event->status = wmi_event->status;
	event->cookie = WMI_NAN_DISC_COOKIE_GET(wmi_event->cookie_low32,
						wmi_event->cookie_high32);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS
extract_nan_disc_match_event_tlv(wmi_unified_t wmi_handle, void *evt_buf,
				 struct nan_disc_match_event *event)
{
	WMI_NAN_DISC_MATCH_EVENTID_param_tlvs *param_buf;
	wmi_nan_disc_match_event_fixed_param *wmi_event;

	if (!evt_buf) {
		wmi_err("Invalid event buffer");
		return QDF_STATUS_E_INVAL;
	}

	if (!event) {
		wmi_err("Invalid output event buffer");
		return QDF_STATUS_E_INVAL;
	}

	param_buf = (WMI_NAN_DISC_MATCH_EVENTID_param_tlvs *)evt_buf;

	wmi_event = param_buf->fixed_param;
	if (!wmi_event) {
		wmi_err("Invalid fixed param");
		return QDF_STATUS_E_INVAL;
	}

	switch (wmi_event->type) {
	case WMI_NAN_DISC_SERVICE_REQ_PUBLISH:
		event->type = NAN_FUNC_TYPE_PUBLISH;
		break;
	case WMI_NAN_DISC_SERVICE_REQ_SUBSCRIBE:
		event->type = NAN_FUNC_TYPE_SUBSCRIBE;
		break;
	case WMI_NAN_DISC_SERVICE_REQ_FOLLOW_UP:
		event->type = NAN_FUNC_TYPE_FOLLOW_UP;
		break;
	default:
		wmi_err("Invalid event type : %d", wmi_event->type);
		return QDF_STATUS_E_INVAL;
	}

	event->vdev_id = wmi_event->vdev_id;
	event->cookie = WMI_NAN_DISC_COOKIE_GET(wmi_event->cookie_low32,
						wmi_event->cookie_high32);
	event->inst_id = wmi_event->inst_id;
	event->peer_inst_id = wmi_event->peer_inst_id;
	event->sdea_ctrl = wmi_event->sdea_ctrl;
	event->bootstrap_methods = wmi_event->bootstrap_methods;
	event->num_cipher_suites = param_buf->num_cipher_suites;
	event->serv_spec_info = NULL;
	event->cipher_suites = NULL;
	event->scid = NULL;
	event->extra_info = NULL;
	event->ies = NULL;
	event->data_path = wmi_event->data_path;
	event->gtk_required = wmi_event->gtk_required;
	event->pairing_setup = wmi_event->pairing_setup;
	event->pairing_cache = wmi_event->pairing_cache;
	event->pairing_verify = wmi_event->pairing_verify;
	event->rssi = wmi_event->rssi;
	event->rssi_valid = wmi_event->rssi_valid;
	event->serv_spec_info_len = wmi_event->serv_spec_info_len;
	event->scid_len = wmi_event->scid_len;
	event->extra_info_len = wmi_event->extra_info_len;
	event->ies_len = wmi_event->ies_len;

	if (param_buf->peer_addr)
		WLAN_ADDR_COPY(event->peer_addr.bytes, param_buf->peer_addr);


	if (param_buf->service_id &&
	    wmi_event->service_id_len == NDP_SERVICE_ID_LEN) {
		qdf_mem_copy(event->service_id, param_buf->service_id,
			     wmi_event->service_id_len);
	}

	if (param_buf->serv_spec_info && wmi_event->serv_spec_info_len > 0) {
		if (wmi_event->serv_spec_info_len >
		    param_buf->num_serv_spec_info) {
			wmi_err("Invalid serv_spec_info_len %u > num %u",
				wmi_event->serv_spec_info_len,
				param_buf->num_serv_spec_info);
			goto free_nan_match_params;
		}
		event->serv_spec_info =
			qdf_mem_malloc(wmi_event->serv_spec_info_len);
		if (!event->serv_spec_info) {
			wmi_err("Failed to allocate service spec info");
			goto free_nan_match_params;
		}
		qdf_mem_copy(event->serv_spec_info, param_buf->serv_spec_info,
			     wmi_event->serv_spec_info_len);
	}

	if (param_buf->cipher_suites && event->num_cipher_suites > 0) {
		event->cipher_suites = qdf_mem_malloc(
			event->num_cipher_suites * sizeof(uint32_t));
		if (!event->cipher_suites) {
			wmi_err("Failed to allocate cipher suites");
			goto free_nan_match_params;
		}
		qdf_mem_copy(event->cipher_suites, param_buf->cipher_suites,
			     event->num_cipher_suites * sizeof(uint32_t));
	}

	if (param_buf->scid && wmi_event->scid_len > 0) {
		if (wmi_event->scid_len > param_buf->num_scid) {
			wmi_err("Invalid scid_len %u > num %u",
				wmi_event->scid_len, param_buf->num_scid);
			goto free_nan_match_params;
		}
		event->scid = qdf_mem_malloc(wmi_event->scid_len);
		if (!event->scid) {
			wmi_err("Failed to allocate scid");
			goto free_nan_match_params;
		}
		qdf_mem_copy(event->scid, param_buf->scid,
			     wmi_event->scid_len);
	}

	if (param_buf->extra_info && wmi_event->extra_info_len > 0) {
		if (wmi_event->extra_info_len > param_buf->num_extra_info) {
			wmi_err("Invalid extra_info_len %u > num %u",
				wmi_event->extra_info_len,
				param_buf->num_extra_info);
			goto free_nan_match_params;
		}
		event->extra_info = qdf_mem_malloc(wmi_event->extra_info_len);
		if (!event->extra_info) {
			wmi_err("Failed to allocate extra_info");
			goto free_nan_match_params;
		}
		qdf_mem_copy(event->extra_info, param_buf->extra_info,
			     wmi_event->extra_info_len);
	}

	if (param_buf->ies && wmi_event->ies_len > 0) {
		if (wmi_event->ies_len > param_buf->num_ies) {
			wmi_err("Invalid ies_len %u > num %u",
				wmi_event->ies_len, param_buf->num_ies);
			goto free_nan_match_params;
		}
		event->ies = qdf_mem_malloc(wmi_event->ies_len);
		if (!event->ies) {
			wmi_err("Failed to allocate ies");
			goto free_nan_match_params;
		}
		qdf_mem_copy(event->ies, param_buf->ies, wmi_event->ies_len);
	}

	return QDF_STATUS_SUCCESS;

free_nan_match_params:
	qdf_mem_free(event->serv_spec_info);
	event->serv_spec_info = NULL;
	qdf_mem_free(event->cipher_suites);
	event->cipher_suites = NULL;
	qdf_mem_free(event->scid);
	event->scid = NULL;
	qdf_mem_free(event->extra_info);
	event->extra_info = NULL;
	qdf_mem_free(event->ies);
	event->ies = NULL;

	return QDF_STATUS_E_INVAL;
}

static void wmi_nan_attach_add_func_tlv(wmi_unified_t wmi_handle)
{
	wmi_handle->ops->send_add_nan_func_cmd = send_add_nan_func_cmd_tlv;
	wmi_handle->ops->send_nan_del_func_cmd = send_nan_del_func_cmd_tlv;
	wmi_handle->ops->extract_nan_disc_service_rsp_event =
					extract_nan_disc_service_rsp_event_tlv;
	wmi_handle->ops->extract_nan_disc_match_event =
					extract_nan_disc_match_event_tlv;
}
#else
static inline void wmi_nan_attach_add_func_tlv(wmi_unified_t wmi_handle)
{}
#endif

static QDF_STATUS
extract_ndp_host_event_tlv(wmi_unified_t wmi_handle, uint8_t *data,
			   struct nan_datapath_host_event *evt)
{
	WMI_NDP_EVENTID_param_tlvs *event;
	wmi_ndp_event_param *fixed_params;

	event = (WMI_NDP_EVENTID_param_tlvs *)data;
	fixed_params = event->fixed_param;

	evt->vdev =
		wlan_objmgr_get_vdev_by_id_from_psoc(wmi_handle->soc->wmi_psoc,
						     fixed_params->vdev_id,
						     WLAN_NAN_ID);
	if (!evt->vdev) {
		wmi_err("vdev is null");
		return QDF_STATUS_E_INVAL;
	}

	evt->ndp_termination_in_progress =
		       fixed_params->ndp_termination_in_progress ? true : false;

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS extract_ndp_initiator_rsp_tlv(wmi_unified_t wmi_handle,
			uint8_t *data, struct nan_datapath_initiator_rsp *rsp)
{
	WMI_NDP_INITIATOR_RSP_EVENTID_param_tlvs *event;
	wmi_ndp_initiator_rsp_event_fixed_param  *fixed_params;

	event = (WMI_NDP_INITIATOR_RSP_EVENTID_param_tlvs *)data;
	fixed_params = event->fixed_param;

	rsp->vdev =
		wlan_objmgr_get_vdev_by_id_from_psoc(wmi_handle->soc->wmi_psoc,
						     fixed_params->vdev_id,
						     WLAN_NAN_ID);
	if (!rsp->vdev) {
		wmi_err("vdev is null");
		return QDF_STATUS_E_INVAL;
	}

	rsp->transaction_id = fixed_params->transaction_id;
	rsp->ndp_instance_id = fixed_params->ndp_instance_id;
	rsp->status = fixed_params->rsp_status;
	rsp->reason = fixed_params->reason_code;

	return QDF_STATUS_SUCCESS;
}

#define MAX_NAN_MSG_LEN                 400

static QDF_STATUS extract_nan_msg_tlv(uint8_t *data,
				      struct nan_dump_msg *msg)
{
	WMI_NAN_DMESG_EVENTID_param_tlvs *event;
	wmi_nan_dmesg_event_fixed_param *fixed_params;

	event = (WMI_NAN_DMESG_EVENTID_param_tlvs *)data;
	fixed_params = (wmi_nan_dmesg_event_fixed_param *)event->fixed_param;
	if (!fixed_params->msg_len ||
	    fixed_params->msg_len > MAX_NAN_MSG_LEN ||
	    fixed_params->msg_len > event->num_msg)
		return QDF_STATUS_E_FAILURE;

	msg->data_len = fixed_params->msg_len;
	msg->msg = event->msg;

	msg->msg[fixed_params->msg_len - 1] = (uint8_t)'\0';

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS extract_ndp_ind_tlv(wmi_unified_t wmi_handle,
		uint8_t *data, struct nan_datapath_indication_event *rsp)
{
	WMI_NDP_INDICATION_EVENTID_param_tlvs *event;
	wmi_ndp_indication_event_fixed_param *fixed_params;
	size_t total_array_len;

	event = (WMI_NDP_INDICATION_EVENTID_param_tlvs *)data;
	fixed_params =
		(wmi_ndp_indication_event_fixed_param *)event->fixed_param;

	if (fixed_params->ndp_cfg_len > event->num_ndp_cfg) {
		wmi_err("FW message ndp cfg length %d larger than TLV hdr %d",
			 fixed_params->ndp_cfg_len, event->num_ndp_cfg);
		return QDF_STATUS_E_INVAL;
	}

	if (fixed_params->ndp_app_info_len > event->num_ndp_app_info) {
		wmi_err("FW message ndp app info length %d more than TLV hdr %d",
			 fixed_params->ndp_app_info_len,
			 event->num_ndp_app_info);
		return QDF_STATUS_E_INVAL;
	}

	if (fixed_params->nan_scid_len > event->num_ndp_scid) {
		wmi_err("FW msg ndp scid info len %d more than TLV hdr %d",
			 fixed_params->nan_scid_len,
			 event->num_ndp_scid);
		return QDF_STATUS_E_INVAL;
	}

	if (fixed_params->service_id_len > event->num_service_id) {
		wmi_err("FW msg service id len %d more than TLV hdr %d",
			fixed_params->service_id_len,
			event->num_service_id);
		return QDF_STATUS_E_INVAL;
	}

	if (fixed_params->ndp_cfg_len >
		(WMI_SVC_MSG_MAX_SIZE - sizeof(*fixed_params))) {
		wmi_err("excess wmi buffer: ndp_cfg_len %d",
			fixed_params->ndp_cfg_len);
		return QDF_STATUS_E_INVAL;
	}

	total_array_len = fixed_params->ndp_cfg_len +
					sizeof(*fixed_params);

	if (fixed_params->ndp_app_info_len >
		(WMI_SVC_MSG_MAX_SIZE - total_array_len)) {
		wmi_err("excess wmi buffer: ndp_cfg_len %d",
			fixed_params->ndp_app_info_len);
		return QDF_STATUS_E_INVAL;
	}
	total_array_len += fixed_params->ndp_app_info_len;

	if (fixed_params->nan_scid_len >
		(WMI_SVC_MSG_MAX_SIZE - total_array_len)) {
		wmi_err("excess wmi buffer: ndp_cfg_len %d",
			fixed_params->nan_scid_len);
		return QDF_STATUS_E_INVAL;
	}

	total_array_len += fixed_params->nan_scid_len;

	if (fixed_params->service_id_len >
	    (WMI_SVC_MSG_MAX_SIZE - total_array_len)) {
		wmi_err("excess wmi buffer: service_cfg_len %d",
			fixed_params->service_id_len);
		return QDF_STATUS_E_INVAL;
	}

	rsp->vdev =
		wlan_objmgr_get_vdev_by_id_from_psoc(wmi_handle->soc->wmi_psoc,
						     fixed_params->vdev_id,
						     WLAN_NAN_ID);
	if (!rsp->vdev) {
		wmi_err("vdev is null");
		return QDF_STATUS_E_INVAL;
	}
	rsp->service_instance_id = fixed_params->service_instance_id;
	rsp->ndp_instance_id = fixed_params->ndp_instance_id;
	rsp->role = fixed_params->self_ndp_role;
	rsp->policy = fixed_params->accept_policy;
	rsp->ndp_add_params.csid_cap = fixed_params->nan_csid_cap;
	rsp->ndp_add_params.gtk = fixed_params->nan_gtk_required;

	WMI_MAC_ADDR_TO_CHAR_ARRAY(&fixed_params->peer_ndi_mac_addr,
				rsp->peer_mac_addr.bytes);
	WMI_MAC_ADDR_TO_CHAR_ARRAY(&fixed_params->peer_discovery_mac_addr,
				rsp->peer_discovery_mac_addr.bytes);

	wmi_debug("WMI_NDP_INDICATION_EVENTID(0x%X) received. vdev %d service_instance %d, ndp_instance %d, role %d, policy %d csid: %d, scid_len: %d, peer_addr: "QDF_MAC_ADDR_FMT", peer_disc_addr: "QDF_MAC_ADDR_FMT" ndp_cfg - %d bytes ndp_app_info - %d bytes ndp_csid_caps %d, ndp_gtk_required %d",
		 WMI_NDP_INDICATION_EVENTID, fixed_params->vdev_id,
		 fixed_params->service_instance_id,
		 fixed_params->ndp_instance_id, fixed_params->self_ndp_role,
		 fixed_params->accept_policy, fixed_params->nan_csid,
		 fixed_params->nan_scid_len,
		 QDF_MAC_ADDR_REF(rsp->peer_mac_addr.bytes),
		 QDF_MAC_ADDR_REF(rsp->peer_discovery_mac_addr.bytes),
		 fixed_params->ndp_cfg_len,
		 fixed_params->ndp_app_info_len, rsp->ndp_add_params.csid_cap,
		 rsp->ndp_add_params.gtk);

	rsp->ncs_sk_type = fixed_params->nan_csid;
	if (event->ndp_cfg) {
		rsp->ndp_config.ndp_cfg_len = fixed_params->ndp_cfg_len;
		if (rsp->ndp_config.ndp_cfg_len > NDP_QOS_INFO_LEN)
			rsp->ndp_config.ndp_cfg_len = NDP_QOS_INFO_LEN;
		qdf_mem_copy(rsp->ndp_config.ndp_cfg, event->ndp_cfg,
			     rsp->ndp_config.ndp_cfg_len);
	}

	if (event->ndp_app_info) {
		rsp->ndp_info.ndp_app_info_len = fixed_params->ndp_app_info_len;
		if (rsp->ndp_info.ndp_app_info_len > NDP_APP_INFO_LEN)
			rsp->ndp_info.ndp_app_info_len = NDP_APP_INFO_LEN;
		qdf_mem_copy(rsp->ndp_info.ndp_app_info, event->ndp_app_info,
			     rsp->ndp_info.ndp_app_info_len);
	}

	if (event->ndp_scid) {
		rsp->scid.scid_len = fixed_params->nan_scid_len;
		if (rsp->scid.scid_len > NDP_SCID_BUF_LEN)
			rsp->scid.scid_len = NDP_SCID_BUF_LEN;
		qdf_mem_copy(rsp->scid.scid, event->ndp_scid,
			     rsp->scid.scid_len);
	}

	if (event->ndp_transport_ip_param &&
	    event->num_ndp_transport_ip_param) {
		if (event->ndp_transport_ip_param->ipv6_addr_present) {
			rsp->is_ipv6_addr_present = true;
			qdf_mem_copy(rsp->ipv6_addr,
				event->ndp_transport_ip_param->ipv6_intf_addr,
				WMI_NDP_IPV6_INTF_ADDR_LEN);
		}
	}
	wmi_debug("IPv6 addr present: %d, addr: %pI6",
		 rsp->is_ipv6_addr_present, rsp->ipv6_addr);

	rsp->is_service_id_present = false;
	if (fixed_params->service_id_len && event->service_id) {
		if (fixed_params->service_id_len < NDP_SERVICE_ID_LEN) {
			wmi_err("Invalid service id length %d",
				event->num_service_id);
			return QDF_STATUS_E_INVAL;
		}
		rsp->is_service_id_present = true;
		qdf_mem_copy(rsp->service_id, event->service_id,
			     NDP_SERVICE_ID_LEN);
	}

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS extract_ndp_confirm_tlv(wmi_unified_t wmi_handle,
			uint8_t *data, struct nan_datapath_confirm_event *rsp)
{
	uint8_t i;
	WMI_HOST_WLAN_PHY_MODE ch_mode;
	WMI_NDP_CONFIRM_EVENTID_param_tlvs *event;
	wmi_ndp_confirm_event_fixed_param *fixed_params;
	size_t total_array_len;
	bool ndi_dbs = wmi_service_enabled(wmi_handle,
					   wmi_service_ndi_dbs_support);

	event = (WMI_NDP_CONFIRM_EVENTID_param_tlvs *) data;
	fixed_params = (wmi_ndp_confirm_event_fixed_param *)event->fixed_param;
	wmi_debug("WMI_NDP_CONFIRM_EVENTID(0x%X) received. vdev %d, ndp_instance %d, rsp_code %d, reason_code: %d, num_active_ndps_on_peer: %d num_ch: %d",
		 WMI_NDP_CONFIRM_EVENTID, fixed_params->vdev_id,
		 fixed_params->ndp_instance_id, fixed_params->rsp_code,
		 fixed_params->reason_code,
		 fixed_params->num_active_ndps_on_peer,
		 fixed_params->num_ndp_channels);

	if (fixed_params->ndp_cfg_len > event->num_ndp_cfg) {
		wmi_err("FW message ndp cfg length %d larger than TLV hdr %d",
			 fixed_params->ndp_cfg_len, event->num_ndp_cfg);
		return QDF_STATUS_E_INVAL;
	}

	if (fixed_params->ndp_app_info_len > event->num_ndp_app_info) {
		wmi_err("FW message ndp app info length %d more than TLV hdr %d",
			 fixed_params->ndp_app_info_len,
			 event->num_ndp_app_info);
		return QDF_STATUS_E_INVAL;
	}

	wmi_debug("ndp_cfg - %d bytes, ndp_app_info - %d bytes",
		 fixed_params->ndp_cfg_len, fixed_params->ndp_app_info_len);

	if (fixed_params->ndp_cfg_len >
			(WMI_SVC_MSG_MAX_SIZE - sizeof(*fixed_params))) {
		wmi_err("excess wmi buffer: ndp_cfg_len %d",
			fixed_params->ndp_cfg_len);
		return QDF_STATUS_E_INVAL;
	}

	total_array_len = fixed_params->ndp_cfg_len +
				sizeof(*fixed_params);

	if (fixed_params->ndp_app_info_len >
		(WMI_SVC_MSG_MAX_SIZE - total_array_len)) {
		wmi_err("excess wmi buffer: ndp_cfg_len %d",
			fixed_params->ndp_app_info_len);
		return QDF_STATUS_E_INVAL;
	}

	if (fixed_params->num_ndp_channels > event->num_ndp_channel_list ||
	    fixed_params->num_ndp_channels > event->num_nss_list) {
		wmi_err("NDP Ch count %d greater than NDP Ch TLV len(%d) or NSS TLV len(%d)",
			 fixed_params->num_ndp_channels,
			 event->num_ndp_channel_list,
			 event->num_nss_list);
		return QDF_STATUS_E_INVAL;
	}

	if (ndi_dbs &&
	    fixed_params->num_ndp_channels > event->num_ndp_channel_info) {
		wmi_err("NDP Ch count %d greater than NDP Ch info(%d)",
			 fixed_params->num_ndp_channels,
			 event->num_ndp_channel_info);
		return QDF_STATUS_E_INVAL;
	}

	rsp->vdev =
		wlan_objmgr_get_vdev_by_id_from_psoc(wmi_handle->soc->wmi_psoc,
						     fixed_params->vdev_id,
						     WLAN_NAN_ID);
	if (!rsp->vdev) {
		wmi_err("vdev is null");
		return QDF_STATUS_E_INVAL;
	}
	rsp->ndp_instance_id = fixed_params->ndp_instance_id;
	rsp->rsp_code = fixed_params->rsp_code;
	rsp->reason_code = fixed_params->reason_code;
	rsp->num_active_ndps_on_peer = fixed_params->num_active_ndps_on_peer;
	rsp->num_channels = fixed_params->num_ndp_channels;
	WMI_MAC_ADDR_TO_CHAR_ARRAY(&fixed_params->peer_ndi_mac_addr,
				   rsp->peer_ndi_mac_addr.bytes);
	rsp->ndp_info.ndp_app_info_len = fixed_params->ndp_app_info_len;

	if (rsp->ndp_info.ndp_app_info_len > NDP_APP_INFO_LEN)
		rsp->ndp_info.ndp_app_info_len = NDP_APP_INFO_LEN;

	qdf_mem_copy(rsp->ndp_info.ndp_app_info, event->ndp_app_info,
		     rsp->ndp_info.ndp_app_info_len);

	if (rsp->num_channels > NAN_CH_INFO_MAX_CHANNELS) {
		wmi_err("too many channels");
		rsp->num_channels = NAN_CH_INFO_MAX_CHANNELS;
	}

	for (i = 0; i < rsp->num_channels; i++) {
		rsp->ch[i].freq = event->ndp_channel_list[i].mhz;
		rsp->ch[i].nss = event->nss_list[i];
		ch_mode = WMI_GET_CHANNEL_MODE(&event->ndp_channel_list[i]);
		rsp->ch[i].phymode = ch_mode;
		rsp->ch[i].ch_width = wmi_get_ch_width_from_phy_mode(wmi_handle,
								     ch_mode);
		if (ndi_dbs) {
			rsp->ch[i].mac_id = event->ndp_channel_info[i].mac_id;
			wmi_debug("Freq: %d, ch_mode: %d, nss: %d mac_id: %d",
				 rsp->ch[i].freq, rsp->ch[i].ch_width,
				 rsp->ch[i].nss, rsp->ch[i].mac_id);
		} else {
			wmi_debug("Freq: %d, ch_mode: %d, nss: %d",
				 rsp->ch[i].freq, rsp->ch[i].ch_width,
				 rsp->ch[i].nss);
		}
	}

	if (event->ndp_transport_ip_param &&
	    event->num_ndp_transport_ip_param) {
		if (event->ndp_transport_ip_param->ipv6_addr_present) {
			rsp->is_ipv6_addr_present = true;
			qdf_mem_copy(rsp->ipv6_addr,
				event->ndp_transport_ip_param->ipv6_intf_addr,
				WMI_NDP_IPV6_INTF_ADDR_LEN);
		}

		if (event->ndp_transport_ip_param->trans_port_present) {
			rsp->is_port_present = true;
			rsp->port =
			    event->ndp_transport_ip_param->transport_port;
		}

		if (event->ndp_transport_ip_param->trans_proto_present) {
			rsp->is_protocol_present = true;
			rsp->protocol =
			    event->ndp_transport_ip_param->transport_protocol;
		}
	}
	wmi_debug("IPv6 addr present: %d, addr: %pI6 port: %d present: %d protocol: %d present: %d",
		 rsp->is_ipv6_addr_present, rsp->ipv6_addr, rsp->port,
		 rsp->is_port_present, rsp->protocol, rsp->is_protocol_present);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS extract_ndp_responder_rsp_tlv(wmi_unified_t wmi_handle,
			uint8_t *data, struct nan_datapath_responder_rsp *rsp)
{
	WMI_NDP_RESPONDER_RSP_EVENTID_param_tlvs *event;
	wmi_ndp_responder_rsp_event_fixed_param  *fixed_params;

	event = (WMI_NDP_RESPONDER_RSP_EVENTID_param_tlvs *)data;
	fixed_params = event->fixed_param;

	rsp->vdev =
		wlan_objmgr_get_vdev_by_id_from_psoc(wmi_handle->soc->wmi_psoc,
						     fixed_params->vdev_id,
						     WLAN_NAN_ID);
	if (!rsp->vdev) {
		wmi_err("vdev is null");
		return QDF_STATUS_E_INVAL;
	}
	rsp->transaction_id = fixed_params->transaction_id;
	rsp->reason = fixed_params->reason_code;
	rsp->status = fixed_params->rsp_status;
	rsp->create_peer = fixed_params->create_peer;
	WMI_MAC_ADDR_TO_CHAR_ARRAY(&fixed_params->peer_ndi_mac_addr,
				   rsp->peer_mac_addr.bytes);
	wmi_debug("WMI_NDP_RESPONDER_RSP_EVENTID(0x%X) received. vdev_id: %d, peer_mac_addr: "QDF_MAC_ADDR_FMT",transaction_id: %d, status_code %d, reason_code: %d, create_peer: %d",
		 WMI_NDP_RESPONDER_RSP_EVENTID, fixed_params->vdev_id,
		 QDF_MAC_ADDR_REF(rsp->peer_mac_addr.bytes),
		 rsp->transaction_id,
		 rsp->status, rsp->reason, rsp->create_peer);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS extract_ndp_end_rsp_tlv(wmi_unified_t wmi_handle,
			uint8_t *data, struct nan_datapath_end_rsp_event *rsp)
{
	WMI_NDP_END_RSP_EVENTID_param_tlvs *event;
	wmi_ndp_end_rsp_event_fixed_param *fixed_params = NULL;

	event = (WMI_NDP_END_RSP_EVENTID_param_tlvs *) data;
	fixed_params = (wmi_ndp_end_rsp_event_fixed_param *)event->fixed_param;
	wmi_debug("WMI_NDP_END_RSP_EVENTID(0x%X) received. transaction_id: %d, rsp_status: %d, reason_code: %d",
		 WMI_NDP_END_RSP_EVENTID, fixed_params->transaction_id,
		 fixed_params->rsp_status, fixed_params->reason_code);

	rsp->vdev = wlan_objmgr_get_vdev_by_opmode_from_psoc(
			wmi_handle->soc->wmi_psoc, QDF_NDI_MODE, WLAN_NAN_ID);
	if (!rsp->vdev) {
		wmi_err("vdev is null");
		return QDF_STATUS_E_INVAL;
	}
	rsp->transaction_id = fixed_params->transaction_id;
	rsp->reason = fixed_params->reason_code;
	rsp->status = fixed_params->rsp_status;

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS extract_ndp_end_ind_tlv(wmi_unified_t wmi_handle,
		uint8_t *data, struct nan_datapath_end_indication_event **rsp)
{
	uint32_t i, buf_size;
	wmi_ndp_end_indication *ind;
	struct qdf_mac_addr peer_addr;
	WMI_NDP_END_INDICATION_EVENTID_param_tlvs *event;

	event = (WMI_NDP_END_INDICATION_EVENTID_param_tlvs *) data;
	ind = event->ndp_end_indication_list;

	if (event->num_ndp_end_indication_list == 0) {
		wmi_err("Error: Event ignored, 0 ndp instances");
		return QDF_STATUS_E_INVAL;
	}

	wmi_debug("number of ndp instances = %d",
		 event->num_ndp_end_indication_list);

	if (event->num_ndp_end_indication_list > ((UINT_MAX - sizeof(**rsp))/
						sizeof((*rsp)->ndp_map[0]))) {
		wmi_err("num_ndp_end_ind_list %d too large",
			 event->num_ndp_end_indication_list);
		return QDF_STATUS_E_INVAL;
	}

	buf_size = sizeof(**rsp) + event->num_ndp_end_indication_list *
			sizeof((*rsp)->ndp_map[0]);
	*rsp = qdf_mem_malloc(buf_size);
	if (!(*rsp))
		return QDF_STATUS_E_NOMEM;

	(*rsp)->num_ndp_ids = event->num_ndp_end_indication_list;
	for (i = 0; i < (*rsp)->num_ndp_ids; i++) {
		WMI_MAC_ADDR_TO_CHAR_ARRAY(&ind[i].peer_ndi_mac_addr,
					   peer_addr.bytes);
		wmi_debug("ind[%d]: type %d, reason_code %d, instance_id %d num_active %d ",
			 i, ind[i].type, ind[i].reason_code,
			 ind[i].ndp_instance_id,
			 ind[i].num_active_ndps_on_peer);
		/* Add each instance entry to the list */
		(*rsp)->ndp_map[i].ndp_instance_id = ind[i].ndp_instance_id;
		(*rsp)->ndp_map[i].vdev_id = ind[i].vdev_id;
		WMI_MAC_ADDR_TO_CHAR_ARRAY(&ind[i].peer_ndi_mac_addr,
			(*rsp)->ndp_map[i].peer_ndi_mac_addr.bytes);
		(*rsp)->ndp_map[i].num_active_ndp_sessions =
			ind[i].num_active_ndps_on_peer;
		(*rsp)->ndp_map[i].type = ind[i].type;
		(*rsp)->ndp_map[i].reason_code = ind[i].reason_code;
	}

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS extract_ndp_sch_update_tlv(wmi_unified_t wmi_handle,
		uint8_t *data, struct nan_datapath_sch_update_event *ind)
{
	uint8_t i;
	WMI_HOST_WLAN_PHY_MODE ch_mode;
	WMI_NDL_SCHEDULE_UPDATE_EVENTID_param_tlvs *event;
	wmi_ndl_schedule_update_fixed_param *fixed_params;
	bool ndi_dbs = wmi_service_enabled(wmi_handle,
					   wmi_service_ndi_dbs_support);

	event = (WMI_NDL_SCHEDULE_UPDATE_EVENTID_param_tlvs *)data;
	fixed_params = event->fixed_param;

	wmi_debug("flags: %d, num_ch: %d, num_ndp_instances: %d",
		 fixed_params->flags, fixed_params->num_channels,
		 fixed_params->num_ndp_instances);

	if (fixed_params->num_channels > event->num_ndl_channel_list ||
	    fixed_params->num_channels > event->num_nss_list) {
		wmi_err("Channel count %d greater than NDP Ch list TLV len(%d) or NSS list TLV len(%d)",
			 fixed_params->num_channels,
			 event->num_ndl_channel_list,
			 event->num_nss_list);
		return QDF_STATUS_E_INVAL;
	}

	if (ndi_dbs &&
	    fixed_params->num_channels > event->num_ndp_channel_info) {
		wmi_err("Channel count %d greater than NDP Ch info(%d)",
			 fixed_params->num_channels,
			 event->num_ndp_channel_info);
		return QDF_STATUS_E_INVAL;
	}

	if (fixed_params->num_ndp_instances > event->num_ndp_instance_list) {
		wmi_err("NDP Instance count %d greater than NDP Instancei TLV len %d",
			 fixed_params->num_ndp_instances,
			 event->num_ndp_instance_list);
		return QDF_STATUS_E_INVAL;
	}

	ind->vdev =
		wlan_objmgr_get_vdev_by_id_from_psoc(wmi_handle->soc->wmi_psoc,
						     fixed_params->vdev_id,
						     WLAN_NAN_ID);
	if (!ind->vdev) {
		wmi_err("vdev is null");
		return QDF_STATUS_E_INVAL;
	}

	ind->flags = fixed_params->flags;
	ind->num_channels = fixed_params->num_channels;
	ind->num_ndp_instances = fixed_params->num_ndp_instances;
	WMI_MAC_ADDR_TO_CHAR_ARRAY(&fixed_params->peer_macaddr,
				   ind->peer_addr.bytes);

	if (ind->num_ndp_instances > NDP_NUM_INSTANCE_ID) {
		wmi_err("uint32 overflow");
		wlan_objmgr_vdev_release_ref(ind->vdev, WLAN_NAN_ID);
		return QDF_STATUS_E_INVAL;
	}

	qdf_mem_copy(ind->ndp_instances, event->ndp_instance_list,
		     sizeof(uint32_t) * ind->num_ndp_instances);

	if (ind->num_channels > NAN_CH_INFO_MAX_CHANNELS) {
		wmi_err("too many channels");
		ind->num_channels = NAN_CH_INFO_MAX_CHANNELS;
	}

	for (i = 0; i < ind->num_channels; i++) {
		ind->ch[i].freq = event->ndl_channel_list[i].mhz;
		ind->ch[i].nss = event->nss_list[i];
		ch_mode = WMI_GET_CHANNEL_MODE(&event->ndl_channel_list[i]);
		ind->ch[i].phymode = ch_mode;
		ind->ch[i].ch_width = wmi_get_ch_width_from_phy_mode(wmi_handle,
								     ch_mode);
		if (ndi_dbs) {
			ind->ch[i].mac_id = event->ndp_channel_info[i].mac_id;
			wmi_debug("Freq: %d, ch_mode: %d, nss: %d mac_id: %d",
				 ind->ch[i].freq, ind->ch[i].ch_width,
				 ind->ch[i].nss, ind->ch[i].mac_id);
		} else {
			wmi_debug("Freq: %d, ch_mode: %d, nss: %d",
				 ind->ch[i].freq, ind->ch[i].ch_width,
				 ind->ch[i].nss);
		}
	}

	for (i = 0; i < fixed_params->num_ndp_instances; i++)
		wmi_debug("instance_id[%d]: %d",
			 i, event->ndp_instance_list[i]);

	return QDF_STATUS_SUCCESS;
}

#if defined(WLAN_FEATURE_NAN) && defined(FEATURE_WLAN_SUPPORT_NAN_STANDARD_MODE)
/**
 * extract_nan_join_cluster_event_tlv() - Extract NAN cluster event (TLV format)
 * @evt_buf: Event buffer from firmware
 * @cluster_event: Output structure to fill
 *
 * This function extracts cluster event information from the WMI TLV buffer
 * sent by firmware.
 *
 * Return: QDF_STATUS_SUCCESS on success, error code otherwise
 */
static QDF_STATUS
extract_nan_join_cluster_event_tlv(uint8_t *evt_buf,
				   struct nan_cluster_event *cluster_event)
{
	WMI_NAN_JOINED_CLUSTER_EVENTID_param_tlvs *param_buf;
	wmi_nan_joined_cluster_event_fixed_param *fixed_param;

	param_buf = (WMI_NAN_JOINED_CLUSTER_EVENTID_param_tlvs *)evt_buf;
	if (!param_buf) {
		wmi_err("Invalid cluster event buffer");
		return QDF_STATUS_E_INVAL;
	}

	fixed_param = param_buf->fixed_param;
	if (!fixed_param) {
		wmi_err("Invalid cluster event fixed param");
		return QDF_STATUS_E_INVAL;
	}

	cluster_event->vdev_id = fixed_param->vdev_id;
	/* Copy 4-byte cluster ID (MAC address format) */
	qdf_mem_copy(&cluster_event->cluster_id, &cluster_id,
		     NAN_CLUSTER_MATCH_SIZE);

	/* FW sent in big-endian order */
	cluster_event->cluster_id[NAN_CLUSTER_MATCH_SIZE] =
		(fixed_param->nan_cluster_id >> 8) & 0xFF;
	cluster_event->cluster_id[NAN_CLUSTER_MATCH_SIZE + 1] =
		fixed_param->nan_cluster_id & 0xFF;

	wmi_debug("Extracted join cluster event: vdev_id=%d, event_type=%d",
		  cluster_event->vdev_id, cluster_event->event_type);

	return QDF_STATUS_SUCCESS;
}

/**
 * extract_nan_cluster_event_tlv() - Extract NAN cluster event (TLV format)
 * @wmi_handle: WMI handle
 * @evt_buf: Event buffer from firmware
 * @cluster_event: Output structure to fill
 *
 * This function extracts cluster event information from the WMI TLV buffer
 * sent by firmware.
 *
 * Return: QDF_STATUS_SUCCESS on success, error code otherwise
 */
static QDF_STATUS extract_nan_cluster_event_tlv(
				wmi_unified_t wmi_handle,
				uint8_t *evt_buf,
				struct nan_cluster_event *cluster_event)
{
	QDF_STATUS status;
	WMI_NAN_STARTED_CLUSTER_EVENTID_param_tlvs *param_buf;
	wmi_nan_started_cluster_event_fixed_param *fixed_param;

	if (cluster_event->event_type == NAN_CLUSTER_EVENT_JOINED) {
		status =
			extract_nan_join_cluster_event_tlv(evt_buf,
							   cluster_event);
		return status;
	}

	param_buf = (WMI_NAN_STARTED_CLUSTER_EVENTID_param_tlvs *)evt_buf;
	if (!param_buf) {
		wmi_err("Invalid cluster event buffer");
		return QDF_STATUS_E_INVAL;
	}

	fixed_param = param_buf->fixed_param;
	if (!fixed_param) {
		wmi_err("Invalid cluster event fixed param");
		return QDF_STATUS_E_INVAL;
	}

	cluster_event->vdev_id = fixed_param->vdev_id;

	/* Copy 4-byte cluster ID (MAC address format) */
	qdf_mem_copy(&cluster_event->cluster_id, &cluster_id,
		     NAN_CLUSTER_MATCH_SIZE);

	/* FW sent in big-endian order */
	cluster_event->cluster_id[NAN_CLUSTER_MATCH_SIZE] =
		(fixed_param->nan_cluster_id >> 8) & 0xFF;
	cluster_event->cluster_id[NAN_CLUSTER_MATCH_SIZE + 1] =
		fixed_param->nan_cluster_id & 0xFF;
	cluster_event->event_type = NAN_CLUSTER_EVENT_STARTED;

	wmi_debug("Extracted start cluster event: vdev_id=%d, event_type=%d",
		  cluster_event->vdev_id, cluster_event->event_type);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS
extract_nan_next_dw_info_event_tlv(wmi_unified_t wmi_handle,
				   uint8_t *evt_buf,
				   struct nan_next_dw_info_event *event)
{
	WMI_NAN_NEXT_DW_INFO_EVENTID_param_tlvs *param_buf;
	wmi_nan_next_dw_info_fixed_param *fixed_param;

	if (!wmi_handle || !evt_buf || !event) {
		wmi_err("Invalid parameters");
		return QDF_STATUS_E_INVAL;
	}

	param_buf = (WMI_NAN_NEXT_DW_INFO_EVENTID_param_tlvs *)evt_buf;
	if (!param_buf) {
		wmi_err("Invalid param_buf");
		return QDF_STATUS_E_INVAL;
	}

	fixed_param = param_buf->fixed_param;
	if (!fixed_param) {
		wmi_err("Invalid fixed_param");
		return QDF_STATUS_E_INVAL;
	}

	/* Extract parameters from WMI structure */
	event->vdev_id = fixed_param->vdev_id;
	event->channel_freq = fixed_param->dw_chan_freq;

	wmi_err("NAN Next DW Info: vdev_id=%u, freq=%u",
		event->vdev_id, event->channel_freq);

	return QDF_STATUS_SUCCESS;
}

/**
 * extract_nan_dfs_channel_availability_ind_tlv() - Extract NAN DFS channel
 *     availability indication event (TLV format)
 * @wmi_handle: WMI handle
 * @evt_buf: Event data buffer
 * @event: Output structure to fill
 *
 * Return: QDF_STATUS_SUCCESS on success, error code otherwise
 */
static QDF_STATUS
extract_nan_dfs_channel_availability_ind_tlv(
	wmi_unified_t wmi_handle,
	uint8_t *evt_buf,
	struct nan_dfs_channel_availability_ind *event)
{
	WMI_NAN_DFS_CHANNEL_AVAILABILITY_IND_EVENTID_param_tlvs *param_buf;
	wmi_nan_dfs_channel_availability_ind_event_fixed_param *fixed_param;

	if (!wmi_handle || !evt_buf || !event) {
		wmi_err("Invalid parameters");
		return QDF_STATUS_E_INVAL;
	}

	param_buf = (WMI_NAN_DFS_CHANNEL_AVAILABILITY_IND_EVENTID_param_tlvs *)
			evt_buf;
	if (!param_buf) {
		wmi_err("Invalid param_buf");
		return QDF_STATUS_E_INVAL;
	}

	fixed_param = param_buf->fixed_param;
	if (!fixed_param) {
		wmi_err("Invalid fixed_param");
		return QDF_STATUS_E_INVAL;
	}

	event->vdev_id = fixed_param->vdev_id;
	event->status  = fixed_param->status;

	wmi_debug("NAN DFS channel availability: vdev_id=%u status=%u",
		  event->vdev_id, event->status);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS nan_local_schedule_cmd_tlv(wmi_unified_t wmi_handle,
					     struct nan_local_sched_params *req)
{
	uint16_t len;
	wmi_buf_t buf;
	uint8_t *tlv_ptr;
	QDF_STATUS status;
	wmi_nan_local_schedule_cmd_fixed_param *cmd;
	wmi_nan_channel *ch_tlv;
	uint32_t schedule_len, channels_len;
	uint8_t i;
	uint32_t vdev_id;

	if (!req) {
		wmi_err("Invalid parameters");
		return QDF_STATUS_E_INVAL;
	}

	vdev_id = req->vdev_id;

	/* Calculate lengths (4-byte aligned) */
	/* Schedule is uint8_t[512] = 512 bytes */
	schedule_len = qdf_roundup(NAN_MAX_SCHEDULE_SLOTS * sizeof(uint8_t),
				   sizeof(uint32_t));
	channels_len = qdf_roundup(req->num_channels * sizeof(wmi_nan_channel),
				   sizeof(uint32_t));

	/* Total length */
	len = sizeof(*cmd) + WMI_TLV_HDR_SIZE + schedule_len +
	      WMI_TLV_HDR_SIZE + channels_len;

	buf = wmi_buf_alloc(wmi_handle, len);
	if (!buf)
		return QDF_STATUS_E_NOMEM;

	tlv_ptr = (uint8_t *)wmi_buf_data(buf);

	cmd = (wmi_nan_local_schedule_cmd_fixed_param *)wmi_buf_data(buf);
	qdf_mem_zero(cmd, sizeof(*cmd));

	WMITLV_SET_HDR(&cmd->tlv_header,
		       WMITLV_TAG_STRUC_wmi_nan_local_schedule_cmd_fixed_param,
		       WMITLV_GET_STRUCT_TLVLEN(
				wmi_nan_local_schedule_cmd_fixed_param));

	cmd->vdev_id = vdev_id;
	cmd->num_channels = req->num_channels;
	cmd->schedule_chan_bitmap_len = NAN_MAX_SCHEDULE_SLOTS;

	tlv_ptr += sizeof(*cmd);

	/* Pack schedule bitmap */
	WMITLV_SET_HDR(tlv_ptr, WMITLV_TAG_ARRAY_BYTE, schedule_len);
	tlv_ptr += WMI_TLV_HDR_SIZE;
	qdf_mem_copy(tlv_ptr, req->schedule, NAN_MAX_SCHEDULE_SLOTS);
	tlv_ptr += schedule_len;

	/* Pack channel list */
	WMITLV_SET_HDR(tlv_ptr, WMITLV_TAG_ARRAY_STRUC, channels_len);
	tlv_ptr += WMI_TLV_HDR_SIZE;

	ch_tlv = (wmi_nan_channel *)tlv_ptr;
	for (i = 0; i < req->num_channels; i++) {
		WMITLV_SET_HDR(&ch_tlv[i], WMITLV_TAG_STRUC_wmi_channel,
			       WMITLV_GET_STRUCT_TLVLEN(wmi_nan_channel));
		ch_tlv[i].chan_freq = req->ch[i].freq;
		ch_tlv[i].center_freq1 = req->ch[i].center_freq1;
		ch_tlv[i].center_freq2 = req->ch[i].center_freq2;
		ch_tlv[i].chan_width = req->ch[i].ch_width;
		ch_tlv[i].rx_nss = req->ch[i].rx_nss;
		qdf_mem_copy(ch_tlv[i].chan_entry_fields,
			     req->ch[i].channel_entry, NAN_CHANNEL_ENTRY_LEN);

		wmi_debug("Channel[%d]: freq=%d, ch_width=%d, cf1=%d, cf2=%d rx_nss %d",
			  i, ch_tlv[i].chan_freq, ch_tlv[i].chan_width,
			  ch_tlv[i].center_freq1, ch_tlv[i].center_freq2,
			  ch_tlv[i].rx_nss);

		tlv_ptr += sizeof(wmi_nan_channel);
	}

	wmi_debug("vdev_id=%d, num_channels=%d", vdev_id, req->num_channels);

	wmi_mtrace(WMI_NAN_LOCAL_SCHEDULE_CMDID, vdev_id, 0);
	status = wmi_unified_cmd_send(wmi_handle, buf, len,
				      WMI_NAN_LOCAL_SCHEDULE_CMDID);
	if (QDF_IS_STATUS_ERROR(status)) {
		wmi_err("WMI_NAN_LOCAL_SCHEDULE_CMDID failed, ret: %d",
			status);
		wmi_buf_free(buf);
	}

	return status;
}

static QDF_STATUS
extract_nan_local_schedule_cnf_tlv(wmi_unified_t wmi_handle,
				   uint8_t *data,
				   struct nan_local_sched_rsp *rsp)
{
	WMI_NAN_LOCAL_SCHEDULE_CNF_EVENTID_param_tlvs *event;
	wmi_nan_local_schedule_cnf_event_fixed_param *fixed_params;

	event = (WMI_NAN_LOCAL_SCHEDULE_CNF_EVENTID_param_tlvs *)data;
	fixed_params = event->fixed_param;

	if (!fixed_params) {
		wmi_err("fixed_params is null");
		return QDF_STATUS_E_INVAL;
	}

	rsp->vdev_id = fixed_params->vdev_id;
	rsp->status = fixed_params->status;
	rsp->reason = fixed_params->reason;

	wmi_debug("WMI_NAN_LOCAL_SCHEDULE_CNF_EVENTID: vdev_id=%d, status=%d, reason=%d",
		  fixed_params->vdev_id, rsp->status, rsp->reason);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS nan_peer_schedule_cmd_tlv(wmi_unified_t wmi_handle,
					    struct nan_peer_sched_params *req)
{
	uint16_t len;
	wmi_buf_t buf;
	uint8_t *tlv_ptr;
	QDF_STATUS status;
	wmi_nan_peer_schedule_cmd_fixed_param *cmd;
	uint32_t schedule_len, channels_len;
	uint32_t vdev_id;
	wmi_nan_channel *ch_tlv;
	int i;

	if (!req) {
		wmi_err("Invalid parameters");
		return QDF_STATUS_E_INVAL;
	}

	vdev_id = req->vdev_id;

	/* Calculate lengths (4-byte aligned) */
	/* Schedule is uint8_t[512] = 512 bytes */
	schedule_len = qdf_roundup(NAN_MAX_SCHEDULE_SLOTS * sizeof(uint8_t), 4);
	channels_len = req->num_channels * sizeof(wmi_nan_channel);

	/* Total length */
	len = sizeof(*cmd) + WMI_TLV_HDR_SIZE + schedule_len +
	      WMI_TLV_HDR_SIZE + channels_len;

	buf = wmi_buf_alloc(wmi_handle, len);
	if (!buf)
		return QDF_STATUS_E_NOMEM;

	cmd = (wmi_nan_peer_schedule_cmd_fixed_param *)wmi_buf_data(buf);
	WMITLV_SET_HDR(&cmd->tlv_header,
		       WMITLV_TAG_STRUC_wmi_nan_peer_schedule_cmd_fixed_param,
		       WMITLV_GET_STRUCT_TLVLEN(
				wmi_nan_peer_schedule_cmd_fixed_param));

	cmd->vdev_id = vdev_id;
	WMI_CHAR_ARRAY_TO_MAC_ADDR(req->peer_addr.bytes,
				   &cmd->peer_nmi_addr);
	cmd->schedule_chan_bitmap_len = NAN_MAX_SCHEDULE_SLOTS;
	cmd->peer_committed_dw = req->committed_dw;
	cmd->peer_max_chan_switch = req->max_chan_switch;

	tlv_ptr = (uint8_t *)&cmd[1];

	/* Pack schedule bitmap */
	WMITLV_SET_HDR(tlv_ptr, WMITLV_TAG_ARRAY_BYTE, schedule_len);
	qdf_mem_copy(&tlv_ptr[WMI_TLV_HDR_SIZE], req->maps[0].schedule,
		     NAN_MAX_SCHEDULE_SLOTS * sizeof(uint8_t));
	tlv_ptr += WMI_TLV_HDR_SIZE + schedule_len;

	/* Pack channel list */
	WMITLV_SET_HDR(tlv_ptr, WMITLV_TAG_ARRAY_STRUC, channels_len);
	ch_tlv = (wmi_nan_channel *)&tlv_ptr[WMI_TLV_HDR_SIZE];
	for (i = 0; i < req->num_channels; i++) {
		WMITLV_SET_HDR(&ch_tlv[i], WMITLV_TAG_STRUC_wmi_nan_channel,
			       WMITLV_GET_STRUCT_TLVLEN(wmi_nan_channel));
		ch_tlv[i].chan_freq = req->nan_channels[i].freq;
		ch_tlv[i].center_freq1 = req->nan_channels[i].center_freq1;
		ch_tlv[i].center_freq2 = req->nan_channels[i].center_freq2;
		ch_tlv[i].chan_width = req->nan_channels[i].ch_width;
		ch_tlv[i].rx_nss = req->nan_channels[i].rx_nss;

		qdf_mem_copy(ch_tlv[i].chan_entry_fields,
			     req->nan_channels[i].channel_entry,
			     NAN_CHANNEL_ENTRY_LEN);

		wmi_debug("channel_entry:");
		QDF_TRACE_HEX_DUMP(QDF_MODULE_ID_WMI, QDF_TRACE_LEVEL_DEBUG,
				   ch_tlv[i].chan_entry_fields, 6);
		wmi_debug("Channel[%d]: freq=%d, ch_width=%d, cf1=%d, cf2=%d rx_nss %d",
			  i, ch_tlv[i].chan_freq, ch_tlv[i].chan_width,
			  ch_tlv[i].center_freq1,
			  ch_tlv[i].center_freq2, ch_tlv[i].rx_nss);
	}

	wmi_debug("vdev_id=%d, peer_addr= " QDF_MAC_ADDR_FMT,
		  vdev_id, QDF_MAC_ADDR_REF(req->peer_addr.bytes));

	wmi_mtrace(WMI_NAN_PEER_SCHEDULE_CMDID, vdev_id, 0);
	status = wmi_unified_cmd_send(wmi_handle, buf, len,
				      WMI_NAN_PEER_SCHEDULE_CMDID);
	if (QDF_IS_STATUS_ERROR(status)) {
		wmi_err("WMI_NAN_PEER_SCHEDULE_CMDID failed, ret: %d",
			status);
		wmi_buf_free(buf);
	}

	return status;
}

static QDF_STATUS extract_nan_peer_schedule_cnf_tlv(
						wmi_unified_t wmi_handle,
						uint8_t *data,
						struct nan_peer_sched_rsp *rsp)
{
	WMI_NAN_PEER_SCHEDULE_CNF_EVENTID_param_tlvs *event;
	wmi_nan_peer_schedule_cnf_event_fixed_param *fixed_params;

	event = (WMI_NAN_PEER_SCHEDULE_CNF_EVENTID_param_tlvs *)data;
	fixed_params = event->fixed_param;

	if (!fixed_params) {
		wmi_err("fixed_params is null");
		return QDF_STATUS_E_INVAL;
	}

	rsp->vdev_id = fixed_params->vdev_id;
	rsp->status = fixed_params->status;

	wmi_debug("WMI_NAN_PEER_SCHEDULE_CNF_EVENTID: vdev_id=%d, status=%d",
		  fixed_params->vdev_id, rsp->status);

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS nan_peer_params_cmd_tlv(wmi_unified_t wmi_handle,
					  struct nan_peer_params_req *req)
{
	uint16_t len;
	wmi_buf_t buf;
	uint8_t *tlv_ptr;
	QDF_STATUS status;
	wmi_nan_peer_params_cmd_fixed_param *cmd;
	uint32_t peer_caps_ie_len_aligned;

	if (!req) {
		wmi_err("Invalid parameters");
		return QDF_STATUS_E_INVAL;
	}

	/* Calculate aligned length for capability IEs */
	peer_caps_ie_len_aligned = qdf_roundup(req->peer_cap_len, 4);

	/* Total length */
	len = sizeof(*cmd) + WMI_TLV_HDR_SIZE + peer_caps_ie_len_aligned;

	buf = wmi_buf_alloc(wmi_handle, len);
	if (!buf)
		return QDF_STATUS_E_NOMEM;

	cmd = (wmi_nan_peer_params_cmd_fixed_param *)wmi_buf_data(buf);
	WMITLV_SET_HDR(&cmd->tlv_header,
		       WMITLV_TAG_STRUC_wmi_nan_peer_params_cmd_fixed_param,
		       WMITLV_GET_STRUCT_TLVLEN(
				wmi_nan_peer_params_cmd_fixed_param));

	cmd->vdev_id = req->vdev_id;
	WMI_CHAR_ARRAY_TO_MAC_ADDR(req->peer_nmi_addr.bytes,
				   &cmd->peer_nmi_addr);
	WMI_CHAR_ARRAY_TO_MAC_ADDR(req->peer_ndi_mac_addr.bytes,
				   &cmd->peer_ndi_macaddr);
	cmd->peer_flags = req->peer_flags;
	cmd->peer_caps_ie_len = req->peer_cap_len;

	tlv_ptr = (uint8_t *)&cmd[1];

	/* Pack capability IEs data */
	WMITLV_SET_HDR(tlv_ptr, WMITLV_TAG_ARRAY_BYTE,
		       peer_caps_ie_len_aligned);
	if (req->peer_cap_len && req->peer_cap) {
		qdf_mem_copy(&tlv_ptr[WMI_TLV_HDR_SIZE], req->peer_cap,
			     req->peer_cap_len);
	}

	wmi_debug("vdev_id=%d, peer_nmi_addr=" QDF_MAC_ADDR_FMT
		  ", peer_ndi_mac_addr=" QDF_MAC_ADDR_FMT
		  ", peer_flags=0x%x, peer_caps_ie_len=%d",
		  req->vdev_id, QDF_MAC_ADDR_REF(req->peer_nmi_addr.bytes),
		  QDF_MAC_ADDR_REF(req->peer_ndi_mac_addr.bytes),
		  cmd->peer_flags, cmd->peer_caps_ie_len);

	wmi_mtrace(WMI_NAN_PEER_PARAMS_CMDID, req->vdev_id, 0);
	status = wmi_unified_cmd_send(wmi_handle, buf, len,
				      WMI_NAN_PEER_PARAMS_CMDID);
	if (QDF_IS_STATUS_ERROR(status)) {
		wmi_err("WMI_NAN_PEER_PARAMS_CMDID failed, ret: %d", status);
		wmi_buf_free(buf);
	}

	return status;
}

static QDF_STATUS extract_nan_peer_params_cnf_tlv(
						wmi_unified_t wmi_handle,
						uint8_t *data,
						struct nan_peer_params_rsp *rsp)
{
	WMI_NAN_PEER_PARAMS_CNF_EVENTID_param_tlvs *event;
	wmi_nan_peer_params_cnf_event_fixed_param *fixed_params;

	event = (WMI_NAN_PEER_PARAMS_CNF_EVENTID_param_tlvs *)data;
	fixed_params = event->fixed_param;

	if (!fixed_params) {
		wmi_err("fixed_params is null");
		return QDF_STATUS_E_INVAL;
	}

	rsp->vdev_id = fixed_params->vdev_id;
	rsp->status = fixed_params->status;

	wmi_debug("WMI_NAN_PEER_PARAMS_CNF_EVENTID: vdev_id=%d, status=%d",
		  fixed_params->vdev_id, rsp->status);

	return QDF_STATUS_SUCCESS;
}

/**
 * wmi_nan_attach_schedule_ops_tlv() - Attach NAN schedule ops
 * @ops: pointer to wmi_ops structure
 *
 * This function attaches NAN schedule specific operations to wmi_ops.
 * It is guarded by feature flags and called from wmi_nan_attach_tlv.
 *
 * Return: void
 */
static void wmi_nan_attach_schedule_ops_tlv(struct wmi_ops *ops)
{
	ops->send_nan_local_schedule_cmd = nan_local_schedule_cmd_tlv;
	ops->extract_nan_local_schedule_cnf =
					extract_nan_local_schedule_cnf_tlv;
	ops->send_nan_peer_schedule_cmd = nan_peer_schedule_cmd_tlv;
	ops->extract_nan_peer_schedule_cnf = extract_nan_peer_schedule_cnf_tlv;
	ops->send_nan_peer_params_cmd = nan_peer_params_cmd_tlv;
	ops->extract_nan_peer_params_cnf = extract_nan_peer_params_cnf_tlv;
}
#else
/**
 * wmi_nan_attach_schedule_ops_tlv() - Stub for NAN schedule ops
 * @ops: pointer to wmi_ops structure
 *
 * This is a stub function when NAN schedule feature is not enabled.
 *
 * Return: void
 */
static void wmi_nan_attach_schedule_ops_tlv(struct wmi_ops *ops)
{
}
#endif /* FEATURE_WLAN_SUPPORT_NAN_STANDARD_MODE */

#if defined(WLAN_FEATURE_NAN) && defined(FEATURE_WLAN_SUPPORT_NAN_STANDARD_MODE)
static
void wmi_nan_attach_dw_info_tlv(wmi_unified_t wmi_handle)
{
	struct wmi_ops *ops = wmi_handle->ops;

	ops->extract_nan_next_dw_info = extract_nan_next_dw_info_event_tlv;
	ops->extract_nan_dfs_channel_availability_ind =
			extract_nan_dfs_channel_availability_ind_tlv;
}

static
void wmi_nan_attach_cluster_info_tlv(wmi_unified_t wmi_handle)
{
	struct wmi_ops *ops = wmi_handle->ops;

	ops->extract_nan_cluster_event = extract_nan_cluster_event_tlv;
}

static void wmi_nan_standard_mode_event_ops(struct wmi_ops *ops)
{
	ops->extract_nan_disable_rsp_event = extract_nan_disable_rsp_event_tlv;
	ops->extract_nan_disable_ind_event = extract_nan_disable_ind_event_tlv;
	ops->extract_nan_enable_rsp_event = extract_nan_enable_rsp_event_tlv;
	ops->send_nan_stop_req_cmd = send_nan_stop_req_cmd_tlv;
	ops->send_nan_start_req_cmd = send_nan_start_req_cmd_tlv;
	ops->send_nan_change_conf_req_cmd = send_nan_change_conf_req_cmd_tlv;
}
#else
static inline
void wmi_nan_attach_dw_info_tlv(wmi_unified_t wmi_handle)
{}

static inline
void wmi_nan_attach_cluster_info_tlv(wmi_unified_t wmi_handle)
{}

static inline void wmi_nan_standard_mode_event_ops(struct wmi_ops *ops)
{
}
#endif

void wmi_nan_attach_tlv(wmi_unified_t wmi_handle)
{
	struct wmi_ops *ops = wmi_handle->ops;

	ops->send_nan_req_cmd = send_nan_req_cmd_tlv;
	ops->send_nan_disable_req_cmd = send_nan_disable_req_cmd_tlv;
	ops->extract_nan_event_rsp = extract_nan_event_rsp_tlv;
	wmi_nan_standard_mode_event_ops(ops);
	ops->send_terminate_all_ndps_req_cmd = send_terminate_all_ndps_cmd_tlv;
	ops->send_ndp_initiator_req_cmd = nan_ndp_initiator_req_tlv;
	ops->send_ndp_responder_req_cmd = nan_ndp_responder_req_tlv;
	ops->send_ndp_end_req_cmd = nan_ndp_end_req_tlv;
	ops->send_ndp_update_config_cmd = nan_ndp_update_config_tlv;
	ops->extract_ndp_initiator_rsp = extract_ndp_initiator_rsp_tlv;
	ops->extract_ndp_ind = extract_ndp_ind_tlv;
	ops->extract_nan_msg = extract_nan_msg_tlv,
	ops->extract_ndp_confirm = extract_ndp_confirm_tlv;
	ops->extract_ndp_responder_rsp = extract_ndp_responder_rsp_tlv;
	ops->extract_ndp_end_rsp = extract_ndp_end_rsp_tlv;
	ops->extract_ndp_end_ind = extract_ndp_end_ind_tlv;
	ops->extract_ndp_sch_update = extract_ndp_sch_update_tlv;
	ops->extract_ndp_host_event = extract_ndp_host_event_tlv;
	wmi_nan_attach_dw_info_tlv(wmi_handle);
	wmi_nan_attach_cluster_info_tlv(wmi_handle);
	wmi_nan_attach_schedule_ops_tlv(ops);
	wmi_nan_attach_add_func_tlv(wmi_handle);
}
