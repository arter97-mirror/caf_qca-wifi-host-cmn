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
 * DOC: wlan_ipa_ring_stats_ucfg_api.h
 *
 * This header file maintains API declaration required for northbound interaction
 * for IPA statistics feature.
 */

#ifndef __WLAN_IPA_RING_STATS_UCFG_API_H__
#define __WLAN_IPA_RING_STATS_UCFG_API_H__

#include <qdf_status.h>
#include <wlan_ipa_ring_stats_api.h>

#ifdef WLAN_FEATURE_IPA_RING_STATS

/**
 * ucfg_ipa_send_stats_req() - API to send IPA ring statistics request
 * @psoc: pointer to psoc object
 * @path: IPA path selector (REO/TCL)
 *
 * This API is used by northbound modules (such as HDD/cfg80211) to request
 * IPA ring statistics from the firmware. The function will forward the request
 * to the lower layers (tgt/target_if/WMI) for actual handling.
 *
 * Return: QDF_STATUS_SUCCESS on success, QDF_STATUS_E_** on error
 */
QDF_STATUS
ucfg_get_ipa_ring_stats(struct wlan_objmgr_psoc *psoc,
		   uint32_t ring_id,
		   struct ipa_ring_stat *out_stat);

#endif /* WLAN_FEATURE_IPA_RING_STATS */

#endif /* __WLAN_IPA_RING_STATS_UCFG_API_H__ */
