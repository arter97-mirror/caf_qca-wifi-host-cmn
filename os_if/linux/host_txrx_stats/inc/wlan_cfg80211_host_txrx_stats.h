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
 * DOC: wlan_cfg80211_host_txrx_stats.h
 *
 * This header file provides declaration for cfg80211 command handler API
 * related to host driver side TX/RX stats and IPA RX internal exception drop count.
 */

#ifndef __WLAN_CFG80211_HOST_TXRX_STATS_H__
#define __WLAN_CFG80211_HOST_TXRX_STATS_H__

#include <wlan_objmgr_cmn.h>
#include <qdf_types.h>
#include <net/cfg80211.h>
#include <qca_vendor.h>

extern const struct nla_policy
	wlan_cfg80211_host_txrx_stats_policy
	[QCA_WLAN_VENDOR_ATTR_HOST_TXRX_STATS_PARAM_MAX + 1];

/**
 * wlan_cfg80211_host_txrx_stats - Get wlan host TX/RX and IPA RX drop statistics
 * @wiphy: pointer to wiphy
 * @wdev: pointer to wireless_dev
 * @pdev: pointer to pdev
 * @data: unused vendor command data from userspace
 * @data_len: length of @data
 *
 * This function:
 *  - Aggregates host driver side TX/RX stats into:
 *      tx_pkts,
 *      tx_dropped,
 *      rx_pkts,
 *      rx_dropped
 *  - Reads IPA RX internal drop count from the IPA pdev/psoc private object
 *  - Returns all 5 values via a cfg80211 vendor reply skb using:
 *      QCA_WLAN_VENDOR_ATTR_HOST_TXRX_STATS_PARAM_TX_PKTS
 *      QCA_WLAN_VENDOR_ATTR_HOST_TXRX_STATS_PARAM_TX_DROPPED
 *      QCA_WLAN_VENDOR_ATTR_HOST_TXRX_STATS_PARAM_RX_PKTS
 *      QCA_WLAN_VENDOR_ATTR_HOST_TXRX_STATS_PARAM_RX_DROPPED
 *      QCA_WLAN_VENDOR_ATTR_HOST_TXRX_STATS_PARAM_IPA_EXCEPTION_RX_DROPPED
 *
 * Note: Input attributes are ignored; @data/@data_len are only logged.
 *
 * Return: 0 on success; negative errno on failure.
 */

int wlan_cfg80211_host_txrx_stats(struct wiphy *wiphy,
				   struct wireless_dev *wdev,
				   struct wlan_objmgr_pdev *pdev,
				   const void *data, int data_len);


#endif /* __WLAN_CFG80211_HOST_TXRX_STATS_H__ */

