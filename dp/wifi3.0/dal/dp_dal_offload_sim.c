/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: ISC
 */

#include "dp_dal_offload_sim.h"
#include "dp_dal_sim.h"
#include <qdf_mem.h>
#include "dal_vndr_hal_be.h"
/* Maximum length for IRQ name strings */
#define OFFLOAD_SIM_IRQ_NAME_LEN 40
#ifdef FEATURE_DP_DAL_SIM

/**
 * dp_dal_offload_sim_hal_ring_init() - Init dal_vndr_hal_srng structure.
 * @offload_sim_ctx: offload sim ctx
 * @hal_srng: Pointer to destination dal_vndr_hal_srng structure
 * @sim_srng: Pointer to source dal_sim_srng structure
 *
 * Copies ring information field by field from dal_sim_srng to dal_vndr_hal_srng
 */
static void dp_dal_offload_sim_hal_ring_init(
	struct dp_dal_offload_sim_ctx *offload_sim_ctx,
	struct dal_vndr_hal_srng *hal_srng,
	struct dal_sim_srng *sim_srng)
{
	/* Copy basic ring information */
}
int dp_dal_offload_sim_init(struct dp_dal_sim_ctx *dal_sim_ctx)
{
	struct dp_dal_offload_sim_ctx *offload_ctx;
	int i;
	int status = 0;

	if (!dal_sim_ctx) {
		dp_err("NULL DAL sim context");
		return -EINVAL;
	}

	/* Allocate offload simulation context */
	offload_ctx = qdf_mem_malloc(sizeof(*offload_ctx));
	if (!offload_ctx) {
		dp_err("Failed to allocate offload sim context");
		return -ENOMEM;
	}
	/* Zero out the structure */
	qdf_mem_zero(offload_ctx, sizeof(*offload_ctx));

	offload_ctx->hal_soc.ops = qdf_mem_malloc(
					sizeof(*offload_ctx->hal_soc.ops));
	if (!offload_ctx->hal_soc.ops) {
		status = -ENOMEM;
		goto free_offload_ctx;
	}

	/* Store the dev base addr here from dal_sim_ctx.
	 * Google DAL has to assign BAR address here.
	 */
	offload_ctx->hal_soc.dev_base_addr = dal_sim_ctx->dev_base_addr;
	/* Copy dal_vndr_hal_srng from dal_sim_srng structures */
	/* RX rings */
	for (i = 0; i < DAL_RX_RINGS_MAX; i++) {
		dp_dal_offload_sim_hal_ring_init(
					offload_ctx,
					&offload_ctx->rx_ring_hal_srng[i],
					&dal_sim_ctx->rx_ring[i]);
	}

	/* TX completion rings */
	for (i = 0; i < DAL_TX_RINGS_MAX; i++) {
		dp_dal_offload_sim_hal_ring_init(
					offload_ctx,
					&offload_ctx->tx_cmpl_ring_hal_srng[i],
					&dal_sim_ctx->tx_cmpl_ring[i]);
	}

	/* TX rings */
	for (i = 0; i < DAL_TX_RINGS_MAX; i++) {
		dp_dal_offload_sim_hal_ring_init(
					offload_ctx,
					&offload_ctx->tx_ring_hal_srng[i],
					&dal_sim_ctx->tx_ring[i]);
	}

	/* RX refill ring */
	dp_dal_offload_sim_hal_ring_init(offload_ctx,
					 &offload_ctx->rx_refill_ring_hal_srng,
					 &dal_sim_ctx->rx_refill_ring);

	/* Vendor HAL ops can be overridden here if needed with target_type*/
	dal_vndr_hal_default_ops_attach_be(&offload_ctx->hal_soc);

	/* Mark as offload_sim_ctx_initialized */
	offload_ctx->offload_sim_ctx_initialized = true;

	/* Assign to dal_sim_ctx */
	dal_sim_ctx->offload_sim_ctx = offload_ctx;

	return 0;
free_offload_ctx:
	qdf_mem_free(offload_ctx);
	return status;
}

void dp_dal_offload_sim_deinit(struct dp_dal_sim_ctx *dal_sim_ctx)
{
	struct dp_dal_offload_sim_ctx *offload_ctx;

	if (!dal_sim_ctx) {
		dp_warn("NULL DAL sim context in deinit");
		return;
	}

	offload_ctx =
		(struct dp_dal_offload_sim_ctx *)dal_sim_ctx->offload_sim_ctx;
	if (!offload_ctx) {
		dp_warn("NULL offload sim context in deinit");
		return;
	}

	dp_info("Deinitializing offload simulation context");

	/* Free IRQs before deinitializing */
	dp_dal_offload_sim_free_irq(dal_sim_ctx);

	/* Mark as not initialized */
	offload_ctx->offload_sim_ctx_initialized = false;
	/* Free hal_soc ops memory allocated in init */
	qdf_mem_free(offload_ctx->hal_soc.ops);
	/* Free the context */
	qdf_mem_free(offload_ctx);

	/* Nullify the pointer in dal_sim_ctx */
	dal_sim_ctx->offload_sim_ctx = NULL;
	dp_info("Offload simulation context deinitialized successfully");
}

void dp_dal_offload_sim_free_irq(struct dp_dal_sim_ctx *dal_sim_ctx)
{
}
#endif /* FEATURE_DP_DAL_SIM */
