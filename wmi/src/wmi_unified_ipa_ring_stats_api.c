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

/*
 * DOC: wmi_unified_ipa_ring_stats_api.c
 *
 * Implement unified WMI wrapper APIs for IPA ring statistics.
 */

#include "wmi_unified_priv.h"
#include "wmi_unified_ipa_ring_stats_api.h"

#ifdef WLAN_FEATURE_IPA_RING_STATS
QDF_STATUS
wmi_unified_ipa_ring_stats_req_cmd_send(wmi_unified_t wmi_handle,
				   enum hal_srng_ring_id srng_id)
{

	if (!wmi_handle || !wmi_handle->ops)
		return QDF_STATUS_E_FAILURE;

	if (wmi_handle->ops->send_ipa_ring_stats_req_cmd)
		return wmi_handle->ops->send_ipa_ring_stats_req_cmd(wmi_handle,
							       srng_id);

	return QDF_STATUS_E_FAILURE;
}

QDF_STATUS
wmi_extract_ipa_ring_stats_event(wmi_unified_t wmi_handle,
			    void *evt_buf,
			    uint32_t evt_len,
			    struct ipa_ring_stats_event_params *param)
{
	if (!wmi_handle || !wmi_handle->ops || !param)
		return QDF_STATUS_E_FAILURE;

	if (wmi_handle->ops->extract_ipa_ring_stats_event)
		return wmi_handle->ops->extract_ipa_ring_stats_event(wmi_handle,
								evt_buf,
								evt_len,
								param);

	return QDF_STATUS_E_FAILURE;
}

#else
QDF_STATUS
wmi_unified_ipa_ring_stats_req_cmd_send(wmi_unified_t wmi_handle,
				   enum hal_srng_ring_id srng_id)
{
	return QDF_STATUS_SUCCESS;
}

QDF_STATUS
wmi_extract_ipa_ring_stats_event(wmi_unified_t wmi_handle,
			    void *evt_buf,
			    uint32_t evt_len,
			    struct ipa_ring_stats_event_params *param)
{
	return QDF_STATUS_SUCCESS;
}

#endif
