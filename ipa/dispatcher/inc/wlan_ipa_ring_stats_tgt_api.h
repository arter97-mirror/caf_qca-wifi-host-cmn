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
 * DOC: wlan_ipa_ring_stats_tgt_api.h
 *
 * This header file provides API declarations to send IPA statistics
 * requests from the interface layer to lower layers.
 */

#ifndef __WLAN_IPA_STATS_TGT_API_H__
#define __WLAN_IPA_STATS_TGT_API_H__

#ifdef WLAN_FEATURE_IPA_RING_STATS

#include <qdf_status.h>

/**
 * tgt_ipa_send_stats_req() - API to send IPA statistics request
 * @psoc:    pointer to psoc object
 * @ring_id: IPA statistics ring identifier
 *
 * This API is used by upper layers to trigger an IPA statistics request
 * towards the target (lmac / firmware) through the registered tx ops.
 *
 * Return: QDF_STATUS of the operation
 */
QDF_STATUS tgt_send_ipa_ring_stats_req(struct wlan_objmgr_psoc *psoc,
                                  uint32_t ring_id);

#else /* !WLAN_FEATURE_IPA_RING_STATS */

/**
 * tgt_ipa_send_stats_req() - Stub implementation when IPA stats is disabled
 * @psoc:    pointer to psoc object (unused)
 * @ring_id: IPA statistics ring identifier (unused)
 *
 * Return: QDF_STATUS_SUCCESS
 */
static inline QDF_STATUS
tgt_ipa_send_stats_req(struct wlan_objmgr_psoc *psoc,
		       uint32_t ring_id)
{
	return QDF_STATUS_SUCCESS;
}

#endif /* WLAN_FEATURE_IPA_RING_STATS */
#endif /* __WLAN_IPA_STATS_TGT_API_H__ */
