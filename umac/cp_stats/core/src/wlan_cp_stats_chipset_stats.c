/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: ISC
 */

/**
 * DOC: This file contains chipset stats implementstion
 */

#ifdef WLAN_CHIPSET_STATS
#include <qdf_mem.h>
#include <qdf_types.h>
#include <qdf_status.h>
#include <qdf_trace.h>
#include <qdf_time.h>
#include <qdf_mc_timer.h>
#include <qdf_lock.h>
#include <qdf_str.h>
#include <qdf_module.h>
#include <wlan_nlink_common.h>
#include <wlan_cp_stats_chipset_stats.h>
#include "wlan_cp_stats_obj_mgr_handler.h"

static struct chipset_stats cstats;
static struct cstats_node *gcstats_buffer[CSTATS_MAX_TYPE];

void wlan_cp_stats_enable_direct_log_dispatch(struct wlan_objmgr_psoc *psoc,
					      bool direct_log_dispatch)
{
	cstats.is_direct_log_dispatch_enabled = direct_log_dispatch;
}

QDF_STATUS wlan_cp_stats_cstats_init(struct wlan_objmgr_psoc *psoc)
{
	qdf_list_node_t *tmp_node = NULL;
	int i, j, k, node_count;

	if (!wlan_cp_stats_get_chipset_stats_enable(psoc)) {
		qdf_info("Chipset Stats feature is disabled");
		cstats.is_cstats_ini_enabled = false;
		return QDF_STATUS_SUCCESS;
	}

	cstats.is_cstats_ini_enabled = true;

	cstats.chipset_stats_push_rbs_delay_val_ms =
				wlan_cp_stats_get_user_delay_value_ms(psoc);
	cstats.chipset_stats_push_rbs_delay_interval =
				wlan_cp_stats_get_user_delay_interval(psoc);

	cstats.is_cp_stats_debug_logging_enable  =
			wlan_cp_stats_is_debug_logging_enabled(psoc);

	for (i = 0; i < CSTATS_MAX_TYPE; i++) {
		qdf_spinlock_create(&cstats.cstats_lock[i]);

		if (i == CSTATS_HOST_TYPE)
			node_count = HOST_TYPE_NODE_COUNT;
		else if (i == CSTATS_FW_TYPE)
			node_count = FW_TYPE_NODE_COUNT;
		else
			continue;

		gcstats_buffer[i] = qdf_mem_valloc(node_count *
						   sizeof(struct cstats_node));

		if (!gcstats_buffer[i]) {
			qdf_err("Could not allocate memory for chipset stats");
			for (k = 0; k < i ; k++) {
				qdf_spin_lock_bh(&cstats.cstats_lock[k]);
				cstats.ccur_node[k] = NULL;
				qdf_spin_unlock_bh(&cstats.cstats_lock[k]);

				if (gcstats_buffer[k])
					qdf_mem_vfree(gcstats_buffer[k]);
			}

			return QDF_STATUS_E_NOMEM;
		}

		qdf_mem_zero(gcstats_buffer[i], node_count *
			     sizeof(struct cstats_node));

		qdf_spin_lock_bh(&cstats.cstats_lock[i]);
		qdf_init_list_head(&cstats.cstat_free_list[i].anchor);
		qdf_init_list_head(&cstats.cstat_filled_list[i].anchor);

		for (j = 0; j < node_count; j++) {
			qdf_list_insert_front(&cstats.cstat_free_list[i],
					      &gcstats_buffer[i][j].node);
			gcstats_buffer[i][j].index = j;
		}

		qdf_list_remove_front(&cstats.cstat_free_list[i], &tmp_node);

		cstats.ccur_node[i] =
			qdf_container_of(tmp_node, struct cstats_node, node);
		cstats.cstats_no_flush[i] = true;
		qdf_spin_unlock_bh(&cstats.cstats_lock[i]);
	}

	return 0;
}

