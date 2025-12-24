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
 * DOC: target_if_ipa_ring_stats.h
 *
 * Offload LMAC interface APIs for IPA stats.
 */

#ifndef __TARGET_IF_IPA_RING_STATS_H__
#define __TARGET_IF_IPA_RING_STATS_H__

#ifdef WLAN_FEATURE_IPA_RING_STATS

#include <qdf_status.h>

struct wlan_lmac_if_tx_ops;

/**
 * target_if_ipa_stats_register_tx_ops() - register IPA stats tx ops funcs
 * @tx_ops: pointer to LMAC tx ops
 *
 * This API is used by the LMAC interface layer to register IPA stats
 * transmit callbacks (send_ipa_stats_req) into the tx_ops table.
 *
 * Return: QDF_STATUS_SUCCESS on success, QDF_STATUS_E_* on error
 */
QDF_STATUS
target_if_ipa_ring_stats_register_tx_ops(struct wlan_lmac_if_tx_ops *tx_ops);

#else
static inline QDF_STATUS
target_if_ipa_ring_stats_register_tx_ops(struct wlan_lmac_if_tx_ops *tx_ops)
{
	return QDF_STATUS_SUCCESS;
}

#endif

#endif /* __TARGET_IF_IPA_RING_STATS_H__ */
