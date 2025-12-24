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
 * DOC: wmi_unified_ipa_ring_stats_api.h
 *
 * This header file provides declaration for unified WMI wrapper APIs
 * for IPA ring statistics.
 */

#ifndef _WMI_UNIFIED_IPA_STATS_API_H_
#define _WMI_UNIFIED_IPA_STATS_API_H_

#include <wmi_unified_param.h>
#include <hal_api.h>
#include <wlan_ipa_ring_stats_api.h>

/**
 * wmi_unified_ipa_ring_stats_req_cmd_send() - WMI ring IPA stats request function
 * @wmi_handle: handle to WMI.
 * @param: pointer to hold IPA ring stats request parameters
 *
 * Send WMI IPA stats request to firmware. The request typically carries
 * information about the IPA/statistics path, such as ring or queue
 * identifiers, and an optional stats identifier.
 *
 * Return: QDF_STATUS_SUCCESS on success and QDF_STATUS_E_FAILURE on failure.
 */

QDF_STATUS
wmi_unified_ipa_ring_stats_req_cmd_send(wmi_unified_t wmi_handle,
				   enum hal_srng_ring_id srng_id);


/**
 * wmi_extract_ipa_ring_stats_event() - WMI IPA ring stats event extract function
 * @wmi_handle: handle to WMI.
 * @evt_buf: pointer to raw WMI event buffer.
 * @param: pointer to hold extracted IPA ring stats event parameters
 *
 * Extract IPA stats information from WMI event sent by firmware. The event
 * typically includes head pointer (HP), tail pointer (TP) and pending data,
 * and may also include the corresponding stats identifier.
 *
 * Return: QDF_STATUS_SUCCESS on success and QDF_STATUS_E_FAILURE on failure.
 */

QDF_STATUS
wmi_extract_ipa_ring_stats_event(wmi_unified_t wmi_handle,
			    void *evt_buf,
			    uint32_t evt_len,
			    struct ipa_ring_stats_event_params *param);

#endif /* _WMI_UNIFIED_IPA_STATS_API_H_ */
