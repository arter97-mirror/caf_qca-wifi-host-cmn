/*
 * Copyright (c) 2016-2018, 2020 The Linux Foundation. All rights reserved.
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

#include "wmi_unified_action_oui_tlv.h"
#include "wmi_unified_priv.h"

bool wmi_get_action_oui_id(enum action_oui_id action_id,
			   wmi_vendor_oui_action_id *id)
{
	switch (action_id) {

	case ACTION_OUI_CONNECT_1X1:
		*id = WMI_VENDOR_OUI_ACTION_CONNECTION_1X1;
		return true;

	case ACTION_OUI_ITO_EXTENSION:
		*id = WMI_VENDOR_OUI_ACTION_ITO_EXTENSION;
		return true;

	case ACTION_OUI_CCKM_1X1:
		*id = WMI_VENDOR_OUI_ACTION_CCKM_1X1;
		return true;

	case ACTION_OUI_ITO_ALTERNATE:
		*id = WMI_VENDOR_OUI_ACTION_ALT_ITO;
		return true;

	case ACTION_OUI_SWITCH_TO_11N_MODE:
		*id = WMI_VENDOR_OUI_ACTION_SWITCH_TO_11N_MODE;
		return true;

	case ACTION_OUI_CONNECT_1X1_WITH_1_CHAIN:
		*id = WMI_VENDOR_OUI_ACTION_CONNECTION_1X1_NUM_TX_RX_CHAINS_1;
		return true;

	case ACTION_OUI_DISABLE_AGGRESSIVE_TX:
		*id = WMI_VENDOR_OUI_ACTION_DISABLE_AGGRESSIVE_TX;
		return true;

	case ACTION_OUI_DISABLE_TWT:
		*id = WMI_VENDOR_OUI_ACTION_DISABLE_FW_TRIGGERED_TWT;
		return true;

	case ACTION_OUI_EXTEND_WOW_ITO:
		*id = WMI_VENDOR_OUI_ACTION_EXTEND_WOW_ITO;
		return true;
	case ACTION_OUI_11BE_OUI_ALLOW:
		*id = WMI_VENDOR_OUI_ACTION_ALLOW_11BE;
		return true;
	case ACTION_OUI_DISABLE_DYNAMIC_QOS_NULL_TX_RATE:
		*id = WMI_VENDOR_OUI_ACTION_DISABLE_DYNAMIC_QOS_NULL_TX_RATE;
		return true;
	case ACTION_OUI_ENABLE_CTS2SELF_WITH_QOS_NULL:
		*id = WMI_VENDOR_OUI_ACTION_ENABLE_CTS2SELF_WITH_QOS_NULL;
		return true;
	case ACTION_OUI_SEND_SMPS_FRAME_WITH_OMN:
		*id = WMI_VENDOR_OUI_ACTION_SEND_SMPS_FRAME_WITH_OMN;
		return true;
	case ACTION_OUI_RESTRICT_SLO:
		*id = WMI_VENDOR_OUI_ACTION_RESTRICT_SLO;
		return true;
	case ACTION_OUI_DISABLE_AUX_LISTEN:
		*id = WMI_VENDOR_OUI_ACTION_DISABLE_AUXL;
		return true;
	case ACTION_OUI_RESTRICT_MAX_MLO_LINKS:
		*id = WMI_VENDOR_OUI_ACTION_RESTRICT_MAX_MLO_LINKS;
		return true;
	case ACTION_OUI_AUTH_ASSOC_6MBPS_2GHZ:
		*id = WMI_VENDOR_OUI_ACTION_AUTH_ASSOC_6MBPS_2GHZ;
		return true;
	case ACTION_OUI_DISABLE_DYNAMIC_SMPS:
		*id = WMI_VENDOR_OUI_ACTION_DISABLE_DYNAMIC_SMPS;
		return true;
	case ACTION_OUI_FORCE_TX_NULL_FRAME_ON_P2P:
		*id = WMI_VENDOR_OUI_ACTION_FORCE_TX_NULL_FRAME_ON_P2P;
		return true;
	case ACTION_OUI_ALLOW_NSS_GREATER_THAN_2:
		*id = WMI_VENDOR_OUI_ACTION_ALLOW_NSS_GREATER_THAN_2;
		return true;
	case ACTION_OUI_DISALLOW_NSS_GREATER_THAN_2:
		*id = WMI_VENDOR_OUI_ACTION_DISALLOW_NSS_GREATER_THAN_2;
		return true;
	default:
		return false;
	}
}

uint32_t wmi_get_action_oui_info_mask(uint32_t info_mask,
				      bool is_mac_exclusion_enabled)
{
	uint32_t info_presence = 0;

	if (info_mask & ACTION_OUI_INFO_OUI)
		info_presence |= WMI_BEACON_INFO_PRESENCE_OUI_EXT;

	if (info_mask & ACTION_OUI_INFO_MAC_ADDRESS)
		info_presence |= WMI_BEACON_INFO_PRESENCE_MAC_ADDRESS;

	if (info_mask & ACTION_OUI_INFO_AP_CAPABILITY_NSS)
		info_presence |= WMI_BEACON_INFO_PRESENCE_AP_CAPABILITY_NSS;

	if (info_mask & ACTION_OUI_INFO_AP_CAPABILITY_HT)
		info_presence |= WMI_BEACON_INFO_PRESENCE_AP_CAPABILITY_HT;

	if (info_mask & ACTION_OUI_INFO_AP_CAPABILITY_VHT)
		info_presence |= WMI_BEACON_INFO_PRESENCE_AP_CAPABILITY_VHT;

	if (info_mask & ACTION_OUI_INFO_AP_CAPABILITY_BAND)
		info_presence |= WMI_BEACON_INFO_PRESENCE_AP_CAPABILITY_BAND;

	/* Convert MAC exclusion bit only if enabled */
	if ((info_mask & ACTION_OUI_INFO_MAC_EXCLUSION) &&
	    is_mac_exclusion_enabled)
		info_presence |= WMI_BEACON_INFO_PRESENCE_EXCLUSIVE_MAC_ADDRESS;

	return info_presence;
}

