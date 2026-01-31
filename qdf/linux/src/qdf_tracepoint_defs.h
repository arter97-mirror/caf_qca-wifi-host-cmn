/*
 * Copyright (c) 2021-2024 Qualcomm Innovation Center, Inc. All rights reserved.
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
 * DOC: qdf_tracepoint_defs.h
 * This file provides OS abstraction for function tracing.
 */

#if  !defined(_QDF_TRACEPOINT_DEFS_H) || defined(TRACE_HEADER_MULTI_READ)
#define _QDF_TRACEPOINT_DEFS_H

#include <linux/tracepoint.h>
#include <linux/skbuff.h>

#undef TRACE_SYSTEM
#define TRACE_SYSTEM wlan

#ifndef WLAN_TRACEPOINTS
#undef TRACE_EVENT
#define TRACE_EVENT(name, proto, args, tstruct, assign, print) \
	static inline void trace_##name(proto) {}              \
	static inline bool trace_##name##_enabled(void)        \
	{                                                      \
		return false;                                  \
	}

#undef DECLARE_EVENT_CLASS
#define DECLARE_EVENT_CLASS(name, proto, args, tstruct, assign, print)

#undef DEFINE_EVENT
#define DEFINE_EVENT(evt_class, name, proto, args)      \
	static inline void trace_##name(proto) {}       \
	static inline bool trace_##name##_enabled(void) \
	{                                               \
		return false;                           \
	}
#endif /* WLAN_TRACEPOINTS */

DECLARE_EVENT_CLASS(dp_trace_tcp_pkt_class,
		    TP_PROTO(struct sk_buff *skb, uint32_t tcp_seq_num,
			     uint32_t tcp_ack_num, uint16_t srcport,
			     uint16_t dstport, uint64_t tdelta, uint8_t status),
		    TP_ARGS(skb, tcp_seq_num, tcp_ack_num, srcport,
			    dstport, tdelta, status),
		    TP_STRUCT__entry(
			__field(void *, skb)
			__field(uint32_t, tcp_seq_num)
			__field(uint32_t, tcp_ack_num)
			__field(uint16_t, srcport)
			__field(uint16_t, dstport)
			__field(uint64_t, tdelta)
			__field(uint8_t, status)
		    ),
		    TP_fast_assign(
			__entry->skb = skb;
			__entry->tcp_seq_num = tcp_seq_num;
			__entry->tcp_ack_num = tcp_ack_num;
			__entry->srcport = srcport;
			__entry->dstport = dstport;
			__entry->tdelta = tdelta;
			__entry->status = status;
		    ),
		    TP_printk("skb=%pK seqnum=%u acknum=%u srcport=%u dstport=%u latency(us)=%llu status=%d",
			      __entry->skb, __entry->tcp_seq_num,
			      __entry->tcp_ack_num, __entry->srcport,
			      __entry->dstport, __entry->tdelta,
			      __entry->status)
);

DEFINE_EVENT(dp_trace_tcp_pkt_class, dp_rx_tcp_pkt,
	     TP_PROTO(struct sk_buff *skb, uint32_t tcp_seq_num,
		      uint32_t tcp_ack_num, uint16_t srcport,
		      uint16_t dstport, uint64_t tdelta, uint8_t status),
	     TP_ARGS(skb, tcp_seq_num, tcp_ack_num, srcport, dstport, tdelta,
		     status)
);

DEFINE_EVENT(dp_trace_tcp_pkt_class, dp_tx_comp_tcp_pkt,
	     TP_PROTO(struct sk_buff *skb, uint32_t tcp_seq_num,
		      uint32_t tcp_ack_num, uint16_t srcport,
		      uint16_t dstport, uint64_t tdelta, uint8_t status),
	     TP_ARGS(skb, tcp_seq_num, tcp_ack_num, srcport, dstport, tdelta,
		     status)
);