void wlan_cp_stats_cstats_deinit(void)
{
	int i;

	if (!cstats.is_cstats_ini_enabled) {
		qdf_info("Chipset Stats feature is disabled");
		return;
	}

	for (i = 0; i < CSTATS_MAX_TYPE; i++) {
		qdf_spin_lock_bh(&cstats.cstats_lock[i]);
		cstats.ccur_node[i] = NULL;
		cstats.cstat_drop_cnt[i] = 0;
		qdf_spin_unlock_bh(&cstats.cstats_lock[i]);

		if (gcstats_buffer[i]) {
			qdf_mem_vfree(gcstats_buffer[i]);
			gcstats_buffer[i] = NULL;
		}
	}
}

void wlan_cp_stats_cstats_register_tx_rx_ops(struct cstats_tx_rx_ops *ops)
{
	cstats.ops.cstats_send_data_to_usr = ops->cstats_send_data_to_usr;
}

/* Need to call this with spin_lock acquired */
static void wlan_cp_stats_get_cstats_free_node(enum cstats_types type)
{
	qdf_list_node_t *tmp_node = NULL;

	if (cstats.ccur_node[type]->filled_length) {
		qdf_list_insert_back(&cstats.cstat_filled_list[type],
				     &cstats.ccur_node[type]->node);
	} else {
		return;
	}

	if (!qdf_list_empty(&cstats.cstat_free_list[type])) {
		qdf_list_remove_front(&cstats.cstat_free_list[type], &tmp_node);
		cstats.ccur_node[type] =
			qdf_container_of(tmp_node, struct cstats_node, node);
	} else if (!qdf_list_empty(&cstats.cstat_filled_list[type])) {
		qdf_list_remove_front(&cstats.cstat_filled_list[type],
				      &tmp_node);
		cstats.ccur_node[type] =
			qdf_container_of(tmp_node, struct cstats_node, node);
		qdf_err("Dropping a chipset stats node : count %d",
			++(cstats.cstat_drop_cnt[type]));
	}

	/* Reset the current node values */
	cstats.ccur_node[type]->filled_length = 0;
}

void wlan_cp_stats_fw_log_event_direct_flush(enum cstats_types type,
					     void *to_be_sent, uint32_t plen)
{
	char *buf, *ptr;
	unsigned int total_len;
	uint64_t *event_l, second_64_hex;

	if (!cstats.is_cstats_ini_enabled || !to_be_sent)
		return;

	/* ensure buffer node exists before writing */
	if (!cstats.ccur_node[type]) {
		qdf_err("Current Node is NULL");
		return;
	}

	/*
	 * If debug logging is enabled, extract timestamp from payload and
	 * log it.
	 */
	if (cstats.is_cp_stats_debug_logging_enable) {
		event_l = (uint64_t *)to_be_sent;
		/* timestamp is stored in the second 64 bits */
		qdf_mem_copy(&second_64_hex, event_l + 1, 8);
		cp_stats_debug("CSTATS FW EVENT received at timestamp: %llu, event_len: %d",
			       second_64_hex, plen);
	}

	qdf_spin_lock_bh(&cstats.cstats_lock[type]);

	/* point to beginning of log buffer */
	buf = cstats.ccur_node[type]->logbuf;

	/* point to buffer position after header to insert content */
	ptr = &buf[sizeof(tAniNlHdr)];
	/* insert firmware start marker */
	memcpy(ptr, CSTATS_FW_START_MARKER, CSTATS_MARKER_SZ);
	/* copy actual firmware payload */
	memcpy(ptr + CSTATS_MARKER_SZ, to_be_sent, plen);
	/* append firmware end marker */
	memcpy(ptr + CSTATS_MARKER_SZ + plen, CSTATS_FW_END_MARKER,
	       CSTATS_MARKER_SZ);

	/* calculate total message length including header and markers */
	total_len = sizeof(tAniNlHdr) + plen + 2 * CSTATS_MARKER_SZ;

	/*
	 * no need to check for space in the current buffer, firmware handles
	 * it so sends logs directly to userspace.
	 */
	if (cstats.ops.cstats_send_data_to_usr)
		cstats.ops.cstats_send_data_to_usr(buf, total_len, type,
				cstats.is_cp_stats_debug_logging_enable);

	/* reset the current node values */
	cstats.ccur_node[type]->filled_length = 0;

	qdf_spin_unlock_bh(&cstats.cstats_lock[type]);
}