void wmi_fill_oui_extensions(struct action_oui_extension *extension,
			     uint32_t no_oui_extns,
			     wmi_vendor_oui_ext *cmd_ext,
			     bool is_action_oui_v2_enabled,
			     bool is_mac_exclusion_enabled)
{
	uint32_t i;
	uint32_t buffer_length;

	for (i = 0; i < no_oui_extns; i++) {
		WMITLV_SET_HDR(&cmd_ext->tlv_header,
			       WMITLV_TAG_STRUC_wmi_vendor_oui_ext,
			       WMITLV_GET_STRUCT_TLVLEN(wmi_vendor_oui_ext));
		cmd_ext->info_presence_bit_mask =
			wmi_get_action_oui_info_mask(extension->info_mask,
						     is_mac_exclusion_enabled);

		cmd_ext->oui_header_length = extension->oui_length;
		cmd_ext->oui_data_length = extension->data_length;
		cmd_ext->mac_address_length = extension->mac_addr_length;
		cmd_ext->capability_data_length =
					extension->capability_length;
		cmd_ext->exclusive_mac_address_length = 0;

		buffer_length = extension->oui_length +
				extension->data_length +
				extension->data_mask_length +
				extension->mac_addr_length +
				extension->mac_mask_length +
				extension->capability_length;

		/* Add MAC exclusion fields if present and enabled */
		if (is_mac_exclusion_enabled) {
			cmd_ext->exclusive_mac_address_length =
				extension->mac_exclusion_length;
			buffer_length += extension->mac_exclusion_length +
					 extension->mac_exclusion_mask_length;
			if (extension->mac_exclusion_length)
				wmi_debug("Extension %d has MAC exclusion: "
					QDF_MAC_ADDR_FMT " mask=0x%02X", i,
					QDF_MAC_ADDR_REF(extension->mac_exclusion.mac_addr),
					extension->mac_exclusion.mac_addr_mask);
		}

		if (is_action_oui_v2_enabled && !extension->and_oui_index)
			cmd_ext->buf_data_length = buffer_length + 2;
		else
			cmd_ext->buf_data_length = buffer_length + 1;

		cmd_ext++;
		extension++;
	}
}