DECLARE_EVENT_CLASS(dp_trace_udp_pkt_class,
		    TP_PROTO(struct sk_buff *skb, uint16_t ip_id,
			     uint16_t srcport, uint16_t dstport,
			     uint64_t tdelta, uint8_t status),
		    TP_ARGS(skb, ip_id, srcport, dstport, tdelta, status),
		    TP_STRUCT__entry(
			__field(void *, skb)
			__field(uint16_t, ip_id)
			__field(uint16_t, srcport)
			__field(uint16_t, dstport)
			__field(uint64_t, tdelta)
			__field(uint8_t, status)
		    ),
		    TP_fast_assign(
			__entry->skb = skb;
			__entry->ip_id = ip_id;
			__entry->srcport = srcport;
			__entry->dstport = dstport;
			__entry->tdelta = tdelta;
			__entry->status = status;
		    ),
		    TP_printk("skb=%pK ip_id=%u srcport=%u dstport=%d latency(us)=%llu status=%d",
			      __entry->skb, __entry->ip_id,
			      __entry->srcport, __entry->dstport,
			      __entry->tdelta, __entry->status)
);

DEFINE_EVENT(dp_trace_udp_pkt_class, dp_rx_udp_pkt,
	     TP_PROTO(struct sk_buff *skb, uint16_t ip_id, uint16_t srcport,
		      uint16_t dstport, uint64_t tdelta, uint8_t status),
	     TP_ARGS(skb, ip_id, srcport, dstport, tdelta, status)
);

DEFINE_EVENT(dp_trace_udp_pkt_class, dp_tx_comp_udp_pkt,
	     TP_PROTO(struct sk_buff *skb, uint16_t ip_id, uint16_t srcport,
		      uint16_t dstport, uint64_t tdelta, uint8_t status),
	     TP_ARGS(skb, ip_id, srcport, dstport, tdelta, status)
);

DECLARE_EVENT_CLASS(dp_trace_generic_ip_pkt_class,
		    TP_PROTO(struct sk_buff *skb, uint8_t ip_proto,
			     uint16_t ip_id, uint32_t trans_hdr_4_bytes,
			     uint64_t tdelta),
		    TP_ARGS(skb, ip_proto, ip_id,
			    trans_hdr_4_bytes, tdelta),
		    TP_STRUCT__entry(
			__field(void *, skb)
			__field(uint8_t, ip_proto)
			__field(uint16_t, ip_id)
			__field(uint32_t, trans_hdr_4_bytes)
			__field(uint64_t, tdelta)
		    ),
		    TP_fast_assign(
			__entry->skb = skb;
			__entry->ip_proto = ip_proto;
			__entry->ip_id = ip_id;
			__entry->trans_hdr_4_bytes = trans_hdr_4_bytes;
			__entry->tdelta = tdelta;
		    ),
		    TP_printk("skb=%pK ip_proto=0x%x ip_id=0x%x, transport_hdr[4]:0x%08x, latency(us)=%llu",
			      __entry->skb, __entry->ip_proto,
			      __entry->ip_id,  __entry->trans_hdr_4_bytes,
			      __entry->tdelta)
);

DEFINE_EVENT(dp_trace_generic_ip_pkt_class, dp_rx_generic_ip_pkt,
	     TP_PROTO(struct sk_buff *skb, uint8_t ip_proto,
		      uint16_t ip_id, uint32_t trans_hdr_4_bytes,
		      uint64_t tdelta),
	     TP_ARGS(skb, ip_proto, ip_id,
		     trans_hdr_4_bytes,
		     tdelta)
);

DEFINE_EVENT(dp_trace_generic_ip_pkt_class, dp_tx_comp_generic_ip_pkt,
	     TP_PROTO(struct sk_buff *skb, uint8_t ip_proto,
		      uint16_t ip_id, uint32_t trans_hdr_4_bytes,
		      uint64_t tdelta),
	     TP_ARGS(skb, ip_proto, ip_id,
		     trans_hdr_4_bytes,
		     tdelta)
);

DECLARE_EVENT_CLASS(dp_trace_pkt_class,
		    TP_PROTO(struct sk_buff *skb, uint16_t ether_type,
			     uint64_t tdelta),
		    TP_ARGS(skb, ether_type, tdelta),
		    TP_STRUCT__entry(
			__field(void *, skb)
			__field(uint16_t, ether_type)
			__field(uint64_t, tdelta)
		    ),
		    TP_fast_assign(
			__entry->skb = skb;
			__entry->ether_type = ether_type;
			__entry->tdelta = tdelta;
		    ),
		    TP_printk("skb=%pK ether_type=0x%x latency(us)=%llu",
			      __entry->skb, __entry->ether_type,
			      __entry->tdelta)
);

DEFINE_EVENT(dp_trace_pkt_class, dp_rx_pkt,
	     TP_PROTO(struct sk_buff *skb, uint16_t ether_type,
		      uint64_t tdelta),
	     TP_ARGS(skb, ether_type, tdelta)
);