void wlan_cp_stats_host_append_and_flush(enum cstats_types type,
					 void *to_be_sent, uint32_t plen)
{
	struct cstats_node *node;
	char *buf, *ptr;
	unsigned int *pfilled_length;
	uint32_t header_len = sizeof(tAniNlHdr);
	uint32_t marker_len = CSTATS_MARKER_SZ;
	uint32_t total_len;

	if (!cstats.is_cstats_ini_enabled || type != CSTATS_HOST_TYPE ||
	    !to_be_sent)
		return;

	node = cstats.ccur_node[type];
	if (!node) {
		qdf_err("Current Node is NULL");
		return;
	}

	qdf_spin_lock_bh(&cstats.cstats_lock[type]);

	buf = node->logbuf;
	pfilled_length = &node->filled_length;

	/* Total required space: start + payload + end */
	total_len = 2 * marker_len + plen;

	/* If buffer can't fit new data, flush it */
	if ((header_len + *pfilled_length + total_len) >
	    MAX_CSTATS_NODE_LENGTH) {
		/*
		 * In hdd_cstats_send_data_to_userspace(), the host appends the
		 * Netlink header to the buffer.
		 */
		if (cstats.ops.cstats_send_data_to_usr)
			cstats.ops.cstats_send_data_to_usr(buf,
				header_len + *pfilled_length, type,
				cstats.is_cp_stats_debug_logging_enable);
		/* Reset buffer after flush */
		*pfilled_length = 0;
		qdf_spin_unlock_bh(&cstats.cstats_lock[type]);
		return;
	}

	/*
	 * Format of accommodated buffer is like:
	 * [START_MARKER][PAYLOAD_1][END_MARKER]
	 * [START_MARKER][PAYLOAD_2][END_MARKER]
	 * .........
	 * [START_MARKER][PAYLOAD_n][END_MARKER]
	 */

	/* Compute where to start writing in buffer
	 * (after header + existing content)
	 */
	ptr = &buf[header_len + *pfilled_length];
	/* Copy start marker */
	memcpy(ptr, CSTATS_HOST_START_MARKER, marker_len);
	/* Copy payload immediately after marker */
	memcpy(ptr + marker_len, to_be_sent, plen);
	/* Copy end marker after payload*/
	memcpy(ptr + marker_len + plen, CSTATS_HOST_END_MARKER, marker_len);

	/* Update filled length to reflect newly added data */
	*pfilled_length += total_len;

	if (cstats.is_cp_stats_debug_logging_enable)
		qdf_debug("Updated pfilled_length: %u", *pfilled_length);

	qdf_spin_unlock_bh(&cstats.cstats_lock[type]);
}

