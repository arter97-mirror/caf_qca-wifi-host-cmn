/*
 * Copyright (c) 2021, 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
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
 * DOC: qdf_tracepoint.h
 * This file defines HLOS agnostic functions providing external interface
 * for triggering tracepoints.
 */
#if  !defined(_QDF_TRACEPOINT_H)
#define _QDF_TRACEPOINT_H

#include <i_qdf_tracepoint.h>
#include <qdf_nbuf.h>
#include <qdf_trace.h>

#ifdef WLAN_TRACEPOINTS
/**
 * qdf_trace_dp_packet() - Trace packet in tx or rx path
 * @nbuf: network buffer pointer
 * @dir: tx or rx direction
 * @tso_desc: TSO descriptor
 * @enq_time: tx hw enqueue wall clock time in milliseconds
 * @status: Tx/Rx status
 *
 * Return: None
 */
void qdf_trace_dp_packet(qdf_nbuf_t nbuf, enum qdf_proto_dir dir,
			 struct qdf_tso_seg_elem_t *tso_desc,
			 uint64_t enq_time, uint8_t status);
#else
static inline
void qdf_trace_dp_packet(qdf_nbuf_t nbuf, enum qdf_proto_dir dir,
			 struct qdf_tso_seg_elem_t *tso_desc,
			 uint64_t enq_time, uint8_t status)
{
}
#endif

/**
 * qdf_trace_dp_fisa_trace_rdi_invalid_enabled() - Check if
 * dp_fisa_trace_rdi_invalid tracepoint is enabled
 *
 * Return: true if enabled, false otherwise
 */
static inline
bool qdf_trace_dp_fisa_trace_rdi_invalid_enabled(void)
{
	return __qdf_trace_dp_fisa_trace_rdi_invalid_enabled();
}

/**
 * qdf_trace_dp_fisa_trace_rdi_invalid() - Trace invalid REO destination
 * indicator
 * @reo_dest_ind: REO destination indicator value that resulted in a drop
 * or error
 *
 * This function triggers the dp_fisa_trace_rdi_invalid tracepoint, which
 * logs the REO destination indicator when it is invalid or maps to TCL,
 * aiding diagnosis of improper REO remap decisions.
 *
 * Return: None
 */
static inline
void qdf_trace_dp_fisa_trace_rdi_invalid(uint8_t reo_dest_ind)
{
	__qdf_trace_dp_fisa_trace_rdi_invalid(reo_dest_ind);
}

/**
 * qdf_trace_dp_fisa_flush_vdev_fail_enabled() - Check if
 * dp_fisa_flush_vdev_fail tracepoint is enabled
 *
 * Return: true if enabled, false otherwise
 */
static inline
bool qdf_trace_dp_fisa_flush_vdev_fail_enabled(void)
{
	return __qdf_trace_dp_fisa_flush_vdev_fail_enabled();
}

/**
 * qdf_trace_dp_fisa_flush_vdev_fail() - Trace FISA flush failure details
 * @head_vdev_id: VDEV ID associated with the head skb
 * @flow_vdev_id: VDEV ID associated with the flow
 * @reason: Reason code for the failure
 * @flow_id: Flow identifier
 * @head_vdev_ptr: Pointer to the head vdev object
 * @flow_vdev_ptr: Pointer to the flow vdev object
 *
 * This function triggers the dp_fisa_flush_vdev_fail tracepoint, which logs
 * the details of a FISA flush failure including mismatch conditions and
 * reference errors between the head vdev and the flow vdev.
 *
 * Return: None
 */
static inline
void qdf_trace_dp_fisa_flush_vdev_fail(uint8_t head_vdev_id,
				       uint8_t flow_vdev_id, uint8_t reason,
				       uint32_t flow_id,
				       const void *head_vdev_ptr,
				       const void *flow_vdev_ptr,
				       uint32_t flush_head_vdev_ref_fail,
				       uint32_t flush_flow_vdev_ref_fail,
				       uint32_t flush_vdev_ptr_mismatch,
				       uint32_t flush_mld_mismatch_drop)
{
	__qdf_trace_dp_fisa_flush_vdev_fail(head_vdev_id, flow_vdev_id,
					    reason, flow_id, head_vdev_ptr,
					    flow_vdev_ptr,
					    flush_head_vdev_ref_fail,
					    flush_flow_vdev_ref_fail,
					    flush_vdev_ptr_mismatch,
					    flush_mld_mismatch_drop);
}

