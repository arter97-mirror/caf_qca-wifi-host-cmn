/*
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

#if  !defined(_QDF_PAGE_POOL_H)
#define _QDF_PAGE_POOL_H

#include <i_qdf_page_pool.h>
#include <qdf_util.h>

typedef __qdf_page_pool_t qdf_page_pool_t;

/**
 * qdf_page_pool_alloc_page: Allocate full page from page pool
 *
 * @pp: Page Pool reference
 *
 * Return: Page reference
 */
static inline qdf_page_t qdf_page_pool_alloc_page(qdf_page_pool_t pp)
{
	return __qdf_page_pool_alloc_page(pp);
}

/**
 * qdf_page_pool_get_dma_addr: Get DMA address of the page pool page
 *
 * @page: Reference to the page
 *
 * Return: DMA address of the page
 */
static inline dma_addr_t qdf_page_pool_get_dma_addr(qdf_page_t page)
{
	return __qdf_page_pool_get_dma_addr(page);
}

/**
 * qdf_page_pool_empty() - Check page pool empty condition
 *
 * @pp: Page Pool Reference
 *
 * Return: true/false
 */
static inline bool qdf_page_pool_empty(qdf_page_pool_t pp)
{
	return __qdf_page_pool_empty(pp);
}

/**
 * qdf_page_pool_alloc_frag() - Allocate frag buffer from page pool
 *
 * @pp: Page Pool Reference
 * @offset: Buffer offset reference within the page
 * @size: Buffer size
 *
 * Return: Allocated page reference
 */
static inline qdf_page_t
qdf_page_pool_alloc_frag(qdf_page_pool_t pp, uint32_t *offset, size_t size)
{
	return __qdf_page_pool_alloc_frag(pp, offset, size);
}

/**
 * qdf_page_pool_put_page() - Decrement frag reference count of page pool page
 *
 * @pp: Page Pool reference
 * @page: Page reference
 * @direct_recycle: Direct recycle to lockless cache in page pool
 *
 * Return: None
 */
static inline void
qdf_page_pool_put_page(qdf_page_pool_t pp, qdf_page_t page,
		       bool direct_recycle)
{
	return __qdf_page_pool_put_page(pp, page, direct_recycle);
}

/**
 * qdf_page_pool_create() - Create page_pool
 *
 * @osdev: Device handle
 * @pool_size: Pool Size
 * @pp_page_size: Page pool page size
 * @dir: DMA direction
 * @rx_pp_track_id: Pointer to store the track id assigned by tracker
 *
 * Return: Page Pool Reference
 */
static inline qdf_page_pool_t
qdf_page_pool_create(qdf_device_t osdev, size_t pool_size, size_t pp_page_size,
		     qdf_dma_dir_t dir, int *rx_pp_track_id)
{
	return __qdf_page_pool_create(osdev, pool_size, pp_page_size, dir,
				      rx_pp_track_id);
}

/**
 * qdf_page_pool_destroy() - Destroy page_pool
 * @pp: Page Pool Reference
 *
 * Return: None
 */
static inline void qdf_page_pool_destroy(qdf_page_pool_t pp)
{
	return __qdf_page_pool_destroy(pp);
}

/**
 * qdf_page_pool_get_page_hold_cnt() - Get total pages ever allocated
 *
 * @pp: Page Pool reference
 *
 * Return: pages_state_hold_cnt (monotonically increasing)
 */
static inline uint32_t
qdf_page_pool_get_page_hold_cnt(qdf_page_pool_t pp)
{
	return __qdf_page_pool_get_page_hold_cnt(pp);
}

/**
 * qdf_page_pool_get_inflight_cnt() - Get number of inflight pages
 *
 * @pp: Page Pool reference
 *
 * Returns pages_state_hold_cnt - pages_state_release_cnt, matching the
 * value reported by the kernel's page_pool_release_retry() warning.
 * Non-zero at destroy time indicates premature-destroy race condition.
 *
 * Return: Number of inflight pages
 */
static inline uint32_t
qdf_page_pool_get_inflight_cnt(qdf_page_pool_t pp)
{
	return __qdf_page_pool_get_inflight_cnt(pp);
}

/**
 * qdf_page_pool_inc_buf_count() - Increment in-flight buffers count
 *
 * @nbuf: Network buffer
 *
 * Return: None
 */
static inline void
qdf_page_pool_inc_buf_count(struct sk_buff *nbuf)
{
	__qdf_page_pool_inc_buf_count(nbuf);
}

/**
 * qdf_page_pool_dec_buf_count() - Decrement in-flight buffers count
 *
 * @nbuf: Network buffer
 *
 * Return: None
 */
static inline void
qdf_page_pool_dec_buf_count(struct sk_buff *nbuf)
{
	__qdf_page_pool_dec_buf_count(nbuf);
}

/**
 * qdf_page_pool_check_inflight_buffers() - Check if page pool has in-flight
 *					    buffers
 *
 * @pp: Page pool pointer
 * @rx_pp_idx: Page pool tracker index for rx page pool
 *
 * Return: true if there are in-flight buffers, false otherwise
 */
static inline bool
qdf_page_pool_check_inflight_buffers(qdf_page_pool_t pp, int rx_pp_idx)
{
	return __qdf_page_pool_check_inflight_buffers(pp, rx_pp_idx);
}

/**
 * qdf_page_pool_get_buf_count() - Read raw in-flight buffer counter for a pool
 *
 * @pp: Page pool pointer
 * @rx_pp_idx: Page pool tracker index
 *
 * Return: buff_count value, or -1 if pp/idx is invalid or mismatched
 */
static inline int
qdf_page_pool_get_buf_count(qdf_page_pool_t pp, int rx_pp_idx)
{
	return __qdf_page_pool_get_buf_count(pp, rx_pp_idx);
}

/**
 * qdf_page_pool_get_alloc_cache_count() - Get per-CPU alloc cache page count
 *
 * @pp: Page pool pointer
 *
 * Returns pp->alloc.count.  In the recycling fast-path a page goes:
 *   kfree_skb -> page_pool_put_full_page -> alloc.cache
 * buff_count is decremented (in __qdf_nbuf_unmap) BEFORE the page lands
 * in alloc.cache.  So when buff_count==0 but alloc_cache_count < pool_size,
 * one or more pages are still mid-recycle and the pool must not be destroyed.
 *
 * Return: number of pages in the per-CPU alloc cache
 */
static inline u32
qdf_page_pool_get_alloc_cache_count(qdf_page_pool_t pp)
{
	return __qdf_page_pool_get_alloc_cache_count(pp);
}

#endif /* _QDF_PAGE_POOL_H */
