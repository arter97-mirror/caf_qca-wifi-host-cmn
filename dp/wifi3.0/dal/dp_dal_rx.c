/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: ISC
 */

#include "dp_dal_rx.h"
#include "dp_rx.h"
#include "dp_dal.h"

#define BUFFER_ADDR_INFO_SIZE 8

/**
 * dp_dal_rx_replenish_alloc_vendor_cb - vendor callback to alloc rx buffers
 * @priv: pointer to dp dal context
 * @count: number of buffers to allocate
 *
 * Return: 0 on success
 */
int dp_dal_rx_replenish_alloc_vendor_cb(void *priv, uint16_t count)
{
	struct dp_dal_ctx *dal_ctx = (struct dp_dal_ctx *)priv;
	struct dp_soc *soc;
	struct rx_desc_pool *rx_desc_pool;
	struct dp_pdev *pdev;
	struct dp_rx_nbuf_frag_info nf_info;
	union dp_rx_desc_list_elem_t *desc_list;
	union dp_rx_desc_list_elem_t *tail;
	union dp_rx_desc_list_elem_t *next;
	void *buffer_addr_info;
	void *orig_buffer_addr_info;
	uint16_t num_buffer_info_per_page;
	uint16_t nr_desc;
	uint8_t total_pages;
	uint8_t page_idx;
	int mac_id = 0;
	int total_nbuf_count = 0;
	int nbuf_count;
	int i;
	int ret;
	bool dp_buf_page_frag_alloc_enable;

	if (!dal_ctx) {
		dp_err("dal_ctx is NULL");
		return -EINVAL;
	}

	soc = dal_ctx->soc;
	if (!soc) {
		dp_err("soc is NULL in dal_ctx");
		return -EINVAL;
	}

	pdev = soc->pdev_list[0];
	rx_desc_pool = &soc->rx_desc_buf[mac_id];
	num_buffer_info_per_page = DP_BLOCKMEM_SIZE / BUFFER_ADDR_INFO_SIZE;

	dp_buf_page_frag_alloc_enable =
		wlan_cfg_is_dp_buf_page_frag_alloc_enable(soc->wlan_cfg_ctx);

	nr_desc = dp_rx_get_free_desc_list(soc, mac_id, rx_desc_pool,
					   count, &desc_list, &tail);
	if (!nr_desc) {
		dp_err("dp_dal: no free rx_descs in freelist");
		DP_STATS_INC(pdev, err.desc_alloc_fail, count);
		return -ENOMEM;
	}

	total_pages = nr_desc / num_buffer_info_per_page;
	if (nr_desc % num_buffer_info_per_page)
		total_pages++;

	orig_buffer_addr_info = qdf_mem_malloc(DP_BLOCKMEM_SIZE);
	if (!orig_buffer_addr_info) {
		dp_err("dp_dal:failed to allocate buffer_addr_info array");
		dp_rx_add_desc_list_to_free_list(soc, &desc_list, &tail,
						 mac_id, rx_desc_pool);
		return -ENOMEM;
	}

	for (page_idx = 0; page_idx < total_pages; page_idx++) {
		buffer_addr_info = orig_buffer_addr_info;
		nbuf_count = 0;

		for (i = 0; i < num_buffer_info_per_page; i++) {
			if (total_nbuf_count >= nr_desc)
				break;

			qdf_mem_zero(&nf_info,
				     sizeof(struct dp_rx_nbuf_frag_info));

			if (qdf_unlikely(rx_desc_pool->rx_mon_dest_frag_enable))
				ret = dp_pdev_frag_alloc_and_map(soc,
								 &nf_info, pdev,
								 rx_desc_pool);
			else
				ret = dp_pdev_nbuf_alloc_and_map_replenish(soc, mac_id,
									   count, &nf_info,
									   pdev, rx_desc_pool);

			if (qdf_unlikely(QDF_IS_STATUS_ERROR(ret))) {
				if (qdf_unlikely(ret  == QDF_STATUS_E_FAULT))
					continue;
				break;
			}

			nbuf_count++;

			next = (desc_list)->next;

			if (qdf_unlikely(rx_desc_pool->rx_mon_dest_frag_enable))
				dp_rx_desc_frag_prep(&desc_list->rx_desc,
						     &nf_info);
			else
				dp_rx_desc_prep(&desc_list->rx_desc,
						&nf_info);

			desc_list->rx_desc.in_use = 1;
			desc_list->rx_desc.in_err_state = 0;
			dp_rx_desc_update_dbg_info(&desc_list->rx_desc,
						   __func__,
						   RX_DESC_REPLENISHED);
			dp_verbose_debug("rx_buf=%pK, paddr=0x%llx, cookie=%d",
					 nf_info.virt_addr.nbuf,
					 (unsigned long long)(nf_info.paddr),
					 desc_list->rx_desc.cookie);

			if (qdf_likely(!rx_desc_pool->rx_mon_dest_frag_enable))
				qdf_assert_always(nf_info.paddr ==
						  QDF_NBUF_CB_PADDR(desc_list->rx_desc.nbuf));

			hal_rxdma_buff_addr_info_set(soc->hal_soc,
						     buffer_addr_info,
						     nf_info.paddr,
						     desc_list->rx_desc.cookie,
						     rx_desc_pool->owner);

			total_nbuf_count++;
			buffer_addr_info += BUFFER_ADDR_INFO_SIZE;
			desc_list = next;
		}

		if (nbuf_count) {
			ret = global_plat_ops->rxbm_sync(dal_ctx, nbuf_count,
							 &buffer_addr_info);
			if (ret) {
				dp_err("DAL rxbm_sync failed");
				break;
			}
		}
	}

	qdf_mem_free(orig_buffer_addr_info);

	if (desc_list)
		dp_rx_add_desc_list_to_free_list(soc, &desc_list, &tail,
						 mac_id, rx_desc_pool);
	return 0;
}