/**
 * qdf_trace_dp_fisa_hex_dump_skb_data_enabled() - Check if
 * dp_fisa_hex_dump_skb_data tracepoint is enabled
 *
 * Return: true if enabled, false otherwise
 */
static inline
bool qdf_trace_dp_fisa_hex_dump_skb_data_enabled(void)
{
	return __qdf_trace_dp_fisa_hex_dump_skb_data_enabled();
}

/**
 * qdf_trace_dp_fisa_hex_dump_skb_data() - Trace context for skb data dump
 * @reason: Reason code indicating the context (e.g., head or frag)
 * @ip: Pointer to IP header or related structure
 * @index: Index or offset associated with the dump
 * @skb: Pointer to the skb
 * @next: Pointer to the next skb (if any)
 * @frag_list: Pointer to the fragment list (if any)
 * @data: Pointer to the data to be dumped
 * @len: Total length mapped for the dump
 * @data_len: Actual data length in the region of interest
 *
 * This function triggers the dp_fisa_hex_dump_skb_data tracepoint, which
 * logs pointers and metadata useful for correlating hex dumps of skb data
 * regions during FISA processing.
 *
 * Return: None
 */
static inline
void qdf_trace_dp_fisa_hex_dump_skb_data(uint8_t reason, const void *ip,
					 uint32_t index, const void *skb,
					 const void *next,
					 const void *frag_list,
					 const void *data, uint32_t len,
					 uint32_t data_len)
{
	__qdf_trace_dp_fisa_hex_dump_skb_data(reason, ip, index, skb, next,
					      frag_list, data, len, data_len);
}

/**
 * qdf_trace_dp_fisa_udp_aggr_append_fail_enabled() - Check if
 * dp_fisa_udp_aggr_append_fail tracepoint is enabled
 *
 * Return: true if enabled, false otherwise
 */
static inline
bool qdf_trace_dp_fisa_udp_aggr_append_fail_enabled(void)
{
	return __qdf_trace_dp_fisa_udp_aggr_append_fail_enabled();
}

/**
 * qdf_trace_dp_fisa_udp_aggr_append_fail() - Trace UDP aggregation append
 * failure
 * @flow_id: Flow identifier
 * @nbuf: Pointer to the buffer (nbuf) that failed to append
 * @len: Length of the buffer
 * @head_skb: Pointer to the head skb of the aggregation
 * @udp_aggr_append_fail: UDP aggregation append failure count
 *
 * This function triggers the dp_fisa_udp_aggr_append_fail tracepoint,
 * which logs failures encountered while appending buffers to an ongoing
 * UDP aggregation flow.
 *
 * Return: None
 */
static inline
void qdf_trace_dp_fisa_udp_aggr_append_fail(uint32_t flow_id, const void *nbuf,
					    uint32_t len, const void *head_skb,
					    uint32_t udp_aggr_append_fail)
{
	__qdf_trace_dp_fisa_udp_aggr_append_fail(flow_id, nbuf, len, head_skb,
						 udp_aggr_append_fail);
}

/**
 * qdf_trace_dp_fisa_udp_flush_fail_summary_enabled() - Check if
 * dp_fisa_udp_flush_fail_summary tracepoint is enabled
 *
 * Return: true if enabled, false otherwise
 */
static inline
bool qdf_trace_dp_fisa_udp_flush_fail_summary_enabled(void)
{
	return __qdf_trace_dp_fisa_udp_flush_fail_summary_enabled();
}

/**
 * qdf_trace_dp_fisa_udp_flush_fail_summary() - Trace summary of UDP
 * flush failures
 * @linear_fail: Count of linear skb flush failures to OSIF RX
 * @sanity_drop: Count of sanity check drops (e.g., length mismatch)
 * @nonlinear_fail: Count of non-linear skb flush failures to OSIF RX
 *
 * This function triggers the dp_fisa_udp_flush_fail_summary tracepoint,
 * which logs aggregated counters summarizing different categories of
 * UDP flush failures.
 *
 * Return: None
 */
static inline
void qdf_trace_dp_fisa_udp_flush_fail_summary(uint32_t linear_fail,
					      uint32_t sanity_drop,
					      uint32_t nonlinear_fail)
{
	__qdf_trace_dp_fisa_udp_flush_fail_summary(linear_fail, sanity_drop,
						   nonlinear_fail);
}