void wlan_cp_stats_cstats_write_to_buff(enum cstats_types type,
					void *to_be_sent,
					uint32_t plen)
{
	char *ptr;
	unsigned int *pfilled_length;
	unsigned int tlen;
	uint64_t *event_l;
	uint64_t second_64_hex;

	if (!cstats.is_cstats_ini_enabled)
		return;

	/* tAniNlHdr + Start Marker + End Marker */
	tlen = sizeof(tAniNlHdr) + (2 * CSTATS_MARKER_SZ);

	if ((tlen + plen) > MAX_CSTATS_NODE_LENGTH) {
		qdf_err("The Buffer is too long");
		return;
	}

	if (!cstats.ccur_node[type]) {
		qdf_err("Current Node is NULL");
		return;
	}

	if (cstats.is_cp_stats_debug_logging_enable) {
		event_l = (uint64_t *)to_be_sent;
		/* second 64 bits of the event represent the timestamp */
		qdf_mem_copy(&second_64_hex, event_l + 1, 8);
		cp_stats_debug("CSTATS FW EVENT received at timestamp: %llu, event_len: %d",
			       second_64_hex, plen);
	}

	qdf_spin_lock_bh(&cstats.cstats_lock[type]);

	pfilled_length = &cstats.ccur_node[type]->filled_length;

	/* Check if we can accommodate more log into current node/buffer */
	if ((MAX_CSTATS_NODE_LENGTH - *pfilled_length) < (tlen + plen)) {
		wlan_cp_stats_get_cstats_free_node(type);
		pfilled_length = &cstats.ccur_node[type]->filled_length;
	}

	if (type == CSTATS_HOST_TYPE) {
		/* Marker will be added while flushing to userspace*/
		ptr = &cstats.ccur_node[type]->logbuf[sizeof(tAniHdr) +
						      CSTATS_MARKER_SZ];
		memcpy(&ptr[*pfilled_length], to_be_sent, plen);
		*pfilled_length += plen;
	} else if (type == CSTATS_FW_TYPE) {
		ptr = &cstats.ccur_node[type]->logbuf[sizeof(tAniHdr)];
		memcpy(&ptr[*pfilled_length], CSTATS_FW_START_MARKER,
		       CSTATS_MARKER_SZ);
		memcpy(&ptr[*pfilled_length + CSTATS_MARKER_SZ], to_be_sent,
		       plen);
		memcpy(&ptr[*pfilled_length + CSTATS_MARKER_SZ + plen],
		       CSTATS_FW_END_MARKER, CSTATS_MARKER_SZ);
		*pfilled_length += (plen + 2 * CSTATS_MARKER_SZ);
	}

	qdf_spin_unlock_bh(&cstats.cstats_lock[type]);
}

void wlan_cp_stats_host_log_event_dispatcher(enum cstats_types type,
					     void *event, uint32_t event_len)
{
	if (cstats.is_direct_log_dispatch_enabled)
		wlan_cp_stats_host_append_and_flush(type, event, event_len);
	else
		wlan_cp_stats_cstats_write_to_buff(type, event, event_len);
}

void wlan_cp_stats_fw_log_event_dispatcher(enum cstats_types type, void *event,
					   int event_len)
{
	if (cstats.is_direct_log_dispatch_enabled)
		wlan_cp_stats_fw_log_event_direct_flush(type, event, event_len);
	else
		wlan_cp_stats_cstats_write_to_buff(type, event, event_len);
}

static int wlan_cp_stats_cstats_send_version_to_usr(void)
{
	uint8_t buff[MAX_CSTATS_VERSION_BUFF_LENGTH] = {0};
	uint8_t n;
	int metadata_len;
	int ret = -1;

	metadata_len = sizeof(tAniHdr) + (2 * CSTATS_MARKER_SZ);

	memcpy(&buff[sizeof(tAniHdr)], CSTATS_HOST_START_MARKER,
	       CSTATS_MARKER_SZ);

	n = scnprintf(&buff[sizeof(tAniHdr) + CSTATS_MARKER_SZ],
		      MAX_CSTATS_VERSION_BUFF_LENGTH - metadata_len,
		      "[%s : %d, %s : %d, %s : %d]",
		      "Chispet stats - hdr_version",
		      CHIPSET_STATS_HDR_VERSION, "Endianness",
		      CHIPSET_STATS_MACHINE_ENDIANNESS, "Drop cnt",
		      cstats.cstat_drop_cnt[CSTATS_HOST_TYPE]);

	qdf_mem_copy(&buff[sizeof(tAniHdr) + CSTATS_MARKER_SZ + n],
		     CSTATS_HOST_END_MARKER, CSTATS_MARKER_SZ);

	buff[metadata_len + n] = '\0';

	if (cstats.ops.cstats_send_data_to_usr) {
		ret = cstats.ops.cstats_send_data_to_usr(buff,
							 metadata_len + n,
							 CSTATS_HOST_TYPE,
							 false);
	}

	if (ret)
		qdf_err("failed to send version info");

	return ret;
}