QDF_STATUS
wmi_fill_oui_extensions_buffer(struct action_oui_extension *extension,
			       wmi_vendor_oui_ext *cmd_ext,
			       uint32_t no_oui_extns,
			       uint32_t rem_var_buf_len,
			       uint8_t *var_buf,
			       bool is_action_oui_v2_enabled,
			       bool is_mac_exclusion_enabled)
{
	uint8_t i, and_oui_num = 0;
	uint8_t *and_oui_num_ptr = NULL;
	uint8_t *buf;

	for (i = 0; i < (uint8_t)no_oui_extns; i++) {
		if ((rem_var_buf_len - cmd_ext->buf_data_length) < 0) {
			wmi_err("Invalid action oui command length");
			return QDF_STATUS_E_INVAL;
		}

		buf = var_buf;
		if (is_action_oui_v2_enabled) {
			if (!extension->and_oui_index) {
				/* Fill and oui num for last and expression */
				if (and_oui_num_ptr)
					*and_oui_num_ptr = and_oui_num;
				/* Remember location of and oui num for current and expression */
				and_oui_num_ptr = &var_buf[0];
				and_oui_num = 1;
				var_buf[1] = i;
				var_buf += 2;
			} else {
				and_oui_num++;
				var_buf[0] = i;
				var_buf++;
			}
		} else {
			var_buf[0] = i;
			var_buf++;
		}

		if (extension->oui_length) {
			qdf_mem_copy(var_buf, extension->oui,
				     extension->oui_length);
			var_buf += extension->oui_length;
		}

		if (extension->data_length) {
			qdf_mem_copy(var_buf, extension->data,
				     extension->data_length);
			var_buf += extension->data_length;
		}

		if (extension->data_mask_length) {
			qdf_mem_copy(var_buf, extension->data_mask,
				     extension->data_mask_length);
			var_buf += extension->data_mask_length;
		}

		if (extension->mac_addr_length) {
			qdf_mem_copy(var_buf, extension->mac_addr,
				     extension->mac_addr_length);
			var_buf += extension->mac_addr_length;
		}

		if (extension->mac_mask_length) {
			qdf_mem_copy(var_buf, extension->mac_mask,
				     extension->mac_mask_length);
			var_buf += extension->mac_mask_length;
		}

		if (extension->capability_length) {
			qdf_mem_copy(var_buf, extension->capability,
				     extension->capability_length);
			var_buf += extension->capability_length;
		}

		/* Copy MAC exclusion address if present and enabled */
		if (is_mac_exclusion_enabled && extension->mac_exclusion_length) {
			qdf_mem_copy(var_buf,
				     extension->mac_exclusion.mac_addr,
				     QDF_MAC_ADDR_SIZE);
			var_buf += extension->mac_exclusion_length;
			wmi_debug("Copied MAC exclusion addr " QDF_MAC_ADDR_FMT,
				  QDF_MAC_ADDR_REF(extension->mac_exclusion.mac_addr));
		}

		/* Copy MAC exclusion mask if present and enabled */
		if (is_mac_exclusion_enabled && extension->mac_exclusion_mask_length) {
			var_buf[0] = extension->mac_exclusion.mac_addr_mask;
			var_buf += extension->mac_exclusion_mask_length;
			wmi_debug("Copied MAC exclusion mask 0x%02X",
				  extension->mac_exclusion.mac_addr_mask);
		}

		rem_var_buf_len -= cmd_ext->buf_data_length;
		qdf_trace_hex_dump(QDF_MODULE_ID_WMI, QDF_TRACE_LEVEL_TRACE,
				   buf, cmd_ext->buf_data_length);
		cmd_ext++;
		extension++;
	}

	/* Fill and oui num for last and expression */
	if (and_oui_num_ptr)
		*and_oui_num_ptr = and_oui_num;

	return QDF_STATUS_SUCCESS;
}

/* If both host and F/W enabled action oui v2, oui is sent to F/W by action oui
 * v2 format, otherwise, oui is sent to F/W by legacy format.
 *
 * Action oui v2 wmi format for "OUI0 && OUI1 ||OUI2 ||OUI3 && OUI4"
 * [2][0]OUI0[1]OUI1[1][2]OUI2[2][3]OUI3[4]OUI
 * 2 means 2 and OUI, such as OUI0 && OUI1.
 * 1 means 1 and OUI: OUI2.
 *
 * Action oui v2 wmi format for "OUI0 OUI1 OUI2"
 * [1][0]OUI0[1][1]OUI1[1][2]OUI2
 *
 * Legacy wmi format for "OUI0 OUI1 OUI2"
 * [0]OUI0[1]OUI1[2]OUI2
 */