DEFINE_EVENT(dp_trace_pkt_class, dp_tx_comp_pkt,
	     TP_PROTO(struct sk_buff *skb, uint16_t ether_type,
		      uint64_t tdelta),
	     TP_ARGS(skb, ether_type, tdelta)
);

TRACE_EVENT(dp_peer_link_info,
	    TP_PROTO(uint8_t link_id, uint32_t freq, uint8_t link_id_valid,
		     const uint8_t *peer_mac),
	    TP_ARGS(link_id, freq, link_id_valid, peer_mac),
	    TP_STRUCT__entry(
		__field(uint8_t, link_id)
		__field(uint32_t, freq)
		__field(uint8_t, link_id_valid)
		__array(uint8_t, peer_mac, 6)
	    ),
	    TP_fast_assign(
		__entry->link_id = link_id;
		__entry->freq = freq;
		__entry->link_id_valid = link_id_valid;
		memcpy(__entry->peer_mac, peer_mac, sizeof(__entry->peer_mac));
	    ),
	    TP_printk("mac_addr = %02x:%02x:%02x:**:**:%02x, link_id = %u, freq = %d, link_id_valid = %u,",
		      __entry->peer_mac[0], __entry->peer_mac[1],
		      __entry->peer_mac[2], __entry->peer_mac[5],
		      __entry->link_id, __entry->freq, __entry->link_id_valid)
);

TRACE_EVENT(dp_band_link_peer_info,
	    TP_PROTO(uint32_t freq, uint8_t band, uint8_t link_id,
		     const uint8_t *peer_mac),
	    TP_ARGS(freq, band, link_id, peer_mac),
	    TP_STRUCT__entry(
		__field(uint32_t, freq)
		__field(uint8_t, band)
		__field(uint8_t, link_id)
		__array(uint8_t, peer_mac, 6)
	   ),
	   TP_fast_assign(
		__entry->freq = freq;
		__entry->band = band;
		__entry->link_id = link_id;
		memcpy(__entry->peer_mac, peer_mac, sizeof(__entry->peer_mac));
	   ),
	   TP_printk("mac_addr: %02x:%02x:%02x:**:**:%02x, Band(Freq: %d): %u mapped to Link ID: %u",
		     __entry->peer_mac[0], __entry->peer_mac[1],
		     __entry->peer_mac[2], __entry->peer_mac[5],
		     __entry->freq, __entry->band, __entry->link_id)
);

TRACE_EVENT(dp_peer_info,
	    TP_PROTO(uint32_t freq, uint8_t vdev_id, const uint8_t *peer_mac),
	    TP_ARGS(freq, vdev_id, peer_mac),
	    TP_STRUCT__entry(
		__field(uint32_t, freq)
		__field(uint8_t, vdev_id)
		__array(uint8_t, peer_mac, 6)
	    ),
	    TP_fast_assign(
		__entry->freq = freq;
		__entry->vdev_id = vdev_id;
		memcpy(__entry->peer_mac, peer_mac, sizeof(__entry->peer_mac));
	    ),
	    TP_printk("mac_addr = %02x:%02x:%02x:**:**:%02x, freq = %d, vdev_id = %u",
		      __entry->peer_mac[0], __entry->peer_mac[1],
		      __entry->peer_mac[2], __entry->peer_mac[5],
		      __entry->freq, __entry->vdev_id)
);

TRACE_EVENT(dp_band,
	    TP_PROTO(uint8_t band),
	    TP_ARGS(band),
	    TP_STRUCT__entry(
		__field(uint8_t, band)
	    ),
	    TP_fast_assign(
		__entry->band = band;
	    ),
	    TP_printk("band=%u", __entry->band)
);

TRACE_EVENT(dp_band_link_id,
	    TP_PROTO(uint8_t band, uint8_t link_id),
	    TP_ARGS(band, link_id),
	    TP_STRUCT__entry(
		__field(uint8_t, band)
		__field(uint8_t, link_id)
	    ),
	    TP_fast_assign(
		__entry->band = band;
		__entry->link_id = link_id;
	    ),
	    TP_printk("band=%u link_id=%u", __entry->band, __entry->link_id)
);

