/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: ISC
 */

#include "dp_dal.h"
#include "dp_dal_rx.h"
#include "dp_dal_tx.h"
#include <wlan_cfg.h>
#include <qdf_types.h>
#include "qdf_mem.h"
#include "dp_rx.h"
#include "dp_peer.h"

/* DAL poll timer interval in milliseconds */
#define DAL_POLL_TIMER_INTERVAL_MS 10
#define DAL_POLL_TIMER_MAX_COUNT 10

/* DAL rx replenish retry timer interval in milliseconds */
#define DAL_RX_REPLENISH_RETRY_TIMER_MS 10
#define DAL_RX_REPLENISH_MAX_RETRIES 10
#define DAL_RX_REPLENISH_BACKOFF_MULTIPLIER 2

/**
 * dp_dal_bus_init_bypass_mode() - Skeleton for platform bus init
 * in bypass mode
 *
 * @priv: private data
 *
 * Return: 0 on success
 */
static int dp_dal_bus_init_bypass_mode(void *priv)
{
	return 0;
}

/**
 * dp_dal_bus_exit_bypass_mode() - Skeleton for platform bus exit in bypass mode
 *
 * @priv: private data
 */
static void dp_dal_bus_exit_bypass_mode(void *priv)
{
}

/**
 * dp_dal_bus_start_bypass_mode() - Skeleton for platform bus start
 * in bypass mode
 *
 * @priv: private data
 *
 * Return: 0 on success
 */
static int dp_dal_bus_start_bypass_mode(void *priv)
{
	return 0;
}

/**
 * dp_dal_bus_stop_bypass_mode() - Skeleton for platform bus stop in bypass mode
 *
 * @priv: private data
 */
static void dp_dal_bus_stop_bypass_mode(void *priv)
{
}

/**
 * dp_dal_request_irq_bypass_mode() - Skeleton for platform bus request irq in
 * bypass mode
 *
 * @priv: private data
 *
 * Return: 0 on success
 */
static int dp_dal_request_irq_bypass_mode(void *priv)
{
	return 0;
}

/**
 * dp_dal_sta_active_bypass_mode() - Skeleton for platform bus sta active
 * in bypass mode
 *
 * @priv: private data
 * @info: sta info
 * @enable: enable
 *
 * Return: 0 on success
 */
static int dp_dal_sta_active_bypass_mode(void *priv,
					 struct sta_info *info, bool enable)
{
	return 0;
}

/**
 * dp_dal_notify_suspend_bypass_mode() - Skeleton for platform bus notify
 * suspend in bypass mode
 *
 * @priv: private data
 *
 * Return: 0 on success
 */
static int dp_dal_notify_suspend_bypass_mode(void *priv)
{
	return 0;
}

/**
 * dp_dal_notify_resume_bypass_mode() - Skeleton for platform bus notify resume
 * in bypass mode
 *
 * @priv: private data
 *
 * Return: 0 on success
 */
static int dp_dal_notify_resume_bypass_mode(void *priv)
{
	return 0;
}

/**
 * dp_dal_ssr_dump_bypass_mode() - Skeleton for platform bus ssr dump
 * in bypass mode
 *
 * @segment: segment
 */
static void dp_dal_ssr_dump_bypass_mode(void *segment)
{
}

/**
 * dp_dal_intf_init_bypass_mode() - interface initialization in bypass mode
 * @priv: pointer to dal context
 * @intf_info: interface info
 *
 * Return: 0 on success
 */
static int dp_dal_intf_init_bypass_mode(void *priv, void *intf_info)
{
	return 0;
}

/**
 * dp_dal_intf_deinit_bypass_mode() - interface deinitialization in bypass mode
 * @priv: pointer to dal context
 * @vdev_id: vdev id corresponds to interface
 *
 * Return: 0 on success
 */
static int dp_dal_intf_deinit_bypass_mode(void *priv, uint16_t vdev_id)
{
	return 0;
}

/**
 * dp_dal_pdev_set_default_routing_helper() - Helper to set default routing
 * @soc: pointer to dp_soc structure
 * @peer: pointer to dp_peer structure
 * @arg: pointer to dp_pdev structure
 *
 * Return: None
 */
static void dp_dal_pdev_set_default_routing_helper(struct dp_soc *soc,
						   struct dp_peer *peer,
						   void *arg)
{
	struct dp_pdev *pdev = (struct dp_pdev *)arg;
	struct dp_vdev *vdev = peer->vdev;
	uint32_t reo_dest;
	uint8_t lmac_peer_id_msb = 0;
	bool hash_based;

	if (soc->dp_dal_mode == DAL_DP_OFFLOAD_MODE) {
		hash_based = false;
		reo_dest = (peer->vdev->qdf_opmode == QDF_STA_MODE) ?
			DAL_DP_DEFAULT_REO_STA : DAL_DP_DEFAULT_REO_SAP;
	} else {
		dp_vdev_get_default_reo_hash(vdev, &reo_dest, &hash_based);
	}

	if (soc->cdp_soc.ol_ops->peer_set_default_routing)
		soc->cdp_soc.ol_ops->peer_set_default_routing(soc->ctrl_psoc,
							      pdev->pdev_id,
							      peer->mac_addr.raw,
							      peer->vdev->vdev_id,
							      hash_based,
							      reo_dest,
							      lmac_peer_id_msb);
}

/**
 * dp_dal_pdev_set_default_routing - Iterate over pdev->vdev->peer list
 * and set default routing for each peer.
 * @pdev: pointer to dp_pdev structure
 *
 * This function is called during mode switch from bypass to offload
 * and vice versa.
 *
 * Return: None
 */
static void dp_dal_pdev_set_default_routing(struct dp_pdev *pdev)
{
	struct dp_soc *soc;
	struct dp_vdev *vdev;

	if (!pdev) {
		dp_err("Invalid pdev");
		return;
	}

	soc = pdev->soc;
	if (!soc) {
		dp_err("Invalid soc");
		return;
	}

	qdf_spin_lock_bh(&pdev->vdev_list_lock);
	DP_PDEV_ITERATE_VDEV_LIST(pdev, vdev) {
		if (dp_vdev_get_ref(soc, vdev, DP_MOD_ID_CDP))
			continue;

		if (vdev->qdf_opmode == QDF_STA_MODE ||
		    vdev->qdf_opmode == QDF_SAP_MODE)
			dp_vdev_iterate_peer(vdev,
					     dp_dal_pdev_set_default_routing_helper,
					     pdev, DP_MOD_ID_CDP);

		dp_vdev_unref_delete(soc, vdev, DP_MOD_ID_CDP);
	}
	qdf_spin_unlock_bh(&pdev->vdev_list_lock);
}

/**
 * dp_dal_vdev_pause_unpause_queues() - Pause/unpause vdevs queues
 * @pdev: pointer to dp_pdev structure
 * @pause: true for pause, false for unpause
 *
 * This function is called to pause or unpause STA/SAP vdevs.
 *
 * Return: None
 */
static void dp_dal_vdev_pause_unpause_queues(struct dp_pdev *pdev, bool pause)
{
	struct dp_soc *soc;
	struct dp_vdev *vdev;

	if (!pdev) {
		dp_err("Invalid pdev");
		return;
	}

	soc = pdev->soc;
	if (!soc) {
		dp_err("Invalid soc");
		return;
	}

	if (!soc->pause_cb) {
		dp_err("pause_cb is not registered");
		return;
	}

	qdf_spin_lock_bh(&pdev->vdev_list_lock);
	DP_PDEV_ITERATE_VDEV_LIST(pdev, vdev) {
		if (dp_vdev_get_ref(soc, vdev, DP_MOD_ID_CDP))
			continue;

		if (vdev->qdf_opmode == QDF_STA_MODE ||
		    vdev->qdf_opmode == QDF_SAP_MODE) {
			if (pause) {
				soc->pause_cb(vdev->vdev_id,
					      WLAN_STOP_ALL_NETIF_QUEUE,
					      WLAN_DAL_DP_MODE_SWITCH);
			} else {
				soc->pause_cb(vdev->vdev_id,
					      WLAN_START_ALL_NETIF_QUEUE,
					      WLAN_DAL_DP_MODE_SWITCH);
			}
		}

		dp_vdev_unref_delete(soc, vdev, DP_MOD_ID_CDP);
	}
	qdf_spin_unlock_bh(&pdev->vdev_list_lock);
}

