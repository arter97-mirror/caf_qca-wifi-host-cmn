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
 * DOC:wlan_ipa_ring_stats_tgt_api.c
 *
 * This file provides API definitions to send IPA ring statistics request from
 * interface to lower layers.
 */

#include <wlan_ipa_ring_stats_priv_api.h>
#include <wlan_ipa_ring_stats_tgt_api.h>
#include <wlan_cfg80211.h>
#include <qdf_module.h>
#include <wlan_ipa_ring_stats_api.h>

QDF_STATUS tgt_send_ipa_ring_stats_req(struct wlan_objmgr_psoc *psoc,
				  uint32_t ring_id)
{
	struct wlan_lmac_if_ipa_ring_stats_tx_ops *ipa_ring_stats_tx_ops;

	if (!psoc) {
		qdf_err("NULL psoc");
		return QDF_STATUS_E_NULL_VALUE;
	}

	ipa_ring_stats_tx_ops = wlan_psoc_get_ipa_ring_stats_txops(psoc);
	if (!ipa_ring_stats_tx_ops ||
	    !ipa_ring_stats_tx_ops->send_ipa_ring_stats_req) {
		qdf_err("IPA tx_ops not registered");
		return QDF_STATUS_E_NULL_VALUE;
	}

	return ipa_ring_stats_tx_ops->send_ipa_ring_stats_req(psoc, ring_id);
}

qdf_export_symbol(tgt_send_ipa_ring_stats_req);
