/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: ISC
 */

#include "dp_dal_tx.h"

/**
 * dp_dal_tx_bypass_mode() - Skeleton for platform bus tx in bypass mode
 *
 * @priv: private data
 * @pkt: tx packet
 * @ifidx: interface index
 *
 * Return: 0 on success
 */
int dp_dal_tx_bypass_mode(void *priv, void *pkt, u32 ifidx)
{
	return 0;
}

/**
 * dp_dal_tx_cpl_bypass_mode() - Skeleton for platform bus tx completion
 * handler in bypass mode
 *
 * @priv: private data
 * @cnt: packet count
 *
 * Return: true on success
 */
bool dp_dal_tx_cpl_bypass_mode(void *priv, u32 *cnt)
{
	return true;
}

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
int dp_dal_tx_queue_active_bypass_mode(void *priv, u16 flowid, bool enable)
{
	return 0;
}

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
				struct dp_tx_msdu_info_s *msdu_info)
{
	/* TODO: implement hardware enqueue logic */
	return QDF_STATUS_SUCCESS;
}
