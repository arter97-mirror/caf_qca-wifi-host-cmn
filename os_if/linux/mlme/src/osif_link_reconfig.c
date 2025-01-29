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
 * osif_get_link_reconfig_rsp_ie_data() - Get link reconfig resp ie
 * @link_reconfig_res: link reconfig response element
 * @ie_data_len: ie len
 * @ie_data_ptr: ie data
 *
 * Return: NA
 */
static void osif_get_link_reconfig_rsp_ie_data(
				struct element_info *link_reconfig_res,
				size_t *ie_data_len,
				const uint8_t **ie_data_ptr)
{
	/* Validate IE and length */
	if (!link_reconfig_res->len || !link_reconfig_res->ptr)
		return;

	*ie_data_len = link_reconfig_res->len;
	*ie_data_ptr = link_reconfig_res->ptr;
}

/**
 * osif_get_link_reconfig_req_ie_data() - Get link reconfig req ie
 * @link_reconfig_req: link reconfig request element
 * @ie_data_len: ie len
 * @ie_data_ptr: ie data
 *
 * Return: NA
 */
static void osif_get_link_reconfig_req_ie_data(
				struct element_info *link_reconfig_req,
				size_t *ie_data_len,
				const uint8_t **ie_data_ptr)
{
	/* Validate IE and length */
	if (!link_reconfig_req->len || !link_reconfig_req->ptr)
		return;

	*ie_data_len = link_reconfig_req->len;
	*ie_data_ptr = link_reconfig_req->ptr;
}

/**
 * os_if_get_link_reconfig_status_from_link_id() - get status from link_id
 * @list: link reconfig status list
 * @link_id: link id for which status is required
 *
 * Return: wlan status code
 */
static enum wlan_status_code
os_if_get_link_reconfig_status_from_link_id(
		struct mlo_link_recfg_status_list *list, uint8_t link_id)
{
	uint8_t i;

	for (i = 0; i < WLAN_MAX_ML_RECFG_LINK_COUNT; i++) {
		if (list[i].link_id == link_id)
			return list[i].status_code;
	}
	return STATUS_UNSPECIFIED_FAILURE;
}

/**
 * osif_update_params_from_recfg_req_and_rsp() - Update response param from
 * req and response structure
 * @vdev: objmgr vdev
 * @rsp: link reconfig response
 * @cfg_rsp: cfg link reconfig response
 * @link_id: link id
 * @freq: freq
 * @op_code: op code
 *
 * This API updates params of link structure of cfg response at link_id
 * index. So that Kernel don't parse entire add/del link array. Kernel will get
 * bits from add/del valid_links and will parse only those indexes.
 *
 * Return : NA
 */
static void
osif_update_params_from_recfg_req_and_rsp(
				struct wlan_objmgr_vdev *vdev,
				struct wlan_mlo_link_recfg_rsp *rsp,
				struct cfg80211_links_reconfig_info *cfg_rsp,
				uint8_t link_id, qdf_freq_t *freq,
				enum links_reconfig_op *op_code)
{
	struct mlo_link_recfg_context *context;
	struct wlan_mlo_link_recfg_info add_link_info;
	struct wlan_mlo_link_recfg_info del_link_info;
	uint8_t i;
	uint8_t num_add_links;
	uint8_t num_del_links;

	context = vdev->mlo_dev_ctx->link_recfg_ctx;
	num_add_links = context->curr_recfg_req.add_link_info.num_links;
	num_del_links = context->curr_recfg_req.del_link_info.num_links;

	add_link_info = context->curr_recfg_req.add_link_info;
	for (i = 0; i < num_add_links; i++) {
		if (link_id == add_link_info.link[i].link_id) {
			cfg_rsp->added_links[link_id].bssid =
				add_link_info.link[i].ap_link_addr.bytes;
			cfg_rsp->added_links[link_id].addr =
				add_link_info.link[i].self_link_addr.bytes;
			cfg_rsp->added_links[link_id].status =
				os_if_get_link_reconfig_status_from_link_id(
					rsp->recfg_status_list, link_id);
			*op_code = LINKS_RECONFIG_OP_ADD;
			cfg_rsp->add_valid_links |=
				1 << add_link_info.link[i].link_id;
			*freq = add_link_info.link[i].freq;
			break;
		}
	}

