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
 * DOC: wlan_ipa_ring_stats_api.h
 *
 * This header file provides API declarations required for IPA ring stats
 * that are called by other components.
 */

#ifndef __WLAN_IPA_RING_STATS_API_H__
#define __WLAN_IPA_RING_STATS_API_H__

#include <qdf_types.h>

/**
 * struct ipa_ring_stat - IPA ring statistics
 * @ring_id: identifier for the specific IPA ring (e.g., TCL, REO, etc.)
 * @hp_idx: Head pointer index of the ring
 * @tp_idx: Tail pointer index of the ring
 * @free_entries: Number of free entries in the ring
 * @ring_entries: Total number of entries allocated for the ring
 */
struct ipa_ring_stat {
	uint32_t ring_id;
	uint32_t hp_idx;
	uint32_t tp_idx;
	uint32_t free_entries;
	uint32_t ring_entries;
};

/**
 * struct ipa_stats_event_params - IPA ring stats event params
 * @version: Version of the statistics event structure for compatibility
 * @num_rings: Number of valid ring statistics entries
 * @max_rings: Maximum capacity of the ipa ring
 * @ring_stats: Pointer to the array containing individual ring statistics
 */
struct ipa_ring_stats_event_params {
	uint32_t version;
	uint32_t num_rings;
	uint32_t max_rings;
	struct ipa_ring_stat *ring_stats;
};

/**
 * wlan_ipa_stats_init() - API to init IPA ring stats component
 */

#ifdef WLAN_FEATURE_IPA_RING_STATS

QDF_STATUS wlan_ipa_ring_stats_init(void);

/**
 * wlan_ipa_stats_deinit() - API to deinit IPA ring stats component
 */
QDF_STATUS wlan_ipa_ring_stats_deinit(void);

#else

static inline
QDF_STATUS wlan_ipa_ring_stats_init(void)
{
	return QDF_STATUS_SUCCESS;
}

static inline
QDF_STATUS wlan_ipa_ring_stats_deinit(void)
{
	return QDF_STATUS_SUCCESS;
}

#endif /* WLAN_FEATURE_IPA_RING_STATS */
#endif /* __WLAN_IPA_RING_STATS_API_H__ */
