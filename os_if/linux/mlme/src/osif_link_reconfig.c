/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
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
 * DOC: osif_link_reconfig.c
 *
 * This file maintains definitaions of link reconfig request/response
 * common apis.
 */
#include <include/wlan_mlme_cmn.h>
#include "osif_cm_util.h"
#include "wlan_osif_priv.h"
#include "wlan_cfg80211.h"
#include "osif_cm_rsp.h"
#include "wlan_cfg80211_scan.h"
#include "wlan_mlo_mgr_sta.h"
#include "wlan_mlo_mgr_public_structs.h"
#include "wlan_mlo_link_recfg.h"
#include "qdf_status.h"
#include "osif_cm_util.h"
#include <wlan_cfg80211.h>

enum links_reconfig_op {
	LINKS_RECONFIG_OP_ADD,
	LINKS_RECONFIG_OP_REMOVE,
	LINKS_RECONFIG_MAX,
};

#if defined(WLAN_FEATURE_11BE_MLO) && \
defined(CFG80211_SETUP_LINK_RECONFIG_SUPPORT)
/**
 * osif_get_net_dev_from_vdev() - Get netdev object from vdev
 * @vdev: Pointer to vdev manager
 * @out_net_dev: Pointer to output netdev
 *
 * Return: 0 on success, error code on failure
 */
int osif_get_net_dev_from_vdev(struct wlan_objmgr_vdev *vdev,
			       struct net_device **out_net_dev)
{
	struct vdev_osif_priv *priv;

	if (!vdev)
		return -EINVAL;
	priv = wlan_vdev_get_ospriv(vdev);
	if (!priv || !priv->wdev || !priv->wdev->netdev)
		return -EINVAL;

	*out_net_dev = priv->wdev->netdev;
	return 0;
}

/**
 * osif_get_link_reconfig_rsp_frame() - Get link reconfig resp
 * @link_reconfig_res: link reconfig response
 * @frame_len: frame len
 * @frame_ptr: frame data
 *
 * Return: NA
 */
static void osif_get_link_reconfig_rsp_frame(
				struct element_info *link_reconfig_res,
				size_t *frame_len,
				const uint8_t **frame_ptr)
{
	/* Validate IE and length */
	if (!link_reconfig_res->len || !link_reconfig_res->ptr)
		return;

	*frame_len = link_reconfig_res->len;
	*frame_ptr = link_reconfig_res->ptr;
}

/**
 * osif_fill_link_reconfig_deleted_links_params() - Update cfg response params
 * @recfg_ctx: reconfig context
 * @delete_valid_links: delete links bitmap
 *
 * Return : NA
 */
static void
osif_fill_link_reconfig_deleted_links_params(
				 struct mlo_link_recfg_context *recfg_ctx,
				 uint16_t *delete_valid_links)
{
	uint8_t i = 0;
	uint8_t num_del_links;
	struct wlan_mlo_link_recfg_info del_link_info;

	if (!recfg_ctx) {
		osif_err("Recfg ctx is NULL");
		return;
	}
	num_del_links = recfg_ctx->curr_recfg_req.del_link_info.num_links;

	del_link_info = recfg_ctx->curr_recfg_req.del_link_info;
	for (i = 0; i < num_del_links && i < IEEE80211_MLD_MAX_NUM_LINKS; i++) {
		if (del_link_info.link[i].link_id != WLAN_INVALID_LINK_ID)
			*delete_valid_links |=
				1 << del_link_info.link[i].link_id;
	}

	osif_debug("links deleted bit map %d", *delete_valid_links);
}

/**
 * osif_fill_link_reconfig_added_links_params() - Update cfg response params
 * @vdev: vdev
 * @cfg_rsp: link reconfig response
 *
 * Return : NA
 */
static void
osif_fill_link_reconfig_added_links_params(
				 struct wlan_objmgr_vdev *vdev,
				 struct cfg80211_mlo_reconf_done_data *cfg_rsp)
{
	struct wiphy *wiphy;
	uint8_t ssid[WLAN_SSID_MAX_LEN] = {0};
	uint8_t ssid_len;
	struct mlo_link_recfg_context *recfg_context;
	struct ieee80211_channel *channel;
	struct wlan_mlo_link_recfg_info add_link_info;
	enum wlan_status_code status_code;
	uint8_t i;
	uint8_t num_add_links;
	uint8_t link_id;
	QDF_STATUS status;

	wiphy = osif_get_wiphy_from_vdev(vdev);
	if (!wiphy) {
		osif_err("Invalid wiphy");
		return;
	}

