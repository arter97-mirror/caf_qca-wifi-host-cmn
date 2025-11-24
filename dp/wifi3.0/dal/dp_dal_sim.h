/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: ISC
 */

#ifndef DP_DAL_SIM_H
#define DP_DAL_SIM_H

#include "dp_dal.h"
#include <qdf_atomic.h>
#include <qdf_list.h>

/**
 * struct dp_dal_sim_desc_list - Descriptor list for ring IDs
 * @hp: Head pointer
 * @tp: Tail pointer
 * @entries: Array of descriptor entries
 * @list_size: Maximum size of the list
 * @lock: Spinlock to protect the list
 */
struct dp_dal_sim_desc_list {
	uint16_t hp;
	uint16_t tp;
	void **entries;
	uint16_t list_size;
	qdf_spinlock_t lock;
};

/* Default descriptor list size */
#define DP_DAL_SIM_DESC_LIST_SIZE 128

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
 * struct dal_sim_error_stats - DAL simulator error statistics
 * @mode_switch_desc_list_timeout: Count of forced mode switches due to desc
 * list processing  timeout
 * @bypass_mode_switch_ind_fail: bypass mode switch indication failure count
 * @offload_mode_switch_ind_fail: offload mode switch indication failure count
 */
struct dal_sim_error_stats {
	uint32_t mode_switch_desc_list_timeout;
	uint32_t bypass_mode_switch_ind_fail;
	uint32_t offload_mode_switch_ind_fail;
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
 * @error_stats: Error statistics for DAL simulator
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
	struct dal_sim_error_stats error_stats;
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
 * @rx_desc_list: Descriptor lists for RX rings (per ring ID)
 * @tx_cpl_desc_list: Descriptor lists for TX completion rings (per ring ID)
 * @stats: Statistics for the simulator
 * @sim_ctx_initialized: Flag indicating if context is initialized
 * @dev: Pointer to device
 * @intf_info: Interface information maintained in DAL sim context
 * @dev_base_addr: device base address
 * @sim_mode_switch_in_progress: Flag indicating if mode switch is in progress
 * @rxbm_sync_lock: Lock for rxbm_sync operations during mode switch
 * @active_tx_desc_list_cnt: count of active tx desc list being processed
 * @active_rx_desc_list_cnt: count of active rx desc list being processed
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

	/* Descriptor lists for maintaining descriptors per ring ID */
	struct dp_dal_sim_desc_list rx_desc_list[DAL_SIM_NUM_RX_RINGS];
	struct dp_dal_sim_desc_list tx_cpl_desc_list[DAL_SIM_NUM_TX_RINGS];

	/* Statistics */
	struct dal_sim_stats stats;

	/* Initialization flag */
	bool sim_ctx_initialized;

	struct device *dev;

	/* Interface information maintained in DAL sim context */
	struct dal_intf_info intf_info[DAL_INTF_TYPE_MAX];
	void *dev_base_addr;

	/* Mode switch control */
	qdf_atomic_t sim_mode_switch_in_progress;
	qdf_spinlock_t rxbm_sync_lock;