/**
 *dp_dal_rx_isr_vendor_cb - rx ISR vendor callback
 *@ring_num: rx ring number
 *@priv: pointer to dp dal context
 *
 * Return: 0 on success
 */
int dp_dal_rx_isr_vendor_cb(int ring_num, void *priv)
{
	struct dp_dal_ctx *dal_ctx = (struct dp_dal_ctx *)priv;
	struct dp_soc *soc;
	int grp_id;

	if (!dal_ctx) {
		dp_err("dal_ctx is NULL");
		return -EINVAL;
	}

	soc = dal_ctx->soc;
	if (!soc) {
		dp_err("soc is NULL");
		return -EINVAL;
	}

	grp_id = dp_dal_get_ext_grp_id(dal_ctx, ring_num, REO_DST);
	if (grp_id >= HIF_MAX_GROUP) {
		dp_err("invalid group id:%d ring_num:%d ring_type:%s",
		       grp_id, ring_num, " REO_DEST");
		QDF_BUG(0);
		return -EINVAL;
	}

	hif_ext_grp_napi_schedule(soc->hif_handle, grp_id);

	return 0;
}

/**
 * dp_dal_rx_bypass_mode() - Skeleton for platform bus rx in bypass mode
 *
 * @priv: private data
 * @cnt: count
 *
 * Return: false
 */
bool dp_dal_rx_bypass_mode(void *priv, u32 *cnt)
{
	return false;
}

/**
 * dp_dal_rx_replenish_bypass_mode() - replenish rx buffers
 * in bypass mode
 *
 * @priv: private data
 * @cnt: count
 * @use_rsv_pktid: use reserved packet id
 *
 * Return: 0 on success
 */
int dp_dal_rx_replenish_bypass_mode(void *priv, u32 cnt, bool use_rsv_pktid)
{
	struct dp_dal_ctx *dal_ctx = (struct dp_dal_ctx *)priv;
	struct dp_soc *soc;
	struct rx_desc_pool *rx_desc_pool;
	union dp_rx_desc_list_elem_t *desc_list_head = NULL;
	union dp_rx_desc_list_elem_t *desc_list_tail = NULL;
	int mac_id = 0;

	if (!dal_ctx) {
		dp_err("DAL context is NULL");
		return -EINVAL;
	}

	soc = dal_ctx->soc;
	if (!soc) {
		dp_err("SOC is NULL in DAL context");
		return -EINVAL;
	}

	rx_desc_pool = &soc->rx_desc_buf[mac_id];
	dp_rx_buffers_replenish(soc, mac_id,
				&soc->rx_refill_buf_ring[mac_id],
				rx_desc_pool, cnt,
				&desc_list_head, &desc_list_tail, true);

	return 0;
}

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
int dp_dal_rx_rxbm_sync_bypass_mode(void *priv, u32 cnt, void **rxbm)
{
	return 0;
}

/**
 * dp_dal_rx_process_nbuf_list - Process a list of qdf_nbufs for RX path.
 * @soc: DP SOC context.
 * @nbuf_list: Head of the qdf_nbuf list to be processed.
 *
 * This is a placeholder (skeleton) implementation that iterates over the
 * provided nbuf list. The actual processing logic should be added by the
 * driver developer.
 *
 * Return: %QDF_STATUS_SUCCESS on success, or an appropriate error code.
 */
QDF_STATUS dp_dal_rx_process_nbuf_list(struct dp_soc *soc,
				       qdf_nbuf_t nbuf_list)
{
	/* TODO: Implement RX processing of the nbuf list */
	return QDF_STATUS_SUCCESS;
}

