/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: ISC
 */

#ifndef DP_DAL_SIM_H
#define DP_DAL_SIM_H

#include "dp_dal.h"
#include <qdf_atomic.h>

/* Externs */
extern struct platform_bus_ops *global_plat_ops;
extern struct platform_bus_ops plat_ops_bypass_mode;
extern struct vendor_cb_ops vendor_cb;
/**
 * dp_dal_sim_platform_bus_ops_attach() - Attach platform bus operations
 *
 * Based on g_dal_sim_curr_mode, assigns the appropriate platform operations
 * to the global platform ops structure:
 * - Mode 0 (bypass): Assigns plat_ops_bypass_mode
 * - Mode 1 (offload): Assigns dal_sim_plat_ops
 */
void dp_dal_sim_platform_bus_ops_attach(void);

#ifdef FEATURE_DP_DAL_SIM

/* Number of RX and TX rings for DAL simulator */
#define DAL_SIM_NUM_RX_RINGS DAL_RX_RINGS_MAX
#define DAL_SIM_NUM_TX_RINGS DAL_TX_RINGS_MAX

/**
 * struct dal_sim_work_ctx - Work context for ring-specific processing
 * @sim_ctx: Pointer to DAL simulation context
 * @ring_id: Ring ID (0 or 1)
 */
struct dal_sim_work_ctx {
	struct dp_dal_sim_ctx *sim_ctx;
	uint8_t ring_id;
};

/**
 * struct dal_sim_stats - DAL simulator statistics per ring
 * @tx_enqueued: Number of TX packets enqueued
 * @tx_completed: Number of TX completions processed
 * @rx_received: Number of RX packets received
 * @rx_replenished: Number of RX buffers replenished (global)
 * @rx_isr_count: Number of RX ISR invocations
 * @tx_cpl_isr_count: Number of TX completion ISR invocations
 * @rx_work_queued: Number of times RX work was queued
 * @tx_cpl_work_queued: Number of times TX completion work was queued
 * @rx_work_scheduled: Number of times RX work was scheduled
 * @tx_work_scheduled: Number of times TX completion work was scheduled
 */
struct dal_sim_stats {
	uint64_t tx_enqueued[DAL_SIM_NUM_TX_RINGS];
	uint64_t tx_completed[DAL_SIM_NUM_TX_RINGS];
	uint64_t rx_received[DAL_SIM_NUM_RX_RINGS];
	uint64_t rx_replenished;
	uint64_t rx_isr_count[DAL_SIM_NUM_RX_RINGS];
	uint64_t tx_cpl_isr_count[DAL_SIM_NUM_TX_RINGS];
	uint64_t rx_work_queued[DAL_SIM_NUM_RX_RINGS];
	uint64_t tx_cpl_work_queued[DAL_SIM_NUM_TX_RINGS];
	uint64_t rx_work_scheduled[DAL_SIM_NUM_RX_RINGS];
	uint64_t tx_work_scheduled[DAL_SIM_NUM_TX_RINGS];
};

/**
 * struct dal_sim_srng - Ring information for DAL sim rings
 * @hal_ring_id: hal ring id
 * @initialized: ring initialized or not
 * @ring_base_paddr: ring base paddr
 * @ring_base_vaddr: ring base vaddr
 * @num_entries: number of entries in this ring
 * @ring_size: ring size
 * @ring_size_mask: ring size mask
 * @entry_size: size of the ring entry
 * @lmac_ring: ring is lmac ring or not
 * @ring_type: HAL ring type
 * @ring_dir: ring direction src or dest
 * @u: Union containing ring pointer information based on ring direction
 * @u.dst_ring: Destination ring pointer structure
 * @u.dst_ring.tp: Tail pointer value for destination ring
 * @u.dst_ring.hp_addr: Head pointer address (UMAC: HP offset from BAR,
 *                      LMAC: HP physical addr)
 * @u.dst_ring.tp_addr: Tail pointer address (UMAC: TP offset from BAR,
 *                      LMAC: TP physical addr)
 * @u.src_ring: Source ring pointer structure
 * @u.src_ring.hp: Head pointer value for source ring
 * @u.src_ring.tp_addr: Tail pointer address (UMAC: TP offset from BAR,
 *                      LMAC: TP physical addr)
 * @u.src_ring.hp_addr: Head pointer address (UMAC: HP offset from BAR,
 *                      LMAC: HP physical addr)
 * @ring_num: ring number used to indicate isr indication
 * @irq_num: irq number
 * @grp_id: ext group context id associated with the ring
 * @msi_addr: MSI address
 * @msi_data: MSI Data
 *
 * This structure has all the ring information that will be passed to DAL
 * during init.
 */
