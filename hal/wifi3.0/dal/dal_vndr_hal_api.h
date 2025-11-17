/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: ISC
 */

#ifndef _DAL_VNDR_HAL_API_H_
#define _DAL_VNDR_HAL_API_H_

#include <linux/types.h>
#include <linux/io.h>
#include "dal_vndr_hal_internal.h"

/*
 * BAR + 4K is always accessible, any access outside this
 * space requires force wake procedure.
 * OFFSET = 4K - 32 bytes = 0xFE0
 */
#define MAPPED_REF_OFF 0xFE0

/**
 * dal_vndr_hal_write32_mb() - write a value to a register
 * @hal_soc: hal soc handle
 * @offset: offset from bar
 * @value: value to write
 */
static inline void dal_vndr_hal_write32_mb(
		struct dal_vndr_hal_soc *hal_soc, uint32_t offset,
		uint32_t value)
{
}

/**
 * dal_vndr_hal_write_address_32_mb() - write a value to a register
 * @hal_soc: hal soc handle
 * @addr: I/O memory address to write
 * @value: value to write
 */
static inline void dal_vndr_hal_write_address_32_mb(
			struct dal_vndr_hal_soc *hal_soc,
			void *addr, uint32_t value)
{
}

/**
 * dal_vndr_hal_srng_write_address_32_mb() - write to srng register
 * @hal_soc: hal soc handle
 * @srng: hal srng
 * @addr: I/O memory address to write
 * @value: value to write
 */
static inline void dal_vndr_hal_srng_write_address_32_mb(
					struct dal_vndr_hal_soc *hal_soc,
					struct dal_vndr_hal_srng *srng,
					void *addr,
					uint32_t value)
{
}

/**
 * dal_vndr_hal_srng_access_start() - Start ring access
 *
 * @hal_soc_hdl: Opaque HAL SOC handle
 * @hal_ring_hdl: Ring pointer (Source or Destination ring)
 *
 * This API doesn't implement any byte-order conversion on reading hp/tp.
 * So, Use API only for those srngs for which the target writes hp/tp values to
 * the DDR in the Host order.
 *
 * Return: 0 on success; error on failire
 */
static inline int dal_vndr_hal_srng_access_start(
			void *hal_soc_hdl,
			void *hal_ring_hdl)
{
	return 0;
}

/**
 * dal_vndr_hal_srng_dst_get_next() - Get next entry from a destination ring
 * @hal_soc: Opaque HAL SOC handle
 * @hal_ring_hdl: Destination ring pointer
 *
 * Return: Opaque pointer for next ring entry; NULL on failure
 */
static inline void *dal_vndr_hal_srng_dst_get_next(
		void *hal_soc,
		void *hal_ring_hdl)
{
	return NULL;
}

/**
 * dal_vndr_hal_srng_dst_num_valid() - Returns number of valid entries
 * (to be processed by SW) in destination ring
 * @hal_soc: Opaque HAL SOC handle
 * @hal_ring_hdl: Destination ring pointer
 * @sync_hw_ptr: Sync cached head pointer with HW
 *
 * Return: number of valid entries
 */
static inline uint32_t dal_vndr_hal_srng_dst_num_valid(
		void *hal_soc,
		void *hal_ring_hdl,
		int sync_hw_ptr)
{
	return 0;
}

/**
 * dal_vndr_hal_srng_src_num_avail() - Returns number of available entries in
 * src ring
 * @hal_soc: Opaque HAL SOC handle
 * @hal_ring_hdl: Source ring pointer
 * @sync_hw_ptr: Sync cached tail pointer with HW
 *
 * Return: number of available entries
 */
static inline uint32_t dal_vndr_hal_srng_src_num_avail(
		void *hal_soc,
		void *hal_ring_hdl, int sync_hw_ptr)
{
	return 0;
}

/**
 * dal_vndr_hal_srng_src_get_next() - Get next entry from a source ring and
 * move cached tail pointer
 * @hal_soc: Opaque HAL SOC handle
 * @hal_ring_hdl: Source ring pointer
 *
 * Return: Opaque pointer for next ring entry; NULL on failure
 */
static inline void *dal_vndr_hal_srng_src_get_next(
			void *hal_soc,
			void *hal_ring_hdl)
{
	return NULL;
}

/**
 * dal_vndr_hal_srng_access_end() - Update cached ring
 *                          head/tail pointers to HW
 * @hal_soc: Opaque HAL SOC handle
 * @hal_ring_hdl: Ring pointer (Source or Destination ring)
 *
 * The target expects cached head/tail pointer to be updated to the
 * shared location in the little-endian order, This API ensures that.
 * This API should be used only if dal_vndr_hal_srng_access_start was used to
 * start ring access
 *
 */
static inline void dal_vndr_hal_srng_access_end(
		void *hal_soc, void *hal_ring_hdl)
{
}

/**
 * dal_vndr_hal_tx_desc_sync() - Commit the descriptor to Hardware
 * @hal_tx_desc_cached: Cached descriptor that software maintains
 * @hw_desc: Hardware descriptor to be updated
 * @num_bytes: descriptor size
 */
static inline void dal_vndr_hal_tx_desc_sync(
			void *hal_tx_desc_cached,
			void *hw_desc, uint8_t num_bytes)
{
}

/**
 * dal_vndr_hal_srng_dst_dec_tp() - decrement the TP of the Dst ring by one
 * @hal_soc: Opaque HAL SOC handle
 * @hal_ring_hdl: Destination ring pointer
 *
 * reset the tail pointer in the destination ring by one entry
 *
 */
static inline void dal_vndr_hal_srng_dst_dec_tp(
			void *hal_soc, void *hal_ring_hdl)
{
	struct dal_vndr_hal_srng *srng =
				(struct dal_vndr_hal_srng *)hal_ring_hdl;

	if (!srng->u.dst_ring.tp)
		srng->u.dst_ring.tp = (srng->ring_size - srng->entry_size);
	else
		srng->u.dst_ring.tp -= srng->entry_size;
}

/**
 * dal_vndr_hal_rxbm_sync() - Sync the buff addr info to refill ring
 * @hal_soc_hdl: HAL SoC handle
 * @ring_desc: Ring descriptor pointer
 * @buff_addr_info: Buffer address info pointer
 *
 * This function syncs the buffer address info to the refill ring.
 *
 * Return: void
 */
static inline void dal_vndr_hal_rxbm_sync(void *hal_soc_hdl, void *ring_desc,
					  void *buff_addr_info)
{
	struct dal_vndr_hal_soc *hal_soc =
				(struct dal_vndr_hal_soc *)hal_soc_hdl;

	hal_soc->ops->dal_vndr_hal_rxbm_sync(ring_desc, buff_addr_info);
}

/**
 * dal_vndr_hal_get_sw_hptp() - Get SW head and tail pointer for any ring
 * @hal_soc: Opaque HAL SOC handle
 * @hal_ring_hdl: Source ring pointer
 * @tailp: Tail Pointer
 * @headp: Head Pointer
 *
 * Return: Update tail pointer and head pointer in arguments.
 */
static inline
void dal_vndr_hal_get_sw_hptp(void *hal_soc, void *hal_ring_hdl,
			      uint32_t *tailp, uint32_t *headp)
{
}

/**
 * dal_vndr_hal_ops_attach() - Attach HAL operations
 * @hal_soc: HAL SoC handle
 *
 * This function attaches the default HAL operations.
 *
 * Return: void
 */
void dal_vndr_hal_ops_attach(void *hal_soc);

#endif /* _DAL_VNDR_HAL_API_H_ */
