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
 * DOC: wlan_cfg80211_host_txrx_stats.c
 */

#include <qdf_list.h>
#include <qdf_status.h>
#include <linux/wireless.h>
#include <linux/netdevice.h>
#include <wlan_cfg80211.h>
#include <wlan_hdd_main.h>
#include <wlan_osif_priv.h>
#include <wlan_ipa_ucfg_api.h>
#include <wlan_cfg80211_host_txrx_stats.h>
#include "qdf_module.h"

/*
 * WLAN host txrx stats netlink attribute policy
 */
const struct nla_policy
wlan_cfg80211_host_txrx_stats_policy[QCA_WLAN_VENDOR_ATTR_HOST_TXRX_STATS_PARAM_MAX + 1] = {
};
qdf_export_symbol(wlan_cfg80211_host_txrx_stats_policy);

/**
 * wlan_cfg80211_host_txrx_stats - Get wlan host TX/RX and IPA RX drop statistics
 * @wiphy: pointer to wiphy
 * @wdev: pointer to wireless_dev
 * @pdev: pointer to pdev
 * @data: unused vendor command data from userspace
 * @data_len: length of @data
 *
 * Return: 0 on success; negative errno on failure.
 */
int
wlan_cfg80211_host_txrx_stats(struct wiphy *wiphy,
			       struct wireless_dev *wdev,
			       struct wlan_objmgr_pdev *pdev,
			       const void *data,
			       int data_len)
{
	struct net_device *dev;
	struct hdd_adapter *adapter;
	struct hdd_context *hdd_ctx;
	struct hdd_tx_rx_stats *stats;
	struct wlan_ipa_priv *ipa_priv;
	struct sk_buff *skb;
	uint32_t tx_pkts = 0;
	uint32_t tx_dropped = 0;
	uint32_t rx_pkts = 0;
	uint32_t rx_dropped = 0;
	uint64_t ipa_dropped = 0;
	int i;
	int ret;

	if (data && data_len)
		osif_debug("drop_cnt cmd ignores input attributes, data_len=%d",
			   data_len);

	hdd_ctx = wiphy_priv(wiphy);
	if (!hdd_ctx) {
		osif_err_rl("hdd_ctx is NULL");
		return -EINVAL;
	}

	if (!wdev) {
		osif_err_rl("wdev is NULL");
		return -EINVAL;
	}

	dev = wdev->netdev;
	if (!dev) {
		osif_err_rl("netdev is NULL");
		return -EINVAL;
	}

	adapter = WLAN_HDD_GET_PRIV_PTR(dev);
	if (!adapter) {
		osif_err_rl("adapter is NULL");
		return -EINVAL;
	}

	if (wlan_hdd_validate_vdev_id(adapter->vdev_id)) {
		osif_err_rl("Invalid vdev id %d", adapter->vdev_id);
		return -EINVAL;
	}

	stats = &adapter->hdd_stats.tx_rx_stats;

	for (i = 0; i < NUM_CPUS; i++) {
		tx_pkts     += stats->per_cpu[i].tx_called;
		tx_dropped  += stats->per_cpu[i].tx_dropped;
		rx_pkts     += stats->per_cpu[i].rx_packets;
		rx_dropped  += stats->per_cpu[i].rx_dropped;
	}

	ipa_priv = ipa_pdev_get_priv_obj(pdev);
	if (!ipa_priv) {
		osif_warn("IPA priv object is NULL, report IPA drop as 0");
	} else {
		ipa_dropped = ipa_priv->ipa_rx_internal_drop_count;
	}

	skb = wlan_cfg80211_vendor_cmd_alloc_reply_skb(
			wiphy,
			nla_total_size(sizeof(u32)) * 4 + nla_total_size(sizeof(u64)));
	if (!skb) {
		osif_err_rl("Failed to allocate skb for host TX/RX stats reply");
		return -ENOMEM;
	}

	ret = nla_put_u32(skb,
			  QCA_WLAN_VENDOR_ATTR_HOST_TXRX_STATS_PARAM_TX_PKTS,
			  tx_pkts);
	if (ret)
		goto fail;

	ret = nla_put_u32(skb,
			  QCA_WLAN_VENDOR_ATTR_HOST_TXRX_STATS_PARAM_TX_DROPPED,
			  tx_dropped);
	if (ret)
		goto fail;

	ret = nla_put_u32(skb,
	  		  QCA_WLAN_VENDOR_ATTR_HOST_TXRX_STATS_PARAM_RX_PKTS,
	  		  rx_pkts);
	if (ret)
		goto fail;

	ret = nla_put_u32(skb,
			  QCA_WLAN_VENDOR_ATTR_HOST_TXRX_STATS_PARAM_RX_DROPPED,
			  rx_dropped);
	if (ret)
		goto fail;

	ret = nla_put(skb,
		      QCA_WLAN_VENDOR_ATTR_HOST_TXRX_STATS_PARAM_IPA_EXCEPTION_RX_DROPPED,
		      sizeof(u64),
		      &ipa_dropped);

	if (ret)
		goto fail;

	return wlan_cfg80211_vendor_cmd_reply(skb);

fail:
	osif_err_rl("Failed to put drop/txrx attrs, ret=%d", ret);
	kfree_skb(skb);
	return ret;
}

qdf_export_symbol(wlan_cfg80211_host_txrx_stats);

