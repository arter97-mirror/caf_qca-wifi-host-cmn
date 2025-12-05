/*
 * Copyright (c) 2020-2021, The Linux Foundation. All rights reserved.
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "osdep.h"
#include "wmi.h"
#include "wmi_unified_priv.h"

/**
 * validate_dcs_event_params() - validate DCS event parameters
 * @param_buf: pointer to DCS interference event TLVs
 *
 * This function checks whether the provided event buffer and its
 * fixed parameters are valid. If either is NULL, it returns an
 * error status; otherwise, it returns success.
 *
 * Return: QDF_STATUS_SUCCESS on success, QDF_STATUS_E_INVAL on invalid input
 */
static QDF_STATUS
validate_dcs_event_params(WMI_DCS_INTERFERENCE_EVENTID_param_tlvs *param_buf)
{
	if (!param_buf) {
		wmi_err("Invalid evt buf");
		return QDF_STATUS_E_INVAL;
	}

	if (!param_buf->fixed_param) {
		wmi_err("Invalid fixed param");
		return QDF_STATUS_E_INVAL;
	}

	return QDF_STATUS_SUCCESS;
}

/**
 * copy_dcs_im_stats() - Copy DCS interference mitigation target stats
 * @dest: Pointer to destination structure - wlan_host_dcs_im_tgt_stats
 * @src:  Pointer to source structure - wlan_dcs_im_tgt_stats_t
 *
 * This helper function copies DCS (Dynamic Channel Selection) interference
 * mitigation statistics from the firmware-provided structure into the host
 * representation. It performs a field-by-field assignment, including:
 *   - TSF timestamp
 *   - Last ACK RSSI
 *   - TX waste time and RX time
 *   - PHY error count
 *   - MIB statistics (listen time, TX/RX frame counts, RX clear counts,
 *     cycle counts, OFDM/CCK PHY error counts)
 *   - Channel noise floor
 *   - BSS RX cycle count
 *
 * Used to normalize firmware stats into host data structures for further
 * processing and reporting.
 */
static void copy_dcs_im_stats(struct wlan_host_dcs_im_tgt_stats *dest,
			      const wlan_dcs_im_tgt_stats_t *src)
{
	dest->reg_tsf32 = src->reg_tsf32;
	dest->last_ack_rssi = src->last_ack_rssi;
	dest->tx_waste_time = src->tx_waste_time;
	dest->rx_time = src->rx_time;
	dest->phyerr_cnt = src->phyerr_cnt;
	dest->mib_stats.listen_time = src->listen_time;
	dest->mib_stats.reg_tx_frame_cnt = src->reg_tx_frame_cnt;
	dest->mib_stats.reg_rx_frame_cnt = src->reg_rx_frame_cnt;
	dest->mib_stats.reg_rxclr_cnt = src->reg_rxclr_cnt;
	dest->mib_stats.reg_cycle_cnt = src->reg_cycle_cnt;
	dest->mib_stats.reg_rxclr_ext_cnt = src->reg_rxclr_ext_cnt;
	dest->mib_stats.reg_ofdm_phyerr_cnt = src->reg_ofdm_phyerr_cnt;
	dest->mib_stats.reg_cck_phyerr_cnt = src->reg_cck_phyerr_cnt;
	dest->chan_nf = src->chan_nf;
	dest->my_bss_rx_cycle_count = src->my_bss_rx_cycle_count;
}

/**
 * extract_dcs_interference_type_tlv() - extract dcs interference type
 * from event
 * @wmi_handle: wmi handle
 * @evt_buf: pointer to event buffer
 * @param: Pointer to hold dcs interference param
 *
 * Return: 0 for success or error code
 */
