/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: ISC
 */

#ifndef DP_DAL_RX_H
#define DP_DAL_RX_H

#include "dp_dal.h"
#include "qdf_nbuf.h"

#ifdef FEATURE_DAL_DP_SUPPORT
/**
 *dp_dal_rx_bypass_mode() - Skeleton for platform bus rx in bypass mode
 *
 * @priv: private data
 * @cnt: count
 *
 * Return: false
 */
bool dp_dal_rx_bypass_mode(void *priv, u32 *cnt);

/**
 * dp_dal_rx_replenish_bypass_mode() - Skeleton for platform bus rx replenish
 * in bypass mode
 *
 * @priv: private data
 * @cnt: count
 * @use_rsv_pktid: use reserved packet id
 *
 * Return: 0 on success
 */
int dp_dal_rx_replenish_bypass_mode(void *priv, u32 cnt, bool use_rsv_pktid);

/**
 * dp_dal_rx_rxbm_sync_bypass_mode() - Skeleton for platform bus rxbm sync
 * in bypass mode
 *
 * @priv: private data
 * @cnt: count
 * @rxbm: rxbm
 *
 * Return: 0 on success
 */
int dp_dal_rx_rxbm_sync_bypass_mode(void *priv, u32 cnt, void **rxbm);

/**
 * dp_dal_rx_process_nbuf_list - Process a list of qdf_nbufs for RX path.
 * @soc: DP SOC context.
 * @nbuf_list: Head of the qdf_nbuf list to be processed.
 *
 * This is a placeholder (skeleton) implementation. The actual RX processing
 * logic should be added by the driver developer.
 *
 * Return: %QDF_STATUS_SUCCESS on success, or an appropriate error code.
 */
QDF_STATUS dp_dal_rx_process_nbuf_list(struct dp_soc *soc,
				       qdf_nbuf_t nbuf_list);

/**
 *dp_dal_rx_isr_vendor_cb - rx ISR vendor callback
 *@ring_num: rx ring number
 *@priv: pointer to dp dal context
 *
 * Return: 0 on success
 */
int dp_dal_rx_isr_vendor_cb(int ring_num, void *priv);

/**
 *dp_dal_rx_replenish_alloc_vendor_cb - vendor callback to alloc rx buffers
 * @priv: pointer to dp dal context
 * @count: number of buffers to allocate
 *
 * Return: 0 on success
 */
int dp_dal_rx_replenish_alloc_vendor_cb(void *priv, uint16_t count);
#endif /* FEATURE_DAL_DP_SUPPORT */
#endif /* DP_DAL_RX_H */