int wlan_cp_stats_cstats_send_buffer_to_user(enum cstats_types type)
{
	int ret = -1;
	size_t counter = 1;
	struct cstats_node *clog_msg;
	struct cstats_node *next;
	int payload_len;
	int mark_total;
	char *ptr = NULL;
	size_t fw_list_len = 0;
	size_t host_list_len = 0;

	if (!cstats.is_cstats_ini_enabled)
		return QDF_STATUS_SUCCESS;

	qdf_spin_lock_bh(&cstats.cstats_lock[type]);
	wlan_cp_stats_get_cstats_free_node(type);
	qdf_spin_unlock_bh(&cstats.cstats_lock[type]);

	if (type == CSTATS_HOST_TYPE) {
		ret = wlan_cp_stats_cstats_send_version_to_usr();
		if (ret)
			return ret;
	}

	/*
	 * For fw stats the markers are already added at start and end of the
	 * each event
	 */
	if (type == CSTATS_HOST_TYPE)
		mark_total = (2 * CSTATS_MARKER_SZ);
	else if (type == CSTATS_FW_TYPE)
		mark_total = 0;

	qdf_list_for_each_del(&cstats.cstat_filled_list[type],
			      clog_msg, next, node) {
		qdf_spin_lock_bh(&cstats.cstats_lock[type]);

		/* For host stats marksers are added per node basis*/
		if (type == CSTATS_HOST_TYPE) {
			ptr = &clog_msg->logbuf[sizeof(tAniHdr)];
			qdf_mem_copy(ptr, CSTATS_HOST_START_MARKER,
				     CSTATS_MARKER_SZ);
			ptr = &clog_msg->logbuf[sizeof(tAniHdr) +
						CSTATS_MARKER_SZ +
						clog_msg->filled_length];
			qdf_mem_copy(ptr, CSTATS_HOST_END_MARKER,
				     CSTATS_MARKER_SZ);
		}

		if (type == CSTATS_FW_TYPE)
			fw_list_len++;
		else if (type == CSTATS_HOST_TYPE)
			host_list_len++;

		if (!cstats.cstats_no_flush[type]) {
			qdf_list_remove_node(&cstats.cstat_free_list[type],
					     &clog_msg->node);
		}

		qdf_spin_unlock_bh(&cstats.cstats_lock[type]);

		payload_len = clog_msg->filled_length + sizeof(tAniHdr) +
			      mark_total;

		if (cstats.chipset_stats_push_rbs_delay_interval &&
		    !(counter % cstats.chipset_stats_push_rbs_delay_interval)) {
			qdf_mdelay(cstats.chipset_stats_push_rbs_delay_val_ms);
		}

		if (cstats.ops.cstats_send_data_to_usr) {
			ret = cstats.ops.cstats_send_data_to_usr
			       (clog_msg->logbuf, payload_len, type,
			       cstats.is_cp_stats_debug_logging_enable);
		}

		if (ret) {
			qdf_err("Send Failed %d drop_count = %u", ret,
				++(cstats.cstat_drop_cnt[type]));
		}

		if (!cstats.cstats_no_flush[type]) {
			qdf_spin_lock_bh(&cstats.cstats_lock[type]);
			qdf_list_insert_back(&cstats.cstat_free_list[type],
					     &clog_msg->node);
			qdf_spin_unlock_bh(&cstats.cstats_lock[type]);
		}

		counter++;
	}

	if (cstats.is_cp_stats_debug_logging_enable) {
		if (type == CSTATS_FW_TYPE)
			qdf_debug("FW List len : %zu", fw_list_len);
		else if (type == CSTATS_HOST_TYPE)
			qdf_debug("HOST List len : %zu", host_list_len);
	}

	return ret;
}