struct dal_sim_srng {
	uint8_t hal_ring_id;
	bool initialized;
	qdf_dma_addr_t ring_base_paddr;
	uint32_t *ring_base_vaddr;
	uint32_t num_entries;
	uint32_t ring_size;
	uint32_t ring_size_mask;
	uint32_t entry_size;
	bool lmac_ring;
	enum hal_ring_type ring_type;
	int ring_dir;
	union {
		struct {
			uint32_t tp;
			unsigned long hp_addr;
			unsigned long tp_addr;
		} dst_ring;
		struct {
			uint32_t hp;
			unsigned long tp_addr;
			unsigned long hp_addr;
		} src_ring;
	} u;

	uint8_t ring_num;
	int irq_num;
	uint8_t grp_id;
	qdf_dma_addr_t msi_addr;
	uint32_t msi_data;
};

/**
 * struct dp_dal_sim_ctx - Context structure for DAL simulation
 * @dp_dal_ctx: Pointer to DAL datapath context
 * @offload_sim_ctx: Pointer to offload simulation context
 * @vendor_ops: Structure containing vendor callback operations
 * @rx_process_work: Work structure for RX processing
 * @rx_work_queue: Workqueue for RX tasks
 * @tx_compl_process_work: Work structure for TX completion processing
 * @tx_compl_work_queue: Workqueue for TX completion tasks
 * @rx_work_ctx: Work context for RX processing per ring
 * @tx_cpl_work_ctx: Work context for TX completion processing per ring
 * @rx_work_scheduled: Atomic variable indicating RX work scheduling status
 * @tx_compl_work_scheduled: Atomic variable indicating TX completion work
 *                           scheduling status
 * @rx_ring: Array of HAL SRNG structures for RX rings
 * @tx_cmpl_ring: Array of HAL SRNG structures for TX completion rings
 * @tx_ring: Array of HAL SRNG structures for TX rings
 * @rx_refill_ring: HAL SRNG structure for RX refill ring
 * @stats: Statistics for the simulator
 * @sim_ctx_initialized: Flag indicating if context is initialized
 * @dev: Pointer to device
 * @intf_info: Interface information maintained in DAL sim context
 * @dev_base_addr: device base address
 *
 * This structure maintains all necessary context for DAL simulation,
 * including pointers to datapath context, platform operations, vendor
 * callbacks, work structures, workqueues, atomic scheduling indicators,
 * and ring buffers for RX and TX operations.
 */
struct dp_dal_sim_ctx {
	void *dp_dal_ctx;
	void *offload_sim_ctx;

	/* Work structures for async processing - separate for each ring */
	qdf_work_t rx_process_work[DAL_SIM_NUM_RX_RINGS];
	qdf_workqueue_t *rx_work_queue[DAL_SIM_NUM_RX_RINGS];
	qdf_work_t tx_compl_process_work[DAL_SIM_NUM_TX_RINGS];
	qdf_workqueue_t *tx_compl_work_queue[DAL_SIM_NUM_TX_RINGS];

	/* Work context for each ring */
	struct dal_sim_work_ctx rx_work_ctx[DAL_SIM_NUM_RX_RINGS];
	struct dal_sim_work_ctx tx_cpl_work_ctx[DAL_SIM_NUM_TX_RINGS];

	/* Atomic scheduling indicators - separate for each ring */
	qdf_atomic_t rx_work_scheduled[DAL_SIM_NUM_RX_RINGS];
	qdf_atomic_t tx_compl_work_scheduled[DAL_SIM_NUM_TX_RINGS];

	/* Ring structures */
	struct dal_sim_srng rx_ring[DAL_SIM_NUM_RX_RINGS];
	struct dal_sim_srng tx_cmpl_ring[DAL_SIM_NUM_TX_RINGS];
	struct dal_sim_srng tx_ring[DAL_SIM_NUM_TX_RINGS];
	struct dal_sim_srng rx_refill_ring;

	/* Statistics */
	struct dal_sim_stats stats;

	/* Initialization flag */
	bool sim_ctx_initialized;

	struct device *dev;

	/* Interface information maintained in DAL sim context */
	struct dal_intf_info intf_info[DAL_INTF_TYPE_MAX];
	void *dev_base_addr;
};

/**
 * dp_dal_sim_schedule_work() - Schedule work for interrupt processing
 * @arg: Pointer to offload_sim_irq_ctx structure
 *
 * This function is called by the offload simulation interrupt handler
 * to queue work for processing the interrupt. It checks if work is
 * already scheduled and queues work on the appropriate work queue.
 */
void dp_dal_sim_schedule_work(void *arg);
#endif /* FEATURE_DP_DAL_SIM */
#endif /* DP_DAL_SIM_H */