static QDF_STATUS extract_dcs_interference_type_tlv(
		wmi_unified_t wmi_handle,
		void *evt_buf, struct wlan_host_dcs_interference_param *param)
{
	WMI_DCS_INTERFERENCE_EVENTID_param_tlvs *param_buf;
	QDF_STATUS status;

	param_buf = (WMI_DCS_INTERFERENCE_EVENTID_param_tlvs *)evt_buf;
	status = validate_dcs_event_params(param_buf);
	if (QDF_IS_STATUS_ERROR(status))
		return status;

	param->interference_type = param_buf->fixed_param->interference_type;
	/* Just support tlv currently */
	param->pdev_id = wmi_handle->ops->convert_target_pdev_id_to_host(
					wmi_handle,
					param_buf->fixed_param->pdev_id);

	/*
	 * Will add service capability check. If it's supported
	 * then fill vdev_id from param_buf otherwise fill WLAN_INVALID_VDEV_ID
	 */
	param->vdev_id = param_buf->fixed_param->vdev_id;

	return QDF_STATUS_SUCCESS;
}

/**
 * extract_dcs_im_tgt_stats_tlv() - extract dcs im target stats from event
 * @wmi_handle: wmi handle
 * @evt_buf: pointer to event buffer
 * @wlan_stat: Pointer to hold wlan stats
 *
 * Return: 0 for success or error code
 */
static QDF_STATUS extract_dcs_im_tgt_stats_tlv(
			wmi_unified_t wmi_handle,
			void *evt_buf,
			struct wlan_host_dcs_im_tgt_stats *wlan_stat)
{
	WMI_DCS_INTERFERENCE_EVENTID_param_tlvs *param_buf;
	wlan_dcs_im_tgt_stats_t *ev;
	QDF_STATUS status;

	param_buf = (WMI_DCS_INTERFERENCE_EVENTID_param_tlvs *)evt_buf;
	status = validate_dcs_event_params(param_buf);
	if (QDF_IS_STATUS_ERROR(status))
		return status;

	ev = param_buf->wlan_stat;
	if (!ev) {
		wmi_err("Invalid wlan stat");
		return QDF_STATUS_E_INVAL;
	}

	copy_dcs_im_stats(wlan_stat, ev);

	return QDF_STATUS_SUCCESS;
}

/**
 * extract_dcs_im_tgt_stats_count_tlv() - extract count of dcs im target stats
 * @wmi_handle: wmi handle
 * @evt_buf: pointer to event buffer
 * @num_wlan_stat: Pointer to hold number of wlan stats
 *
 * Return: 0 for success or error code
 */
static QDF_STATUS extract_dcs_im_tgt_stats_count_tlv(
			wmi_unified_t wmi_handle,
			void *evt_buf,
			uint32_t *num_wlan_stat)
{
	WMI_DCS_INTERFERENCE_EVENTID_param_tlvs *param_buf;
	QDF_STATUS status;

	param_buf = (WMI_DCS_INTERFERENCE_EVENTID_param_tlvs *)evt_buf;
	status = validate_dcs_event_params(param_buf);
	if (QDF_IS_STATUS_ERROR(status))
		return status;

	if (!param_buf->wlan_stat) {
		if (param_buf->num_wlan_stat > 0) {
			wmi_err("miss wlan_stats, count %d",
				param_buf->num_wlan_stat);
			*num_wlan_stat = 0;
			return QDF_STATUS_E_INVAL;
		}
		*num_wlan_stat = 0;
		return QDF_STATUS_SUCCESS;
	}

	*num_wlan_stat = param_buf->num_wlan_stat;

	return QDF_STATUS_SUCCESS;
}

/**
 * extract_dcs_im_tgt_stats_idx_tlv() - extract dcs im target stats by index
 * @wmi_handle: wmi handle
 * @evt_buf: pointer to event buffer
 * @idx: index of the VDEV stats to extract
 * @dcs_event: Pointer to hold dcs event
 *
 * Return: 0 for success or error code
 */
static QDF_STATUS extract_dcs_im_tgt_stats_idx_tlv(
			wmi_unified_t wmi_handle,
			void *evt_buf,
			uint32_t idx,
			struct wlan_host_dcs_event *dcs_event)
{
	WMI_DCS_INTERFERENCE_EVENTID_param_tlvs *param_buf;
	wlan_dcs_im_tgt_stats_t *ev;
	QDF_STATUS status;