/**
 * qdf_trace_dp_fisa_udp_flush_stats_enabled() - Check if
 * dp_fisa_udp_flush_stats tracepoint is enabled
 *
 * Return: true if enabled, false otherwise
 */
static inline
bool qdf_trace_dp_fisa_udp_flush_stats_enabled(void)
{
	return __qdf_trace_dp_fisa_udp_flush_stats_enabled();
}

/**
 * qdf_trace_dp_fisa_udp_flush_stats() - Trace FISA UDP flush statistics
 * @flow_id: Flow identifier
 * @reason_code: Reason code indicating which stat was updated
 *               1 - flush_vdev_ref_fail
 *               2 - udp_flush_linear_osif_rx_fail
 *               3 - udp_flush_sanity_len_mismatch_drop
 *               4 - udp_flush_nonlinear_osif_rx_fail
 * @flush_vdev_ref_fail: Count of vdev reference failures
 * @udp_flush_linear_osif_rx_fail: Count of linear skb OSIF RX failures
 * @udp_flush_sanity_len_mismatch_drop: Count of sanity check drops
 * @udp_flush_nonlinear_osif_rx_fail: Count of non-linear skb OSIF RX failures
 *
 * This function triggers the dp_fisa_udp_flush_stats tracepoint, which logs
 * all FISA UDP flush statistics in a single trace point with a reason code
 * to identify which specific stat was updated.
 *
 * Return: None
 */
static inline
void qdf_trace_dp_fisa_udp_flush_stats(uint32_t flow_id, uint8_t reason_code,
				       uint32_t flush_vdev_ref_fail,
				       uint32_t udp_flush_linear_osif_rx_fail,
				       uint32_t udp_flush_sanity_len_mismatch_drop,
				       uint32_t udp_flush_nonlinear_osif_rx_fail)
{
	__qdf_trace_dp_fisa_udp_flush_stats(flow_id, reason_code,
					    flush_vdev_ref_fail,
					    udp_flush_linear_osif_rx_fail,
					    udp_flush_sanity_len_mismatch_drop,
					    udp_flush_nonlinear_osif_rx_fail);
}

/**
 * qdf_trace_dp_fisa_osif_rx_fail_enabled() - Check if dp_fisa_osif_rx_fail
 * tracepoint is enabled
 *
 * Return: true if enabled, false otherwise
 */
static inline
bool qdf_trace_dp_fisa_osif_rx_fail_enabled(void)
{
	return __qdf_trace_dp_fisa_osif_rx_fail_enabled();
}

/**
 * qdf_trace_dp_fisa_osif_rx_fail() - Trace osif_rx failure in FISA RX path
 * @flow_id: Flow identifier (fisa_flow->flow_id)
 * @vdev_id: VDEV ID (vdev->vdev_id)
 * @rx_ctx_id: REO/NAPI context id (QDF_NBUF_CB_RX_CTX_ID(nbuf))
 * @nbuf: nbuf pointer that failed delivery
 * @osif_rx_fail: current osif_rx_fail counter (after increment)
 *
 * This function triggers the dp_fisa_osif_rx_fail tracepoint, providing a
 * dedicated trace hook whenever dp_fisa_rx encounters osif_rx failure and
 * increments fisa_flow->osif_rx_fail.
 *
 * Return: None
 */
static inline
void qdf_trace_dp_fisa_osif_rx_fail(uint32_t flow_id, uint8_t vdev_id,
				    uint8_t rx_ctx_id, const void *nbuf,
				    uint32_t osif_rx_fail)
{
	__qdf_trace_dp_fisa_osif_rx_fail(flow_id, vdev_id, rx_ctx_id, nbuf,
					 osif_rx_fail);
}

/**
 * qdf_trace_dp_band_link_id_enabled() - Check if dp_band_link_id
 * tracepoint is enabled
 *
 * Return: true if enabled, false otherwise
 */
static inline
bool qdf_trace_dp_band_link_id_enabled(void)
{
	return __qdf_trace_dp_band_link_id_enabled();
}

/**
 * qdf_trace_dp_band_link_id() - Trace band and link_id information
 * @band: Band
 * @link_id: Link ID
 *
 * This function triggers the dp_band_link_id tracepoint, which logs
 * the band and link_id.
 *
 * Return: None
 */