static void dp_dal_save_ring_hp_tp(struct dp_dal_ctx *dal_ctx,
				   struct dp_soc *soc)
{
	struct dal_srng *dal_srng;
	struct hal_srng *hal_srng;
	uint8_t ring_num;
	int i;

	/* REO DST ring */
	for (i = 0; i < DAL_RX_RINGS_MAX; i++) {
		dal_srng = &dal_ctx->rx_ring[i];
		ring_num = dal_srng->ring_num;

		if (!dal_srng->initialized ||
		    ring_num >= soc->num_reo_dest_rings)
			continue;

		hal_srng =
		(struct hal_srng *)soc->reo_dest_ring[ring_num].hal_srng;
		if (!hal_srng) {
			dp_err("hal_srng is NULL for REO DST ring:%d",
			       ring_num);
			continue;
		}

		dal_srng->u.dst_ring.tp = hal_srng->u.dst_ring.tp;
		dp_info("updated REO DST ring:%d TP:%u to dal srng",
			ring_num, dal_srng->u.dst_ring.tp);
	}

	/* TX completion ring */
	for (i = 0; i < DAL_TX_RINGS_MAX; i++) {
		dal_srng = &dal_ctx->tx_cmpl_ring[i];
		ring_num = dal_srng->ring_num;

		if (!dal_srng->initialized ||
		    ring_num >= soc->num_tx_comp_rings)
			continue;

		hal_srng =
		(struct hal_srng *)soc->tx_comp_ring[ring_num].hal_srng;
		if (!hal_srng) {
			dp_err("hal_srng is NULL for TX compl ring:%d",
			       ring_num);
			continue;
		}

		dal_srng->u.dst_ring.tp = hal_srng->u.dst_ring.tp;
		dp_info("updated TX comp ring:%d TP:%u to dal srng",
			ring_num, dal_srng->u.dst_ring.tp);
	}
}

/**
 * dp_dal_mode_switch_bypass_to_offload - Handles mode switch indication from
 * bypass to offload
 * @dal_ctx: DAL context
 *
 * This function performs the following actions:
 * 1. Sets a flag indicating that a mode switch is in progress, which prevents
 *	suspension during this transition
 * 2. Updates the DAL mode to ensure that any new peer connection will be set
 *	up with hash-based routing
 * 3. Update the latest TP for DAL owned rings in dal srng.
 * 4. Completes the init sequence with DAL
 * 5. Iterates over the vdev list and send interface information of STA and SAP
 * 6. Iterates over connected peers in STA/SAP modes to enable peer-based
 *	routing for them
 * 7. Resets the mode switch in progress flag to false once the operation
 *	is finished
 *
 * Return: QDF_STATUS
 */
static QDF_STATUS
dp_dal_mode_switch_bypass_to_offload(struct dp_dal_ctx *dal_ctx)
{
	struct dp_soc *soc;
	struct dp_pdev *pdev;
	struct dp_vdev *vdev;
	QDF_STATUS status;

	if (!dal_ctx) {
		dp_err("DAL context is NULL, reject mode switch");
		return QDF_STATUS_E_INVAL;
	}

	soc = dal_ctx->soc;
	if (!soc) {
		dp_err("SOC context is NULL, reject mode switch");
		return QDF_STATUS_E_INVAL;
	}

	pdev = soc->pdev_list[0];
	if (!pdev) {
		dp_err("PDEV is NULL, reject mode switch");
		return QDF_STATUS_E_INVAL;
	}

	qdf_timer_sync_cancel(&dal_ctx->dal_poll_timer);
	dp_dal_vdev_pause_unpause_queues(pdev, true);

	soc->dal_mode_switch_in_progress = true;
	soc->dp_dal_mode = DAL_DP_OFFLOAD_MODE;

	/*
	 * This is necessary when a previous mode switch occurred from offload
	 * to bypass, and the host driver may have polled the rings managed by
	 * OE. Therefore, the latest TP value must be provided to DAL during
	 * the switch back from bypass to offload mode.
	 */
	dp_dal_save_ring_hp_tp(dal_ctx, soc);

	status = dp_dal_bus_init(soc);
	if (status != QDF_STATUS_SUCCESS) {
		dp_err("DAL platform bus init failed during mode switch %d",
		       status);
		goto abort_mode_switch;
	}

	status = dp_dal_bus_request_irq(soc);
	if (status != QDF_STATUS_SUCCESS) {
		dp_err("DAL ptfm bus request IRQ failed during mode switch %d",
		       status);
		goto bus_exit;
	}

	status = dp_dal_bus_start(soc);
	if (status != QDF_STATUS_SUCCESS) {
		dp_err("DAL platform bus start failed during mode switch %d",
		       status);
		goto bus_exit;
	}

	qdf_spin_lock_bh(&pdev->vdev_list_lock);
	DP_PDEV_ITERATE_VDEV_LIST(pdev, vdev) {
		if (dp_vdev_get_ref(soc, vdev, DP_MOD_ID_CDP))
			continue;

		if (vdev->qdf_opmode == QDF_STA_MODE ||
		    vdev->qdf_opmode == QDF_SAP_MODE) {
			status = dp_dal_interface_add(soc, vdev);
			if (status) {
				dp_err("Failed to add interface for vdev_id:%d",
				       vdev->vdev_id);
				dp_vdev_unref_delete(soc, vdev, DP_MOD_ID_CDP);
				qdf_spin_unlock_bh(&pdev->vdev_list_lock);
				goto bus_exit;
			}
		}

		dp_vdev_unref_delete(soc, vdev, DP_MOD_ID_CDP);
	}
	qdf_spin_unlock_bh(&pdev->vdev_list_lock);

	dp_dal_pdev_set_default_routing(pdev);

	soc->dal_mode_switch_in_progress = false;
	dp_info("Mode switch from bypass to offload completed successfully");

	/*
	 * Ensure that any ongoing replenish operations are completed before
	 * returning from mode switch indication to DAL. Also make sure no
	 * further replenish from bypass path after this.
	 */
	qdf_spin_lock_bh(&dal_ctx->dal_replenish_lock);
	qdf_atomic_set(&dal_ctx->bm_replenish_not_allowed, 1);
	qdf_spin_unlock_bh(&dal_ctx->dal_replenish_lock);

	dp_dal_vdev_pause_unpause_queues(pdev, false);

	if (qdf_atomic_read(&dal_ctx->rx_replenish_failures))
		qdf_timer_mod(&dal_ctx->rx_replenish_retry_timer,
			      dal_ctx->rx_replenish_retry_interval_ms);

	return QDF_STATUS_SUCCESS;

bus_exit:
	dp_dal_bus_exit(soc);
abort_mode_switch:
	soc->dp_dal_mode = DAL_DP_BYPASS_MODE;
	soc->dal_mode_switch_in_progress = false;

	qdf_spin_lock_bh(&dal_ctx->dal_replenish_lock);
	qdf_atomic_set(&dal_ctx->bm_replenish_not_allowed, 0);
	qdf_spin_unlock_bh(&dal_ctx->dal_replenish_lock);

	/* set hash-based routing since the offload mode switch failed */
	dp_dal_pdev_set_default_routing(pdev);
	qdf_timer_mod(&dal_ctx->dal_poll_timer, DAL_POLL_TIMER_INTERVAL_MS);
	dp_err("DAL mode switch from bypass to offload aborted due to failure");
	dp_dal_vdev_pause_unpause_queues(pdev, false);
	return QDF_STATUS_E_FAILURE;
}

