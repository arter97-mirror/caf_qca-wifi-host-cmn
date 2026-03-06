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
 * DOC: wlan_cfg80211_ipa_ring_stats.c
 *
 * This file defines driver functions interfacing with linux kernel
 * for IPA stats cfg80211 vendor commands
 */

#include <qdf_list.h>
#include <qdf_status.h>
#include <linux/wireless.h>
#include <linux/netdevice.h>
#include <wlan_cfg80211.h>
#include <wlan_osif_priv.h>
#include <wlan_cfg80211_ipa_ring_stats.h>
#include <wlan_ipa_ring_stats_api.h>
#include <wlan_ipa_ring_stats_ucfg_api.h>
#include "qdf_module.h"

/*
 * IPA stats netlink attribute policy
 */
const struct nla_policy
wlan_cfg80211_ipa_ring_stats_policy[QCA_WLAN_VENDOR_ATTR_IPA_RING_STATS_PARAM_MAX + 1] = {
	[QCA_WLAN_VENDOR_ATTR_IPA_RING_STATS_PARAM_RING_ID] = {
					.type = NLA_U32,
					.len = sizeof(uint32_t) },
};

/**
 * wlan_cfg80211_start_ipa_ring_stats - Handle IPA ring stats request
 * @wiphy: pointer to wiphy
 * @psoc: the pointer of wlan_objmgr_psoc
 * @data: pointer to vendor command data
 * @data_len: data length
 *
 * Return: 0 on success; errno on failure
 */
int
wlan_cfg80211_start_ipa_ring_stats(struct wiphy *wiphy,
			      struct wlan_objmgr_psoc *psoc,
			      const void *data,
			      int data_len)
{
	struct nlattr *attr[QCA_WLAN_VENDOR_ATTR_IPA_RING_STATS_PARAM_MAX + 1];
	struct nlattr *id_attr;
	uint32_t id_val;
	QDF_STATUS status;
	struct ipa_ring_stat stat;
	struct sk_buff *skb;
	int ret;

	if (wlan_cfg80211_nla_parse(attr,
				    QCA_WLAN_VENDOR_ATTR_IPA_RING_STATS_PARAM_MAX,
				    data, data_len,
				    wlan_cfg80211_ipa_ring_stats_policy)) {
		osif_err_rl("Failed to parse IPA stats vendor attributes");
		return -EINVAL;
	}

	id_attr = attr[QCA_WLAN_VENDOR_ATTR_IPA_RING_STATS_PARAM_RING_ID];
	if (!id_attr) {
		osif_err_rl("IPA stats id is missing");
		return -EINVAL;
	}

	id_val = nla_get_u32(id_attr);

	if (id_val >= QCA_WLAN_VENDOR_IPA_RING_STATS_ID_MAX) {
		osif_err_rl("Invalid IPA stats ring id %u", id_val);
		return -EINVAL;
	}

	status = ucfg_get_ipa_ring_stats(psoc, id_val, &stat);
	if (QDF_IS_STATUS_ERROR(status)) {
		osif_err_rl("ucfg_ipa_get_stats failed, status:%d", status);
		return -EIO;
	}

	skb = wlan_cfg80211_vendor_cmd_alloc_reply_skb(
			wiphy,
			nla_total_size(sizeof(uint32_t)) * 5);
	if (!skb)
		return -ENOMEM;

	ret = nla_put_u32(skb,
			  QCA_WLAN_VENDOR_ATTR_IPA_RING_STATS_PARAM_RING_ID,
			  stat.ring_id);
	if (ret)
		goto nla_fail;

	ret = nla_put_u32(skb,
			  QCA_WLAN_VENDOR_ATTR_IPA_RING_STATS_PARAM_HP_IDX,
			  stat.hp_idx);
	if (ret)
		goto nla_fail;

	ret = nla_put_u32(skb,
			  QCA_WLAN_VENDOR_ATTR_IPA_RING_STATS_PARAM_TP_IDX,
			  stat.tp_idx);
	if (ret)
		goto nla_fail;

	ret = nla_put_u32(skb,
			  QCA_WLAN_VENDOR_ATTR_IPA_RING_STATS_PARAM_FREE_ENTRIES,
			  stat.free_entries);
	if (ret)
		goto nla_fail;

	ret = nla_put_u32(skb,
			  QCA_WLAN_VENDOR_ATTR_IPA_RING_STATS_PARAM_RING_ENTRIES,
			  stat.ring_entries);
	if (ret)
		goto nla_fail;

	return wlan_cfg80211_vendor_cmd_reply(skb);

nla_fail:
	kfree_skb(skb);
	return ret;
}
qdf_export_symbol(wlan_cfg80211_start_ipa_ring_stats);
