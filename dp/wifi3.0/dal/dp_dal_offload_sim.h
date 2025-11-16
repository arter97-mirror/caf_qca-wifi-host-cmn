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

#endif /* FEATURE_DAL_DP_SUPPORT */
#endif /* DP_DAL_OFFLOAD_SIM_H */