	/* Descriptor processing counters for mode switch synchronization */
	qdf_atomic_t active_tx_desc_list_cnt;
	qdf_atomic_t active_rx_desc_list_cnt;
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

/**
 * dp_dal_sim_desc_list_init() - Initialize descriptor list
 * @desc_list: Pointer to descriptor list structure
 * @list_size: Maximum size of the list
 *
 * This function initializes a descriptor list with the specified size.
 * It allocates memory for the entries array and initializes HP/TP and lock.
 *
 * Return: 0 on success, negative error code on failure
 */
int dp_dal_sim_desc_list_init(struct dp_dal_sim_desc_list *desc_list,
			      uint16_t list_size);

/**
 * dp_dal_sim_desc_list_deinit() - Deinitialize descriptor list
 * @desc_list: Pointer to descriptor list structure
 *
 * This function deinitializes a descriptor list and frees allocated memory.
 */
void dp_dal_sim_desc_list_deinit(struct dp_dal_sim_desc_list *desc_list);

/**
 * dp_dal_sim_is_desc_list_empty() - Check if descriptor list is empty in a
 * thread-safe manner
 * @desc_list: Pointer to descriptor list structure
 *
 * Returns true if the descriptor list is empty, false otherwise.
 * The function takes and releases the list lock for thread safety.
 */
bool dp_dal_sim_is_desc_list_empty(struct dp_dal_sim_desc_list *desc_list);

/**
 * dp_dal_sim_desc_list_enqueue() - Enqueue descriptor to list
 * @desc_list: Pointer to descriptor list structure
 * @desc: Descriptor pointer to enqueue
 *
 * This function enqueues a descriptor to the list using HP/TP mechanism.
 * It's called by the offload engine to queue descriptors.
 *
 * Return: 0 on success, negative error code on failure
 */
static inline int dp_dal_sim_desc_list_enqueue(
	struct dp_dal_sim_desc_list *desc_list,
	void *desc)
{
	uint16_t hp, tp, num_entries;

	if (!desc_list) {
		dp_err("NULL descriptor list pointer");
		return -EINVAL;
	}

	if (!desc) {
		dp_err("NULL descriptor pointer");
		return -EINVAL;
	}

	hp = desc_list->hp;
	tp = desc_list->tp;

	/* Calculate available entries using IPA-style logic */
	if (tp > hp)
		num_entries = (tp - hp - 1);
	else
		num_entries = (desc_list->list_size - hp + tp - 1);

	if (!num_entries) {
		dp_err_rl("Descriptor list is full, HP=%u, TP=%u",
			  hp, tp);
		return -ENOSPC;
	}

	/* Store descriptor at current HP position */
	desc_list->entries[hp] = desc;

	/* Update HP using bit masking */
	hp++;
	hp &= (desc_list->list_size - 1);
	desc_list->hp = hp;

	dp_debug("Descriptor enqueued, HP=%u, TP=%u", hp, tp);
	return 0;
}

/**
 * dp_dal_sim_desc_list_dequeue() - Dequeue descriptor from list
 * @desc_list: Pointer to descriptor list structure
 *
 * This function dequeues a descriptor from the list using HP/TP mechanism.
 * It's called by DAL sim to get descriptors for WLAN driver.
 *
 * Return: Descriptor pointer on success, NULL if list is empty
 */
static inline void *dp_dal_sim_desc_list_dequeue(
	struct dp_dal_sim_desc_list *desc_list)
{
	void *desc = NULL;
	uint16_t hp, tp;

	if (!desc_list) {
		dp_err("NULL descriptor list pointer");
		return NULL;
	}

	hp = desc_list->hp;
	tp = desc_list->tp;

	/* Check if list is empty */
	if (hp == tp) {
		dp_debug("Descriptor list is empty, HP=%u, TP=%u", hp, tp);
		return NULL;
	}

	/* Get descriptor from current TP position */
	desc = desc_list->entries[tp];

	/* Update TP using bit masking */
	tp++;
	tp &= (desc_list->list_size - 1);
	desc_list->tp = tp;

	dp_debug("Descriptor dequeued, HP=%u, TP=%u", hp, tp);
	return desc;
}

/**
 * dp_dal_sim_desc_list_access_start() - Start accessing descriptor list
 * @desc_list: Pointer to descriptor list structure
 *
 * This function takes the spinlock for the descriptor list to allow
 * safe access for multiple dequeue operations. Must be paired with
 * dp_dal_sim_desc_list_access_end().
 */
static inline void dp_dal_sim_desc_list_access_start(
	struct dp_dal_sim_desc_list *desc_list)
{
	if (!desc_list) {
		dp_err("NULL descriptor list pointer in access_start");
		return;
	}

	qdf_spin_lock_bh(&desc_list->lock);
}

/**
 * dp_dal_sim_desc_list_access_end() - End accessing descriptor list
 * @desc_list: Pointer to descriptor list structure
 *
 * This function releases the spinlock for the descriptor list after
 * completing multiple dequeue operations. Must be paired with
 * dp_dal_sim_desc_list_access_start().
 */
static inline void dp_dal_sim_desc_list_access_end(
	struct dp_dal_sim_desc_list *desc_list)
{
	if (!desc_list) {
		dp_err("NULL descriptor list pointer in access_end");
		return;
	}

	qdf_spin_unlock_bh(&desc_list->lock);
}

/**
 * dp_dal_sim_trigger_mode_switch() - Trigger DAL sim mode switch
 * @soc_hdl: soc handle
 * @mode_requested: requested mode
 * This function sets the sim_mode_switch_in_progress flag to true and
 * blocks all replenish, tx, rx requests from wlan driver.
 *
 * Return: None
 */
void dp_dal_sim_trigger_mode_switch(
	struct cdp_soc_t *soc_hdl, uint8_t mode_requested);

/**
 * dp_dal_sim_get_curr_mode() - Get current mode of dal sim
 *
 * This function gets the current mode of dal sim.
 *
 * Return: current mode of dal sim (0 = bypass, 1 = offload)
 */
uint8_t dp_dal_sim_get_curr_mode(void);

#endif /* FEATURE_DP_DAL_SIM */
#endif /* DP_DAL_SIM_H */