static inline
void qdf_trace_dp_band_link_id(uint8_t band, uint8_t link_id)
{
	__qdf_trace_dp_band_link_id(band, link_id);
}

/**
 * qdf_trace_dp_band_enabled() - Check if dp_band tracepoint is enabled
 *
 * Return: true if enabled, false otherwise
 */
static inline
bool qdf_trace_dp_band_enabled(void)
{
	return __qdf_trace_dp_band_enabled();
}

/**
 * qdf_trace_dp_band() - Trace band information
 * @band: Band value (uint8_t)
 *
 * This function triggers the dp_band tracepoint, which logs
 * the band.
 *
 * Return: None
 */
static inline
void qdf_trace_dp_band(uint8_t band)
{
	__qdf_trace_dp_band(band);
}

/**
 * qdf_trace_dp_peer_info_enabled() - Check if dp_peer_info tracepoint
 * is enabled
 *
 * Return: true if enabled, false otherwise
 */
static inline
bool qdf_trace_dp_peer_info_enabled(void)
{
	return __qdf_trace_dp_peer_info_enabled();
}

/**
 * qdf_trace_dp_peer_info() - Trace peer info
 * @freq: Peer frequency
 * @vdev_id: VDEV ID
 * @peer_mac: Peer MAC address
 *
 * This function triggers the dp_peer_info tracepoint, which logs
 * peer frequency, vdev ID, peer MAC address, link ID validity,
 * and local link ID.
 *
 * Return: None
 */
static inline
void qdf_trace_dp_peer_info(uint32_t freq, uint8_t vdev_id,
			    const uint8_t *peer_mac)
{
	__qdf_trace_dp_peer_info(freq, vdev_id, peer_mac);
}

/**
 * qdf_trace_dp_band_link_peer_info_enabled() - Check dp_band_link_peer_info
 * tracepoint is enabled
 *
 * Return: true if enabled, false otherwise
 */
static inline
bool qdf_trace_dp_band_link_peer_info_enabled(void)
{
	return __qdf_trace_dp_band_link_peer_info_enabled();
}

/**
 * qdf_trace_dp_band_link_peer_info() - Trace band, link ID, and peer MAC info
 * @freq: Frequency
 * @band: Band
 * @link_id: Link ID
 * @peer_mac: Peer MAC address
 *
 * This function triggers the dp_band_link_peer_info tracepoint, which logs
 * frequency, band, link ID, and peer MAC address.
 *
 * Return: None
 */
static inline
void qdf_trace_dp_band_link_peer_info(uint32_t freq, uint8_t band,
				      uint8_t link_id, const uint8_t *peer_mac)
{
	__qdf_trace_dp_band_link_peer_info(freq, band, link_id, peer_mac);
}

/**
 * qdf_trace_dp_peer_link_info_enabled() - Check if dp_peer_link_info
 * tracepoint is enabled
 *
 * Return: true if enabled, false otherwise
 */
static inline
bool qdf_trace_dp_peer_link_info_enabled(void)
{
	return __qdf_trace_dp_peer_link_info_enabled();
}

/**
 * qdf_trace_dp_peer_link_info() - Trace peer link info
 * @link_id: Link ID
 * @freq: Frequency
 * @link_id_valid: Link ID valid flag
 * @peer_mac: Peer MAC address
 *
 * This function triggers the dp_peer_link_info tracepoint, which logs
 * link ID, frequency, link ID validity, and peer MAC address.
 *
 * Return: None
 */
static inline
void qdf_trace_dp_peer_link_info(uint8_t link_id, uint32_t freq,
				 uint8_t link_id_valid, const uint8_t *peer_mac)
{
	__qdf_trace_dp_peer_link_info(link_id, freq, link_id_valid, peer_mac);
}

/**
 * qdf_trace_dp_rx_tcp_pkt_enabled() - Get the dp_rx_tcp_pkt tracepoint
 *  enabled or disabled state
 *
 * Return: True if the tracepoint is enabled else false
 */
static inline
bool qdf_trace_dp_rx_tcp_pkt_enabled(void)
{
	return __qdf_trace_dp_rx_tcp_pkt_enabled();
}