static inline enum cstats_pkt_type
get_cstat_type(enum qdf_proto_type type,
	       enum qdf_proto_subtype subtype)
{
	if (type == QDF_PROTO_TYPE_EAPOL) {
		if (subtype == QDF_PROTO_EAPOL_M1)
			return CSTATS_EAPOL_M1;
		else if (subtype == QDF_PROTO_EAPOL_M2)
			return CSTATS_EAPOL_M2;
		else if (subtype == QDF_PROTO_EAPOL_M3)
			return CSTATS_EAPOL_M3;
		else if (subtype == QDF_PROTO_EAPOL_M4)
			return CSTATS_EAPOL_M4;
	} else if (type == QDF_PROTO_TYPE_DHCP) {
		if (subtype == QDF_PROTO_DHCP_DISCOVER)
			return CSTATS_DHCP_DISCOVER;
		else if (subtype == QDF_PROTO_DHCP_REQUEST)
			return CSTATS_DHCP_REQ;
		else if (subtype == QDF_PROTO_DHCP_OFFER)
			return CSTATS_DHCP_OFFER;
		else if (subtype == QDF_PROTO_DHCP_ACK)
			return CSTATS_DHCP_ACK;
		else if (subtype == QDF_PROTO_DHCP_NACK)
			return CSTATS_DHCP_NACK;
		else if (subtype == QDF_PROTO_DHCP_RELEASE)
			return CSTATS_DHCP_RELEASE;
		else if (subtype == QDF_PROTO_DHCP_DECLINE)
			return CSTATS_DHCP_DECLINE;
		else if (subtype == QDF_PROTO_DHCP_INFORM)
			return CSTATS_DHCP_INFORM;
	}

	return CSTATS_PKT_TYPE_INVALID;
}

static inline enum cstats_dir
get_cstat_dir(enum qdf_proto_dir dir)
{
	switch (dir) {
	case QDF_TX:
		return CSTATS_DIR_TX;
	case QDF_RX:
		return CSTATS_DIR_RX;
	default:
		return CSTATS_DIR_INVAL;
	}
}

static inline enum cstats_pkt_status
get_cstat_pkt_status(enum qdf_dp_tx_rx_status status)
{
	switch (status) {
	case QDF_TX_RX_STATUS_INVALID:
		return CSTATS_STATUS_INVALID;
	case QDF_TX_RX_STATUS_OK:
		return CSTATS_TX_STATUS_OK;
	case QDF_TX_RX_STATUS_FW_DISCARD:
		return CSTATS_TX_STATUS_FW_DISCARD;
	case QDF_TX_RX_STATUS_NO_ACK:
		return CSTATS_TX_STATUS_NO_ACK;
	case QDF_TX_RX_STATUS_DROP:
		return CSTATS_TX_STATUS_DROP;
	case QDF_TX_RX_STATUS_DOWNLOAD_SUCC:
		return CSTATS_TX_STATUS_DOWNLOAD_SUCC;
	case QDF_TX_RX_STATUS_DEFAULT:
		return CSTATS_TX_STATUS_DEFAULT;
	default:
		return CSTATS_STATUS_INVALID;
	}
}

void wlan_cp_stats_cstats_pkt_log(uint8_t *sa, uint8_t *da,
				  enum qdf_proto_type pkt_type,
				  enum qdf_proto_subtype subtype,
				  enum qdf_proto_dir dir,
				  enum qdf_dp_tx_rx_status status,
				  uint8_t vdev_id,
				  enum QDF_OPMODE op_mode)
{
	struct cstats_pkt_info stat = {0};

	stat.cmn.hdr.evt_id = WLAN_CHIPSET_STATS_DATA_PKT_EVENT_ID;
	stat.cmn.hdr.length = sizeof(struct cstats_pkt_info) -
				sizeof(struct cstats_hdr);
	stat.cmn.opmode = op_mode;
	stat.cmn.vdev_id = vdev_id;
	stat.cmn.timestamp_us = qdf_get_time_of_the_day_us();
	stat.cmn.time_tick = qdf_get_log_timestamp();

	CSTATS_MAC_COPY(stat.src_mac, sa);
	CSTATS_MAC_COPY(stat.dst_mac, da);

	stat.type = get_cstat_type(pkt_type, subtype);
	stat.dir = get_cstat_dir(dir);
	stat.status = get_cstat_pkt_status(status);

	wlan_cstats_host_stats(sizeof(struct cstats_pkt_info), &stat);
}

#endif /* WLAN_CHIPSET_STATS */
