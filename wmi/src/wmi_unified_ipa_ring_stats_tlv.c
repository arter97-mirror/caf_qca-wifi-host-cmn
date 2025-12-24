/*
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
 * DOC: wmi_unified_ipa_ring_stats_tlv.c
 *
 * This file provides TLV-based WMI command and event handlers
 * for IPA ring stats, and registers them into the WMI ops table.
 */

#include <osdep.h>
#include <wmi.h>
#include <linux/kernel.h>
#include <wmi_unified.h>
#include <wmi_unified_priv.h>
#include <wmi_unified_ipa_ring_stats_api.h>

#ifdef WLAN_FEATURE_IPA_RING_STATS

/**
 * send_ipa_ring_stats_req_cmd_tlv() - send IPA ring stats request to fw
 * @wmi_handle: wmi handle
 * @srng_id: ring id for which stats are requested
 *
 * Send IPA ring stats request to firmware.
 *
 * Return: QDF_STATUS_SUCCESS on success and QDF_STATUS_E_FAILURE for failure
 */
static QDF_STATUS
send_ipa_ring_stats_req_cmd_tlv(wmi_unified_t wmi_handle,
			   enum hal_srng_ring_id srng_id)
{
	wmi_buf_t buf;
	wmi_ipa_ring_stats_req_cmd_fixed_param *cmd;
	uint32_t len;
	QDF_STATUS ret;

	len = sizeof(*cmd);

	buf = wmi_buf_alloc(wmi_handle, len);
	if (!buf)
		return QDF_STATUS_E_NOMEM;

	cmd = (wmi_ipa_ring_stats_req_cmd_fixed_param *)wmi_buf_data(buf);

	WMITLV_SET_HDR(&cmd->tlv_header,
			WMITLV_TAG_STRUC_wmi_ipa_ring_stats_req_cmd_fixed_param,
	 		WMITLV_GET_STRUCT_TLVLEN(
	 		wmi_ipa_ring_stats_req_cmd_fixed_param));

	cmd->version = WMI_IPA_RING_STATS_EVENT_VERSION1;
	cmd->ring_id = srng_id;

	wmi_mtrace(WMI_IPA_RING_STATS_REQ_CMDID, NO_SESSION, 0);
	ret = wmi_unified_cmd_send(wmi_handle, buf, len,
				   WMI_IPA_RING_STATS_REQ_CMDID);

	if (QDF_IS_STATUS_ERROR(ret)) {
		wmi_err("Sending IPA ring stats cmd failed, status=%d", ret);
		wmi_buf_free(buf);
	}

	return ret;
}

/**
 * wmi_extract_ipa_stats_event_tlv() - extract IPA ring stats event TLV
 * @wmi_handle: wmi handle
 * @evt_buf: pointer to raw WMI event buffer
 * @param: pointer to hold IPA ring stats event params
 *
 * Extract IPA stats information from firmware WMI event.
 *
 * Return: QDF_STATUS_SUCCESS on success and QDF_STATUS_E_FAILURE for failure
 */
static QDF_STATUS
wmi_extract_ipa_ring_stats_event_tlv(wmi_unified_t wmi_handle,
                                     void *evt_buf, u32 evt_len,
                                     struct ipa_ring_stats_event_params *param)
{
    WMI_IPA_RING_STATS_CONF_EVENTID_param_tlvs *tb;
    wmi_ipa_ring_stats_conf_event_fixed_param *fixed;
    wmi_ipa_ring_stat_entry *wmi_stats;
    u32 i, num_rings;

    if (!evt_buf || !param)
        return QDF_STATUS_E_INVAL;

    if (!param->ring_stats || !param->max_rings) {
        wmi_err("IPA ring stats: ring_stats buffer not initialized");
        return QDF_STATUS_E_INVAL;
    }

    tb = (WMI_IPA_RING_STATS_CONF_EVENTID_param_tlvs *)evt_buf;
    fixed = tb->fixed_param;
    wmi_stats = tb->ipa_ring_stat_entry;
    num_rings = tb->num_ipa_ring_stat_entry;

    if (!fixed || !wmi_stats) {
        wmi_err("IPA ring stats: invalid TLV pointers");
        return QDF_STATUS_E_INVAL;
    }

    param->version = fixed->version;

    if (num_rings > param->max_rings)
        num_rings = param->max_rings;
    param->num_rings = num_rings;

    for (i = 0; i < num_rings; i++) {
        param->ring_stats[i].ring_id      = wmi_stats[i].ring_id;
        param->ring_stats[i].hp_idx       = wmi_stats[i].hp_idx;
        param->ring_stats[i].tp_idx       = wmi_stats[i].tp_idx;
        param->ring_stats[i].free_entries = wmi_stats[i].free_entries;
        param->ring_stats[i].ring_entries = wmi_stats[i].ring_entries;

        qdf_print("IPA DEBUG: ring[%u]: id=%u hp=%u tp=%u free=%u entries=%u",
                  i,
                  param->ring_stats[i].ring_id,
                  param->ring_stats[i].hp_idx,
                  param->ring_stats[i].tp_idx,
                  param->ring_stats[i].free_entries,
                  param->ring_stats[i].ring_entries);
    }

    return QDF_STATUS_SUCCESS;
}

void wmi_ipa_ring_stats_attach_tlv(wmi_unified_t wmi_handle)
{
	struct wmi_ops *ops;

	if (!wmi_handle)
		return;

	ops = wmi_handle->ops;
	if (!ops)
		return;

	ops->send_ipa_ring_stats_req_cmd = send_ipa_ring_stats_req_cmd_tlv;
	ops->extract_ipa_ring_stats_event = wmi_extract_ipa_ring_stats_event_tlv;
}

#endif /* WLAN_FEATURE_IPA_RING_STATS */