/**
 * qdf_trace_dp_rx_tcp_pkt() - Trace tcp packet in rx direction
 * @nbuf: pointer to network buffer
 * @tcp_seq_num: TCP sequence number
 * @tcp_ack_num: TCP acknowledgment number
 * @srcport: TCP source port
 * @dstport: TCP destination port
 * @latency: latency in milliseconds
 * @status: Rx status
 *
 * Return: None
 */
static inline
void qdf_trace_dp_rx_tcp_pkt(qdf_nbuf_t nbuf, uint32_t tcp_seq_num,
			     uint32_t tcp_ack_num, uint16_t srcport,
			     uint16_t dstport, uint64_t latency,
			     uint8_t status)
{
	__qdf_trace_dp_rx_tcp_pkt(nbuf, tcp_seq_num, tcp_ack_num,
				  srcport, dstport, latency, status);
}

/**
 * qdf_trace_dp_tx_comp_tcp_pkt_enabled() - Get the dp_tx_comp_tcp_pkt
 *  tracepoint enabled or disabled state
 *
 * Return: True if the tracepoint is enabled else false
 */
static inline
bool qdf_trace_dp_tx_comp_tcp_pkt_enabled(void)
{
	return __qdf_trace_dp_tx_comp_tcp_pkt_enabled();
}

/**
 * qdf_trace_dp_tx_comp_tcp_pkt() - Trace tcp packet in tx completion
 * @nbuf: pointer to network buffer
 * @tcp_seq_num: TCP sequence number
 * @tcp_ack_num: TCP acknowledgment number
 * @srcport: TCP source port
 * @dstport: TCP destination port
 * @latency: latency in milliseconds
 * @status: Tx status
 *
 * Return: None
 */
static inline
void qdf_trace_dp_tx_comp_tcp_pkt(qdf_nbuf_t nbuf, uint32_t tcp_seq_num,
				  uint32_t tcp_ack_num, uint16_t srcport,
				  uint16_t dstport, uint64_t latency,
				  uint8_t status)
{
	__qdf_trace_dp_tx_comp_tcp_pkt(nbuf, tcp_seq_num, tcp_ack_num, srcport,
				       dstport, latency, status);
}

/**
 * qdf_trace_dp_rx_udp_pkt_enabled() - Get the dp_rx_udp_pkt tracepoint
 *  enabled or disabled state
 *
 * Return: True if the tracepoint is enabled else false
 */
static inline
bool qdf_trace_dp_rx_udp_pkt_enabled(void)
{
	return __qdf_trace_dp_rx_udp_pkt_enabled();
}

/**
 * qdf_trace_dp_rx_udp_pkt() - Trace udp packet in rx direction
 * @nbuf: pointer to network buffer
 * @ip_id: ip identification field
 * @srcport: UDP source port
 * @dstport: UDP destination port
 * @latency: latency in milliseconds
 * @status: Rx status
 *
 * Return: None
 */
static inline
void qdf_trace_dp_rx_udp_pkt(qdf_nbuf_t nbuf, uint16_t ip_id,
			     uint16_t srcport, uint16_t dstport,
			     uint64_t latency, uint8_t status)
{
	__qdf_trace_dp_rx_udp_pkt(nbuf, ip_id, srcport, dstport, latency,
				  status);
}

/**
 * qdf_trace_dp_tx_comp_udp_pkt_enabled() - Get the dp_tx_comp_udp_pkt
 *  tracepoint enabled or disabled state
 *
 * Return: True if the tracepoint is enabled else false
 */
static inline
bool qdf_trace_dp_tx_comp_udp_pkt_enabled(void)
{
	return __qdf_trace_dp_tx_comp_udp_pkt_enabled();
}

/**
 * qdf_trace_dp_tx_comp_udp_pkt() - Trace udp packet in tx completion
 * @nbuf: pointer to network buffer
 * @ip_id: ip identification field
 * @srcport: UDP source port
 * @dstport: UDP destination port
 * @latency: latency in milliseconds
 * @status: Tx status
 *
 * Return: None
 */
static inline
void qdf_trace_dp_tx_comp_udp_pkt(qdf_nbuf_t nbuf, uint16_t ip_id,
				  uint16_t srcport, uint16_t dstport,
				  uint64_t latency, uint8_t status)
{
	__qdf_trace_dp_tx_comp_udp_pkt(nbuf, ip_id, srcport, dstport, latency,
				       status);
}

/**
 * qdf_trace_dp_rx_pkt_enabled() - Get the dp_rx_pkt tracepoint
 *  enabled or disabled state
 *
 * Return: True if the tracepoint is enabled else false
 */