TRACE_EVENT(dp_fisa_trace_rdi_invalid,
	    TP_PROTO(uint8_t reo_dest_ind),
	    TP_ARGS(reo_dest_ind),
	    TP_STRUCT__entry(
		__field(uint8_t, reo_dest_ind)
	    ),
	    TP_fast_assign(
		__entry->reo_dest_ind = reo_dest_ind;
	    ),
	    TP_printk("FISA drop: RDI is REO_REMAP_TCL (reo_dest_ind=%u)",
		      __entry->reo_dest_ind)
);

TRACE_EVENT(dp_fisa_flush_vdev_fail,
	    TP_PROTO(uint8_t head_vdev_id, uint8_t flow_vdev_id, uint8_t reason,
		     uint32_t flow_id, const void *head_vdev_ptr,
		     const void *flow_vdev_ptr,
		     uint32_t flush_head_vdev_ref_fail,
		     uint32_t flush_flow_vdev_ref_fail,
		     uint32_t flush_vdev_ptr_mismatch,
		     uint32_t flush_mld_mismatch_drop),
	    TP_ARGS(head_vdev_id, flow_vdev_id, reason, flow_id, head_vdev_ptr,
		    flow_vdev_ptr, flush_head_vdev_ref_fail,
		    flush_flow_vdev_ref_fail, flush_vdev_ptr_mismatch,
		    flush_mld_mismatch_drop),
	    TP_STRUCT__entry(
		__field(uint8_t,       head_vdev_id)
		__field(uint8_t,       flow_vdev_id)
		__field(uint8_t,       reason)
		__field(uint32_t,      flow_id)
		__field(const void *,  head_vdev_ptr)
		__field(const void *,  flow_vdev_ptr)
		__field(uint32_t,      flush_head_vdev_ref_fail)
		__field(uint32_t,      flush_flow_vdev_ref_fail)
		__field(uint32_t,      flush_vdev_ptr_mismatch)
		__field(uint32_t,      flush_mld_mismatch_drop)
	    ),
	    TP_fast_assign(
		__entry->head_vdev_id  = head_vdev_id;
		__entry->flow_vdev_id  = flow_vdev_id;
		__entry->reason        = reason;
		__entry->flow_id       = flow_id;
		__entry->head_vdev_ptr = head_vdev_ptr;
		__entry->flow_vdev_ptr = flow_vdev_ptr;
		__entry->flush_head_vdev_ref_fail = flush_head_vdev_ref_fail;
		__entry->flush_flow_vdev_ref_fail = flush_flow_vdev_ref_fail;
		__entry->flush_vdev_ptr_mismatch = flush_vdev_ptr_mismatch;
		__entry->flush_mld_mismatch_drop = flush_mld_mismatch_drop;
	    ),
	    TP_printk("reason=%u head_vdev_id=%u flow_vdev_id=%u flow_id=%u head_vdev_ptr=%pK flow_vdev_ptr=%pK flush_head_vdev_ref_fail=%u flush_flow_vdev_ref_fail=%u flush_vdev_ptr_mismatch=%u flush_mld_mismatch_drop=%u",
		      __entry->reason, __entry->head_vdev_id,
		      __entry->flow_vdev_id, __entry->flow_id,
		      __entry->head_vdev_ptr, __entry->flow_vdev_ptr,
		      __entry->flush_head_vdev_ref_fail,
		      __entry->flush_flow_vdev_ref_fail,
		      __entry->flush_vdev_ptr_mismatch,
		      __entry->flush_mld_mismatch_drop)
);

TRACE_EVENT(dp_fisa_hex_dump_skb_data,
	    TP_PROTO(uint8_t reason, const void *ip, uint32_t index,
		     const void *skb, const void *next, const void *frag_list,
		     const void *data, uint32_t len, uint32_t data_len),
	    TP_ARGS(reason, ip, index, skb, next, frag_list, data, len,
		    data_len),
	    TP_STRUCT__entry(
		__field(uint8_t,       reason)
		__field(const void *,  ip)
		__field(uint32_t,      index)
		__field(const void *,  skb)
		__field(const void *,  next)
		__field(const void *,  frag_list)
		__field(const void *,  data)
		__field(uint32_t,      len)
		__field(uint32_t,      data_len)
	    ),
	    TP_fast_assign(
		__entry->reason    = reason;
		__entry->ip        = ip;
		__entry->index     = index;
		__entry->skb       = skb;
		__entry->next      = next;
		__entry->frag_list = frag_list;
		__entry->data      = data;
		__entry->len       = len;
		__entry->data_len  = data_len;
	    ),
	    TP_printk("reason=%u index=%u ip=%ps skb=%pK next=%pK frag_list=%pK data=%pK len=%u data_len=%u",
		      __entry->reason, __entry->index, __entry->ip,
		      __entry->skb, __entry->next, __entry->frag_list,
		      __entry->data, __entry->len, __entry->data_len)
);