/**
 * dp_dal_mode_switch_offload_to_bypass - Handle mode switch from offload mode
 * to bypass mode
 * @dal_ctx: DAL context
 *
 * This function performs the following actions:
 * 1. Sets a flag indicating that a mode switch is in progress, which prevents
 *	suspension during this transition
 * 2. Updates the DAL mode to ensure that any new peer connection will be set
 *	up with peer-based routing
 * 3. Iterates over connected peers in STA/SAP modes to enable hash-based
 *	routing for them
 * 4. Retrieves the current HP/TP snapshot from DAL, which will be used to poll
 *	DAL rings after the mode switch completes
 * 5. Resets the mode switch in progress flag to false once the operation is
 *	finished
 *
 * Return: QDF_STATUS
 */
static QDF_STATUS
dp_dal_mode_switch_offload_to_bypass(struct dp_dal_ctx *dal_ctx)
{
	struct dp_soc *soc;
	struct dp_pdev *pdev;

	if (!dal_ctx) {
		dp_err("DAL context is NULL, reject mode switch");
		return QDF_STATUS_E_INVAL;
	}

	soc = dal_ctx->soc;
	if (!soc) {
		dp_err("SOC is NULL, reject mode switch");
		return QDF_STATUS_E_INVAL;
	}

	pdev = soc->pdev_list[0];
	if (!pdev) {
		dp_err("PDEV is NULL reject mode switch");
		return QDF_STATUS_E_INVAL;
	}

	qdf_spin_lock_bh(&dal_ctx->dal_replenish_lock);
	qdf_atomic_set(&dal_ctx->bm_replenish_not_allowed, 0);
	qdf_spin_unlock_bh(&dal_ctx->dal_replenish_lock);

	soc->dal_mode_switch_in_progress = true;
	soc->dp_dal_mode = DAL_DP_BYPASS_MODE;

	dp_dal_pdev_set_default_routing(pdev);

	/* Start polling timer */
	dal_ctx->poll_count = 0;
	qdf_timer_mod(&dal_ctx->dal_poll_timer, DAL_POLL_TIMER_INTERVAL_MS);

	soc->dal_mode_switch_in_progress = false;
	return QDF_STATUS_SUCCESS;
}

/**
 * dp_dal_mode_switch_ind_handler - handler for mode switch indication
 * @priv: pointer to dal context
 * @cur_mode: current operating mode
 * @new_mode: new operating mode
 *
 * Return: 0 on success, non-zero on failure.
 */
static int dp_dal_mode_switch_ind_handler(void *priv, u8 cur_mode, u8 new_mode)
{
	struct dp_dal_ctx *dal_ctx = (struct dp_dal_ctx *)priv;

	if (cur_mode == DAL_DP_BYPASS_MODE &&
	    new_mode == DAL_DP_OFFLOAD_MODE) {
		if (dp_dal_mode_switch_bypass_to_offload(dal_ctx) !=
		    QDF_STATUS_SUCCESS)
			return -EINVAL;
	} else if (cur_mode == DAL_DP_OFFLOAD_MODE &&
		   new_mode == DAL_DP_BYPASS_MODE) {
		if (dp_dal_mode_switch_offload_to_bypass(dal_ctx) !=
		    QDF_STATUS_SUCCESS)
			return -EINVAL;
	} else {
		dp_err("invalid mode switch ind rcvd cur_mode:%d new_mode:%d",
		       cur_mode, new_mode);
		return -EINVAL;
	}

	return 0;
}

struct platform_bus_ops plat_ops_bypass_mode = {
	.init = dp_dal_bus_init_bypass_mode,
	.exit = dp_dal_bus_exit_bypass_mode,
	.start = dp_dal_bus_start_bypass_mode,
	.stop = dp_dal_bus_stop_bypass_mode,
	.request_irq = dp_dal_request_irq_bypass_mode,
	.rx = dp_dal_rx_bypass_mode,
	.rx_replenish = dp_dal_rx_replenish_bypass_mode,
	.rxbm_sync = dp_dal_rx_rxbm_sync_bypass_mode,
	.tx = dp_dal_tx_bypass_mode,
	.tx_cpl = dp_dal_tx_cpl_bypass_mode,
	.tx_queue_active = dp_dal_tx_queue_active_bypass_mode,
	.sta_active = dp_dal_sta_active_bypass_mode,
	.notify_suspend = dp_dal_notify_suspend_bypass_mode,
	.notify_resume = dp_dal_notify_resume_bypass_mode,
	.ssr_dump = dp_dal_ssr_dump_bypass_mode,
	.intf_init = dp_dal_intf_init_bypass_mode,
	.intf_deinit = dp_dal_intf_deinit_bypass_mode,
	.rx_pkt_reinject = dp_dal_rx_pkt_reinject_bypass_mode,
};

struct platform_bus_ops *global_plat_ops = &plat_ops_bypass_mode;

static inline void
dp_dal_fill_srng_params(struct hal_srng *srng,
			struct hal_srng_params *params,
			uint64_t msi_addr, uint32_t msi_data)
{
	params->msi_addr = msi_addr;
	params->msi_data = msi_data;
	params->intr_timer_thres_us = srng->intr_timer_thres_us;
	params->intr_batch_cntr_thres_entries =
					srng->intr_batch_cntr_thres_entries;
}

static int
dp_dal_set_msi_config(void *priv, uint8_t ring_num, uint8_t ring_type,
		      uint64_t msi_addr, uint32_t msi_data)
{
	struct dp_dal_ctx *dal_ctx = (struct dp_dal_ctx *)priv;
	struct dp_soc *dp_soc = dal_ctx->soc;
	struct hal_srng *srng;
	struct hal_srng_params params = {0};

	/*
	 * This API is invoked only when the mode is set to offload.
	 * Store the mode that will be applied when sending the default
	 * routing configuration to the peer.
	 */
	if (dp_soc->dp_dal_mode != DAL_DP_OFFLOAD_MODE)
		dp_soc->dp_dal_mode = DAL_DP_OFFLOAD_MODE;

	dp_info("DAL: ring_type:%d num:%d msi_addr:%llu msi_data:%u",
		ring_type, ring_num, msi_addr, msi_data);

	if (ring_type == REO_DST) {
		if (ring_num >= dp_soc->num_reo_dest_rings) {
			dp_err("Invalid REO ring_num:%d received", ring_num);
			return QDF_STATUS_E_INVAL;
		}

		srng =
		(struct hal_srng *)dp_soc->reo_dest_ring[ring_num].hal_srng;

		dp_dal_fill_srng_params(srng, &params, msi_addr, msi_data);
		return hal_srng_set_msi_irq_config(dp_soc->hal_soc,
						   (hal_ring_handle_t)srng,
						   &params);
	} else if (ring_type == COMP_RING_TYPE) {
		if (ring_num >= dp_soc->num_tx_comp_rings) {
			dp_err("Invalid TX comp ring_num:%d received",
			       ring_num);
			return QDF_STATUS_E_INVAL;
		}

		srng =
		(struct hal_srng *)dp_soc->tx_comp_ring[ring_num].hal_srng;

		dp_dal_fill_srng_params(srng, &params, msi_addr, msi_data);
		return hal_srng_set_msi_irq_config(dp_soc->hal_soc,
						   (hal_ring_handle_t)srng,
						   &params);
	} else {
		dp_err("Invalid ring_type:%d received", ring_type);
		return QDF_STATUS_E_INVAL;
	}
}

