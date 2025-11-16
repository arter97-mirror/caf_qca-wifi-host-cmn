/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: ISC
 */

#ifndef DP_DAL_OFFLOAD_SIM_H
#define DP_DAL_OFFLOAD_SIM_H

#include "dp_dal_sim.h"
#include "dal_vndr_hal_api.h"
#ifdef FEATURE_DP_DAL_SIM
/**
 * enum offload_sim_ring_type - Ring type for interrupt handling
 * @OFFLOAD_SIM_RING_TYPE_RX: RX ring (REO destination)
 * @OFFLOAD_SIM_RING_TYPE_TX_CPL: TX completion ring (WBM2SW)
 */
enum offload_sim_ring_type {
	OFFLOAD_SIM_RING_TYPE_RX = 0,
	OFFLOAD_SIM_RING_TYPE_TX_CPL = 1,
};

/**
 * struct offload_sim_irq_ctx - IRQ context for interrupt handler
 * @dal_sim_ctx: Pointer to DAL simulation context
 * @ring_id: Ring ID (0 or 1)
 * @ring_type: Ring type (RX or TX completion)
 * @irq_configured: Flag indicating if IRQ has been successfully configured
 */
struct offload_sim_irq_ctx {
	struct dp_dal_sim_ctx *dal_sim_ctx;
	uint8_t ring_id;
	enum offload_sim_ring_type ring_type;
	bool irq_configured;
};

/**
 * struct dp_dal_offload_sim_ctx - Offload simulation context
 * @hal_soc: Vendor HAL SoC structure
 * @rx_ring_hal_srng: Array of vendor HAL SRNG structures for RX rings
 * @tx_cmpl_ring_hal_srng: Array of vendor HAL SRNG structures for
 * TX completion rings
 * @tx_ring_hal_srng: Array of vendor HAL SRNG structures for TX rings
 * @rx_refill_ring_hal_srng: Vendor HAL SRNG structure for RX refill ring
 * @rx_irq_ctx: IRQ context for RX rings
 * @tx_cpl_irq_ctx: IRQ context for TX completion rings
 * @offload_sim_ctx_initialized: Flag indicating if context is initialized
 * @dev_base_addr: device base address
 *
 * This structure maintains the offload simulation context with vendor HAL
 * ring structures that are populated from dal_sim_srng structures.
 */
struct dp_dal_offload_sim_ctx {
	struct dal_vndr_hal_soc hal_soc;
	struct dal_vndr_hal_srng rx_ring_hal_srng[DAL_RX_RINGS_MAX];
	struct dal_vndr_hal_srng tx_cmpl_ring_hal_srng[DAL_TX_RINGS_MAX];
	struct dal_vndr_hal_srng tx_ring_hal_srng[DAL_TX_RINGS_MAX];
	struct dal_vndr_hal_srng rx_refill_ring_hal_srng;
	struct offload_sim_irq_ctx rx_irq_ctx[DAL_RX_RINGS_MAX];
	struct offload_sim_irq_ctx tx_cpl_irq_ctx[DAL_TX_RINGS_MAX];
	bool offload_sim_ctx_initialized;
	void *dev_base_addr;
};

/**
 * dp_dal_offload_sim_init() - Initialize offload simulation context
 * @dal_sim_ctx: Pointer to DAL simulation context
 *
 * This function allocates and initializes the offload simulation context.
 * It assigns values from dal_sim_srng structures (present in dal_sim_ctx)
 * to dal_vndr_hal_srng structures in the offload_sim_ctx.
 *
 * Return: 0 on success, error code on failure
 */
int dp_dal_offload_sim_init(struct dp_dal_sim_ctx *dal_sim_ctx);

/**
 * dp_dal_offload_sim_deinit() - Deinitialize offload simulation context
 * @dal_sim_ctx: Pointer to DAL simulation context
 *
 * This function deinitializes and frees the offload simulation context.
 */
void dp_dal_offload_sim_deinit(struct dp_dal_sim_ctx *dal_sim_ctx);
/**
 * dp_dal_offload_sim_free_irq() - Free IRQs for offload simulation
 * @dal_sim_ctx: Pointer to DAL simulation context
 *
 * This function frees interrupt handlers for RX and TX completion rings.
 */
void dp_dal_offload_sim_free_irq(struct dp_dal_sim_ctx *dal_sim_ctx);
#endif /* FEATURE_DAL_DP_SUPPORT */
#endif /* DP_DAL_OFFLOAD_SIM_H */