TRACE_EVENT(dp_fisa_udp_aggr_append_fail,
	    TP_PROTO(uint32_t flow_id, const void *nbuf, uint32_t len,
		     const void *head_skb, uint32_t udp_aggr_append_fail),
	    TP_ARGS(flow_id, nbuf, len, head_skb, udp_aggr_append_fail),
	    TP_STRUCT__entry(
		__field(uint32_t, flow_id)
		__field(const void *, nbuf)
		__field(uint32_t, len)
		__field(const void *, head_skb)
		__field(uint32_t, udp_aggr_append_fail)
	    ),
	    TP_fast_assign(
		__entry->flow_id = flow_id;
		__entry->nbuf    = nbuf;
		__entry->len     = len;
		__entry->head_skb = head_skb;
		__entry->udp_aggr_append_fail = udp_aggr_append_fail;
	    ),
	    TP_printk("flow_id=%u last_skb=NULL head_skb=%pK nbuf=%pK len=%u udp_aggr_append_fail=%u",
		      __entry->flow_id, __entry->head_skb, __entry->nbuf,
		      __entry->len, __entry->udp_aggr_append_fail)
);

TRACE_EVENT(dp_fisa_udp_flush_fail_summary,
	    TP_PROTO(uint32_t linear_fail, uint32_t sanity_drop,
		     uint32_t nonlinear_fail),
	    TP_ARGS(linear_fail, sanity_drop, nonlinear_fail),
	    TP_STRUCT__entry(
		__field(uint32_t, linear_fail)
		__field(uint32_t, sanity_drop)
		__field(uint32_t, nonlinear_fail)
	    ),
	    TP_fast_assign(
		__entry->linear_fail    = linear_fail;
		__entry->sanity_drop    = sanity_drop;
		__entry->nonlinear_fail = nonlinear_fail;
	    ),
	    TP_printk("udp_flush_linear_osif_rx_fail: %u udp_flush_sanity_len_mismatch_drop: %u udp_flush_nonlinear_osif_rx_fail: %u",
		      __entry->linear_fail,
		      __entry->sanity_drop,
		      __entry->nonlinear_fail)
);

TRACE_EVENT(dp_fisa_udp_flush_stats,
	    TP_PROTO(uint32_t flow_id, uint8_t reason_code,
		     uint32_t flush_vdev_ref_fail,
		     uint32_t udp_flush_linear_osif_rx_fail,
		     uint32_t udp_flush_sanity_len_mismatch_drop,
		     uint32_t udp_flush_nonlinear_osif_rx_fail),
	    TP_ARGS(flow_id, reason_code, flush_vdev_ref_fail,
		    udp_flush_linear_osif_rx_fail,
		    udp_flush_sanity_len_mismatch_drop,
		    udp_flush_nonlinear_osif_rx_fail),
	    TP_STRUCT__entry(
		__field(uint32_t, flow_id)
		__field(uint8_t, reason_code)
		__field(uint32_t, flush_vdev_ref_fail)
		__field(uint32_t, udp_flush_linear_osif_rx_fail)
		__field(uint32_t, udp_flush_sanity_len_mismatch_drop)
		__field(uint32_t, udp_flush_nonlinear_osif_rx_fail)
	    ),
	    TP_fast_assign(
		__entry->flow_id = flow_id;
		__entry->reason_code = reason_code;
		__entry->flush_vdev_ref_fail = flush_vdev_ref_fail;
		__entry->udp_flush_linear_osif_rx_fail = udp_flush_linear_osif_rx_fail;
		__entry->udp_flush_sanity_len_mismatch_drop = udp_flush_sanity_len_mismatch_drop;
		__entry->udp_flush_nonlinear_osif_rx_fail = udp_flush_nonlinear_osif_rx_fail;
	    ),
	    TP_printk("flow_id=%u reason_code=%u flush_vdev_ref_fail=%u udp_flush_linear_osif_rx_fail=%u udp_flush_sanity_len_mismatch_drop=%u udp_flush_nonlinear_osif_rx_fail=%u",
		      __entry->flow_id, __entry->reason_code,
		      __entry->flush_vdev_ref_fail,
		      __entry->udp_flush_linear_osif_rx_fail,
		      __entry->udp_flush_sanity_len_mismatch_drop,
		      __entry->udp_flush_nonlinear_osif_rx_fail)
);

