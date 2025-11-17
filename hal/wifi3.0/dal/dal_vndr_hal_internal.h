/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: ISC
 */

#ifndef _DAL_VNDR_HAL_INTERNAL_H_
#define _DAL_VNDR_HAL_INTERNAL_H_

#include <linux/types.h>
#include <linux/spinlock.h>
#define dal_vndr_hal_srng_lock_t spinlock_t
#define DAL_VNDR_SRNG_LOCK_INIT(_lock) spin_lock_init(_lock)
#define DAL_VNDR_SRNG_LOCK(_lock) spin_lock_bh(_lock)
#define DAL_VNDR_SRNG_TRY_LOCK(_lock) spin_trylock_bh(_lock)
#define DAL_VNDR_SRNG_UNLOCK(_lock) spin_unlock_bh(_lock)
#define DAL_VNDR_SRNG_LOCK_DESTROY(_lock) /* nothing to do */

/**
 * struct dal_vndr_hal_buf_info - RX MSDU Descriptor
 * @paddr: physical address
 * @sw_cookie: cookie used by SW to identify the buffer
 * @rbm: rbm id
 *
 */
struct dal_vndr_hal_buf_info {
	uint64_t paddr;
	uint32_t sw_cookie;
	uint8_t rbm;
};

/**
 * struct dal_vndr_hal_rx_msdu_desc_info - RX MSDU Descriptor
 * @msdu_flags: [0] first_msdu_in_mpdu
 *              [1] last_msdu_in_mpdu
 *              [2] msdu_continuation - MSDU spread across buffers
 *             [23] sa_is_valid - SA match in peer table
 *             [24] sa_idx_timeout - Timeout while searching for SA match
 *             [25] da_is_valid - Used to identtify intra-bss forwarding
 *             [26] da_is_MCBC
 *             [27] da_idx_timeout - Timeout while searching for DA match
 * @msdu_len: length of the MSDU (14 bits of length)
 *
 */
struct dal_vndr_hal_rx_msdu_desc_info {
	uint32_t msdu_flags;
	uint16_t msdu_len;
};

struct dal_vndr_hal_hw_txrx_ops {
	/* tx */
	void (*dal_vndr_hal_tx_desc_set_lmac_id)(void *desc, uint8_t lmac_id);
	void (*dal_vndr_hal_tx_desc_set_buf_addr)(void *desc, dma_addr_t paddr,
						  uint8_t pool_id,
						  uint32_t desc_id,
						  uint8_t type);
	void (*dal_vndr_hal_tx_desc_set_buf_length)(void *desc,
						    uint16_t data_length);
	void (*dal_vndr_hal_tx_desc_set_buf_offset)(void *desc, uint8_t offset);
	void (*dal_vndr_hal_tx_desc_set_l3_checksum_en)(void *desc, uint8_t en);
	void (*dal_vndr_hal_tx_desc_set_l4_checksum_en)(void *desc, uint8_t en);
	void (*dal_vndr_hal_tx_desc_set_bank_id)(void *desc, uint8_t bank_id);
	void (*dal_vndr_hal_tx_desc_set_vdev_id)(void *desc, uint8_t vdev_id);
	void (*dal_vndr_hal_tx_desc_set_hlos_tid)(void *desc, uint8_t hlos_tid);
	void (*dal_vndr_hal_tx_desc_set_flow_override_enable)(void *desc,
							      uint8_t val);
	void (*dal_vndr_hal_tx_desc_set_flow_override)(void *desc, uint8_t val);
	void (*dal_vndr_hal_tx_desc_set_who_classify_info_sel)(void *desc,
							       uint8_t val);
	void (*dal_vndr_hal_tx_desc_set_tx_notify_frame)(void *desc,
							 uint8_t val);
	void (*dal_vndr_hal_tx_comp_get_status)(void *desc, void *ts1);
	dma_addr_t (*dal_vndr_hal_tx_comp_get_paddr)(void *hal_desc);
	/* rx */
	uint8_t (*dal_vndr_hal_rx_error_status_get)(void *reo_desc);
	uint32_t (*dal_vndr_hal_rx_reo_buf_cookie_get)(void *reo_desc);
	uint8_t (*dal_vndr_hal_rx_ret_buf_manager_get)(void *ring_desc);
	void (*dal_vndr_hal_rx_reo_get_details)(
				void *ring_desc,
				struct dal_vndr_hal_buf_info *buf_info);
	uint32_t (*dal_vndr_hal_rx_msdu_start_nss_get)(uint8_t *buff);
	uint32_t (*dal_vndr_hal_rx_tlv_sgi_get)(uint8_t *buf);
	uint32_t (*dal_vndr_hal_rx_tlv_rate_mcs_get)(uint8_t *buf);
	uint32_t (*dal_vndr_hal_rx_tlv_bw_get)(uint8_t *buf);
	uint32_t (*dal_vndr_hal_rx_tlv_get_pkt_type)(uint8_t *buf);
	uint8_t (*dal_vndr_hal_rx_get_l3_pad_bytes)(uint8_t *buf);
	uint32_t (*dal_vndr_hal_rx_tlv_msdu_done_get)(uint8_t *buf);
	dma_addr_t (*dal_vndr_hal_rx_reo_buf_paddr_get)(void *ring_desc);
	void (*dal_vndr_hal_rx_msdu_desc_info_get)(
			void *desc_addr,
			struct dal_vndr_hal_rx_msdu_desc_info *msdu_desc_info);
	void (*dal_vndr_hal_rxdma_buff_addr_info_set)(void *rxdma_entry,
						      dma_addr_t paddr,
						      uint32_t cookie,
						      uint8_t manager);
	void (*dal_vndr_hal_rxbm_sync)(void *ring_desc, void *buff_addr_info);
};

