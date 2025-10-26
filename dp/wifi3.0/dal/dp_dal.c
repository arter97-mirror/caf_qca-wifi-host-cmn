/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: ISC
 */

#include "dp_dal.h"
#include "dp_dal_rx.h"
#include "dp_dal_tx.h"
#include <wlan_cfg.h>

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
};

struct platform_bus_ops *global_plat_ops = &plat_ops_bypass_mode;

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
	/* TODO: implement actual deinit logic */
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

/**
 * dp_dal_soc_init - Initialize DP DAL for SOC
 * @soc: pointer to dp_soc structure
 *
 * Return: QDF_STATUS_SUCCESS on success, error code on failure.
 */
QDF_STATUS dp_dal_soc_init(struct dp_soc *soc)
{
	/* TODO: implement actual init logic */
	return QDF_STATUS_SUCCESS;
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
 * dp_dal_bus_rx_buffer_enqueue() - DAL RX buffer enqueue function
 * @soc: pointer to DP SoC
 * @cnt: Number of RX buffers to replenish
 *
 * Called during cdp_soc_attach_target() and during RX replenish, this function
 * enqueues RX buffers to DAL, DAL/OE will in turn update the buffers into
 * SW2FW ring.
 *
 * Return: int
 */
int dp_dal_bus_rx_buffer_enqueue(struct dp_soc *soc, uint32_t cnt)
{
	struct dp_dal_ctx *dal_ctx = soc->dal_ctx;

	if (!dal_ctx)
		return -EINVAL;

	if (global_plat_ops->rx_replenish)
		return global_plat_ops->rx_replenish(dal_ctx, cnt, false);

	return 0;
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

			/* call platform_bus_rx() */
		}
	}

	if (dal_tx_mask) {
		for (i = 0; i < soc->num_tx_comp_rings; i++) {
			if (!(1 <<  wlan_cfg_get_wbm_ring_num_for_index(soc->wlan_cfg_ctx, i) &
			      dal_tx_mask))
				continue;

			/* call platform_bus_tx_cpl() */
		}
	}

	return 0;
}
