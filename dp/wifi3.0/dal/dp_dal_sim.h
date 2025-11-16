/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: ISC
 */

#ifndef DP_DAL_SIM_H
#define DP_DAL_SIM_H

#include "dp_dal.h"
#include <qdf_atomic.h>

/* Externs */
extern struct platform_bus_ops *global_plat_ops;
extern struct platform_bus_ops plat_ops_bypass_mode;
extern struct vendor_cb_ops vendor_cb;
/**
 * dp_dal_sim_platform_bus_ops_attach() - Attach platform bus operations
 *
 * Based on g_dal_sim_curr_mode, assigns the appropriate platform operations
 * to the global platform ops structure:
 * - Mode 0 (bypass): Assigns plat_ops_bypass_mode
 * - Mode 1 (offload): Assigns dal_sim_plat_ops
 */
void dp_dal_sim_platform_bus_ops_attach(void);

#ifdef FEATURE_DP_DAL_SIM

/* Number of RX and TX rings for DAL simulator */
#define DAL_SIM_NUM_RX_RINGS DAL_RX_RINGS_MAX
#define DAL_SIM_NUM_TX_RINGS DAL_TX_RINGS_MAX

/**
 * struct dp_dal_sim_ctx - Context structure for DAL simulation
 * @dp_dal_ctx: Pointer to DAL datapath context
 */
struct dp_dal_sim_ctx {
	void *dp_dal_ctx;
};
#endif /* FEATURE_DP_DAL_SIM */
#endif /* DP_DAL_SIM_H */