TRACE_EVENT(dp_fisa_osif_rx_fail,
	    TP_PROTO(uint32_t flow_id, uint8_t vdev_id, uint8_t rx_ctx_id,
		     const void *nbuf, uint32_t osif_rx_fail),
	    TP_ARGS(flow_id, vdev_id, rx_ctx_id, nbuf, osif_rx_fail),
	    TP_STRUCT__entry(
		__field(uint32_t, flow_id)
		__field(uint8_t, vdev_id)
		__field(uint8_t, rx_ctx_id)
		__field(const void *, nbuf)
		__field(uint32_t, osif_rx_fail)
	    ),
	    TP_fast_assign(
		__entry->flow_id = flow_id;
		__entry->vdev_id = vdev_id;
		__entry->rx_ctx_id = rx_ctx_id;
		__entry->nbuf = nbuf;
		__entry->osif_rx_fail = osif_rx_fail;
	    ),
	    TP_printk("osif_rx_fail: flow_id=%u vdev_id=%u rx_ctx_id=%u nbuf=%pK osif_rx_fail=%u",
		      __entry->flow_id, __entry->vdev_id,
		      __entry->rx_ctx_id, __entry->nbuf,
		      __entry->osif_rx_fail)
);

TRACE_EVENT(dp_del_reg_write,
	    TP_PROTO(uint8_t srng_id, uint32_t enq_val, uint32_t deq_val,
		     uint64_t sched_time, uint64_t enq_time, uint64_t deq_time),
	    TP_ARGS(srng_id, enq_val, deq_val, sched_time, enq_time, deq_time),
	    TP_STRUCT__entry(
		__field(uint8_t, srng_id)
		__field(uint32_t, enq_val)
		__field(uint32_t, deq_val)
		__field(uint64_t, sched_time)
		__field(uint64_t, enq_time)
		__field(uint64_t, deq_time)
	    ),
	    TP_fast_assign(
		__entry->srng_id = srng_id;
		__entry->enq_val = enq_val;
		__entry->deq_val = deq_val;
		__entry->sched_time = sched_time;
		__entry->enq_time = enq_time;
		__entry->deq_time = deq_time;
	    ),
	    TP_printk("srng_id=%u enq_val=%u deq_val=%u sched_time=0x%llx enq_time=0x%llx deq_time=0x%llx",
		      __entry->srng_id, __entry->enq_val, __entry->deq_val,
		      __entry->sched_time, __entry->enq_time, __entry->deq_time)
);

TRACE_EVENT(dp_ce_tasklet_sched_latency,
	    TP_PROTO(uint8_t ce_id, uint64_t sched_latency),
	    TP_ARGS(ce_id, sched_latency),
	    TP_STRUCT__entry(
		__field(uint8_t, ce_id)
		__field(uint64_t, sched_latency)
	    ),
	    TP_fast_assign(
		__entry->ce_id = ce_id;
		__entry->sched_latency = sched_latency;
	    ),
	    TP_printk("ce_id=%u latency(ns)=%llu", __entry->ce_id,
		      __entry->sched_latency)
);

TRACE_EVENT(dp_tx_enqueue,
	    TP_PROTO(struct sk_buff *skb, uint32_t hp, uint8_t ring_id,
		     int coalesce),
	    TP_ARGS(skb, hp, ring_id, coalesce),
	    TP_STRUCT__entry(
		__field(void *, skb)
		__field(uint32_t, hp)
		__field(uint8_t, ring_id)
		__field(int, coalesce)
	    ),
	    TP_fast_assign(
		__entry->skb = skb;
		__entry->hp = hp;
		__entry->ring_id = ring_id;
		__entry->coalesce = coalesce;
	    ),
	    TP_printk("skb=%pK hp=%d ring_id=%d coalesce=%d", __entry->skb,
		      __entry->hp, __entry->ring_id, __entry->coalesce)
);

