/*
 * Copyright (c) 2016-2021 The Linux Foundation. All rights reserved.
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * Permission to use, copy, modify, and/or distribute this software for
 * any purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all
 * copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/**
 * DOC: cdp_txrx_mon.h
 * Define the monitor mode API functions
 * called by the host control SW and the OS interface module
 */

#ifndef _CDP_TXRX_MON_H_
#define _CDP_TXRX_MON_H_
#include "cdp_txrx_handle.h"
#include <cdp_txrx_cmn.h>

static inline QDF_STATUS cdp_reset_monitor_mode(ol_txrx_soc_handle soc,
						uint8_t pdev_id,
						u_int8_t smart_monitor)
{
	if (!soc || !soc->ops) {
		dp_cdp_debug("Invalid Instance");
		return 0;
	}

	if (!soc->ops->mon_ops ||
	    !soc->ops->mon_ops->txrx_reset_monitor_mode)
		return 0;

	return soc->ops->mon_ops->txrx_reset_monitor_mode(soc, pdev_id,
							  smart_monitor);
}

/**
 * cdp_deliver_tx_mgmt() - Deliver mgmt frame for tx capture
 * @soc: Datapath SOC handle
 * @pdev_id: id of datapath PDEV handle
 * @nbuf: Management frame buffer
 */
static inline QDF_STATUS
cdp_deliver_tx_mgmt(ol_txrx_soc_handle soc, uint8_t pdev_id,
		    qdf_nbuf_t nbuf)
{
	if (!soc || !soc->ops) {
		dp_cdp_debug("Invalid Instance");
		return QDF_STATUS_E_FAILURE;
	}

	if (!soc->ops->mon_ops ||
	    !soc->ops->mon_ops->txrx_deliver_tx_mgmt)
		return QDF_STATUS_E_FAILURE;

	return soc->ops->mon_ops->txrx_deliver_tx_mgmt(soc, pdev_id, nbuf);
}

#ifdef QCA_RSSI_DB2DBM
/**
 * cdp_set_params_rssi_dbm_conversion - Set the rssi dbm conversion params
 *					into dp_pdev structure
 * @soc: soc txrx handler
 * @params: cdp_rssi_db2dbm_param_dp pointer
 *
 */
static inline QDF_STATUS
cdp_set_params_rssi_dbm_conversion(ol_txrx_soc_handle soc,
				   struct cdp_rssi_db2dbm_param_dp *params)
{
	if (!soc || !soc->ops) {
		dp_cdp_debug("Invalid Instance:");
		return QDF_STATUS_E_FAILURE;
	}

	if (!soc->ops->mon_ops ||
	    !soc->ops->mon_ops->txrx_set_mon_pdev_params_rssi_dbm_conv)
		return QDF_STATUS_E_FAILURE;

	return soc->ops->mon_ops->txrx_set_mon_pdev_params_rssi_dbm_conv
							    (soc, params);
}
#endif

#ifdef WLAN_CONFIG_TELEMETRY_AGENT
/*
 * cdp_update_pdev_mon_telemetry_airtime_stats() - update telemetry airtime
 * stats in monitor pdev
 *
 *@soc: dp soc handle
 *@pdev_id: pdev id
 *
 * This API is used to update telemetry airtime stats in monitor pdev
 *
 * Return: Success if stats are updated, else failure
 */
static inline QDF_STATUS
cdp_update_pdev_mon_telemetry_airtime_stats(ol_txrx_soc_handle soc,
					    uint8_t pdev_id)
{
	if (!soc || !soc->ops) {
		dp_cdp_debug("Invalid Instance");
		return QDF_STATUS_E_FAILURE;
	}

	if (!soc->ops->mon_ops ||
	    !soc->ops->mon_ops->txrx_update_pdev_mon_telemetry_airtime_stats)
		return QDF_STATUS_E_FAILURE;

	return soc->ops->mon_ops->txrx_update_pdev_mon_telemetry_airtime_stats(
						soc, pdev_id);
}
#endif

#ifdef WLAN_FEATURE_LOCAL_PKT_CAPTURE
/**
 * cdp_start_local_pkt_capture() - start local pkt capture
 * @soc: opaque soc handle
 * @pdev_id: pdev id
 * @filter: monitor filter config
 *
 * Return: QDF_STATUS_SUCCESS if success
 *         QDF_STATUS_E_FAILURE if error
 */
static inline
QDF_STATUS cdp_start_local_pkt_capture(ol_txrx_soc_handle soc,
				       uint8_t pdev_id,
				       struct cdp_monitor_filter *filter)
{
	if (!soc || !soc->ops) {
		dp_cdp_debug("Invalid Instance");
		return QDF_STATUS_E_FAILURE;
	}

	if (!soc->ops->mon_ops ||
	    !soc->ops->mon_ops->start_local_pkt_capture)
		return QDF_STATUS_E_FAILURE;

	return soc->ops->mon_ops->start_local_pkt_capture(soc, pdev_id, filter);
}

/**
 * cdp_stop_local_pkt_capture() - stop local pkt capture
 * @soc: opaque soc handle
 * @pdev_id: pdev_id
 *
 * Return: QDF_STATUS_SUCCESS if success
 *         QDF_STATUS_E_FAILURE if error
 */
static inline
QDF_STATUS cdp_stop_local_pkt_capture(ol_txrx_soc_handle soc, uint8_t pdev_id)
{
	if (!soc || !soc->ops) {
		dp_cdp_debug("Invalid Instance");
		return QDF_STATUS_E_FAILURE;
	}

	if (!soc->ops->mon_ops ||
	    !soc->ops->mon_ops->stop_local_pkt_capture)
		return QDF_STATUS_E_FAILURE;

	return soc->ops->mon_ops->stop_local_pkt_capture(soc, pdev_id);
}

/**
 * cdp_is_local_pkt_capture_running() - get is local packet capture running
 * @soc: opaque soc handle
 * @pdev_id: pdev id
 *
 * Return: true if running
 *         false if not running
 */
static inline
bool cdp_is_local_pkt_capture_running(ol_txrx_soc_handle soc, uint8_t pdev_id)
{
	if (!soc || !soc->ops) {
		dp_cdp_debug("Invalid Instance");
		return false;
	}

	if (!soc->ops->mon_ops ||
	    !soc->ops->mon_ops->is_local_pkt_capture_running)
		return false;

	return soc->ops->mon_ops->is_local_pkt_capture_running(soc, pdev_id);
}
#else
static inline
QDF_STATUS cdp_start_local_pkt_capture(ol_txrx_soc_handle soc,
				       uint8_t pdev_id,
				       struct cdp_monitor_filter *filter)
{
	return QDF_STATUS_E_NOSUPPORT;
}

static inline
QDF_STATUS cdp_stop_local_pkt_capture(ol_txrx_soc_handle soc, uint8_t pdev_id)
{
	return QDF_STATUS_E_NOSUPPORT;
}

static inline
bool cdp_is_local_pkt_capture_running(ol_txrx_soc_handle soc, uint8_t pdev_id)
{
	return false;
}
#endif /* WLAN_FEATURE_LOCAL_PKT_CAPTURE */

#endif