	del_link_info = context->curr_recfg_req.del_link_info;
	for (i = 0; i < num_del_links; i++) {
		if (link_id == del_link_info.link[i].link_id) {
			cfg_rsp->deleted_links[link_id].bssid =
				del_link_info.link[i].ap_link_addr.bytes;
			cfg_rsp->deleted_links[link_id].status =
				os_if_get_link_reconfig_status_from_link_id(
					rsp->recfg_status_list, link_id);
			*op_code = LINKS_RECONFIG_OP_REMOVE;
			cfg_rsp->delete_valid_links |=
				1 << del_link_info.link[i].link_id;
			break;
		}
	}
}

/**
 * osif_fill_link_reconfig_rsp_mlo_params() - Update cfg response params
 * @vdev: vdev
 * @rsp: link reconfig response
 * @cfg_rsp: cfg link reconfig response
 * @link_id: link id
 * @index: index
 *
 *
 * Return : NA
 */
static void
osif_fill_link_reconfig_rsp_mlo_params(
				 struct wlan_objmgr_vdev *vdev,
				 struct wlan_mlo_link_recfg_rsp *rsp,
				 struct cfg80211_links_reconfig_info *cfg_rsp,
				 uint8_t link_id, uint8_t index)
{
	struct wiphy *wiphy;
	uint8_t ssid[WLAN_SSID_MAX_LEN] = {0};
	uint8_t ssid_len;
	qdf_freq_t freq;
	QDF_STATUS status;
	enum links_reconfig_op op_code = LINKS_RECONFIG_MAX;

	wiphy = osif_get_wiphy_from_vdev(vdev);
	if (!wiphy) {
		osif_err("Invalid wiphy");
		goto end;
	}

	osif_update_params_from_recfg_req_and_rsp(
					vdev, rsp, cfg_rsp, link_id,
					&freq, &op_code);

	if (op_code == LINKS_RECONFIG_OP_ADD) {
		cfg_rsp->added_links[link_id].channel =
					ieee80211_get_channel(wiphy, freq);
		if (!cfg_rsp->added_links[link_id].channel) {
			osif_err("failed to get ieee chan");
			goto end;
		}
		status = wlan_vdev_mlme_get_ssid(vdev, ssid, &ssid_len);
		if (QDF_IS_STATUS_ERROR(status)) {
			osif_err("failed to get ssid");
			goto end;
		}
		cfg_rsp->added_links[link_id].bss =
			wlan_cfg80211_get_bss(
					 wiphy,
					 cfg_rsp->added_links[link_id].channel,
					 cfg_rsp->added_links[link_id].bssid,
					 ssid, ssid_len);
		if (!cfg_rsp->added_links[link_id].bss) {
			osif_err("failed to get BSS");
			goto end;
		}
	}

end:
	osif_debug("Link_id: %d op_code %d status %d",
		   link_id, op_code, op_code == LINKS_RECONFIG_OP_ADD ?
		   cfg_rsp->added_links[link_id].status :
		   cfg_rsp->deleted_links[link_id].status);
}

QDF_STATUS
osif_link_reconfig_status_cb(struct wlan_objmgr_vdev *vdev)
{
	struct cfg80211_links_reconfig_info link_reconfig_rsp_params = {0};
	struct mlo_link_recfg_context *recfg_ctx;
	struct wlan_mlo_link_recfg_rsp *rsp;
	struct net_device *dev = NULL;
	uint8_t i;
	int errno;
	uint8_t valid_link_index = 0;

	/*
	 * TODO: Add CFG check, though recfg_ctx check is already present
	 *	which should be NULL in case CFG is disabled.
	 */
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
	rsp = &recfg_ctx->curr_recfg_rsp;
	qdf_mem_zero(&link_reconfig_rsp_params,
		     sizeof(link_reconfig_rsp_params));
	link_reconfig_rsp_params.status = recfg_ctx->link_recfg_status;

	osif_get_link_reconfig_req_ie_data(
					&recfg_ctx->req_frame,
					&link_reconfig_rsp_params.req_ie_len,
					&link_reconfig_rsp_params.req_ie);

	osif_get_link_reconfig_rsp_ie_data(
					&recfg_ctx->rsp_frame,
					&link_reconfig_rsp_params.resp_ie_len,
					&link_reconfig_rsp_params.resp_ie);
	for (i = 0; i < rsp->count; i++) {
		valid_link_index = rsp->recfg_status_list[i].link_id;
		osif_fill_link_reconfig_rsp_mlo_params(
			     vdev, rsp, &link_reconfig_rsp_params,
			     valid_link_index, i);
	}

	osif_debug("link reconfig resp status  %d",
		    link_reconfig_rsp_params.status);
	cfg80211_links_reconfigured(dev, &link_reconfig_rsp_params,
				    qdf_mem_malloc_flags());
	return QDF_STATUS_SUCCESS;
}
#endif