QDF_STATUS
send_action_oui_cmd_tlv(wmi_unified_t wmi_handle,
			struct action_oui_request *req)
{
	wmi_pdev_config_vendor_oui_action_fixed_param *cmd;
	wmi_vendor_oui_ext *cmd_ext;
	wmi_buf_t wmi_buf;
	struct action_oui_extension *extension;
	uint32_t len;
	uint32_t i;
	uint8_t *buf_ptr;
	uint32_t no_oui_extns;
	uint32_t total_no_oui_extns;
	uint32_t cmd_total_data_buf_len = 0;
	wmi_vendor_oui_action_id action_id;
	bool valid;
	uint32_t rem_var_buf_len;
	QDF_STATUS status;
	bool is_mac_exclusion_enabled;
	uint32_t ouis_sent = 0;
	uint32_t ouis_to_send = 0;

	if (!req) {
		wmi_err("action oui is empty");
		return QDF_STATUS_E_INVAL;
	}

	no_oui_extns = req->no_oui_extensions;
	total_no_oui_extns = req->total_no_oui_extensions;

	/* Check if MAC exclusion is supported by firmware */
	is_mac_exclusion_enabled =
		wmi_service_enabled(wmi_handle,
				    wmi_service_vendor_oui_support_exclusive_mac_address);

	if (!req->is_action_oui_dynamic &&
	    (no_oui_extns > WMI_MAX_VENDOR_OUI_ACTION_SUPPORTED_PER_ACTION ||
	    (total_no_oui_extns > WMI_VENDOR_OUI_ACTION_MAX_ACTION_ID *
	     WMI_MAX_VENDOR_OUI_ACTION_SUPPORTED_PER_ACTION))) {
		wmi_err("Invalid number of action oui extensions");
		return QDF_STATUS_E_INVAL;
	}

	valid = wmi_get_action_oui_id(req->action_id, &action_id);
	if (!valid) {
		wmi_err("Invalid action id");
		return QDF_STATUS_E_INVAL;
	}

	wmi_debug("wmi action_id %d total_num %d num %d v2 %d mac_exclusion %d",
		  action_id, total_no_oui_extns, no_oui_extns,
		  req->is_action_oui_v2_enabled,
		  is_mac_exclusion_enabled);

