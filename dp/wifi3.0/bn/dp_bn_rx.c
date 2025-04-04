/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include "cdp_txrx_cmn_struct.h"
#include "hal_hw_headers.h"
#include "dp_types.h"
#include "dp_rx.h"
#include "dp_tx.h"
#include "dp_be_rx.h"
#include "dp_peer.h"
#include "hal_rx.h"
#include "hal_bn_rx.h"
#include "hal_be_rx.h"
#include "hal_api.h"
#include "hal_be_api.h"
#include "qdf_nbuf.h"
#ifdef MESH_MODE_SUPPORT
#include "if_meta_hdr.h"
#endif
#include "dp_internal.h"
#include "dp_ipa.h"
#ifdef FEATURE_WDS
#include "dp_txrx_wds.h"
#endif
#include "dp_hist.h"
#include "dp_rx_buffer_pool.h"
#include "hal_internal.h"

uint32_t dp_rx_process_bn(struct dp_intr *int_ctx,
			  hal_ring_handle_t hal_ring_hdl, uint8_t reo_ring_num,
			  uint32_t quota)
{
	hal_ring_desc_t ring_desc;
	hal_soc_handle_t hal_soc;
	struct dp_rx_desc *rx_desc = NULL;
	qdf_nbuf_t nbuf, next;
	bool near_full;
	union dp_rx_desc_list_elem_t *head[MAX_PDEV_CNT];
	union dp_rx_desc_list_elem_t *tail[MAX_PDEV_CNT];
	uint32_t num_pending = 0;
	uint32_t rx_bufs_used = 0, rx_buf_cookie;
	uint16_t msdu_len = 0;
	uint16_t peer_id;
	uint8_t vdev_id;
	struct dp_txrx_peer *txrx_peer = NULL;
	dp_txrx_ref_handle txrx_ref_handle = NULL;
	struct dp_vdev *vdev;
	uint32_t pkt_len = 0;
	enum hal_reo_error_status error;
	uint8_t *rx_tlv_hdr;
	uint32_t rx_bufs_reaped[MAX_PDEV_CNT];
	uint8_t mac_id = 0;
	struct dp_pdev *rx_pdev;
	uint8_t enh_flag;
	struct dp_srng *dp_rxdma_srng;
	struct rx_desc_pool *rx_desc_pool;
	struct dp_soc *soc = int_ctx->soc;
	struct cdp_tid_rx_stats *tid_stats;
	qdf_nbuf_t nbuf_head;
	qdf_nbuf_t nbuf_tail;
	qdf_nbuf_t deliver_list_head;
	qdf_nbuf_t deliver_list_tail;
	uint32_t num_rx_bufs_reaped = 0;
	uint32_t intr_id;
	struct hif_opaque_softc *scn;
	int32_t tid = 0;
	bool is_prev_msdu_last = true;
	uint32_t num_entries_avail = 0;
	uint32_t rx_ol_pkt_cnt = 0;
	uint32_t num_entries = 0;
	QDF_STATUS status;
	qdf_nbuf_t ebuf_head;
	qdf_nbuf_t ebuf_tail;
	uint8_t pkt_capture_offload = 0;
	struct dp_srng *rx_ring = &soc->reo_dest_ring[reo_ring_num];
	int max_reap_limit, ring_near_full;
	uint64_t current_time = 0;
	uint32_t old_tid;
	uint32_t peer_ext_stats;
	uint32_t dsf;
	uint32_t l3_pad;
	uint8_t link_id = 0;
	uint16_t buf_size;
	uint8_t is_ctrl_refill = 0;
	uint8_t buf_type;
	uint8_t cc_status;

	DP_HIST_INIT();

	if (!soc || !hal_ring_hdl)
		return 0;

	hal_soc = soc->hal_soc;
	if (!hal_soc)
		return 0;

	scn = soc->hif_handle;
	intr_id = int_ctx->dp_intr_id;
	num_entries = hal_srng_get_num_entries(hal_soc, hal_ring_hdl);
	dp_runtime_pm_mark_last_busy(soc);
	buf_size = wlan_cfg_rx_buffer_size(soc->wlan_cfg_ctx);

more_data:
	if (qdf_likely(txrx_peer))
		dp_txrx_peer_unref_delete(txrx_ref_handle, DP_MOD_ID_RX);

	/* reset local variables here to be re-used in the function */
	nbuf_head = NULL;
	nbuf_tail = NULL;
	deliver_list_head = NULL;
	deliver_list_tail = NULL;
	txrx_peer = NULL;
	vdev = NULL;
	num_rx_bufs_reaped = 0;
	ebuf_head = NULL;
	ebuf_tail = NULL;
	ring_near_full = 0;
	max_reap_limit = dp_rx_get_loop_pkt_limit(soc);

	qdf_mem_zero(rx_bufs_reaped, sizeof(rx_bufs_reaped));
	qdf_mem_zero(head, sizeof(head));
	qdf_mem_zero(tail, sizeof(tail));
	old_tid = 0xff;
	dsf = 0;
	peer_ext_stats = 0;
	rx_pdev = NULL;
	tid_stats = NULL;

	dp_pkt_get_timestamp(&current_time);

	ring_near_full = _dp_srng_test_and_update_nf_params(soc, rx_ring,
							    &max_reap_limit);

	peer_ext_stats = wlan_cfg_is_peer_ext_stats_enabled(soc->wlan_cfg_ctx);
	if (qdf_unlikely(dp_rx_srng_access_start(int_ctx, soc, hal_ring_hdl))) {
		/*
		 * Need API to convert from hal_ring pointer to
		 * Ring Type / Ring Id combo
		 */
		DP_STATS_INC(soc, rx.err.hal_ring_access_fail, 1);
		QDF_TRACE(QDF_MODULE_ID_TXRX, QDF_TRACE_LEVEL_ERROR,
			  FL("HAL RING Access Failed -- %pK"), hal_ring_hdl);
		goto done;
	}

	hal_srng_update_ring_usage_wm_no_lock(soc->hal_soc, hal_ring_hdl);

	if (!num_pending)
		num_pending = hal_srng_dst_num_valid(hal_soc, hal_ring_hdl, 0);

	if (num_pending > quota)
		num_pending = quota;

	/*
	 * start reaping the buffers from reo ring and queue
	 * them in per vdev queue.
	 * Process the received pkts in a different per vdev loop.
	 */
	while (qdf_likely(num_pending)) {
		ring_desc = dp_srng_dst_get_next(soc, hal_ring_hdl);

		if (qdf_unlikely(!ring_desc))
			break;

		error = HAL_RX_ERROR_STATUS_GET(ring_desc);

		if (qdf_unlikely(error == HAL_REO_ERROR_DETECTED)) {
			dp_rx_err("%pK: HAL RING 0x%pK:error %d",
				  soc, hal_ring_hdl, error);
			DP_STATS_INC(soc, rx.err.hal_reo_error[reo_ring_num],
				     1);
			/* Don't know how to deal with this -- assert */
			qdf_assert(0);
		}

		/* Only MSDU BUFFER type is expected in normal pkt path */
		buf_type = hal_rx_reo_buf_type_get(hal_soc, ring_desc);
		if (qdf_unlikely(buf_type != HAL_RX_REO_MSDU_BUF_ADDR_TYPE)) {
			DP_STATS_INC(soc, rx.err.reo_err_msdu_buf_rcved, 1);
			dp_err("msdu with wrong buffer type received!!!");
			continue;
		}

		cc_status = HAL_RX_REO_CC_STATUS_GET_BN(ring_desc);
		/* cookie conversion status 1, fetch VA directly */
		if (qdf_likely(cc_status)) {
			rx_desc = (struct dp_rx_desc *)
					hal_rx_get_reo_desc_va(ring_desc);
		} else {
			rx_desc = NULL;
			rx_buf_cookie = HAL_RX_BUF_COOKIE_GET(ring_desc);
			dp_rx_desc_sw_cc_check(soc, rx_buf_cookie, &rx_desc);
		}

		dp_rx_ring_record_entry(soc, reo_ring_num, ring_desc);

		status = dp_rx_desc_sanity(soc, hal_soc, hal_ring_hdl,
					   ring_desc, rx_desc);
		if (QDF_IS_STATUS_ERROR(status)) {
			if (qdf_unlikely(rx_desc && rx_desc->nbuf)) {
				qdf_assert_always(!rx_desc->unmapped);
				dp_rx_nbuf_unmap(soc, rx_desc, reo_ring_num);
				dp_rx_buffer_pool_nbuf_free(soc, rx_desc->nbuf,
							    rx_desc->pool_id);
				dp_rx_add_to_free_desc_list(
						&head[rx_desc->pool_id],
						&tail[rx_desc->pool_id],
						rx_desc);
			}
			continue;
		}

		/*
		 * this is a unlikely scenario where the host is reaping
		 * a descriptor which it already reaped just a while ago
		 * but is yet to replenish it back to HW.
		 * In this case host will dump the last 128 descriptors
		 * including the software descriptor rx_desc and assert.
		 */

		if (qdf_unlikely(!rx_desc->in_use)) {
			DP_STATS_INC(soc, rx.err.hal_reo_dest_dup, 1);
			dp_info_rl("Reaping rx_desc not in use!");
			dp_rx_dump_info_and_assert(soc, hal_ring_hdl,
						   ring_desc, rx_desc);
			continue;
		}

		if (qdf_unlikely(!dp_rx_desc_check_magic(rx_desc))) {
			DP_STATS_INC(soc, rx.err.rx_desc_invalid_magic, 1);
			dp_rx_dump_info_and_assert(soc, hal_ring_hdl,
						   ring_desc, rx_desc);
		}

		pkt_capture_offload =
			dp_rx_copy_desc_info_in_nbuf_cb(soc, ring_desc,
							rx_desc->nbuf,
							reo_ring_num,
							&is_ctrl_refill);
		if (is_ctrl_refill)
			goto refill_opt_dp_ctrl;

		if (qdf_unlikely(qdf_nbuf_is_rx_chfrag_cont(rx_desc->nbuf))) {
			/* In dp_rx_sg_create() until the last buffer,
			 * end bit should not be set. As continuation bit set,
			 * this is not a last buffer.
			 */
			qdf_nbuf_set_rx_chfrag_end(rx_desc->nbuf, 0);

			/* previous msdu has end bit set, so current one is
			 * the new MPDU
			 */
			if (is_prev_msdu_last) {
				/* Get number of entries available in HW ring */
				num_entries_avail =
				hal_srng_dst_num_valid(hal_soc,
						       hal_ring_hdl, 1);

				/* For new MPDU check if we can read complete
				 * MPDU by comparing the number of buffers
				 * available and number of buffers needed to
				 * reap this MPDU
				 */
				if ((QDF_NBUF_CB_RX_PKT_LEN(rx_desc->nbuf) /
				     (buf_size -
				      soc->rx_pkt_tlv_size) + 1) >
				    num_pending) {
					DP_STATS_INC(soc,
						     rx.msdu_scatter_wait_break,
						     1);
					dp_rx_cookie_reset_invalid_bit(
								     ring_desc);
					/* As we are going to break out of the
					 * loop because of unavailability of
					 * descs to form complete SG, we need to
					 * reset the TP in the REO destination
					 * ring.
					 */
					hal_srng_dst_dec_tp(hal_soc,
							    hal_ring_hdl);
					break;
				}
				is_prev_msdu_last = false;
			}
		}

		if (!is_prev_msdu_last &&
		    !(qdf_nbuf_is_rx_chfrag_cont(rx_desc->nbuf)))
			is_prev_msdu_last = true;

		/*
		 * move unmap after scattered msdu waiting break logic
		 * in case double skb unmap happened.
		 */
		DP_RX_PROCESS_NBUF(soc, nbuf_head, nbuf_tail, ebuf_head,
				   ebuf_tail, rx_desc);

refill_opt_dp_ctrl:
		dp_rx_nbuf_unmap(soc, rx_desc, reo_ring_num);
		quota -= 1;
		num_pending -= 1;

		if (dp_rx_add_to_ipa_desc_free_list(soc, rx_desc,
						    is_ctrl_refill) !=
						QDF_STATUS_SUCCESS) {
			rx_bufs_reaped[rx_desc->pool_id]++;
			dp_rx_add_to_free_desc_list
				(&head[rx_desc->pool_id],
				 &tail[rx_desc->pool_id],
				 rx_desc);
		}
		num_rx_bufs_reaped++;

		/*
		 * only if complete msdu is received for scatter case,
		 * then allow break.
		 */
		if (is_prev_msdu_last &&
		    dp_rx_reap_loop_pkt_limit_hit(soc, num_rx_bufs_reaped,
						  max_reap_limit))
			break;
	}
done:
	dp_rx_srng_access_end(int_ctx, soc, hal_ring_hdl);
	qdf_dsb();

	dp_rx_per_core_stats_update(soc, reo_ring_num, num_rx_bufs_reaped);

	for (mac_id = 0; mac_id < MAX_PDEV_CNT; mac_id++) {
		/*
		 * continue with next mac_id if no pkts were reaped
		 * from that pool
		 */
		if (!rx_bufs_reaped[mac_id])
			continue;

		dp_rxdma_srng =
			&soc->rx_refill_buf_ring[mac_id];

		rx_desc_pool = &soc->rx_desc_buf[mac_id];

		dp_rx_buffers_replenish_simple(soc, mac_id,
					       dp_rxdma_srng,
					       rx_desc_pool,
					       rx_bufs_reaped[mac_id],
					       &head[mac_id],
					       &tail[mac_id]);
	}

	/* Peer can be NULL is case of LFR */
	if (qdf_likely(txrx_peer))
		vdev = NULL;

	/*
	 * BIG loop where each nbuf is dequeued from global queue,
	 * processed and queued back on a per vdev basis. These nbufs
	 * are sent to stack as and when we run out of nbufs
	 * or a new nbuf dequeued from global queue has a different
	 * vdev when compared to previous nbuf.
	 */
	nbuf = nbuf_head;
	while (nbuf) {
		next = nbuf->next;
		if (qdf_unlikely(dp_rx_is_raw_frame_dropped(nbuf))) {
			nbuf = next;
			dp_verbose_debug("drop raw frame");
			DP_STATS_INC(soc, rx.err.raw_frm_drop, 1);
			continue;
		}

		rx_tlv_hdr = qdf_nbuf_data(nbuf);
		vdev_id = QDF_NBUF_CB_RX_VDEV_ID(nbuf);
		peer_id = dp_rx_get_peer_id_be(nbuf);
		dp_rx_set_mpdu_seq_number_be(nbuf, rx_tlv_hdr);

		if (dp_rx_is_list_ready(deliver_list_head, vdev, txrx_peer,
					peer_id, vdev_id)) {
			dp_rx_deliver_to_stack(soc, vdev, txrx_peer,
					       deliver_list_head,
					       deliver_list_tail);
			deliver_list_head = NULL;
			deliver_list_tail = NULL;
		}

		/* Get TID from struct cb->tid_val, save to tid */
		tid = qdf_nbuf_get_tid_val(nbuf);
		if (qdf_unlikely(tid >= CDP_MAX_DATA_TIDS)) {
			DP_STATS_INC(soc, rx.err.rx_invalid_tid_err, 1);
			dp_verbose_debug("drop invalid tid");
			dp_rx_nbuf_free(nbuf);
			nbuf = next;
			continue;
		}

		if (qdf_unlikely(!txrx_peer)) {
			txrx_peer = dp_rx_get_txrx_peer_and_vdev(soc, nbuf,
								 peer_id,
								 &txrx_ref_handle,
								 pkt_capture_offload,
								 &vdev,
								 &rx_pdev, &dsf,
								 &old_tid);
			if (qdf_unlikely(!txrx_peer) || qdf_unlikely(!vdev)) {
				dp_verbose_debug("drop no peer frame");
				nbuf = next;
				continue;
			}
			enh_flag = rx_pdev->enhanced_stats_en;
		} else if (txrx_peer && txrx_peer->peer_id != peer_id) {
			dp_txrx_peer_unref_delete(txrx_ref_handle,
						  DP_MOD_ID_RX);

			txrx_peer = dp_rx_get_txrx_peer_and_vdev(soc, nbuf,
								 peer_id,
								 &txrx_ref_handle,
								 pkt_capture_offload,
								 &vdev,
								 &rx_pdev, &dsf,
								 &old_tid);
			if (qdf_unlikely(!txrx_peer) || qdf_unlikely(!vdev)) {
				dp_verbose_debug("drop by unmatch peer_id");
				nbuf = next;
				continue;
			}
			enh_flag = rx_pdev->enhanced_stats_en;
		}

		if (txrx_peer) {
			QDF_NBUF_CB_DP_TRACE_PRINT(nbuf) = false;
			qdf_dp_trace_set_track(nbuf, QDF_RX);
			QDF_NBUF_CB_RX_DP_TRACE(nbuf) = 1;
			QDF_NBUF_CB_RX_PACKET_TRACK(nbuf) =
				QDF_NBUF_RX_PKT_DATA_TRACK;
		}

		rx_bufs_used++;

		/* MLD Link Peer Statistics support */
		if (txrx_peer->is_mld_peer && rx_pdev->link_peer_stats) {
			link_id = dp_rx_get_stats_arr_idx_from_link_id(
								nbuf,
								txrx_peer);
		} else {
			link_id = 0;
		}

		dp_rx_set_nbuf_band(nbuf, txrx_peer, link_id);

		/* when hlos tid override is enabled, save tid in
		 * skb->priority
		 */
		if (qdf_unlikely(vdev->skip_sw_tid_classification &
					DP_TXRX_HLOS_TID_OVERRIDE_ENABLED))
			qdf_nbuf_set_priority(nbuf, tid);

		DP_RX_TID_SAVE(nbuf, tid);
		if (qdf_unlikely(dsf) || qdf_unlikely(peer_ext_stats) ||
		    dp_rx_pkt_tracepoints_enabled())
			qdf_nbuf_set_timestamp(nbuf);

		if (qdf_likely(old_tid != tid)) {
			tid_stats =
		&rx_pdev->stats.tid_stats.tid_rx_stats[reo_ring_num][tid];
			old_tid = tid;
		}

		/*
		 * Check if DMA completed -- msdu_done is the last bit
		 * to be written
		 */
		if (qdf_unlikely(!qdf_nbuf_is_rx_chfrag_cont(nbuf) &&
				 !hal_rx_tlv_msdu_done_get_be(rx_tlv_hdr))) {
			DP_STATS_INC(soc, rx.err.msdu_done_fail, 1);
			dp_err("MSDU DONE failure %d",
			       soc->stats.rx.err.msdu_done_fail);
			hal_rx_dump_pkt_tlvs(hal_soc, rx_tlv_hdr,
					     QDF_TRACE_LEVEL_INFO);
			tid_stats->fail_cnt[MSDU_DONE_FAILURE]++;
			dp_rx_nbuf_free(nbuf);
			qdf_assert(0);
			nbuf = next;
			continue;
		}

		DP_HIST_PACKET_COUNT_INC(vdev->pdev->pdev_id);
		/*
		 * First IF condition:
		 * 802.11 Fragmented pkts are reinjected to REO
		 * HW block as SG pkts and for these pkts we only
		 * need to pull the RX TLVS header length.
		 * Second IF condition:
		 * The below condition happens when an MSDU is spread
		 * across multiple buffers. This can happen in two cases
		 * 1. The nbuf size is smaller then the received msdu.
		 *    ex: we have set the nbuf size to 2048 during
		 *        nbuf_alloc. but we received an msdu which is
		 *        2304 bytes in size then this msdu is spread
		 *        across 2 nbufs.
		 *
		 * 2. AMSDUs when RAW mode is enabled.
		 *    ex: 1st MSDU is in 1st nbuf and 2nd MSDU is spread
		 *        across 1st nbuf and 2nd nbuf and last MSDU is
		 *        spread across 2nd nbuf and 3rd nbuf.
		 *
		 * for these scenarios let us create a skb frag_list and
		 * append these buffers till the last MSDU of the AMSDU
		 * Third condition:
		 * This is the most likely case, we receive 802.3 pkts
		 * decapsulated by HW, here we need to set the pkt length.
		 */
		if (qdf_unlikely(qdf_nbuf_is_frag(nbuf))) {
			bool is_mcbc, is_sa_vld, is_da_vld;

			is_mcbc = hal_rx_msdu_end_da_is_mcbc_get(soc->hal_soc,
								 rx_tlv_hdr);
			is_sa_vld =
				hal_rx_msdu_end_sa_is_valid_get(soc->hal_soc,
								rx_tlv_hdr);
			is_da_vld =
				hal_rx_msdu_end_da_is_valid_get(soc->hal_soc,
								rx_tlv_hdr);

			qdf_nbuf_set_da_mcbc(nbuf, is_mcbc);
			qdf_nbuf_set_da_valid(nbuf, is_da_vld);
			qdf_nbuf_set_sa_valid(nbuf, is_sa_vld);

			qdf_nbuf_pull_head(nbuf, soc->rx_pkt_tlv_size);
		} else if (qdf_nbuf_is_rx_chfrag_cont(nbuf)) {
			msdu_len = QDF_NBUF_CB_RX_PKT_LEN(nbuf);
			nbuf = dp_rx_sg_create(soc, nbuf);
			next = nbuf->next;

			if (qdf_nbuf_is_raw_frame(nbuf)) {
				DP_STATS_INC(vdev->pdev, rx_raw_pkts, 1);
				DP_PEER_PER_PKT_STATS_INC_PKT(txrx_peer,
							      rx.raw, 1,
							      msdu_len,
							      link_id);
			} else {
				DP_STATS_INC(soc, rx.err.scatter_msdu, 1);

				if (!dp_rx_is_sg_supported()) {
					dp_rx_nbuf_free(nbuf);
					dp_info_rl("sg msdu len %d, dropped",
						   msdu_len);
					nbuf = next;
					continue;
				}
			}
		} else {
			l3_pad = hal_rx_get_l3_pad_bytes_be(nbuf, rx_tlv_hdr);
			msdu_len = QDF_NBUF_CB_RX_PKT_LEN(nbuf);
			pkt_len = msdu_len + l3_pad + soc->rx_pkt_tlv_size;

			qdf_nbuf_set_pktlen(nbuf, pkt_len);
			dp_rx_skip_tlvs(soc, nbuf, l3_pad);
		}

		dp_rx_send_pktlog(soc, rx_pdev, nbuf, QDF_TX_RX_STATUS_OK);

		if (!dp_wds_rx_policy_check(rx_tlv_hdr, vdev, txrx_peer)) {
			dp_rx_err("%pK: Policy Check Drop pkt", soc);
			DP_PEER_PER_PKT_STATS_INC(txrx_peer,
						  rx.policy_check_drop,
						  1, link_id);
			tid_stats->fail_cnt[POLICY_CHECK_DROP]++;
			/* Drop & free packet */
			dp_rx_nbuf_free(nbuf);
			/* Statistics */
			nbuf = next;
			continue;
		}

		/*
		 * Drop non-EAPOL frames from unauthorized peer.
		 */
		if (qdf_likely(txrx_peer) &&
		    qdf_unlikely(!txrx_peer->authorize) &&
		    !qdf_nbuf_is_raw_frame(nbuf)) {
			bool is_eapol = qdf_nbuf_is_ipv4_eapol_pkt(nbuf) ||
					qdf_nbuf_is_ipv4_wapi_pkt(nbuf);

			if (!is_eapol) {
				DP_PEER_PER_PKT_STATS_INC(txrx_peer,
							  rx.peer_unauth_rx_pkt_drop,
							  1, link_id);
				dp_verbose_debug("drop by unauthorized peer");
				dp_rx_nbuf_free(nbuf);
				nbuf = next;
				continue;
			}
		}

		dp_rx_cksum_offload(vdev->pdev, nbuf, rx_tlv_hdr);

		if (qdf_unlikely(!rx_pdev->rx_fast_flag)) {
			/*
			 * process frame for mulitpass phrase processing
			 */
			if (qdf_unlikely(vdev->multipass_en)) {
				if (dp_rx_multipass_process(txrx_peer, nbuf,
							    tid) == false) {
					DP_PEER_PER_PKT_STATS_INC
						(txrx_peer,
						 rx.multipass_rx_pkt_drop,
						 1, link_id);
					dp_verbose_debug("drop multi pass");
					dp_rx_nbuf_free(nbuf);
					nbuf = next;
					continue;
				}
			}
			if (qdf_unlikely(txrx_peer &&
					 (txrx_peer->nawds_enabled) &&
					 (qdf_nbuf_is_da_mcbc(nbuf)) &&
					 (hal_rx_get_mpdu_mac_ad4_valid_be
						(rx_tlv_hdr) == false))) {
				tid_stats->fail_cnt[NAWDS_MCAST_DROP]++;
				DP_PEER_PER_PKT_STATS_INC(txrx_peer,
							  rx.nawds_mcast_drop,
							  1, link_id);
				dp_verbose_debug("drop nawds");
				dp_rx_nbuf_free(nbuf);
				nbuf = next;
				continue;
			}

			/* Update the protocol tag in SKB based on CCE metadata
			 */
			dp_rx_update_protocol_tag(soc, vdev, nbuf, rx_tlv_hdr,
						  reo_ring_num, false, true);

			if (qdf_unlikely(vdev->mesh_vdev)) {
				if (dp_rx_filter_mesh_packets(vdev, nbuf,
							      rx_tlv_hdr)
						== QDF_STATUS_SUCCESS) {
					dp_rx_info("%pK: mesh pkt filtered",
						   soc);
					tid_stats->fail_cnt[MESH_FILTER_DROP]++;
					DP_STATS_INC(vdev->pdev,
						     dropped.mesh_filter, 1);

					dp_rx_nbuf_free(nbuf);
					nbuf = next;
					continue;
				}
				dp_rx_fill_mesh_stats(vdev, nbuf, rx_tlv_hdr,
						      txrx_peer);
			}
		}

		if (qdf_likely(vdev->rx_decap_type ==
			       htt_cmn_pkt_type_ethernet) &&
		    qdf_likely(!vdev->mesh_vdev)) {
			dp_rx_wds_learn(soc, vdev,
					rx_tlv_hdr,
					txrx_peer,
					nbuf);
		}

		dp_rx_msdu_stats_update(soc, nbuf, rx_tlv_hdr, txrx_peer,
					reo_ring_num, tid_stats, link_id);

		if (qdf_likely(vdev->rx_decap_type ==
			       htt_cmn_pkt_type_ethernet) &&
		    qdf_likely(!vdev->mesh_vdev)) {
			/* Intrabss-fwd */
			if (dp_rx_check_ap_bridge(vdev))
				if (dp_rx_intrabss_fwd_be(soc, txrx_peer,
							  rx_tlv_hdr,
							  nbuf,
							  link_id)) {
					nbuf = next;
					tid_stats->intrabss_cnt++;
					continue; /* Get next desc */
				}
		}

		dp_rx_fill_gro_info(soc, rx_tlv_hdr, nbuf, &rx_ol_pkt_cnt);

		dp_rx_mark_first_packet_after_wow_wakeup(vdev->pdev, rx_tlv_hdr,
							 nbuf);

		dp_rx_update_stats(soc, nbuf);

		dp_pkt_add_timestamp(txrx_peer->vdev, QDF_PKT_RX_DRIVER_ENTRY,
				     current_time, nbuf);

		DP_RX_LIST_APPEND(deliver_list_head,
				  deliver_list_tail,
				  nbuf);

		DP_PEER_TO_STACK_INCC_PKT(txrx_peer, 1,
					  QDF_NBUF_CB_RX_PKT_LEN(nbuf),
					  enh_flag);
		DP_PEER_PER_PKT_STATS_INC_PKT(txrx_peer,
					      rx.rx_success, 1,
					      QDF_NBUF_CB_RX_PKT_LEN(nbuf),
					      link_id);

		if (qdf_unlikely(txrx_peer->in_twt))
			DP_PEER_PER_PKT_STATS_INC_PKT(txrx_peer,
						      rx.to_stack_twt, 1,
						      QDF_NBUF_CB_RX_PKT_LEN(nbuf),
						      link_id);

		tid_stats->delivered_to_stack++;
		nbuf = next;
	}

	DP_RX_DELIVER_TO_STACK(soc, vdev, txrx_peer, peer_id,
			       pkt_capture_offload,
			       deliver_list_head,
			       deliver_list_tail);
	/*
	 * If we are processing in near-full condition, there are 3 scenario
	 * 1) Ring entries has reached critical state
	 * 2) Ring entries are still near high threshold
	 * 3) Ring entries are below the safe level
	 *
	 * One more loop will move the state to normal processing and yield
	 */
	if (ring_near_full && quota)
		goto more_data;

	if (dp_rx_enable_eol_data_check(soc) && rx_bufs_used) {
		if (quota) {
			num_pending =
				dp_rx_srng_get_num_pending(hal_soc,
							   hal_ring_hdl,
							   num_entries,
							   &near_full);
			if (num_pending) {
				DP_STATS_INC(soc, rx.hp_oos2, 1);

				if (!hif_exec_should_yield(scn, intr_id))
					goto more_data;

				if (qdf_unlikely(near_full)) {
					DP_STATS_INC(soc, rx.near_full, 1);
					goto more_data;
				}
			}
		}

		if (vdev && vdev->osif_fisa_flush)
			vdev->osif_fisa_flush(soc, reo_ring_num);

		if (vdev && vdev->osif_gro_flush && rx_ol_pkt_cnt) {
			vdev->osif_gro_flush(vdev->osif_vdev,
					     reo_ring_num);
		}
	}

	if (qdf_likely(txrx_peer))
		dp_txrx_peer_unref_delete(txrx_ref_handle, DP_MOD_ID_RX);

	/* Update histogram statistics by looping through pdev's */
	DP_RX_HIST_STATS_PER_PDEV();

	return rx_bufs_used; /* Assume no scale factor for now */
}

