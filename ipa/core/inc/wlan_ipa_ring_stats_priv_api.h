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
 * DOC: wlan_ipa_ring_stats_priv_api.h
 *
 * This header file provides API declarations required for IPA ring stats
 * that are used internally.
 */

#ifndef __WLAN_IPA_RING_STATS_PRIV_API_H__
#define __WLAN_IPA_RING_STATS_PRIV_API_H__

#include <wlan_objmgr_psoc_obj.h>
#include <wlan_lmac_if_def.h>
#include <qdf_lock.h>
#include <wlan_osif_request_manager.h>
#include <wlan_ipa_ring_stats_api.h>

#define IPA_RING_STATS_MAX_RINGS   4

#define ipa_stats_debug(args...) \
	QDF_TRACE_DEBUG(QDF_MODULE_ID_IPA_RING_STATS, ##args)
#define ipa_stats_err(args...) \
	QDF_TRACE_ERROR(QDF_MODULE_ID_IPA_RING_STATS, ##args)

/**
 * struct ipa_ring_stats_req_priv - private data for IPA ring stats request
 * @evt: event parameters used to carry IPA ring stats from FW/driver
 * @ring_stats: local buffer to store IPA ring stats for all rings in a request
 */
struct ipa_ring_stats_req_priv {
	struct ipa_ring_stats_event_params evt;
	struct ipa_ring_stat ring_stats[IPA_RING_STATS_MAX_RINGS];
};

/**
 * struct ipa_ring_stats_psoc_priv_obj - psoc private object for IPA ring stats
 * @lock: spin lock protecting IPA ring stats psoc private object
 * @psoc: pointer to psoc object
 * @active_request: pointer to the active request used to synchronize the
 *                  wait for firmware response
 */
struct ipa_ring_stats_psoc_priv_obj {
	qdf_spinlock_t lock;
	struct wlan_objmgr_psoc *psoc;
	struct osif_request *active_request;
};

/**
 * ipa_ring_stats_get_psoc_priv_obj() - get IPA ring stats private object
 * @psoc: pointer to psoc object
 *
 * Return: pointer to IPA ring stats psoc private object, or NULL on error
 */
static inline
struct ipa_ring_stats_psoc_priv_obj *
ipa_ring_stats_get_psoc_priv_obj(struct wlan_objmgr_psoc *psoc)
{
	struct ipa_ring_stats_psoc_priv_obj *obj;

	if (!psoc)
		return NULL;

	obj = wlan_objmgr_psoc_get_comp_private_obj(psoc,
						    WLAN_UMAC_COMP_IPA_RING_STATS);

	return obj;
}

/**
 * wlan_psoc_get_ipa_ring_stats_txops() - get IPA ring stats TX ops
 * @psoc: pointer to psoc object
 *
 * Return: pointer to IPA ring stats TX op callbacks, or NULL on error
 */
static inline struct wlan_lmac_if_ipa_ring_stats_tx_ops *
wlan_psoc_get_ipa_ring_stats_txops(struct wlan_objmgr_psoc *psoc)
{
	struct wlan_lmac_if_tx_ops *tx_ops;

	tx_ops = wlan_psoc_get_lmac_if_txops(psoc);
	if (!tx_ops) {
		ipa_stats_err("tx_ops is NULL");
		return NULL;
	}

	return &tx_ops->ipa_ring_stats_tx_ops;
}

#endif /* __WLAN_IPA_RING_STATS_PRIV_API_H__ */