enum dal_vndr_hal_srng_dir {
	DAL_VNDR_HAL_SRNG_SRC_RING,
	DAL_VNDR_HAL_SRNG_DST_RING
};

/**
 * struct dal_vndr_hal_soc - HAL context to be used to access SRNG APIs
 * @dev_base_addr: Device base address
 * @ops: TXRX operations
 */
struct dal_vndr_hal_soc {
	void *dev_base_addr;
	struct dal_vndr_hal_hw_txrx_ops *ops;
};

struct dal_vndr_hal_srng {
	/* Unique SRNG ring ID */
	uint8_t ring_id;
	/* Physical base address of the ring */
	dma_addr_t ring_base_paddr;
	/* base address of the ring used for fetching descriptors */
	uint32_t *ring_base_addr;
	/* Number of entries in ring */
	uint32_t num_entries;
	/* Ring size */
	uint32_t ring_size;
	/* Ring size mask */
	uint32_t ring_size_mask;
	/* Whether LMAC ring */
	bool lmac_ring;
	/* Size of ring entry */
	uint32_t entry_size;
	/* MSI Address */
	dma_addr_t msi_addr;
	/* MSI data */
	uint32_t msi_data;
	/* Misc flags */
	uint32_t flags;
	/* Ring type/name */
	int ring_type;
	/* irq number */
	int irq_num;
	/* Source or Destination ring */
	enum dal_vndr_hal_srng_dir ring_dir;

	union {
		struct {
			/* SW tail pointer */
			uint32_t tp;

			/* Shadow head pointer location to be updated by HW */
			uint32_t *hp_addr;

			/* Cached head pointer */
			uint32_t cached_hp;

			/* Tail pointer location to be updated by SW – This
			 * will be a register address and need not be
			 * accessed through SW structure
			 */
			uint32_t *tp_addr;
		} dst_ring;

		struct {
			/* SW head pointer */
			uint32_t hp;

			/* Shadow tail pointer location to be updated by HW */
			uint32_t *tp_addr;

			/* Cached tail pointer */
			uint32_t cached_tp;

			/* Head pointer location to be updated by SW – This
			 * will be a register address and need not be accessed
			 * through SW structure
			 */
			uint32_t *hp_addr;
		} src_ring;
	} u;

	struct dal_vndr_hal_soc *hal_soc;
	/* Lock for serializing ring index updates */
	dal_vndr_hal_srng_lock_t lock;
};
#endif /* _DAL_VNDR_HAL_INTERNAL_H_ */
