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
 * DOC: wlan_ipa_ring_stats_ucfg_api.c
 *
 * This file contains IPA ring statistics north bound interface definitions
 */

#include <wlan_ipa_ucfg_api.h>
#include <wlan_ipa_ring_stats_tgt_api.h>
#include <wlan_ipa_ring_stats_ucfg_api.h>
#include <wlan_cfg80211.h>
#include "qdf_module.h"
#include <wlan_ipa_priv.h>
#include <wlan_osif_request_manager.h>
#include "wlan_ipa_ring_stats_priv_api.h"

#define IPA_RING_STATS_WAIT_TIMEOUT_MS   5000

QDF_STATUS
ucfg_get_ipa_ring_stats(struct wlan_objmgr_psoc *psoc,
		   uint32_t ring_id,
		   struct ipa_ring_stat *out_stat)
{
	QDF_STATUS status;
	struct osif_request *request;
	struct ipa_ring_stats_req_priv *priv;
	struct ipa_ring_stats_psoc_priv_obj *ipa_obj;
	int errno;
	static const struct osif_request_params params = {
		.priv_size  = sizeof(struct ipa_ring_stats_req_priv),
		.timeout_ms = IPA_RING_STATS_WAIT_TIMEOUT_MS,
		.dealloc    = NULL,
	};

	if (!psoc || !out_stat)
		return QDF_STATUS_E_INVAL;

	ipa_obj = ipa_ring_stats_get_psoc_priv_obj(psoc);
	if (!ipa_obj) {
		qdf_err("ipa_stats: psoc priv obj is NULL");
		return QDF_STATUS_E_INVAL;
	}

	request = osif_request_alloc(&params);
	if (!request) {
		qdf_err("ipa_stats: osif_request_alloc failed");
		return QDF_STATUS_E_NOMEM;
	}

	priv   = osif_request_priv(request);

	qdf_mem_zero(priv, sizeof(*priv));
	priv->evt.max_rings  = IPA_RING_STATS_MAX_RINGS;
	priv->evt.ring_stats = priv->ring_stats;

	qdf_spin_lock_bh(&ipa_obj->lock);
	if (ipa_obj->active_request) {
		qdf_spin_unlock_bh(&ipa_obj->lock);
		qdf_debug("ipa_stats: request already pending");
		osif_request_put(request);
		return QDF_STATUS_E_BUSY;
	}
	ipa_obj->active_request = request;
	qdf_spin_unlock_bh(&ipa_obj->lock);

	status = tgt_send_ipa_ring_stats_req(psoc, ring_id);
	if (QDF_IS_STATUS_ERROR(status)) {
		qdf_spin_lock_bh(&ipa_obj->lock);
		ipa_obj->active_request = NULL;
		qdf_spin_unlock_bh(&ipa_obj->lock);
		osif_request_put(request);
		return status;
	}

	errno = osif_request_wait_for_response(request);

	qdf_spin_lock_bh(&ipa_obj->lock);
	if (ipa_obj->active_request == request) {
	    ipa_obj->active_request = NULL;
	}
	qdf_spin_unlock_bh(&ipa_obj->lock);

	if (errno) {
		qdf_err("ipa_stats: wait failed errno=%d", errno);
		osif_request_put(request);
		return QDF_STATUS_E_TIMEOUT;
	}

	if (priv->evt.num_rings == 0 || !priv->evt.ring_stats) {
		qdf_err("ipa_stats: invalid evt: num_rings=%u",
			  priv->evt.num_rings);
		osif_request_put(request);
		return QDF_STATUS_E_FAILURE;
	}

	*out_stat = priv->evt.ring_stats[0];

	osif_request_put(request);
	return QDF_STATUS_SUCCESS;
}

qdf_export_symbol(ucfg_get_ipa_ring_stats);
