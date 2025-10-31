/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: ISC
 */

#ifndef DP_DAL_TX_H
#define DP_DAL_TX_H

#include "dp_types.h"
#include "cdp_txrx_cmn_struct.h"
#include "dp_dal.h"
#include "qdf_status.h"
#include "dp_tx.h"

/**
 * dp_dal_tx_cmp_isr_vendor_cb - tx cmpl ISR vendor callback
 * @ring_num: tx completion ring number
 * @priv: pointer to dp dal context
 *
 * Return: 0 on success
 */
int dp_dal_tx_cmp_isr_vendor_cb(int ring_num, void *priv);

/**
 * dp_dal_tx_bypass_mode() - Skeleton for platform bus tx in bypass mode
 *
 * @priv: private data
 * @pkt: tx packet
 * @ifidx: interface index
 * @desc: TX descriptor
 *
 * Return: 0 on success
 */
int dp_dal_tx_bypass_mode(void *priv, void *pkt, u32 ifidx, void *desc);

/**
 * dp_dal_tx_cpl_bypass_mode() - Skeleton for platform bus tx completion
 * handler in bypass mode
 *
 * @priv: private data
 * @cnt: packet count
 * @ring_id: Ring Id of the completion ring
 *
 * Return: true on success
 */
bool dp_dal_tx_cpl_bypass_mode(void *priv, u32 *cnt, u16 ring_id);

/**
 * dp_dal_tx_queue_active_bypass_mode() - Skeleton for platform tx queue active
 * in bypass mode
 *
 * @priv: private data
 * @flowid: flow id
 * @enable: enable or not
 *
 * Return: 0 on success
 */
int dp_dal_tx_queue_active_bypass_mode(void *priv, u16 flowid, bool enable);

/**
 * dp_dal_tx_hw_enqueue - Enqueue a BE TX packet (DAL stub).
 * @soc: DP SOC context.
 * @vdev: DP VDEV context.
 * @tx_desc: TX descriptor for the packet.
 * @fw_metadata: Firmware metadata associated with the packet.
 * @metadata: Exception metadata for TX path.
 * @msdu_info: MSDU information for the packet.
 *
 * This is a placeholder implementation that currently returns
 * %QDF_STATUS_SUCCESS. It should be replaced with the actual
 * hardware enqueue logic.
 *
 * Return: %QDF_STATUS_SUCCESS on success.
 */
QDF_STATUS dp_dal_tx_hw_enqueue(struct dp_soc *soc,
				struct dp_vdev *vdev,
				struct dp_tx_desc_s *tx_desc,
				uint16_t fw_metadata,
				struct cdp_tx_exception_metadata *metadata,
				struct dp_tx_msdu_info_s *msdu_info);

/**
 * dp_dal_tx_cpl_cb() - DAL TX completion callback handler
 * @priv: DAL context (dal_ctx)
 * @desc: TX completion descriptor
 * @ring_id: Ring ID
 *
 * This callback handler processes TX completion descriptors received from
 * the DAL module. It does descriptor validation checks from and adds
 * descriptors to a dedicated list within dal_ctx for later processing.
 *
 * Return: 0 for successful processing
 */
int dp_dal_tx_cpl_cb(void *priv, void *desc, u16 ring_id);

/**
 * dp_dal_tx_comp_handler() - DAL TX completion handler
 * @soc: DP SOC context
 * @ring_id: Ring ID
 * @dp_budget: NAPI budget
 *
 * This is the primary API for processing TX completions in the DAL module.
 * It invokes platform_bus_tx_cpl() to get completions from the DAL module,
 * then processes the descriptor list accumulated via tx_cpl_cb() by
 * calling dp_tx_comp_process_desc_list().
 *
 * Return: Number of completions processed
 */
uint32_t dp_dal_tx_comp_handler(struct dp_soc *soc, u16 ring_id,
				uint32_t dp_budget);

#endif /* DP_DAL_TX_H */