static inline
bool qdf_trace_dp_rx_pkt_enabled(void)
{
	return __qdf_trace_dp_rx_pkt_enabled();
}

/**
 * qdf_trace_dp_rx_pkt() - Trace non-tcp/udp packet in rx direction
 * @nbuf: pointer to network buffer
 * @ether_type: type in ethernet header
 * @latency: latency in milliseconds
 *
 * Return: None
 */
static inline
void qdf_trace_dp_rx_pkt(qdf_nbuf_t nbuf, uint16_t ether_type,
			 uint64_t latency)
{
	__qdf_trace_dp_rx_pkt(nbuf, ether_type, latency);
}

/**
 * qdf_trace_dp_tx_comp_pkt_enabled() - Get the dp_tx_comp_pkt tracepoint
 *  enabled or disabled state
 *
 * Return: True if the tracepoint is enabled else false
 */
static inline
bool qdf_trace_dp_tx_comp_pkt_enabled(void)
{
	return __qdf_trace_dp_tx_comp_pkt_enabled();
}

/**
 * qdf_trace_dp_tx_comp_pkt() - Trace non-tcp/udp packet in rx direction
 * @nbuf: pointer to network buffer
 * @ether_type: type in ethernet header
 * @latency: latency in milliseconds
 *
 * Return: None
 */
static inline
void qdf_trace_dp_tx_comp_pkt(qdf_nbuf_t nbuf, uint16_t ether_type,
			      uint64_t latency)
{
	__qdf_trace_dp_tx_comp_pkt(nbuf, ether_type, latency);
}

/**
 * qdf_trace_dp_del_reg_write_enabled() - Get the dp_del_reg_write tracepoint
 *  enabled or disabled state
 *
 * Return: True if the tracepoint is enabled else false
 */
static inline
bool qdf_trace_dp_del_reg_write_enabled(void)
{
	return __qdf_trace_dp_del_reg_write_enabled();
}

/**
 * qdf_trace_dp_del_reg_write() - Trace delayed register writes
 * @srng_id: srng id
 * @enq_val: enqueue value
 * @deq_val: dequeue value
 * @sched_time: scheduled time
 * @enq_time: enqueue time in qtimer ticks
 * @deq_time: dequeue time in qtimer ticks
 *
 * Return: None
 */
static inline
void qdf_trace_dp_del_reg_write(uint8_t srng_id, uint32_t enq_val,
				uint32_t deq_val, uint64_t sched_time,
				uint64_t enq_time, uint64_t deq_time)
{
	__qdf_trace_dp_del_reg_write(srng_id, enq_val, deq_val, sched_time,
				     enq_time, deq_time);
}

/**
 * qdf_trace_dp_tx_enqueue_enabled() - Get the dp_tx_enqueue tracepoint
 *  enabled or disabled state
 *
 * Return: True if the tracepoint is enabled else false
 */
static inline
bool qdf_trace_dp_tx_enqueue_enabled(void)
{
	return __qdf_trace_dp_tx_enqueue_enabled();
}

/**
 * qdf_trace_dp_tx_enqueue() - Trace dp_tx_enqueue
 * @nbuf: pointer to network buffer
 * @hp: head idx
 * @ring_id: TCL ring id
 * @coalesce: TCL register write coalescing
 *
 * Return: None
 */
static inline
void qdf_trace_dp_tx_enqueue(qdf_nbuf_t nbuf, uint32_t hp, uint8_t ring_id,
			     int coalesce)
{
	__qdf_trace_dp_tx_enqueue(nbuf, hp, ring_id, coalesce);
}

/**
 * qdf_trace_dp_tx_pp_alloc_enabled() - Check if dp_tx_pp_alloc tracepoint
 *  is enabled
 *
 * Return: True if the tracepoint is enabled else false
 */
static inline
bool qdf_trace_dp_tx_pp_alloc_enabled(void)
{
	return __qdf_trace_dp_tx_pp_alloc_enabled();
}

/**
 * qdf_trace_dp_tx_pp_alloc() - Trace page pool allocation
 * @pp: Page pool pointer
 * @pool_id: Pool ID
 * @offset: Buffer offset in page
 * @from_cache: Whether allocation was from cache
 * @latency_ns: Latency in nanoseconds
 * @loop_count: Number of pools checked before allocation
 *
 * Return: None
 */