	/* Send OUIs in multiple WMI commands if they don't fit in one */
	extension = req->extension;
	do {
		/* Calculate how many OUIs can fit in this command */
		ouis_to_send = 0;
		cmd_total_data_buf_len = 0;
		len = sizeof(*cmd);

		/* Array of wmi_vendor_oui_ext structures */
		len += WMI_TLV_HDR_SIZE;

		/* Calculate how many complete OUIs can fit in this message */
		for (i = ouis_sent; i < no_oui_extns; i++) {
			uint32_t cur_oui_data_buf_len = 0;
			uint32_t test_len;

			/* Calculate variable buffer length for this OUI */
			cur_oui_data_buf_len = extension[i].oui_length +
					  extension[i].data_length +
					  extension[i].data_mask_length +
					  extension[i].mac_addr_length +
					  extension[i].mac_mask_length +
					  extension[i].capability_length;

			/* to store and OUI num */
			if (req->is_action_oui_v2_enabled &&
			    !extension[i].and_oui_index)
				cur_oui_data_buf_len += 1;

			if (is_mac_exclusion_enabled)
				cur_oui_data_buf_len +=
					extension[i].mac_exclusion_length +
					extension[i].mac_exclusion_mask_length;

			cur_oui_data_buf_len += 1; /* to store index */

			/* Test if adding this OUI would exceed message size */
			test_len = len + sizeof(*cmd_ext) +
				   WMI_TLV_HDR_SIZE +
				   ((cmd_total_data_buf_len +
				     cur_oui_data_buf_len + 3) & ~0x3);

			if (test_len > wmi_get_max_msg_len(wmi_handle)) {
				/* This OUI doesn't fit, stop here */
				if (ouis_to_send == 0) {
					wmi_err("Single OUI too large for WMI message");
					return QDF_STATUS_E_INVAL;
				}
				break;
			}

			/* This OUI fits, include it */
			len += sizeof(*cmd_ext);
			cmd_total_data_buf_len += cur_oui_data_buf_len;
			ouis_to_send++;
		}

		rem_var_buf_len = cmd_total_data_buf_len;
		cmd_total_data_buf_len = (cmd_total_data_buf_len + 3) & ~0x3;
		len += WMI_TLV_HDR_SIZE; /* Variable length buffer */
		len += cmd_total_data_buf_len;

		wmi_buf = wmi_buf_alloc(wmi_handle, len);
		if (!wmi_buf) {
			wmi_err("Failed to allocate wmi buffer");
			return QDF_STATUS_E_FAILURE;
		}

		buf_ptr = (uint8_t *)wmi_buf_data(wmi_buf);
		cmd = (wmi_pdev_config_vendor_oui_action_fixed_param *)buf_ptr;

		WMITLV_SET_HDR(&cmd->tlv_header,
			       WMITLV_TAG_STRUC_wmi_pdev_config_vendor_oui_action_fixed_param,
			       WMITLV_GET_STRUCT_TLVLEN(
					wmi_pdev_config_vendor_oui_action_fixed_param));

		cmd->action_id = action_id;

		/* For dynamic action oui ids, fill the extended fields */
		if (req->is_action_oui_dynamic) {
			cmd->total_num_vendor_oui = 0;
			cmd->num_vendor_oui_ext = 0;
			cmd->total_num_vendor_oui_ext =
					req->total_no_oui_extensions;
			cmd->num_vendor_oui_ext2 = ouis_to_send;
			/* Set more_data flag if there are more OUIs to send */
			cmd->more_data =
			     (ouis_sent + ouis_to_send < no_oui_extns) ? 1 : 0;
		} else {
			cmd->total_num_vendor_oui =
				 req->total_no_oui_extensions;
			cmd->num_vendor_oui_ext = ouis_to_send;
			cmd->total_num_vendor_oui_ext = 0;
			cmd->num_vendor_oui_ext2 = 0;
			cmd->more_data = 0;
		}

		buf_ptr += sizeof(*cmd);
		WMITLV_SET_HDR(buf_ptr, WMITLV_TAG_ARRAY_STRUC,
			       ouis_to_send * sizeof(*cmd_ext));
		buf_ptr += WMI_TLV_HDR_SIZE;
		cmd_ext = (wmi_vendor_oui_ext *)buf_ptr;
		wmi_fill_oui_extensions(&extension[ouis_sent], ouis_to_send,
					cmd_ext, req->is_action_oui_v2_enabled,
					is_mac_exclusion_enabled);

		buf_ptr += ouis_to_send * sizeof(*cmd_ext);
		WMITLV_SET_HDR(buf_ptr, WMITLV_TAG_ARRAY_BYTE,
			       cmd_total_data_buf_len);
		buf_ptr += WMI_TLV_HDR_SIZE;
		status = wmi_fill_oui_extensions_buffer(
						&extension[ouis_sent],
						cmd_ext, ouis_to_send,
						rem_var_buf_len, buf_ptr,
						req->is_action_oui_v2_enabled,
						is_mac_exclusion_enabled);

		if (!QDF_IS_STATUS_SUCCESS(status)) {
			wmi_err("failed to fill oui ext status %d", status);
			goto error;
		}

		buf_ptr += cmd_total_data_buf_len;

		wmi_debug("Sending WMI cmd: ouis_sent=%d ouis_to_send=%d more_data=%d len=%d",
			  ouis_sent, ouis_to_send, cmd->more_data, len);
		qdf_trace_hex_dump(QDF_MODULE_ID_WMI, QDF_TRACE_LEVEL_TRACE,
				   wmi_buf_data(wmi_buf), len);

		if (wmi_unified_cmd_send(
			   wmi_handle, wmi_buf, len,
			   WMI_PDEV_CONFIG_VENDOR_OUI_ACTION_CMDID)) {
			wmi_err("WMI_PDEV_CONFIG_VENDOR_OUI_ACTION send fail");
			goto error;
		}

		ouis_sent += ouis_to_send;
	} while (ouis_sent < no_oui_extns);

	wmi_debug("Successfully sent all %d OUIs", no_oui_extns);

	return QDF_STATUS_SUCCESS;

error:
	wmi_buf_free(wmi_buf);
	wmi_buf = NULL;
	return QDF_STATUS_E_FAILURE;
}
