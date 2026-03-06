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
 * DOC: wlan_cfg80211_ipa_ring_stats.h
 *
 * This header file provides declaration for cfg80211 command handler API
 * related to IPA ring stats.
 */

#ifndef __WLAN_CFG80211_IPA_RING_STATS_H__
#define __WLAN_CFG80211_IPA_RING_STATS_H__

#include <wlan_objmgr_cmn.h>
#include <qdf_types.h>
#include <net/cfg80211.h>
#include <qca_vendor.h>

#ifdef WLAN_FEATURE_IPA_RING_STATS

extern const struct nla_policy
	wlan_cfg80211_ipa_ring_stats_policy
	[QCA_WLAN_VENDOR_ATTR_IPA_RING_STATS_PARAM_MAX + 1];

/**
 * wlan_cfg80211_start_ipa_ring_stats() - Handle IPA ring stats cfg80211 command
 * @wiphy: target wiphy
 * @psoc: pointer to psoc common object
 * @data: Pointer to the data passed via vendor interface
 * @data_len: Length of the data passed
 *
 * This API parses the IPA ring stats vendor command attributes from userspace,
 * prepares internal IPA ring stats request and forwards it to lower layers
 * (ucfg/tgt/fw) using appropriate WMI command.
 *
 * Return: 0 on success; negative errno on failure
 */
int wlan_cfg80211_start_ipa_ring_stats(struct wiphy *wiphy,
				  struct wlan_objmgr_psoc *psoc,
				  const void *data, int data_len);

#else
static inline
int wlan_cfg80211_start_ipa_ring_stats(struct wiphy *wiphy,
				  struct wlan_objmgr_psoc *psoc,
				  const void *data, int data_len)
{
	return 0;
}
#endif /* WLAN_FEATURE_IPA_RING_STATS */

#endif /* __WLAN_CFG80211_IPA_RING_STATS_H__ */