/**
 * dp_dal_rx_desc_list_cleanup() - Cleanup DAL RX descriptor lists
 * @dal_ctx: DAL context
 *
 * This function cleans up any remaining RX descriptors in the DAL RX
 * descriptor lists during DAL deinit. It properly frees all allocated
 * dp_dal_rx_desc_node structures and returns RX descriptors to their
 * respective free lists.
 *
 * Return: None
 */
void dp_dal_rx_desc_list_cleanup(struct dp_dal_ctx *dal_ctx)
{
	struct dp_dal_rx_desc_node *node, *next_node;
	struct dp_rx_desc *rx_desc;
	struct dp_soc *soc;
	struct rx_desc_pool *rx_desc_pool;
	union dp_rx_desc_list_elem_t *desc_list_head[MAX_PDEV_CNT] = {NULL};
	union dp_rx_desc_list_elem_t *desc_list_tail[MAX_PDEV_CNT] = {NULL};
	int ring_id, mac_id;

	if (!dal_ctx) {
		dp_err("DAL context is NULL");
		return;
	}

	soc = dal_ctx->soc;
	if (!soc) {
		dp_err("SOC is NULL in DAL context");
		return;
	}

	qdf_spin_lock_bh(&dal_ctx->dal_rx_desc_lock);

	/* Cleanup RX descriptor lists for all rings */
	for (ring_id = 0; ring_id < MAX_REO_DEST_RINGS; ring_id++) {
		node = dal_ctx->rx_desc_head[ring_id];

		while (node) {
			next_node = node->next;
			rx_desc = node->rx_desc;

			if (rx_desc)
				dp_rx_add_to_free_desc_list(
					&desc_list_head[rx_desc->pool_id],
					&desc_list_tail[rx_desc->pool_id],
					rx_desc);

			qdf_mem_free(node);
			node = next_node;
		}

		dal_ctx->rx_desc_head[ring_id] = NULL;
		dal_ctx->rx_desc_tail[ring_id] = NULL;
		dal_ctx->rx_desc_count[ring_id] = 0;
	}

	qdf_spin_unlock_bh(&dal_ctx->dal_rx_desc_lock);

	/* Return descriptors to their respective free lists */
	for (mac_id = 0; mac_id < MAX_PDEV_CNT; mac_id++) {
		if (desc_list_head[mac_id]) {
			rx_desc_pool = &soc->rx_desc_buf[mac_id];
			dp_rx_add_desc_list_to_free_list(
					soc, &desc_list_head[mac_id],
					&desc_list_tail[mac_id],
					mac_id, rx_desc_pool);
		}
	}
}

/**
 * dp_dal_rx_remove_desc_from_head() - Remove rx desc from head of global list
 * @dal_ctx: DAL context
 * @ring_id: Ring ID
 *
 * Function to remove first descriptor from global list.
 *
 * Return: Pointer to removed descriptor, NULL if list is empty
 */
static inline struct dp_rx_desc *
dp_dal_rx_remove_desc_from_head(struct dp_dal_ctx *dal_ctx, uint8_t ring_id)
{
	struct dp_dal_rx_desc_node *node;
	struct dp_rx_desc *rx_desc;

	node = dal_ctx->rx_desc_head[ring_id];
	if (!node)
		return NULL;

	dal_ctx->rx_desc_head[ring_id] = node->next;

	if (!node->next)
		dal_ctx->rx_desc_tail[ring_id] = NULL;

	dal_ctx->rx_desc_count[ring_id]--;
	rx_desc = node->rx_desc;
	qdf_mem_free(node);

	return rx_desc;
}

/**
 * dp_dal_rx_add_desc_to_head() - Add descriptor back to head of global list
 * @dal_ctx: DAL context
 * @ring_id: Ring ID
 * @rx_desc: Descriptor to add back
 *
 * Inline function to add descriptor back to head of global list.
 */
static inline void
dp_dal_rx_add_desc_to_head(struct dp_dal_ctx *dal_ctx, uint8_t ring_id,
			   struct dp_rx_desc *rx_desc)
{
	struct dp_dal_rx_desc_node *node;

	node = qdf_mem_malloc(sizeof(*node));
	if (qdf_unlikely(!node)) {
		dp_err_rl("Failed to allocate DAL RX node for ring %u",
			  ring_id);
		return;
	}

	node->rx_desc = rx_desc;
	node->next = dal_ctx->rx_desc_head[ring_id];

	dal_ctx->rx_desc_head[ring_id] = node;
	if (!dal_ctx->rx_desc_tail[ring_id])
		dal_ctx->rx_desc_tail[ring_id] = node;

	dal_ctx->rx_desc_count[ring_id]++;
}