static inline
void qdf_trace_dp_tx_pp_alloc(void *pp, uint32_t pool_id, uint32_t offset,
			      bool from_cache, uint64_t latency_ns,
			      uint8_t loop_count)
{
	__qdf_trace_dp_tx_pp_alloc(pp, pool_id, offset, from_cache,
				   latency_ns, loop_count);
}

/**
 * qdf_trace_dp_tx_pp_grow_enabled() - Check if dp_tx_pp_grow tracepoint
 *  is enabled
 *
 * Return: True if the tracepoint is enabled else false
 */
static inline
bool qdf_trace_dp_tx_pp_grow_enabled(void)
{
	return __qdf_trace_dp_tx_pp_grow_enabled();
}

/**
 * qdf_trace_dp_tx_pp_grow() - Trace page pool growth
 * @pp: Page pool pointer
 * @pool_id: Pool ID
 * @new_size: Size after growth
 * @latency_ns: Latency in nanoseconds
 *
 * Return: None
 */
static inline
void qdf_trace_dp_tx_pp_grow(void *pp, uint32_t pool_id,
			     uint32_t new_size, uint64_t latency_ns)
{
	__qdf_trace_dp_tx_pp_grow(pp, pool_id, new_size, latency_ns);
}

/**
 * qdf_trace_dp_tx_pp_attach_idle_enabled() - Check if dp_tx_pp_attach_idle
 *  tracepoint is enabled
 *
 * Return: True if the tracepoint is enabled else false
 */
static inline
bool qdf_trace_dp_tx_pp_attach_idle_enabled(void)
{
	return __qdf_trace_dp_tx_pp_attach_idle_enabled();
}

/**
 * qdf_trace_dp_tx_pp_attach_idle() - Trace idle pool attachment
 * @pp: Page pool pointer
 * @pool_id: Pool ID
 * @from_ho: Whether from high-order idle pool
 * @active_count: New active pool count
 * @latency_ns: Latency in nanoseconds
 *
 * Return: None
 */
static inline
void qdf_trace_dp_tx_pp_attach_idle(void *pp, uint32_t pool_id, bool from_ho,
				    uint32_t active_count, uint64_t latency_ns)
{
	__qdf_trace_dp_tx_pp_attach_idle(pp, pool_id, from_ho, active_count,
					 latency_ns);
}

/**
 * qdf_trace_dp_ce_tasklet_sched_latency_enabled() - Get the
 *   dp_ce_tasklet_sched_latency tracepoint enabled or disabled state
 *
 * Return: True if the tracepoint is enabled else false
 */
static inline
bool qdf_trace_dp_ce_tasklet_sched_latency_enabled(void)
{
	return __qdf_trace_dp_ce_tasklet_sched_latency_enabled();
}

/**
 * qdf_trace_dp_ce_tasklet_sched_latency() - Trace ce tasklet scheduling
 *  latency
 * @ce_id: ce id
 * @sched_latency: ce tasklet sched latency in nanoseconds
 *
 * Return: None
 */
static inline void
qdf_trace_dp_ce_tasklet_sched_latency(uint8_t ce_id, uint64_t sched_latency)
{
	__qdf_trace_dp_ce_tasklet_sched_latency(ce_id, sched_latency);
}

/**
 * qdf_trace_hif_hist_event_enabled() - Get the hif event tracepoint
 *  enabled or disabled state
 *
 * Return: True if the tracepoint is enabled else false
 */
static inline
bool qdf_trace_hif_hist_event_enabled(void)
{
	return __qdf_trace_hif_hist_event_enabled();
}

/**
 * qdf_trace_hif_hist_event() - Trace hif history event
 *  latency
 * @ce: copy engine or not
 * @hal_ring_id: ring id
 * @hp: ring hp
 * @tp: ring tp
 * @cpu_id: cpu id
 * @timestamp: time stamp
 * @type: event type
 *
 * Return: None
 */
static inline void
qdf_trace_hif_hist_event(bool ce, uint8_t hal_ring_id, uint32_t hp,
			 uint32_t tp, int cpu_id, uint64_t timestamp,
			 uint8_t type)
{
	__qdf_trace_hif_hist_event(ce, hal_ring_id, hp, tp, cpu_id,
				   timestamp, type);
}
#endif /* _QDF_TRACEPOINT_H */