	param_buf = (WMI_DCS_INTERFERENCE_EVENTID_param_tlvs *)evt_buf;
	status = validate_dcs_event_params(param_buf);
	if (QDF_IS_STATUS_ERROR(status))
		return status;

	if (!param_buf->wlan_stat) {
		wmi_err("NULL wlan_stat");
		return QDF_STATUS_E_INVAL;
	}

	if (idx >= param_buf->num_wlan_stat) {
		wmi_err("Invalid idx %d, num_wlan_stat %d",
			idx, param_buf->num_wlan_stat);
		return QDF_STATUS_E_INVAL;
	}

	ev = &param_buf->wlan_stat[idx];
	if (!ev->vdev_id_and_pdev_id_valid) {
		wmi_err("vdev_id_and_pdev_id_valid not set");
		return QDF_STATUS_E_INVAL;
	}

	dcs_event->dcs_param.interference_type =
		param_buf->fixed_param->interference_type;

	dcs_event->dcs_param.pdev_id =
		wmi_handle->ops->convert_target_pdev_id_to_host(
			wmi_handle,
			param_buf->fixed_param->pdev_id);
	dcs_event->dcs_param.vdev_id = param_buf->fixed_param->vdev_id;
	copy_dcs_im_stats(&dcs_event->wlan_stat, ev);

	return QDF_STATUS_SUCCESS;
}

/*
 * extract_dcs_awgn_info_tlv() - extract DCS AWGN interference from event
 * @wmi_handle: wmi handle
 * @param evt_buf: pointer to event buffer
 * @param awgn_info: Pointer to hold cw interference
 *
 * Return: QDF_STATUS_SUCCESS for success or QDF_STATUS_E_* for error
 */
static QDF_STATUS
extract_dcs_awgn_info_tlv(wmi_unified_t wmi_handle, void *evt_buf,
			  struct wlan_host_dcs_awgn_info *awgn_info)
{
	WMI_DCS_INTERFERENCE_EVENTID_param_tlvs *param_buf;
	wmi_dcs_awgn_int_t *ev;

	param_buf = evt_buf;
	if (!param_buf)
		return QDF_STATUS_E_INVAL;

	ev = param_buf->awgn_int;
	if (!ev) {
		wmi_err("Invalid awgn info");
		return QDF_STATUS_E_INVAL;
	}

	awgn_info->channel_width = wmi_map_ch_width(ev->channel_width);
	awgn_info->center_freq = (qdf_freq_t)ev->chan_freq;
	awgn_info->center_freq0 = (qdf_freq_t)ev->center_freq0;
	awgn_info->center_freq1 = (qdf_freq_t)ev->center_freq1;
	awgn_info->chan_bw_intf_bitmap = ev->chan_bw_interference_bitmap;
	wmi_debug("width: %u, freq: %u, freq0: %u, freq1: %u, bitmap: 0x%x",
		  awgn_info->channel_width, awgn_info->center_freq,
		  awgn_info->center_freq0, awgn_info->center_freq1,
		  awgn_info->chan_bw_intf_bitmap);

	return QDF_STATUS_SUCCESS;
}

void wmi_dcs_attach_tlv(wmi_unified_t wmi_handle)
{
	struct wmi_ops *ops = wmi_handle->ops;

	ops->extract_dcs_interference_type = extract_dcs_interference_type_tlv;
	ops->extract_dcs_im_tgt_stats = extract_dcs_im_tgt_stats_tlv;
	ops->extract_dcs_im_tgt_stats_count =
					extract_dcs_im_tgt_stats_count_tlv;
	ops->extract_dcs_im_tgt_stats_idx = extract_dcs_im_tgt_stats_idx_tlv;
	ops->extract_dcs_awgn_info = extract_dcs_awgn_info_tlv;
}

