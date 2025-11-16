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

#ifdef DAL_OFFLOAD_SIM
/**
 * dp_dal_offload_sim_hal_addrs_params_init() - Form hal_srng address parameters
 * for offload simulation.
 * @offload_sim_ctx: Pointer to offload simulation context
 * @hal_srng: Pointer to destination dal_vndr_hal_srng structure
 * @sim_srng: Pointer to source dal_sim_srng structure
 *
 * hp/tp address are physical address for lmac rings and for umac rings
 * they are offset from BAR. For HP/TP addr, we need to add base address of
 * device to get the correct virtual address. Google offload engine has to
 * use BAR address to form correct physical address. Vendor HAL APIS are
 * agnostic of the address being used in the dal_vndr_hal_srng structure.
 * Care must be taken to assign correct address based on simulation mode or
 * real use case scenario.
 *
 * Return: None
 *
 */
static inline void dp_dal_offload_sim_hal_addrs_params_init(
	struct dp_dal_offload_sim_ctx *offload_sim_ctx,
	struct dal_vndr_hal_srng *hal_srng,
	struct dal_sim_srng *sim_srng)
{
	/* Here in simulation mode ring_base_addr is filled with virtual address
	 * so that vendor hal apis can use directly ring_base_addr to access
	 * descriptors. In real hardware, ring_base_addr will be filled with
	 * physical address.
	 */
	hal_srng->ring_base_addr = sim_srng->ring_base_vaddr;
	hal_srng->ring_base_paddr = sim_srng->ring_base_paddr;

	if (sim_srng->ring_dir == DAL_VNDR_HAL_SRNG_SRC_RING) {
		hal_srng->u.src_ring.hp = sim_srng->u.src_ring.hp;

		if (sim_srng->lmac_ring)
			/* In offload simulation we are filling virtual address
			 * in  hp_addr and tp_addr field. Usage for offload
			 * engine can vary.
			 *
			 * In case of lmac rings, hp_addr is physical address
			 * which needs to be converted to virtual address before
			 * passing to vendor HAL APIs in case of offload engine
			 * simulation. Real offload engine can use physical
			 * address.
			 */
			hal_srng->u.src_ring.hp_addr =
				phys_to_virt(sim_srng->u.src_ring.hp_addr);
		else
			hal_srng->u.src_ring.hp_addr =
				sim_srng->u.src_ring.hp_addr +
				offload_sim_ctx->dev_base_addr;

		hal_srng->u.src_ring.tp_addr =
			phys_to_virt(sim_srng->u.src_ring.tp_addr);
	} else {
		hal_srng->u.dst_ring.tp = sim_srng->u.dst_ring.tp;

		if (sim_srng->lmac_ring)
			hal_srng->u.dst_ring.tp_addr =
				phys_to_virt(sim_srng->u.dst_ring.tp_addr);
		else
			hal_srng->u.dst_ring.tp_addr =
					sim_srng->u.dst_ring.tp_addr +
					offload_sim_ctx->dev_base_addr;

		hal_srng->u.dst_ring.hp_addr =
				phys_to_virt(sim_srng->u.dst_ring.hp_addr);
	}
}
#else
static inline void dp_dal_offload_sim_hal_addrs_params_init(
				struct dp_dal_offload_sim_ctx *offload_sim_ctx,
				struct dal_vndr_hal_srng *hal_srng,
				struct dal_sim_srng *sim_srng)
{
}
#endif
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