static int
dp_dal_store_ring_hp_tp(void *priv, int ring_type,
			int ring_num, uint32_t hp, uint32_t tp)
{
	struct dp_dal_ctx *dal_ctx = (struct dp_dal_ctx *)priv;
	struct dp_soc *soc;
	struct hal_srng *hal_srng;

	if (!dal_ctx) {
		dp_err("DAL ctx is null");
		return -EINVAL;
	}

	soc = dal_ctx->soc;

	switch (ring_type) {
	case REO_DST:
		if (ring_num >= soc->num_reo_dest_rings) {
			dp_err("invalid reo ring num %d rcvc", ring_num);
			return -EINVAL;
		}

		hal_srng =
		(struct hal_srng *)soc->reo_dest_ring[ring_num].hal_srng;
		if (hal_srng) {
			hal_srng->u.dst_ring.tp = tp;
			hal_srng->u.dst_ring.cached_hp = hp;
			dp_info("Updated REO DST ring %d: TP=0x%x HP= 0x%x",
				ring_num, tp, hp);
		} else {
			dp_err("SRNG is null for reo dst ring %d", ring_num);
			return -EINVAL;
		}

		break;
	case COMP_RING_TYPE:
		if (ring_num >= soc->num_tx_comp_rings) {
			dp_err("invalid tx cmpl ring num %d rcvc", ring_num);
			return -EINVAL;
		}

		hal_srng =
		(struct hal_srng *)soc->tx_comp_ring[ring_num].hal_srng;
		if (hal_srng) {
			hal_srng->u.dst_ring.tp = tp;
			hal_srng->u.dst_ring.cached_hp = hp;
			dp_info("Updated TX comp ring %d: TP=0x%x HP= 0x%x",
				ring_num, tp, hp);
		} else {
			dp_err("SRNG is null for tx cmpl ring %d", ring_num);
			return -EINVAL;
		}

		break;
	case TCL_DATA:
		if (ring_num >= soc->num_tcl_data_rings) {
			dp_err("invalid tx cmpl ring num %d rcvc", ring_num);
			return -EINVAL;
		}

		hal_srng =
		(struct hal_srng *)soc->tcl_data_ring[ring_num].hal_srng;
		if (hal_srng) {
			hal_srng->u.src_ring.hp = hp;
			hal_srng->u.src_ring.cached_tp = tp;
			dp_info("Updated tx ring %d:  HP=0x%x TP=0x%x",
				ring_num, hp, tp);
		} else {
			dp_err("invalid tx data ring num %d rcvc", ring_num);
			return -EINVAL;
		}
		break;
	case RXDMA_BUF:
		if (ring_num != 0) {
			dp_err("Invalid Rx refill ring num %d rcvd", ring_num);
			return -EINVAL;
		}

		hal_srng =
		(struct hal_srng *)soc->rx_refill_buf_ring[ring_num].hal_srng;
		if (hal_srng) {
			hal_srng->u.src_ring.hp = hp;
			hal_srng->u.src_ring.cached_tp = tp;
			dp_info("Updated rx refill ring  HP=0x%x TP=0x%x",
				hp, tp);
		} else {
			dp_err("SRNG is null for rx refill ring");
			return -EINVAL;
		}

		break;
	default:
		dp_err("invalid ring type %d received", ring_type);
		return -EINVAL;
	}

	return 0;
}

struct vendor_cb_ops vendor_cb = {
	.rx_isr_cb = dp_dal_rx_isr_vendor_cb,
	.rx_replenish_alloc_cb = dp_dal_rx_replenish_alloc_vendor_cb,
	.rx_cpl_cb = dp_dal_rx_desc_cb,
	.tx_isr_cb = dp_dal_tx_cmp_isr_vendor_cb,
	.tx_cpl_cb = dp_dal_tx_cpl_cb,
	.set_msi_config = dp_dal_set_msi_config,
	.store_ring_hp_tp = dp_dal_store_ring_hp_tp,
	.mode_switch_ind = dp_dal_mode_switch_ind_handler,
};

/**
 * dp_dal_soc_detach - detach DP DAL to SOC
 * @soc: pointer to dp_soc structure
 *
 * Return: None.
 */
void dp_dal_soc_detach(struct dp_soc *soc)
{
	qdf_mem_common_free(soc->dal_ctx);
	soc->dal_ctx = NULL;
	dp_info("DAL context destroyed");
}

/**
 * dp_dal_soc_deinit - De-initialize DP DAL for SOC
 * @soc: pointer to dp_soc structure
 *
 * Return: None.
 */
void dp_dal_soc_deinit(struct dp_soc *soc)
{
	if (!soc || !soc->dal_ctx)
		return;

	qdf_atomic_set(&soc->dal_ctx->deinit_in_progress, 1);

	qdf_timer_sync_cancel(&soc->dal_ctx->dal_poll_timer);
	qdf_timer_sync_cancel(&soc->dal_ctx->rx_replenish_retry_timer);

	dp_dal_rx_desc_list_cleanup(dal_ctx);

	qdf_spinlock_destroy(&soc->dal_ctx->dal_rx_desc_lock);
	qdf_spinlock_destroy(&soc->dal_ctx->dal_tx_cpl_lock);
	qdf_spinlock_destroy(&soc->dal_ctx->dal_replenish_lock);

	dp_dal_bus_stop(soc);
	dp_dal_bus_exit(soc);
	qdf_timer_free(&soc->dal_ctx->dal_poll_timer);
	qdf_timer_free(&soc->dal_ctx->rx_replenish_retry_timer);
}

/**
 * dp_dal_soc_attach - Attach DP DAL to SOC
 * @soc: pointer to dp_soc structure
 *
 * Return: QDF_STATUS_SUCCESS on success, error code on failure.
 */