	recfg_context = vdev->mlo_dev_ctx->link_recfg_ctx;
	num_add_links = recfg_context->curr_recfg_req.add_link_info.num_links;
	if (!num_add_links) {
		osif_debug("no link added via link reconfig request");
		return;
	}

	if (recfg_context->link_recfg_status) {
		osif_debug("add link failure with status %d",
			   recfg_context->link_recfg_status);
		return;
	}

	add_link_info = recfg_context->curr_recfg_req.add_link_info;
	osif_get_link_reconfig_rsp_frame(
					&recfg_context->rsp_rx_frame,
					&cfg_rsp->len,
					&cfg_rsp->buf);

	cfg_rsp->driver_initiated = !recfg_context->curr_recfg_req.is_user_req;
	for (i = 0; i < num_add_links && i < IEEE80211_MLD_MAX_NUM_LINKS; i++) {
		if (add_link_info.link[i].link_id == WLAN_INVALID_LINK_ID) {
			osif_err("link id is invalid %d", WLAN_INVALID_LINK_ID);
			status_code = STATUS_INVALID_PARAMETERS;
			goto end;
		}
		link_id = add_link_info.link[i].link_id;
		status_code = add_link_info.link[i].status_code;
		channel = ieee80211_get_channel(wiphy,
						add_link_info.link[i].freq);
		if (!channel) {
			osif_err("failed to get ieee chan");
			status_code = STATUS_INVALID_PARAMETERS;
			goto end;
		}
		status = wlan_vdev_mlme_get_ssid(vdev, ssid, &ssid_len);
		if (QDF_IS_STATUS_ERROR(status)) {
			osif_err("failed to get ssid");
			status_code = STATUS_INVALID_PARAMETERS;
			goto end;
		}
		cfg_rsp->links[link_id].bss =
			wlan_cfg80211_get_bss(
				wiphy, channel,
				add_link_info.link[i].ap_link_addr.bytes,
				ssid, ssid_len);
		if (!cfg_rsp->links[link_id].bss) {
			osif_err("failed to get BSS");
			status_code = STATUS_INVALID_PARAMETERS;
			goto end;
		}

		cfg_rsp->links[link_id].addr =
				add_link_info.link[i].self_link_addr.bytes;
		if (!cfg_rsp->links[link_id].addr) {
			osif_err("failed to get STA link address");
			status_code = STATUS_INVALID_PARAMETERS;
			goto end;
		}

		cfg_rsp->added_links |=	1 << link_id;
end:
		osif_debug("add link_id %d with status %d freq %d",
			   link_id, status_code, add_link_info.link[i].freq);
	}
}

QDF_STATUS
osif_link_reconfig_status_cb(struct wlan_objmgr_vdev *vdev)
{
	struct wiphy *wiphy;
	struct cfg80211_mlo_reconf_done_data add_link_rsp = {0};
	uint16_t del_link_map = 0;
	struct mlo_link_recfg_context *recfg_ctx;
	struct net_device *dev = NULL;
	int errno;

	wiphy = osif_get_wiphy_from_vdev(vdev);
	if (!wiphy) {
		osif_err("Failed to get wiphy");
		return QDF_STATUS_E_FAILURE;
	}

	errno = osif_get_net_dev_from_vdev(vdev, &dev);
	if (errno) {
		osif_err("failed to get netdev");
		return QDF_STATUS_E_FAILURE;
	}
	osif_enter_dev(dev);
	if (!vdev->mlo_dev_ctx) {
		osif_err("mlo context not valid");
		return QDF_STATUS_E_FAILURE;
	}
	recfg_ctx = vdev->mlo_dev_ctx->link_recfg_ctx;
	if (!recfg_ctx) {
		osif_err("link reconfig context not valid");
		return QDF_STATUS_E_FAILURE;
	}
	qdf_mem_zero(&add_link_rsp, sizeof(add_link_rsp));

	osif_info("add num links %d, del num links %d",
		 recfg_ctx->curr_recfg_req.add_link_info.num_links,
		 recfg_ctx->curr_recfg_req.del_link_info.num_links);

	osif_wiphy_lock(wiphy, NULL);
	if (recfg_ctx->curr_recfg_req.add_link_info.num_links) {
		osif_fill_link_reconfig_added_links_params(vdev,
							   &add_link_rsp);
		cfg80211_mlo_reconf_add_done(dev, &add_link_rsp);
	}

	if (recfg_ctx->curr_recfg_req.del_link_info.num_links) {
		osif_fill_link_reconfig_deleted_links_params(recfg_ctx,
							     &del_link_map);
		cfg80211_links_removed(dev, del_link_map);
	}
	osif_wiphy_unlock(wiphy, NULL);

	return QDF_STATUS_SUCCESS;
}
#endif