TRACE_EVENT(hif_hist_event,
	    TP_PROTO(bool ce, uint8_t hal_ring_id, uint32_t hp,
		     uint32_t tp, int cpu_id, uint64_t timestamp,
		     uint8_t type),
	    TP_ARGS(ce, hal_ring_id, hp, tp, cpu_id, timestamp, type),
	    TP_STRUCT__entry(
		__field(bool, ce)
		__field(uint8_t, hal_ring_id)
		__field(uint32_t, hp)
		__field(uint32_t, tp)
		__field(int, cpu_id)
		__field(uint64_t, timestamp)
		__field(uint8_t, type)
	    ),
	    TP_fast_assign(
		__entry->ce = ce;
		__entry->hal_ring_id = hal_ring_id;
		__entry->hp = hp;
		__entry->tp = tp;
		__entry->cpu_id = cpu_id;
		__entry->timestamp = timestamp;
		__entry->type = type;
	    ),
	    TP_printk("ce=%d ring=%d hp=0x%x tp=0x%x cpu=%d 0x%llx type=0x%x",
		      __entry->ce,
		      __entry->hal_ring_id,
		      __entry->hp, __entry->tp,
		      __entry->cpu_id,
		      __entry->timestamp,
		      __entry->type)
);

TRACE_EVENT(dp_tx_pp_alloc,
	    TP_PROTO(void *pp, uint32_t pool_id, uint32_t offset,
		     bool from_cache, uint64_t latency_ns, uint8_t loop_count),
	    TP_ARGS(pp, pool_id, offset, from_cache, latency_ns, loop_count),
	    TP_STRUCT__entry(
		__field(void *, pp)
		__field(uint32_t, pool_id)
		__field(uint32_t, offset)
		__field(bool, from_cache)
		__field(uint64_t, latency_ns)
		__field(uint8_t, loop_count)
	    ),
	    TP_fast_assign(
		__entry->pp = pp;
		__entry->pool_id = pool_id;
		__entry->offset = offset;
		__entry->from_cache = from_cache;
		__entry->latency_ns = latency_ns;
		__entry->loop_count = loop_count;
	    ),
	    TP_printk("pp=%pK pool_id=%u offset=%u from_cache=%d latency(ns)=%llu loop_count=%u",
		      __entry->pp, __entry->pool_id, __entry->offset,
		      __entry->from_cache, __entry->latency_ns,
		      __entry->loop_count)
);

TRACE_EVENT(dp_tx_pp_grow,
	    TP_PROTO(void *pp, uint32_t pool_id,
		     uint32_t new_size, uint64_t latency_ns),
	    TP_ARGS(pp, pool_id, new_size, latency_ns),
	    TP_STRUCT__entry(
		__field(void *, pp)
		__field(uint32_t, pool_id)
		__field(uint32_t, new_size)
		__field(uint64_t, latency_ns)
	    ),
	    TP_fast_assign(
		__entry->pp = pp;
		__entry->pool_id = pool_id;
		__entry->new_size = new_size;
		__entry->latency_ns = latency_ns;
	    ),
	    TP_printk("pp=%pK pool_id=%u new_size=%u latency(ns)=%llu",
		      __entry->pp, __entry->pool_id,
		      __entry->new_size, __entry->latency_ns)
);

TRACE_EVENT(dp_tx_pp_attach_idle,
	    TP_PROTO(void *pp, uint32_t pool_id, bool from_ho,
		     uint32_t active_count, uint64_t latency_ns),
	    TP_ARGS(pp, pool_id, from_ho, active_count, latency_ns),
	    TP_STRUCT__entry(
		__field(void *, pp)
		__field(uint32_t, pool_id)
		__field(bool, from_ho)
		__field(uint32_t, active_count)
		__field(uint64_t, latency_ns)
	    ),
	    TP_fast_assign(
		__entry->pp = pp;
		__entry->pool_id = pool_id;
		__entry->from_ho = from_ho;
		__entry->active_count = active_count;
		__entry->latency_ns = latency_ns;
	    ),
	    TP_printk("pp=%pK pool_id=%u from_ho=%d active_count=%u latency(ns)=%llu",
		      __entry->pp, __entry->pool_id, __entry->from_ho,
		      __entry->active_count, __entry->latency_ns)
);
#endif /* _QDF_TRACEPOINT_DEFS_H */

/* Below should be outside the protection */
#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH .
#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_FILE qdf_tracepoint_defs
#include <trace/define_trace.h>