QDF_STATUS dp_dal_soc_attach(struct dp_soc *soc)
{
	struct dp_dal_ctx *ctx;

	ctx = qdf_mem_common_alloc(sizeof(*ctx));
	if (!ctx) {
		dp_init_err("Failed to allocate memory for DAL context");
		return QDF_STATUS_E_FAILURE;
	}

	dp_info("DAL context allocated");

	ctx->soc = soc;
	soc->dal_ctx = ctx;

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS
dp_dal_update_ring_grp_id(struct dp_soc *soc,
			  struct dal_srng *dal_ring, enum hal_ring_type type)
{
	struct dp_intr *intr_ctx;
	uint8_t grp_id;
	uint8_t dal_tx_mask;
	uint8_t dal_rx_mask;
	uint8_t ring_idx;
	int i;

	for (i = 0; i < WLAN_CFG_INT_NUM_CONTEXTS; i++) {
		dal_tx_mask = soc->intr_ctx[i].dal_tx_ring_mask;
		dal_rx_mask = soc->intr_ctx[i].dal_rx_ring_mask;

		if (type == REO_DST && dal_rx_mask) {
			if (!(dal_rx_mask & (1 << dal_ring->ring_num)))
				continue;

			intr_ctx = &soc->intr_ctx[i];
			goto get_grp_id;
		}

		if (type == COMP_RING_TYPE && dal_tx_mask) {
			ring_idx = dal_ring->ring_num;

			if (!(1 << wlan_cfg_get_wbm_ring_num_for_index(soc->wlan_cfg_ctx,
								       ring_idx) &
			      dal_tx_mask))
				continue;

			intr_ctx = &soc->intr_ctx[i];
			goto get_grp_id;
		}
	}

	dp_err("Failed to get grp id for the DAL ring %d type %d",
	       dal_ring->ring_num, type);

	return QDF_STATUS_E_FAILURE;

get_grp_id:
	grp_id = hif_get_ext_grp_id(soc->hif_handle, intr_ctx);
	if (grp_id >= HIF_MAX_GROUP) {
		dp_err("failed to get grp id for the DAL ring %d type %d",
		       dal_ring->ring_num, type);
		return QDF_STATUS_E_FAILURE;
	}

	dal_ring->grp_id = grp_id;

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS dp_dal_create_ring_to_grp_mapping(struct dp_soc *soc)
{
	struct dp_dal_ctx *dal_ctx = soc->dal_ctx;
	struct dal_srng *dal_ring;
	int i;

	for (i = 0; i < DAL_RX_RINGS_MAX; i++) {
		dal_ring = &dal_ctx->rx_ring[i];

		if (!dal_ring->initialized)
			continue;

		if (dp_dal_update_ring_grp_id(soc, dal_ring, REO_DST) !=
		    QDF_STATUS_SUCCESS) {
			dp_err("Failed to update grp_id for RX ring %d", i);
			return QDF_STATUS_E_FAILURE;
		}
	}

	for (i = 0; i < DAL_TX_RINGS_MAX; i++) {
		dal_ring = &dal_ctx->tx_cmpl_ring[i];

		if (!dal_ring->initialized)
			continue;

		if (dp_dal_update_ring_grp_id(soc, dal_ring, COMP_RING_TYPE) !=
		    QDF_STATUS_SUCCESS) {
			dp_err("Failed to update grp_id for Tx cmp ring %d", i);
			return QDF_STATUS_E_FAILURE;
		}
	}

	return QDF_STATUS_SUCCESS;
}

/**
 * dp_dal_attach_rx_buffers - attach rx buffers to RXDMA_BUF ring
 * @soc: pointer to dp_soc structure
 *
 * Return: 0 on success
 */
static int dp_dal_attach_rx_buffers(struct dp_soc *soc)
{
	struct rx_desc_pool *rx_desc_pool;
	struct dp_dal_ctx *dal_ctx = soc->dal_ctx;

	if (!dal_ctx) {
		dp_err("DAL context is NULL");
		return -EINVAL;
	}

	rx_desc_pool = &soc->rx_desc_buf[0];

	if (global_plat_ops->rx_replenish) {
		return global_plat_ops->rx_replenish(dal_ctx,
						     rx_desc_pool->pool_size,
						     false);
	} else {
		dp_err("rx_replenish plat op is not registered");
		QDF_BUG(0);
	}

	return -EOPNOTSUPP;
}

/**
 * dp_dal_enable_threaded_napi() - Enable threaded NAPI for DAL rings
 * @dal_ctx: DAL context pointer
 *
 * This function enables threaded NAPI for all DAL RX and TX completion rings.
 *
 * Return: QDF_STATUS_SUCCESS on success, error code on failure
 */
static QDF_STATUS dp_dal_enable_threaded_napi(struct dp_dal_ctx *dal_ctx)
{
	int i;
	QDF_STATUS status;

	if (!dal_ctx || !dal_ctx->soc) {
		dp_err("Invalid DAL context or SoC");
		return QDF_STATUS_E_INVAL;
	}

	/* Enable threaded NAPI for all DAL RX rings */
	for (i = 0; i < DAL_RX_RINGS_MAX; i++) {
		if (!dal_ctx->rx_ring[i].initialized)
			continue;

		status = hif_exec_set_threaded_napi(dal_ctx->soc->hif_handle,
						    dal_ctx->rx_ring[i].grp_id,
						    true);
		if (QDF_IS_STATUS_ERROR(status)) {
			dp_err("Failed to enable threaded NAPI for RX ring %d",
			       i);
			return QDF_STATUS_E_FAILURE;
		}

		dp_info("Enabled threaded NAPI for RX ring %d", i);
	}

	/* Enable threaded NAPI for all DAL TX completion rings */
	for (i = 0; i < DAL_TX_RINGS_MAX; i++) {
		if (!dal_ctx->tx_cmpl_ring[i].initialized)
			continue;

		status = hif_exec_set_threaded_napi(dal_ctx->soc->hif_handle,
						    dal_ctx->tx_cmpl_ring[i].grp_id,
						    true);
		if (QDF_IS_STATUS_ERROR(status)) {
			dp_err("Failed to enable threaded NAPI for TX comp ring %d", i);
			return QDF_STATUS_E_FAILURE;
		}

		dp_info("Enabled threaded NAPI for TX comp ring %d", i);
	}

	return QDF_STATUS_SUCCESS;
}

/**
 * dp_dal_get_intr_ctx_from_ring() - get interrupt context from ring
 * @soc: DP soc handle
 * @ring_num: ring number
 * @ring_type: ring type
 *
 * Return: dp_intr context
 */
static struct dp_intr*
dp_dal_get_intr_ctx_from_ring(struct dp_soc *soc,
			      int ring_num,
			      enum hal_ring_type ring_type)
{
	struct dp_intr *intr_ctx;
	int i;

	for (i = 0; i < WLAN_CFG_INT_NUM_CONTEXTS; i++) {
		intr_ctx = &soc->intr_ctx[i];
		if (ring_type == REO_DST &&
		    (intr_ctx->dal_rx_ring_mask & (1 << ring_num)))
			return intr_ctx;
		else if (ring_type == COMP_RING_TYPE &&
			 (intr_ctx->dal_tx_ring_mask & (1 << ring_num)))
			return intr_ctx;
	}

	return NULL;
}

static void dp_dal_rx_replenish_retry_handler(void *arg)
{
	struct dp_dal_ctx *dal_ctx = (struct dp_dal_ctx *)arg;
	struct dp_soc *soc;
	uint32_t failures;

	if (!dal_ctx) {
		dp_err("DAL context is NULL");
		return;
	}

	soc = dal_ctx->soc;
	if (!soc) {
		dp_err("SOC context is NULL");
		return;
	}

	if (!global_plat_ops || !global_plat_ops->rx_replenish) {
		dp_err("DAL: rx_replenish op not available");
		return;
	}

	if (qdf_atomic_read(&dal_ctx->deinit_in_progress))
		return;

	failures = qdf_atomic_read(&dal_ctx->rx_replenish_failures);
	if (!failures) {
		dal_ctx->rx_replenish_retry_count = 0;
		dal_ctx->rx_replenish_retry_interval_ms =
					DAL_RX_REPLENISH_RETRY_TIMER_MS;
		return;
	}

	if (global_plat_ops->rx_replenish(dal_ctx, failures, false)) {
		/* replenish via bypass path if mode switch in progress */
		if (soc->dal_mode_switch_in_progress) {
			dp_dal_rx_replenish_bypass_mode(dal_ctx,
							failures, false);
			qdf_atomic_sub(failures,
				       &dal_ctx->rx_replenish_failures);
			dal_ctx->rx_replenish_retry_count = 0;
			dal_ctx->rx_replenish_retry_interval_ms =
					DAL_RX_REPLENISH_RETRY_TIMER_MS;
		} else {
			dp_err("DAL: rx_replenish failed in retry, failures:%u",
			       failures);
			dal_ctx->rx_replenish_retry_count++;
			dal_ctx->rx_replenish_retry_interval_ms *=
					DAL_RX_REPLENISH_BACKOFF_MULTIPLIER;
		}
	} else {
		qdf_atomic_sub(failures, &dal_ctx->rx_replenish_failures);
		dal_ctx->rx_replenish_retry_count = 0;
		dal_ctx->rx_replenish_retry_interval_ms =
					DAL_RX_REPLENISH_RETRY_TIMER_MS;
	}

	/* start timer again if replenish_failure count is non-zero */
	if (!qdf_atomic_read(&dal_ctx->deinit_in_progress) &&
	    qdf_atomic_read(&dal_ctx->rx_replenish_failures))
		qdf_timer_mod(&dal_ctx->rx_replenish_retry_timer,
			      dal_ctx->rx_replenish_retry_interval_ms);
}

/**
 * dp_dal_poll_timer_handler() - Timer handler to poll DAL owned rings
 * @arg: pointer to DAL context
 *
 * This timer handler iterates over DAL owned rings and processes pending
 * entries during mode switch from offload to bypass.
 */
static void dp_dal_poll_timer_handler(void *arg)
{
	struct dp_dal_ctx *dal_ctx = (struct dp_dal_ctx *)arg;
	struct dp_soc *soc = dal_ctx->soc;
	struct dp_intr *intr_ctx;
	uint32_t hp, tp;
	int i;
	bool poll_again = false;

	/* Process DAL owned REO destination rings */
	for (i = 0; i < soc->num_reo_dest_rings; i++) {
		if (dp_srng_check_dal_owned_ring(&soc->reo_dest_ring[i])) {
			intr_ctx = dp_dal_get_intr_ctx_from_ring(soc, i,
								 REO_DST);
			if (intr_ctx) {
				soc->arch_ops.dp_rx_process(intr_ctx,
							    soc->reo_dest_ring[i].hal_srng,
							    i, 64);
				hal_get_sw_hptp(soc->hal_soc,
						soc->reo_dest_ring[i].hal_srng,
						&tp, &hp);
				if (tp != hp)
					poll_again = true;
			}
		}
	}

	/* Process DAL owned TX completion rings */
	for (i = 0; i < soc->num_tx_comp_rings; i++) {
		if (dp_srng_check_dal_owned_ring(&soc->tx_comp_ring[i])) {
			intr_ctx =
			dp_dal_get_intr_ctx_from_ring(soc, i, COMP_RING_TYPE);

			if (intr_ctx) {
				dp_tx_comp_handler(intr_ctx, soc,
						   soc->tx_comp_ring[i].hal_srng,
						   i, 64);
				hal_get_sw_hptp(soc->hal_soc,
						soc->tx_comp_ring[i].hal_srng,
						&tp, &hp);
				if (tp != hp)
					poll_again = true;
			}
		}
	}

	dal_ctx->poll_count++;

	/*
	 * Reschedule the timer if poll_again is true and the poll count
	 * is less than DAL_POLL_TIMER_MAX_COUNT.
	 */
	if (poll_again &&
	    dal_ctx->poll_count < DAL_POLL_TIMER_MAX_COUNT)
		qdf_timer_mod(&dal_ctx->dal_poll_timer,
			      DAL_POLL_TIMER_INTERVAL_MS);
}

/**
 * dp_dal_soc_init - Initialize DP DAL for SOC
 * @soc: pointer to dp_soc structure
 *
 * Return: QDF_STATUS_SUCCESS on success, error code on failure.
 */
QDF_STATUS dp_dal_soc_init(struct dp_soc *soc)
{
	struct dp_dal_ctx *dal_ctx;
	QDF_STATUS status;

	if (!soc || !soc->dal_ctx)
		return QDF_STATUS_E_INVAL;

	dal_ctx = soc->dal_ctx;

	qdf_spinlock_create(&dal_ctx->dal_tx_cpl_lock);
	qdf_spinlock_create(&dal_ctx->dal_rx_desc_lock);
	qdf_spinlock_create(&dal_ctx->dal_replenish_lock);
	qdf_atomic_init(&dal_ctx->rx_replenish_failures);
	qdf_atomic_init(&dal_ctx->deinit_in_progress);
	qdf_atomic_init(&dal_ctx->bm_replenish_not_allowed);

	dal_ctx->rx_replenish_retry_interval_ms =
					DAL_RX_REPLENISH_RETRY_TIMER_MS;

	qdf_timer_init(soc->osdev, &dal_ctx->dal_poll_timer,
		       dp_dal_poll_timer_handler, dal_ctx,
		       QDF_TIMER_TYPE_WAKE_APPS);
	qdf_timer_init(soc->osdev, &dal_ctx->rx_replenish_retry_timer,
		       dp_dal_rx_replenish_retry_handler, dal_ctx,
		       QDF_TIMER_TYPE_WAKE_APPS);

	status = dp_dal_create_ring_to_grp_mapping(soc);
	if (status != QDF_STATUS_SUCCESS) {
		dp_err("failed to create DAL ring to grp mapping %d", status);
		goto destroy_lock;
	}

	status = dp_dal_enable_threaded_napi(soc->dal_ctx);
	if (QDF_IS_STATUS_ERROR(status)) {
		dp_err("failed to create napi thread for dal %d", status);
		return status;
	}

	status = dp_dal_bus_init(soc);
	if (status) {
		dp_err("DAL platform bus init failed %d", status);
		goto destroy_lock;
	}

	status = dp_dal_bus_request_irq(soc);
	if (status) {
		dp_err("DAL platform bus request IRQ failed %d", status);
		goto bus_deinit;
	}

	status = dp_dal_attach_rx_buffers(soc);
	if (status) {
		dp_err("DAL rx buffer attach failed %d", status);
		goto bus_deinit;
	}

	status = dp_dal_bus_start(soc);
	if (status) {
		dp_err("DAL platform bus start failed %d", status);
		goto bus_deinit;
	}

	dp_info("DAL SOC init completed successfully");

	return QDF_STATUS_SUCCESS;

bus_deinit:
	dp_info("DAL SOC init failed");
	dp_dal_bus_exit(soc);
destroy_lock:
	qdf_spinlock_destroy(&dal_ctx->dal_replenish_lock);
	qdf_spinlock_destroy(&dal_ctx->dal_rx_desc_lock);
	qdf_spinlock_destroy(&dal_ctx->dal_tx_cpl_lock);
	return status;
}

/**
 * dp_dal_bus_exit() - DAL bus exit
 * @soc: pointer to DP SoC
 *
 * Called during driver deinit dp_pdev_deinit(), this function will release all
 * allocated resources in the offload engine and stops the Offload Engine.
 *
 * Return: void
 */
void dp_dal_bus_exit(struct dp_soc *soc)
{
	struct dp_dal_ctx *dal_ctx = soc->dal_ctx;

	if (!dal_ctx)
		return;

	if (global_plat_ops->exit)
		global_plat_ops->exit(dal_ctx);
}

/**
 * dp_dal_bus_init() - DAL bus initialization function
 * @soc: pointer to DP SoC
 *
 * Called during cdp_soc_attach_target(), this function sync TXBM information
 * to the offload engine.
 *
 * Return: int
 */
int dp_dal_bus_init(struct dp_soc *soc)
{
	struct dp_dal_ctx *dal_ctx = soc->dal_ctx;

	if (!dal_ctx)
		return -EINVAL;

	if (global_plat_ops->init)
		return global_plat_ops->init(dal_ctx);

	return 0;
}

/**
 * dp_dal_bus_stop - Stop DP DAL bus
 * @soc: pointer to dp_soc structure
 *
 * This function stops the DP DAL bus associated with the given SOC.
 */
void dp_dal_bus_stop(struct dp_soc *soc)
{
	struct dp_dal_ctx *dal_ctx = soc->dal_ctx;

	if (!dal_ctx)
		return;

	if (global_plat_ops->stop)
		global_plat_ops->stop(dal_ctx);
}

/**
 * dp_dal_bus_start() - DAL bus start function
 * @soc: pointer to DP SoC
 *
 * Called during cdp_soc_attach_target(), this function sync ring information
 * to the offload engine.
 *
 * Return: int
 */
int dp_dal_bus_start(struct dp_soc *soc)
{
	struct dp_dal_ctx *dal_ctx = soc->dal_ctx;

	if (!dal_ctx)
		return -EINVAL;

	if (global_plat_ops->start)
		return global_plat_ops->start(dal_ctx);

	return 0;
}

/**
 * dp_dal_bus_request_irq() - DAL IRQ registration function
 * @soc: pointer to DP SoC
 *
 * Called during cdp_soc_attach_target(), this function sync IRQ info to OE,
 * OE will register Tx & Rx interrupts.
 *
 * Return: int
 */
int dp_dal_bus_request_irq(struct dp_soc *soc)
{
	struct dp_dal_ctx *dal_ctx = soc->dal_ctx;

	if (!dal_ctx)
		return -EINVAL;

	if (global_plat_ops->request_irq)
		return global_plat_ops->request_irq(dal_ctx);

	return 0;
}

/**
 * dp_dal_rx_buffers_replenish() - RX buffer enqueue function used from
 * non-DAL path
 * @soc: pointer to DP SoC
 * @mac_id: mac id
 * @rx_desc_pool: pointer to rx desc pool
 * @num_req_buffers: Number of Rx buffers to replenish
 * @desc_list: HEAD pointer to rx desc list elem list
 * @tail: TAIL pointer to rx desc list elem list
 *
 * Invoked from a non-DAL path, such as the non-DAL REO DEST ring process, the
 * Rx error path replenishes buffers for processed descriptors. Since the OE
 * manages the rx buffer refill ring, all rx buffer replenishments must be
 * performed through the OE.
 *
 * Return: int
 */
int dp_dal_rx_buffers_replenish(struct dp_soc *soc, uint32_t mac_id,
				struct rx_desc_pool *rx_desc_pool,
				uint32_t num_req_buffers,
				union dp_rx_desc_list_elem_t **desc_list,
				union dp_rx_desc_list_elem_t **tail)
{
	struct dp_dal_ctx *dal_ctx = soc->dal_ctx;
	int ret;

	if (!dal_ctx)
		return -EINVAL;

	if (desc_list && *desc_list)
		dp_rx_add_desc_list_to_free_list(soc, desc_list, tail,
						 mac_id, rx_desc_pool);

	if (!global_plat_ops || !global_plat_ops->rx_replenish) {
		dp_err("DAL: no op registers for rx_replenish req_buf:%u",
		       num_req_buffers);
		return QDF_STATUS_E_FAILURE;
	}

	ret = global_plat_ops->rx_replenish(dal_ctx, num_req_buffers, false);
	if (ret) {
		if (soc->dal_mode_switch_in_progress)
			dp_dal_rx_replenish_bypass_mode(dal_ctx,
							num_req_buffers, false);
		else
			qdf_atomic_add(num_req_buffers,
				       &dal_ctx->rx_replenish_failures);
	}

	return ret;
}

static enum dal_intf_type
qdf_opmode_to_dal_intf_type(enum QDF_OPMODE mode)
{
	switch (mode) {
	case QDF_STA_MODE:
		return DAL_INTF_TYPE_STA;
	case QDF_SAP_MODE:
		return DAL_INTF_TYPE_SAP;
	default:
		return DAL_INTF_TYPE_MAX;
	}
}

/**
 * dp_dal_interface_add() - DAL interface add
 * @soc: pointer to DP SoC
 * @vdev: DP vdev structure
 *
 * Called during dp_vdev_attach_wifi3(), this function will add interface
 * details to offload engine.
 *
 * Return: int
 */
int dp_dal_interface_add(struct dp_soc *soc, struct dp_vdev *vdev)
{
	struct dp_dal_ctx *dal_ctx = soc->dal_ctx;
	struct dal_intf_info intf_info = {0};
	enum dal_intf_type type;
	int status;

	if (!dal_ctx) {
		dp_err("DAL context is NULL, cannot add interface");
		return QDF_STATUS_E_FAILURE;
	}

	type = qdf_opmode_to_dal_intf_type(vdev->qdf_opmode);
	if (type >= DAL_INTF_TYPE_MAX)
		return 0;

	intf_info.type = type;
	intf_info.vdev_id = vdev->vdev_id;
	intf_info.tcl_bank_id = vdev->bank_id;

	if (type == DAL_INTF_TYPE_STA)
		intf_info.tx_ring_id = DAL_TX_RING_ID0_STA;
	else if (type == DAL_TX_RING_ID0_SAP)
		intf_info.tx_ring_id = DAL_TX_RING_ID0_SAP;
	intf_info.tx_rbm_id =
		wlan_cfg_get_rbm_id_for_index(soc->wlan_cfg_ctx,
					      intf_info.tx_ring_id);

	qdf_mem_copy(&intf_info.mac_address[0],
		     &vdev->mac_addr.raw[0], QDF_MAC_ADDR_SIZE);

	if (global_plat_ops->intf_init) {
		status = global_plat_ops->intf_init(dal_ctx,
						    &intf_info);
		if (status) {
			dp_err("dal interface add failed vdev_id:%d status %d",
			       vdev->vdev_id, status);
			return status;
		}
	}

	if (global_plat_ops->tx_queue_active) {
		status = global_plat_ops->tx_queue_active(dal_ctx,
							  vdev->vdev_id, true);
		if (status) {
			dp_err("dal tx queue active failed vdev_id:%d status %d",
			       vdev->vdev_id, status);
			/* Cleanup the interface that was just initialized */
			if (global_plat_ops->intf_deinit)
				global_plat_ops->intf_deinit(dal_ctx,
							     vdev->vdev_id);
			return status;
		}
	}

	return 0;
}

/**
 * dp_dal_interface_remove() - DAL interface remove
 * @soc: pointer to DP SoC
 * @vdev_id: vdev ID of the interface
 *
 * Called during dp_vdev_detach_wifi3(), this function will remove interface
 * details from the offload engine.
 *
 * Return: None
 */
void dp_dal_interface_remove(struct dp_soc *soc, uint16_t vdev_id)
{
	struct dp_dal_ctx *dal_ctx = soc->dal_ctx;
	int status;

	if (!dal_ctx) {
		dp_err("DAL context is NULL, cannot remove interface");
		return;
	}

	if (global_plat_ops->tx_queue_active) {
		status = global_plat_ops->tx_queue_active(dal_ctx,
							  vdev_id, false);
		if (status)
			dp_err("dal txq deactivate failed vdev_id:%d status %d",
			       vdev_id, status);
		/* Continue to intf_deinit despite error */
	}

	if (global_plat_ops->intf_deinit) {
		status = global_plat_ops->intf_deinit(dal_ctx, vdev_id);
		if (status)
			dp_err("dal intf remove failed vdev_id:%d status %d",
			       vdev_id, status);
	}
}

/**
 * dp_dal_sta_active() - DAL API to send STA information
 * @soc: pointer to DP SoC
 * @info: station information
 * @enable: 0: disconnect, 1: connect
 *
 * Called during STA connect/disconnect, this function will share station
 * information to the offload engine.
 *
 * Return: int
 */
int dp_dal_sta_active(struct dp_soc *soc, struct sta_info *info, bool enable)
{
	struct dp_dal_ctx *dal_ctx = soc->dal_ctx;

	if (!dal_ctx || !info)
		return -EINVAL;

	if (global_plat_ops->sta_active)
		return global_plat_ops->sta_active(dal_ctx, info, enable);

	return 0;
}

/**
 * dp_dal_notify_suspend() - DAL wrapper for platform notify suspend
 * @soc: pointer to DP SoC
 *
 * This function calls the global platform ops notify_suspend function.
 * When this returns successfully, it means there are no pending transactions
 * from the DAL and the device can suspend.
 *
 * Return: QDF_STATUS_SUCCESS on success, QDF_STATUS_E_FAILURE on failure
 */
QDF_STATUS dp_dal_notify_suspend(struct dp_soc *soc)
{
	struct dp_dal_ctx *dal_ctx;
	QDF_STATUS status;
	int ret = -EOPNOTSUPP;

	if (!soc) {
		dp_err("Invalid SoC pointer");
		return QDF_STATUS_E_INVAL;
	}

	dal_ctx = soc->dal_ctx;
	if (!dal_ctx) {
		dp_err("DAL context is NULL");
		return QDF_STATUS_E_INVAL;
	}

	if (global_plat_ops && global_plat_ops->notify_suspend)
		ret = global_plat_ops->notify_suspend(dal_ctx);

	if (ret) {
		dp_err_rl("Suspend notify to DAL failed %d", ret);
		return QDF_STATUS_E_FAILURE;
	}

	/* Wait for pending tasks to complete */
	status = hif_try_complete_dp_tasks(soc->hif_handle);
	if (QDF_IS_STATUS_ERROR(status)) {
		dp_err("Failed to complete DP tasks");
		return status;
	}

	return QDF_STATUS_SUCCESS;
}

/**
 * dp_dal_notify_resume() - DAL wrapper for platform notify resume
 * @soc: pointer to DP SoC
 *
 * This function calls the global platform ops notify_resume function.
 * This is called when the device is resuming from suspend state.
 *
 * Return: QDF_STATUS_SUCCESS on success, QDF_STATUS_E_FAILURE on failure
 */
QDF_STATUS dp_dal_notify_resume(struct dp_soc *soc)
{
	struct dp_dal_ctx *dal_ctx;
	int ret = -EOPNOTSUPP;

	if (!soc) {
		dp_err("Invalid SoC pointer");
		return QDF_STATUS_E_INVAL;
	}

	dal_ctx = soc->dal_ctx;
	if (!dal_ctx) {
		dp_err("DAL context is NULL");
		return QDF_STATUS_E_INVAL;
	}

	if (global_plat_ops && global_plat_ops->notify_resume)
		ret = global_plat_ops->notify_resume(dal_ctx);

	if (ret) {
		dp_err_rl("Resume notify to DAL failed %d", ret);
		return QDF_STATUS_E_FAILURE;
	}

	return QDF_STATUS_SUCCESS;
}

static void dp_dal_update_ring_params(struct dp_soc *soc,
				      struct hal_srng *srng,
				      struct dal_srng *dal_ring)
{
	struct hal_soc *hal = (struct hal_soc *)soc->hal_soc;

	dal_ring->hal_ring_id = srng->ring_id;
	dal_ring->ring_base_paddr = srng->ring_base_paddr;
	dal_ring->ring_base_vaddr = srng->ring_base_vaddr;
	dal_ring->num_entries = srng->num_entries;
	dal_ring->ring_size = srng->ring_size;
	dal_ring->ring_size_mask = srng->ring_size_mask;
	dal_ring->entry_size = srng->entry_size;
	dal_ring->ring_type = srng->ring_type;
	dal_ring->ring_dir = srng->ring_dir;
	dal_ring->lmac_ring = srng->flags & HAL_SRNG_LMAC_RING ? true : false;

	if (srng->ring_dir == HAL_SRNG_SRC_RING) {
		dal_ring->u.src_ring.hp = srng->u.src_ring.hp;

		if (dal_ring->lmac_ring)
			dal_ring->u.src_ring.hp_addr =
				virt_to_phys(srng->u.src_ring.hp_addr);
		else
			dal_ring->u.src_ring.hp_addr =
					srng->u.src_ring.hp_addr -
					(uint32_t *)(hal->dev_base_addr);

		dal_ring->u.src_ring.tp_addr =
			virt_to_phys(srng->u.src_ring.tp_addr);
	} else {
		dal_ring->u.dst_ring.tp = srng->u.dst_ring.tp;

		if (dal_ring->lmac_ring)
			dal_ring->u.dst_ring.tp_addr =
				virt_to_phys(srng->u.src_ring.tp_addr);
		else
			dal_ring->u.dst_ring.tp_addr =
					srng->u.dst_ring.tp_addr -
					(uint32_t *)(hal->dev_base_addr);

		dal_ring->u.dst_ring.hp_addr =
				virt_to_phys(srng->u.dst_ring.hp_addr);
	}
}

void dp_dal_save_srng_info(struct dp_soc *soc, struct dp_srng *srng,
			   enum hal_ring_type type, int ring_num)
{
	struct dp_dal_ctx *dal_ctx = soc->dal_ctx;
	struct dal_srng *dal_ring;
	int ring_info_cnt;

	if (!dp_srng_check_dal_owned_ring(srng))
		return;

	switch (type) {
	case REO_DST:
		ring_info_cnt = dal_ctx->num_rx_ring_info;
		if (ring_info_cnt >= DAL_RX_RINGS_MAX) {
			dp_err("Max rx ring info limit:%d reached",
			       ring_info_cnt);
			return;
		}

		dal_ring = &dal_ctx->rx_ring[ring_info_cnt];
		dp_dal_update_ring_params(soc,
					  (struct hal_srng *)srng->hal_srng,
					  dal_ring);
		dal_ring->ring_num = ring_num;
		dal_ring->initialized = true;
		dal_ctx->num_rx_ring_info++;
		break;
	case TCL_DATA:
		ring_info_cnt = dal_ctx->num_tx_ring_info;
		if (ring_info_cnt >= DAL_TX_RINGS_MAX) {
			dp_err("Max tx ring info limit:%d reached",
			       ring_info_cnt);
			return;
		}

		dal_ring = &dal_ctx->tx_ring[ring_info_cnt];
		dp_dal_update_ring_params(soc,
					  (struct hal_srng *)srng->hal_srng,
					  dal_ring);
		dal_ring->ring_num = ring_num;
		dal_ring->initialized = true;
		dal_ctx->num_tx_ring_info++;
		break;
	case COMP_RING_TYPE:
		ring_info_cnt = dal_ctx->num_tx_cmpl_ring_info;
		if (ring_info_cnt >= DAL_TX_RINGS_MAX) {
			dp_err("Max tx cmpl ring info limit:%d reached",
			       ring_info_cnt);
			return;
		}

		dal_ring = &dal_ctx->tx_cmpl_ring[ring_info_cnt];
		dp_dal_update_ring_params(soc,
					  (struct hal_srng *)srng->hal_srng,
					  dal_ring);
		dal_ring->ring_num = ring_num;
		dal_ring->initialized = true;
		dal_ctx->num_tx_cmpl_ring_info++;
		break;
	case RXDMA_BUF:
		dal_ring = &dal_ctx->rx_refill_ring;
		dp_dal_update_ring_params(soc,
					  (struct hal_srng *)srng->hal_srng,
					  dal_ring);
		dal_ring->initialized = true;
		break;
	default:
		dp_err("Invalid ring info rcvd srng %pK type %d ring_num %d",
		       srng, type, ring_num);
	}
}

int dp_dal_get_ext_grp_id(struct dp_dal_ctx *dal_ctx,
			  int ring_num, enum hal_ring_type type)
{
	struct dal_srng *dal_ring;
	int grp_id = 0xFF;
	int i;

	if (type == REO_DST) {
		for (i = 0; i < DAL_RX_RINGS_MAX; i++) {
			dal_ring = &dal_ctx->rx_ring[i];
			if (dal_ring->ring_num == ring_num)
				return dal_ring->grp_id;
		}
	} else if (type == COMP_RING_TYPE) {
		for (i = 0; i < DAL_TX_RINGS_MAX; i++) {
			dal_ring = &dal_ctx->tx_cmpl_ring[i];
			if (dal_ring->ring_num == ring_num)
				return dal_ring->grp_id;
		}
	} else {
		dp_err("invalid ring_type:%d received", type);
	}

	return grp_id;
}

uint32_t dp_service_dal_srngs(void *dp_ctx, uint32_t dp_budget, int cpu)
{
	struct dp_intr *int_ctx = (struct dp_intr *)dp_ctx;
	struct dp_soc *soc = int_ctx->soc;
	int dal_tx_mask = 0;
	int dal_rx_mask = 0;
	int i;

	dal_tx_mask = int_ctx->dal_tx_ring_mask;
	dal_rx_mask = int_ctx->dal_rx_ring_mask;

	if (dal_rx_mask) {
		for (i = 0; i < soc->num_reo_dest_rings; i++) {
			if (!(dal_rx_mask & (1 << i)))
				continue;

			dp_dal_rx_handler(soc, i, dp_budget);
		}
	}

	if (dal_tx_mask) {
		for (i = 0; i < soc->num_tx_comp_rings; i++) {
			if (!(1 <<  wlan_cfg_get_wbm_ring_num_for_index(soc->wlan_cfg_ctx, i) &
			      dal_tx_mask))
				continue;

			dp_dal_tx_comp_handler(soc, i, dp_budget);
		}
	}

	return 0;
}
