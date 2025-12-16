/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: ISC
 */

#include "dp_dal_sim.h"
#include "dp_dal.h"
#include <qdf_mem.h>
#include <qdf_trace.h>
#include <qdf_module.h>
#include "dp_internal.h"
#include "dp_dal_offload_sim.h"

#ifdef FEATURE_DP_DAL_SIM
/* RX budget for processing descriptors */
#define DP_DAL_SIM_RX_BUDGET 64
#define DP_DAL_SIM_TX_BUDGET 64

/* Mode switch descriptor list processing wait parameters */
#define DP_DAL_SIM_MODE_SWITCH_MAX_RETRIES	10
#define DP_DAL_SIM_MODE_SWITCH_TIMEOUT_MS	10

struct platform_bus_ops *global_plat_ops;
extern struct vendor_cb_ops vendor_cb;
extern struct platform_bus_ops plat_ops_bypass_mode;

/* ========================================================================
 * Descriptor List Management Functions
 * ========================================================================
 */

int dp_dal_sim_desc_list_init(struct dp_dal_sim_desc_list *desc_list,
			      uint16_t list_size)
{
	if (!desc_list) {
		dp_err("NULL descriptor list pointer");
		return -EINVAL;
	}

	if (list_size == 0 || (list_size & (list_size - 1)) != 0) {
		dp_err("Invalid list size: %u (must be power of 2)", list_size);
		return -EINVAL;
	}

	/* Allocate memory for entries array */
	desc_list->entries = qdf_mem_malloc(sizeof(void *) * list_size);
	if (!desc_list->entries) {
		dp_err("Failed to allocate memory for descriptor entries");
		return -ENOMEM;
	}

	/* Initialize HP/TP and list size */
	desc_list->hp = 0;
	desc_list->tp = 0;
	desc_list->list_size = list_size;

	/* Initialize spinlock */
	qdf_spinlock_create(&desc_list->lock);

	dp_debug("Descriptor list initialized with size %u", list_size);
	return 0;
}

void dp_dal_sim_desc_list_deinit(struct dp_dal_sim_desc_list *desc_list)
{
	if (!desc_list) {
		dp_warn("NULL descriptor list pointer in deinit");
		return;
	}

	/* Free entries array */
	if (desc_list->entries) {
		qdf_mem_free(desc_list->entries);
		desc_list->entries = NULL;
		/* Destroy spinlock */
		qdf_spinlock_destroy(&desc_list->lock);
	}

	/* Reset HP/TP and list size */
	desc_list->hp = 0;
	desc_list->tp = 0;
	desc_list->list_size = 0;

	dp_debug("Descriptor list deinitialized");
}

/**
 * dp_dal_sim_is_desc_list_empty() - Check if descriptor list is empty in a
 * thread-safe manner
 * @desc_list: Pointer to descriptor list structure
 *
 * Takes the list lock, checks if hp == tp, releases the lock,
 * returns true if the descriptor list is empty, false otherwise.
 */
bool dp_dal_sim_is_desc_list_empty(struct dp_dal_sim_desc_list *desc_list)
{
	bool empty;

	if (!desc_list)
		return true;

	qdf_spin_lock_bh(&desc_list->lock);
	empty = (desc_list->hp == desc_list->tp);
	qdf_spin_unlock_bh(&desc_list->lock);

	return empty;
}

/**
 * dp_dal_sim_desc_lists_init() - Initialize all descriptor lists
 * @sim_ctx: Pointer to DAL simulation context
 *
 * This function initializes all RX and TX completion descriptor lists
 * for all rings in the simulation context.
 *
 * Return: 0 on success, negative error code on failure
 */
static int dp_dal_sim_desc_lists_init(struct dp_dal_sim_ctx *sim_ctx)
{
	int i, status;

	if (!sim_ctx) {
		dp_err("NULL simulation context");
		return -EINVAL;
	}

	/* Initialize descriptor lists for RX rings */
	for (i = 0; i < DAL_SIM_NUM_RX_RINGS; i++) {
		status = dp_dal_sim_desc_list_init(&sim_ctx->rx_desc_list[i],
						   DP_DAL_SIM_DESC_LIST_SIZE);
		if (status) {
			dp_err("Failed to init RX desc list for ring %d", i);
			goto cleanup_rx_lists;
		}
	}

	/* Initialize descriptor lists for TX completion rings */
	for (i = 0; i < DAL_SIM_NUM_TX_RINGS; i++) {
		status = dp_dal_sim_desc_list_init(
					&sim_ctx->tx_cpl_desc_list[i],
					DP_DAL_SIM_DESC_LIST_SIZE);
		if (status) {
			dp_err("Failed to init TX cpl desc list for ring %d",
			       i);
			goto cleanup_tx_lists;
		}
	}

	dp_debug("All descriptor lists initialized successfully");
	return 0;

cleanup_tx_lists:
	for (i = 0; i < DAL_SIM_NUM_TX_RINGS; i++)
		dp_dal_sim_desc_list_deinit(&sim_ctx->tx_cpl_desc_list[i]);

cleanup_rx_lists:
	for (i = 0; i < DAL_SIM_NUM_RX_RINGS; i++)
		dp_dal_sim_desc_list_deinit(&sim_ctx->rx_desc_list[i]);

	return status;
}

/**
 * dp_dal_sim_desc_lists_deinit() - Deinitialize all descriptor lists
 * @sim_ctx: Pointer to DAL simulation context
 *
 * This function deinitializes all RX and TX completion descriptor lists
 * for all rings in the simulation context.
 */
static void dp_dal_sim_desc_lists_deinit(struct dp_dal_sim_ctx *sim_ctx)
{
	int i;

	if (!sim_ctx) {
		dp_warn("NULL simulation context in desc lists deinit");
		return;
	}

	for (i = 0; i < DAL_SIM_NUM_RX_RINGS; i++)
		dp_dal_sim_desc_list_deinit(&sim_ctx->rx_desc_list[i]);

	for (i = 0; i < DAL_SIM_NUM_TX_RINGS; i++)
		dp_dal_sim_desc_list_deinit(&sim_ctx->tx_cpl_desc_list[i]);

	dp_debug("All descriptor lists deinitialized successfully");
}

/* ========================================================================
 * Platform Bus Operations - Offload Mode Implementation
 * ========================================================================
 */

/**
 * dp_dal_sim_calc_msi() - Calculate MSI and store in dal_sim_srng
 * @sim_ctx: DAL sim context
 * @sim_ring: DAL sim ring pointer
 *
 * Return: 0 on success, error code on failure
 */
static int dp_dal_sim_calc_msi(struct dp_dal_sim_ctx *sim_ctx,
			       struct dal_sim_srng *sim_ring)
{
	if (!sim_ctx || !sim_ring) {
		dp_info("Null context");
		return -EINVAL;
	}

	/* Calculate msi addr, msi data using grp_id */
	int msi_vector_count, ret, grp_id, irq_num;
	uint32_t msi_base_data, msi_vector_start, addr_low, addr_high;
	unsigned int vector;

	struct dp_dal_ctx *dp_dal_ctx =
			(struct dp_dal_ctx *)sim_ctx->dp_dal_ctx;
	struct dp_soc *soc = (struct dp_soc *)(dp_dal_ctx->soc);

	if (!soc) {
		dp_err("Null dp soc context");
		return -EINVAL;
	}

	ret = pld_get_user_msi_assignment(soc->osdev->dev, "DP",
					  &msi_vector_count,
					  &msi_base_data,
					  &msi_vector_start);
	if (ret) {
		dp_err("get user msi failed with ret = %d", ret);
		return ret;
	}
	grp_id = sim_ring->grp_id;
	vector = (grp_id % msi_vector_count) + msi_vector_start;
	pld_get_msi_address(soc->osdev->dev, &addr_low, &addr_high);

	sim_ring->msi_data = vector;
	sim_ring->msi_addr = addr_low;
	sim_ring->msi_addr |= (qdf_dma_addr_t)(((uint64_t)addr_high) << 32);
	irq_num = pld_get_msi_irq(soc->osdev->dev, vector);

	sim_ring->irq_num = irq_num;
	return 0;
}

/**
 * dp_dal_sim_ring_init() - Parse ring information from driver and save in dal
 * sim context
 * @sim_ctx: Pointer to sim ctx
 * @ring_info: Pointer to ring info
 * @sim_ring: DAL sim ring pointer
 *
 * Parse and initialize the dal_sim_srng.
 *
 * Return: 0 on success, error code on failure
 */
static int dp_dal_sim_ring_init(struct dp_dal_sim_ctx *sim_ctx,
				struct dal_srng *ring_info,
				struct dal_sim_srng *sim_ring)
{
	if (!sim_ctx || !sim_ring) {
		dp_info("Null context");
		return -EINVAL;
	}

	/* Copy basic ring information */
	sim_ring->hal_ring_id = ring_info->hal_ring_id;
	sim_ring->initialized = ring_info->initialized;
	sim_ring->ring_base_paddr = ring_info->ring_base_paddr;
	sim_ring->ring_base_vaddr = ring_info->ring_base_vaddr;
	sim_ring->num_entries = ring_info->num_entries;
	sim_ring->ring_size = ring_info->ring_size;
	sim_ring->ring_size_mask = ring_info->ring_size_mask;
	sim_ring->entry_size = ring_info->entry_size;
	sim_ring->lmac_ring = ring_info->lmac_ring;
	sim_ring->ring_type = ring_info->ring_type;
	sim_ring->ring_dir = ring_info->ring_dir;

	/* Copy ring-specific pointers based on ring direction */
	if (sim_ring->ring_dir == HAL_SRNG_SRC_RING) {
		sim_ring->u.src_ring.hp = ring_info->u.src_ring.hp;
		sim_ring->u.src_ring.tp_addr = ring_info->u.src_ring.tp_addr;
		sim_ring->u.src_ring.hp_addr = ring_info->u.src_ring.hp_addr;
	} else {
		sim_ring->u.dst_ring.tp = ring_info->u.dst_ring.tp;
		sim_ring->u.dst_ring.hp_addr = ring_info->u.dst_ring.hp_addr;
		sim_ring->u.dst_ring.tp_addr = ring_info->u.dst_ring.tp_addr;
	}

	sim_ring->ring_num = ring_info->ring_num;
	sim_ring->grp_id = ring_info->grp_id;
	return dp_dal_sim_calc_msi(sim_ctx, sim_ring);
}

/**
 * dp_dal_sim_parse_ring_info() - Parse ring information from driver
 * @sim_ctx: Pointer to simulator context
 * @priv: Pointer to private data from driver
 *
 * Parses the ring information passed from the driver and stores it
 * in the simulator context.
 *
 * Return: 0 on success, error code on failure
 */
static int dp_dal_sim_parse_ring_info(struct dp_dal_sim_ctx *sim_ctx,
				      void *priv)
{
	struct dp_dal_ctx *dal_ctx = (struct dp_dal_ctx *)priv;
	int i, ret;

	if (!dal_ctx || !sim_ctx) {
		dp_err("NULL context in parse_ring_info");
		return -EINVAL;
	}

	/* Parse RX ring information */
	for (i = 0; i < DAL_SIM_NUM_RX_RINGS; i++) {
		if (dal_ctx->rx_ring[i].initialized) {
			ret = dp_dal_sim_ring_init(sim_ctx,
						   &dal_ctx->rx_ring[i],
						   &sim_ctx->rx_ring[i]);
			if (ret)
				return ret;
		} else {
			dp_err("Invalid ring info for RX ring[%d]", i);
			return -EINVAL;
		}
	}

	/* Parse TX completion ring information */
	for (i = 0; i < DAL_SIM_NUM_TX_RINGS; i++) {
		if (dal_ctx->tx_cmpl_ring[i].initialized) {
			ret = dp_dal_sim_ring_init(sim_ctx,
						   &dal_ctx->tx_cmpl_ring[i],
						   &sim_ctx->tx_cmpl_ring[i]);
			if (ret)
				return ret;
		} else {
			dp_err("Invalid ring info for TX compl ring[%d]", i);
			return -EINVAL;
		}
	}

	/* Parse TX ring information */
	for (i = 0; i < DAL_SIM_NUM_TX_RINGS; i++) {
		if (dal_ctx->tx_ring[i].initialized) {
			ret = dp_dal_sim_ring_init(sim_ctx,
						   &dal_ctx->tx_ring[i],
						   &sim_ctx->tx_ring[i]);
			if (ret)
				return ret;
		} else {
			dp_err("Invalid ring info for TX ring[%d]", i);
			return -EINVAL;
		}
	}

	/* Parse RX refill ring information */
	if (dal_ctx->rx_refill_ring.initialized) {
		ret = dp_dal_sim_ring_init(sim_ctx, &dal_ctx->rx_refill_ring,
					   &sim_ctx->rx_refill_ring);
		if (ret)
			return ret;
	} else {
		dp_err("Invalid ring info for RX refill ring");
		return -EINVAL;
	}

	return 0;
}

/**
 * dp_dal_sim_rx_work_handler() - RX work handler
 * @arg: Pointer to work context
 *
 * Work handler for processing RX interrupts. Extracts ring_id from
 * work context and calls the vendor RX ISR callback to notify the driver.
 */
static void dp_dal_sim_rx_work_handler(void *arg)
{
	struct dal_sim_work_ctx *work_ctx = (struct dal_sim_work_ctx *)arg;
	struct dp_dal_sim_ctx *sim_ctx;
	int ring_id;
	uint8_t ring_num;

	sim_ctx = work_ctx->sim_ctx;
	ring_id = work_ctx->ring_id;

	if (!sim_ctx) {
		dp_err("NULL sim context in RX work handler");
		return;
	}

	qdf_atomic_set(&sim_ctx->rx_work_scheduled[ring_id], 0);

	/* Get ring_num from dal_sim_srng structure */
	ring_num = sim_ctx->rx_ring[ring_id].ring_num;

	/* Update statistics per ring */
	sim_ctx->stats.rx_work_scheduled[ring_id]++;

	dp_debug("RX work handler executing for ring_id=%d, ring_num=%d",
		 ring_id, ring_num);

	/* Call vendor RX ISR callback to notify driver */
	if (vendor_cb.rx_isr_cb)
		vendor_cb.rx_isr_cb(ring_num, sim_ctx->dp_dal_ctx);
	else
		dp_warn("RX ISR callback not registered");

}

/**
 * dp_dal_sim_tx_cpl_work_handler - TX completion worker handler
 * @arg: Pointer to work context
 *
 * This function processes TX completions in the worker thread context.
 * Extracts ring_id from work context and processes only that specific ring.
 * It reaps the WBM2SW ring, calls the vendor callback with descriptor info,
 * frees the TX buffer, and returns the software TX descriptor to the free list.
 *
 * Return: None
 */
static void dp_dal_sim_tx_cpl_work_handler(void *arg)
{
	struct dal_sim_work_ctx *work_ctx = (struct dal_sim_work_ctx *)arg;
	struct dp_dal_sim_ctx *sim_ctx;
	int ring_id;
	uint8_t ring_num;

	sim_ctx = work_ctx->sim_ctx;
	ring_id = work_ctx->ring_id;

	if (!sim_ctx) {
		dp_err("NULL sim context in TX completion work handler");
		return;
	}

	qdf_atomic_set(&sim_ctx->tx_compl_work_scheduled[ring_id], 0);

	/* Get ring_num from dal_sim_srng structure */
	ring_num = sim_ctx->tx_cmpl_ring[ring_id].ring_num;

	/* Update statistics per ring */
	sim_ctx->stats.tx_work_scheduled[ring_id]++;

	dp_debug("TX compl work handler executing for ring_id=%d, ring_num=%d",
		 ring_id, ring_num);

	/* Call vendor TX ISR callback to notify driver */
	if (vendor_cb.tx_isr_cb)
		vendor_cb.tx_isr_cb(ring_num, sim_ctx->dp_dal_ctx);
	else
		dp_warn("TX ISR callback not registered");

}

/**
 * dp_dal_sim_destroy_work() - Destroy all workqueues and work items
 * present in dal sim
 * @dal_sim_ctx: Pointer to DAL simulation context
 *
 * This function destroys all RX and TX completion workqueues and disables
 * all work items to ensure proper cleanup during deinit or error handling.
 */
static void dp_dal_sim_destroy_work(struct dp_dal_sim_ctx *dal_sim_ctx)
{
	int i;

	if (!dal_sim_ctx) {
		dp_err("NULL DAL sim context in destroy_work");
		return;
	}

	/* Flush and destroy all work */
	for (i = 0; i < DAL_SIM_NUM_RX_RINGS; i++) {
		if (dal_sim_ctx->rx_work_queue[i]) {
			qdf_flush_workqueue(0, dal_sim_ctx->rx_work_queue[i]);
			qdf_destroy_workqueue(0, dal_sim_ctx->rx_work_queue[i]);
			dal_sim_ctx->rx_work_queue[i] = NULL;
		}
		qdf_flush_work(&dal_sim_ctx->rx_process_work[i]);
		qdf_disable_work(&dal_sim_ctx->rx_process_work[i]);
	}

	/* Flush and destroy all work */
	for (i = 0; i < DAL_SIM_NUM_TX_RINGS; i++) {
		if (dal_sim_ctx->tx_compl_work_queue[i]) {
			qdf_flush_workqueue(
					0, dal_sim_ctx->tx_compl_work_queue[i]);
			qdf_destroy_workqueue(
					0, dal_sim_ctx->tx_compl_work_queue[i]);
			dal_sim_ctx->tx_compl_work_queue[i] = NULL;
		}
		qdf_flush_work(&dal_sim_ctx->tx_compl_process_work[i]);
		qdf_disable_work(&dal_sim_ctx->tx_compl_process_work[i]);
	}

	dp_debug("DAL sim workqueues and work items destroyed successfully");
}

/**
 * dp_dal_sim_init() - Initialize DAL simulator
 * @pdev: Pointer to the platform device associated with initialization
 * @priv: Pointer to driver-specific private data
 *
 * This function initializes the DAL simulator.
 *
 * Return: 0 on success, negative error code on failure
 */
static int dp_dal_sim_init(void *pdev, void *priv)
{
	int i;
	struct hal_soc *hal_soc;

	int status = 0;
	struct dp_dal_ctx *dal_ctx = (struct dp_dal_ctx *)priv;
	struct dp_dal_sim_ctx *sim_ctx = NULL;

	if (!dal_ctx) {
		dp_err("NULL DAL context in offload_mode_init");
		return -EINVAL;
	}

	sim_ctx = dal_ctx->dal_sim_ctx;
	if (!sim_ctx) {
		dp_err("DAL SIM context is NULL");
		return -EINVAL;
	}

	/* Get the device base address*/
	hal_soc = (struct hal_soc *)dal_ctx->soc->hal_soc;
	sim_ctx->dev_base_addr = hal_soc->dev_base_addr;

	/* Get the device ptr */
	sim_ctx->dev = dal_ctx->soc->osdev->dev;
	/* Parse ring information from driver */
	status = dp_dal_sim_parse_ring_info(sim_ctx, priv);
	if (status) {
		dp_err("Failed to parse ring information");
		return status;
	}

	/* Initialize offload_sim_ctx */
	status = dp_dal_offload_sim_init(sim_ctx);
	if (status) {
		dp_err("Failed to initialize offload sim context");
		return status;
	}

	char wq_name[32];
	/* Create high priority work queues and initialize work per ring */
	for (i = 0; i < DAL_SIM_NUM_RX_RINGS; i++) {
		/* Create high priority work queue for RX ring */
		qdf_snprintf(wq_name, sizeof(wq_name), "dal_sim_rx_%d", i);
		sim_ctx->rx_work_queue[i] =
				qdf_alloc_high_prior_ordered_workqueue(wq_name);
		if (!sim_ctx->rx_work_queue[i]) {
			dp_err("Failed to create RX workq for ring %d", i);
			status = -ENOMEM;
			goto free_offload_ctx;
		}
		/* Initialize RX work context */
		sim_ctx->rx_work_ctx[i].sim_ctx = sim_ctx;
		sim_ctx->rx_work_ctx[i].ring_id = i;
		/* Create work structures with work contexts */
		qdf_create_work(0, &sim_ctx->rx_process_work[i],
				dp_dal_sim_rx_work_handler,
				&sim_ctx->rx_work_ctx[i]);
		/* Initialize atomic variables for this ring */
		qdf_atomic_init(&sim_ctx->rx_work_scheduled[i]);
	}

	for (i = 0; i < DAL_SIM_NUM_TX_RINGS; i++) {
		/* Create high priority work queue for TX completion ring */
		qdf_snprintf(wq_name, sizeof(wq_name), "dal_sim_tx_cpl_%d", i);
		sim_ctx->tx_compl_work_queue[i] =
				qdf_alloc_high_prior_ordered_workqueue(wq_name);
		if (!sim_ctx->tx_compl_work_queue[i]) {
			dp_err("Failed to create TX compl workq for ring %d",
			       i);
			status = -ENOMEM;
			goto free_offload_ctx;
		}
		/* Initialize TX completion work context */
		sim_ctx->tx_cpl_work_ctx[i].sim_ctx = sim_ctx;
		sim_ctx->tx_cpl_work_ctx[i].ring_id = i;

		qdf_create_work(0, &sim_ctx->tx_compl_process_work[i],
				dp_dal_sim_tx_cpl_work_handler,
				&sim_ctx->tx_cpl_work_ctx[i]);

		qdf_atomic_init(&sim_ctx->tx_compl_work_scheduled[i]);
	}

	/* Initialize descriptor lists for all rings */
	status = dp_dal_sim_desc_lists_init(sim_ctx);
	if (status) {
		dp_err("Failed to initialize descriptor lists");
		goto free_offload_ctx;
	}

	qdf_spinlock_create(&sim_ctx->rxbm_sync_lock);

	/* Initialize descriptor processing counters */
	qdf_atomic_init(&sim_ctx->active_tx_desc_list_cnt);
	qdf_atomic_init(&sim_ctx->active_rx_desc_list_cnt);

	/* Mark as initialized */
	sim_ctx->sim_ctx_initialized = true;

	dp_info("dal sim init complete");
	return status;
free_offload_ctx:
	dp_dal_offload_sim_deinit(sim_ctx);
	/* Destroy any work queues that were successfully created */
	dp_dal_sim_destroy_work(sim_ctx);
	qdf_spinlock_destroy(&sim_ctx->rxbm_sync_lock);

	return status;
}

/**
 * dp_dal_sim_deinit - Deinitialize DP DAL simulation context.
 * @sim_ctx: Pointer to simulation context to clean up.
 */
void dp_dal_sim_deinit(struct dp_dal_sim_ctx *sim_ctx)
{
	if (!sim_ctx)
		return;

	qdf_spinlock_destroy(&sim_ctx->rxbm_sync_lock);
	/* Destroy all work queues and work items */
	dp_dal_sim_destroy_work(sim_ctx);

	/* Deinitialize descriptor lists for all rings */
	dp_dal_sim_desc_lists_deinit(sim_ctx);

	/* Call offload mode exit api to free offload simulation resources */
	dp_dal_offload_sim_deinit(sim_ctx);

	sim_ctx->sim_ctx_initialized = false;
}

/**
 * dp_dal_sim_exit() - Cleanup DAL simulator
 * @priv: Pointer to private data (DAL simulator context)
 *
 * Cleans up all resources allocated during initialization.
 */
static void dp_dal_sim_exit(void *priv)
{
	struct dp_dal_ctx *dp_dal_ctx = (struct dp_dal_ctx *)priv;
	struct dp_dal_sim_ctx *sim_ctx;

	if (!dp_dal_ctx) {
		dp_err("NULL DP DAL context in bus exit");
		return;
	}

	sim_ctx = dp_dal_ctx->dal_sim_ctx;

	if (!sim_ctx) {
		dp_err("NULL simulator context in bus exit");
		return;
	}

	if (!sim_ctx->sim_ctx_initialized) {
		dp_err("Context not initialized");
		return;
	}

	dp_dal_sim_deinit(sim_ctx);
	dp_info("dal sim exit complete");
}

/**
 * dp_dal_sim_start() - Start DAL simulator operations
 * @priv: Pointer to private data (DAL simulator context)
 *
 * Starts the DAL simulator operations.
 *
 * Return: 0 on success, negative error code on failure
 */
static int dp_dal_sim_start(void *priv)
{
	struct dp_dal_ctx *dp_dal_ctx = (struct dp_dal_ctx *)priv;
	struct dp_dal_sim_ctx *sim_ctx = dp_dal_ctx->dal_sim_ctx;

	if (!sim_ctx) {
		dp_err("NULL context in start");
		return -EINVAL;
	}

	dp_info("Starting DAL simulator operations");

	/* TODO: Start processing operations */

	dp_info("DAL simulator started");

	return 0;
}

/**
 * dp_dal_sim_stop() - Stop DAL simulator operations
 * @priv: Pointer to private data (DAL simulator context)
 *
 * Stops the DAL simulator operations.
 */
static void dp_dal_sim_stop(void *priv)
{
	struct dp_dal_ctx *dp_dal_ctx = (struct dp_dal_ctx *)priv;
	struct dp_dal_sim_ctx *sim_ctx = dp_dal_ctx->dal_sim_ctx;

	if (!sim_ctx) {
		dp_warn("NULL context in stop");
		return;
	}

	dp_info("Stopping DAL simulator operations");

	/* TODO: Stop processing operations */

	dp_info("DAL simulator stopped");
}

/**
 * dp_dal_sim_set_ring_msi() - This function calls vendor cb which sets the msi
 * config for dal owned rings.
 *
 * @dp_dal_ctx: Pointer to DP DAL context
 * @sim_ctx: Pointer to simulator context
 * @sim_srng: Array of ring configurations
 * @num_rings: Number of rings in the array
 *
 * Return: 0 on success, negative error code on failure
 */
static inline int dp_dal_sim_set_ring_msi(
		struct dp_dal_ctx *dp_dal_ctx,
		struct dp_dal_sim_ctx *sim_ctx,
		struct dal_sim_srng *sim_srng,
		int num_rings)
{
	int i, status;

	for (i = 0; i < num_rings; i++) {
		dp_info("MSI cfg for ring_num=%u, ring_type=%d, msi_addr=0x%llx, msi_data=0x%x",
			sim_srng[i].ring_num,
			sim_srng[i].ring_type,
			sim_srng[i].msi_addr,
			sim_srng[i].msi_data);
		status = vendor_cb.set_msi_config(
			dp_dal_ctx,
			sim_srng[i].ring_num,
			sim_srng[i].ring_type,
			sim_srng[i].msi_addr,
			sim_srng[i].msi_data);

		if (status) {
			dp_err("MSI config for ring_num %d fail ret=%d",
			       sim_srng[i].ring_num, status);
			return status;
		}
	}
	return 0;
}

/**
 * dp_dal_sim_request_irq() - Register IRQs for DAL simulator
 * @priv: Pointer to private data (DAL simulator context)
 *
 * Registers interrupt handlers for RX and TX completion rings.
 * Iterates through all rings and calls set_msi_config vendor callback
 * with priv as dp_dal_ctx, ring num, ring type, msi address and msi data.
 *
 * Return: 0 on success, negative error code on failure
 */
static int dp_dal_sim_request_irq(void *priv)
{
	struct dp_dal_ctx *dp_dal_ctx = (struct dp_dal_ctx *)priv;
	struct dp_dal_sim_ctx *sim_ctx;
	int status = 0;

	if (!dp_dal_ctx) {
		dp_err("NULL context in offload_mode_request_irq");
		return -EINVAL;
	}

	sim_ctx = (struct dp_dal_sim_ctx *)dp_dal_ctx->dal_sim_ctx;
	if (!sim_ctx) {
		dp_err("NULL simulator context in request_irq");
		return -EINVAL;
	}

	if (!vendor_cb.set_msi_config) {
		dp_warn("set_msi_config callback not registered");
		return -EINVAL;
	}

	/* Iterate through all RX rings and call set_msi_config */
	status = dp_dal_sim_set_ring_msi(dp_dal_ctx, sim_ctx,
					 sim_ctx->rx_ring,
					 DAL_SIM_NUM_RX_RINGS);
	if (status)
		return status;

	status = dp_dal_sim_set_ring_msi(dp_dal_ctx, sim_ctx,
					 sim_ctx->tx_cmpl_ring,
					 DAL_SIM_NUM_TX_RINGS);
	if (status)
		return status;

	/* Call dp_dal_offload_sim_request_irq to register interrupts */
	status = dp_dal_offload_sim_request_irq(dp_dal_ctx->dal_sim_ctx);

	if (status) {
		dp_err("Failed to register IRQs");
		return status;
	}

	dp_info("IRQ registration complete");

	return status;
}

/**
 * dp_dal_sim_rx - Simulate RX reception
 * @priv: Private data pointer for simulation context
 * @cnt: Pointer to variable storing count of processed descriptors
 * @ring_num: Ring number from dp_dal_sim_srng structure
 *
 * This function reaps the REO2SW ring using vendor HAL APIs,
 * and sends the reaped descriptors to the WLAN driver using the
 * rx_cpl_cb vendor callback. The ring_num parameter is the ring_num
 * field from dp_dal_sim_srng, which is used to find the correct
 * ring array index for accessing ring structures.
 *
 * Return: true if descriptors were processed, false otherwise
 */
static bool dp_dal_sim_rx(void *priv, u32 *cnt, u16 ring_num)
{
	struct dp_dal_ctx *dal_ctx = (struct dp_dal_ctx *)priv;
	struct dp_dal_sim_ctx *sim_ctx;
	u32 desc_count = 0;
	u32 budget = DP_DAL_SIM_RX_BUDGET;
	int reaped_desc;
	void *desc;
	u32 i;
	int ring_id = -1;

	if (!cnt) {
		dp_err("NULL count pointer in offload_mode_rx");
		return false;
	}

	if (!dal_ctx) {
		dp_err("NULL DAL context in offload_mode_rx");
		*cnt = 0;
		return false;
	}
	sim_ctx = dal_ctx->dal_sim_ctx;
	if (!sim_ctx) {
		dp_err("NULL simulator context in offload_mode_rx");
		*cnt = 0;
		return false;
	}

	/* Find the ring array index based on ring_num from dp_dal_sim_srng */
	for (i = 0; i < DAL_SIM_NUM_RX_RINGS; i++) {
		if (sim_ctx->rx_ring[i].ring_num == ring_num) {
			ring_id = i;
			break;
		}
	}

	if (ring_id < 0) {
		dp_err("No RX ring found with ring_num %u", ring_num);
		*cnt = 0;
		return false;
	}

	/* Check if mode switch is in progress - block RX requests */
	if (qdf_atomic_read(&sim_ctx->sim_mode_switch_in_progress)) {
		dp_info_rl("Mode switch in progress - blocking RX[%u]",
			   ring_id);
		*cnt = 0;
		return false;
	}

	dp_debug("Processing RX ring_num %u (array index %d) with budget %u",
		 ring_num, ring_id, budget);

	/* Increment active RX descriptor processing counter */
	qdf_atomic_inc(&sim_ctx->active_rx_desc_list_cnt);

	/* Get REO descriptors from the ring using offload sim wrapper */
	reaped_desc = dp_dal_offload_sim_get_reo_desc(sim_ctx, ring_id, budget);
	if (reaped_desc < 0) {
		dp_err("Failed to get REO descs, reaped_desc=%d", reaped_desc);
		*cnt = 0;
		/* Decrement active RX descriptor processing counter */
		qdf_atomic_dec(&sim_ctx->active_rx_desc_list_cnt);
		return false;
	}

	/* Start accessing descriptor list with lock */
	dp_dal_sim_desc_list_access_start(&sim_ctx->rx_desc_list[ring_id]);

	/* Dequeue descriptors one by one and directly send to WLAN driver */
	for (i = 0; i < reaped_desc; i++) {
		desc = dp_dal_sim_desc_list_dequeue(
					&sim_ctx->rx_desc_list[ring_id]);
		if (!desc) {
			/* No more descriptors available */
			break;
		}

		/* Call vendor RX completion callback */
		if (vendor_cb.rx_cpl_cb) {
			vendor_cb.rx_cpl_cb(dal_ctx, desc, ring_num);
			sim_ctx->stats.rx_received[ring_id]++;
			desc_count++;
		} else {
			dp_warn("RX completion callback not registered");
			break;
		}
	}

	/* End accessing descriptor list and release lock */
	dp_dal_sim_desc_list_access_end(&sim_ctx->rx_desc_list[ring_id]);

	*cnt = desc_count;

	dp_debug("Processed %u RX descriptors for ring_num %u (array index %d)",
		 desc_count, ring_num, ring_id);

	/* Decrement active RX descriptor processing counter */
	qdf_atomic_dec(&sim_ctx->active_rx_desc_list_cnt);

	/* Return true if any descriptors were processed */
	return (desc_count > 0);
}

/**
 * dp_dal_sim_rx_replenish() - Replenish RX buffers in offload mode
 * @priv: Pointer to private data (DAL context)
 * @cnt: Number of buffers to replenish
 * @use_rsv_pktid: Flag indicating whether to use reserved packet IDs
 *
 * This function replenishes RX buffers in the RX refill ring. It determines
 * the number of available entries in the refill ring, calculates the actual
 * number of buffers to replenish (minimum of requested count and available
 * entries), and calls the vendor RX replenish allocation callback to allocate
 * buffers.
 *
 * Return: 0 on success, error code on failure
 */
static int dp_dal_sim_rx_replenish(void *priv, u32 cnt, bool use_rsv_pktid)
{
	struct dp_dal_ctx *dal_ctx = (struct dp_dal_ctx *)priv;
	struct dp_dal_sim_ctx *sim_ctx;
	uint32_t avail_entries;
	int replenish_cnt;
	int ret;

	if (!dal_ctx) {
		dp_err("NULL DAL context in offload_mode_rx_replenish");
		return -EINVAL;
	}

	sim_ctx = (struct dp_dal_sim_ctx *)dal_ctx->dal_sim_ctx;
	if (!sim_ctx) {
		dp_err("NULL simulator context in offload_mode_rx_replenish");
		return -EINVAL;
	}

	/* Check if mode switch is in progress - block replenish requests */
	if (qdf_atomic_read(&sim_ctx->sim_mode_switch_in_progress)) {
		dp_info_rl("Mode switch in progress - blocking replenish req");
		return -EBUSY;
	}

	/* Get available entries in RX refill ring using offload sim wrapper */
	avail_entries = dp_dal_offload_sim_get_rx_refill_avail_entries(sim_ctx);

	/* Calculate minimum of available entries and requested count */
	replenish_cnt = (avail_entries < cnt) ? avail_entries : cnt;

	dp_debug("RX replenish: requested=%u, available=%d",
		 cnt, avail_entries);

	/* Call vendor RX replenish allocation callback to allocate buffers */
	if (vendor_cb.rx_replenish_alloc_cb) {
		ret = vendor_cb.rx_replenish_alloc_cb(dal_ctx, replenish_cnt);
		if (ret) {
			dp_err("RX replenish alloc cb failed, ret=%d", ret);
			return -EINVAL;
		}

	} else {
		dp_warn("RX replenish allocation callback not registered");
	}

	return 0;
}

/**
 * dp_dal_sim_rxbm_sync() - Synchronize RX buffer manager descriptors to refill
 * ring
 * @priv: Pointer to private data (DAL context)
 * @cnt: Number of RX buffer descriptors to synchronize
 * @rxbm: Array of pointers to RX buffer manager descriptors
 *
 * This function synchronizes RX buffer manager descriptors to the RX refill
 * ring in the DAL simulator. It validates the input parameters, calls
 * the offload simulation wrapper to perform the actual synchronization.
 * The function is part of the platform bus operations for RX buffer management
 * in offload mode simulation.
 *
 * The rxbm array contains pointers to RX buffer descriptors that have been
 * allocated by the driver and need to be synchronized to the refill ring.
 * Each descriptor contains buffer address information and metadata required
 * for RX buffer management.
 *
 * Return: Number of descriptors successfully synchronized, 0 on error
 */
static int dp_dal_sim_rxbm_sync(void *priv, u32 cnt, void **rxbm)
{
	struct dp_dal_ctx *dal_ctx = (struct dp_dal_ctx *)priv;
	struct dp_dal_sim_ctx *sim_ctx;
	int synced_cnt = 0;

	if (!dal_ctx) {
		dp_err("NULL DAL context in offload_mode_rxbm_sync");
		return 0;
	}

	sim_ctx = (struct dp_dal_sim_ctx *)dal_ctx->dal_sim_ctx;
	if (!sim_ctx) {
		dp_err("NULL simulator context in offload_mode_rxbm_sync");
		return 0;
	}

	if (!rxbm) {
		dp_err("NULL rxbm array in offload_mode_rxbm_sync");
		return 0;
	}

	/* Check if mode switch is in progress - block rxbm sync requests */
	if (qdf_atomic_read(&sim_ctx->sim_mode_switch_in_progress)) {
		dp_info_rl("Mode switch in progress - blocking rxbm sync req");
		return 0;
	}
	/*Take spinlock to ensure mode switch does not interfere replenish */
	qdf_spin_lock_bh(&sim_ctx->rxbm_sync_lock);
	/* Call dp_dal_offload_sim wrapper to sync descriptors to refill ring */
	synced_cnt = dp_dal_offload_sim_rxbm_sync(sim_ctx, cnt, rxbm);
	qdf_spin_unlock_bh(&sim_ctx->rxbm_sync_lock);
	sim_ctx->stats.rx_replenished += synced_cnt;
	dp_debug("Synced %u RX buffer descriptors", synced_cnt);

	return synced_cnt;
}

/**
 * dp_dal_sim_tx - Transmit packet in offload mode simulation
 * @priv: Private data pointer for driver context
 * @ring_num: Ring number from dp_dal_sim_srng structure
 * @ifidx: Interface index for transmission
 * @desc: Pointer to the TCL descriptor
 * @tx_metadata: Pointer to TX metadata
 *
 * This platform function cb calls the offload engine simulation api to fill and
 * sync tx descriptor to hardware.
 *
 * Return: 0 on success, negative error code on failure
 */
static int dp_dal_sim_tx
	(void *priv, u8 ring_num,
	 u32 ifidx, void *desc, void *tx_metadata)
{
	struct dp_dal_ctx *dal_ctx = (struct dp_dal_ctx *)priv;
	struct dp_dal_sim_ctx *sim_ctx;
	struct dp_dal_tx_metadata *metadata =
		(struct dp_dal_tx_metadata *)tx_metadata;
	int ret = 0;
	u32 i;
	int ring_id = -1;

	if (!dal_ctx) {
		dp_err("NULL DAL context in offload_mode_tx");
		return -EINVAL;
	}

	sim_ctx = (struct dp_dal_sim_ctx *)dal_ctx->dal_sim_ctx;
	if (!sim_ctx) {
		dp_err("NULL simulator context in offload_mode_tx");
		return -EINVAL;
	}

	if (!desc || !metadata) {
		dp_err("Invalid input parameters in offload_mode_tx");
		return -EINVAL;
	}

	/* Find the ring array index based on ring_num from dp_dal_sim_srng */
	for (i = 0; i < DAL_SIM_NUM_TX_RINGS; i++) {
		if (sim_ctx->tx_ring[i].ring_num == ring_num) {
			ring_id = i;
			break;
		}
	}

	if (ring_id < 0) {
		dp_err("No TX ring found with ring_num %u", ring_num);
		return -EINVAL;
	}

	/* Check if mode switch is in progress - block TX requests */
	if (qdf_atomic_read(&sim_ctx->sim_mode_switch_in_progress)) {
		dp_info_rl("Mode switch in progress - blocking TX[%u]",
			   ring_id);
		return -EBUSY;
	}

	dp_debug("Processing TX for ring_num %u (array index %d), ifidx %u",
		 ring_num, ring_id, ifidx);

	/* Call wrapper function to handle ring access and HW enqueue */
	ret = dp_dal_offload_sim_tx_hw_enqueue(sim_ctx, ring_id,
					       desc, tx_metadata);
	if (ret) {
		dp_err("Failed to enqueue TX descriptor, ret=%d", ret);
		return ret;
	}
	sim_ctx->stats.tx_enqueued[ring_id]++;
	dp_debug("TX enqueued successfully for ring_num %u (array index %d)",
		 ring_num, ring_id);

	return ret;
}

/**
 * dp_dal_sim_tx_cpl - Process TX completions in offload mode
 * @priv: Private data pointer for driver context
 * @cnt: Pointer to store the number of completions processed
 * @ring_num: Ring number from dp_dal_sim_srng structure
 *
 * This function processes TX completions in offload mode. It retrieves
 * TX completion descriptors from the WBM ring using vendor HAL APIs,
 * and sends them to the WLAN driver using the tx_cpl_cb vendor callback.
 * The ring_num parameter is the ring_num field from dp_dal_sim_srng,
 * which is used to find the correct ring array index for accessing
 * ring structures.
 *
 * Return: true if completions were processed, false otherwise
 */
static bool dp_dal_sim_tx_cpl(void *priv, u32 *cnt, u16 ring_num)
{
	struct dp_dal_ctx *dal_ctx = (struct dp_dal_ctx *)priv;
	struct dp_dal_sim_ctx *sim_ctx;
	u32 desc_count = 0;
	u32 budget = DP_DAL_SIM_TX_BUDGET;
	int reaped_desc;
	u32 i;
	void *desc;
	int ring_id = -1;

	if (!cnt) {
		dp_err("NULL count pointer in offload_mode_tx_cpl");
		return false;
	}

	if (!dal_ctx) {
		dp_err("NULL DAL context in offload_mode_tx_cpl");
		*cnt = 0;
		return false;
	}

	sim_ctx = (struct dp_dal_sim_ctx *)dal_ctx->dal_sim_ctx;
	if (!sim_ctx) {
		dp_err("NULL simulator context in offload_mode_tx_cpl");
		*cnt = 0;
		return false;
	}

	/* Find the ring array index based on ring_num from dp_dal_sim_srng */
	for (i = 0; i < DAL_SIM_NUM_TX_RINGS; i++) {
		if (sim_ctx->tx_cmpl_ring[i].ring_num == ring_num) {
			ring_id = i;
			break;
		}
	}

	if (ring_id < 0) {
		dp_err("No TX compl ring found with ring_num %u", ring_num);
		*cnt = 0;
		return false;
	}

	/* Check if mode switch is in progress - block tx cpl requests */
	if (qdf_atomic_read(&sim_ctx->sim_mode_switch_in_progress)) {
		*cnt = 0;
		dp_info_rl("Mode switch in progress - blocking tx_compl[%d]",
			   ring_id);
		return false;
	}

	dp_debug("Processing TX compl ring_num %u (ring_id %d) with budget %u",
		 ring_num, ring_id, budget);

	/* Increment active TX descriptor processing counter */
	qdf_atomic_inc(&sim_ctx->active_tx_desc_list_cnt);

	/* Get TX completion descriptors */
	reaped_desc = dp_dal_offload_sim_get_tx_compl_desc(sim_ctx,
							   ring_id, budget);
	if (reaped_desc < 0) {
		dp_err("Failed to get TX completion descs, reaped_desc=%d",
		       reaped_desc);
		*cnt = 0;
		/* Decrement active TX descriptor processing counter */
		qdf_atomic_dec(&sim_ctx->active_tx_desc_list_cnt);
		return false;
	}

	/* Start accessing descriptor list with lock */
	dp_dal_sim_desc_list_access_start(&sim_ctx->tx_cpl_desc_list[ring_id]);

	/* Dequeue descriptors one by one and directly send to WLAN driver */
	for (i = 0; i < reaped_desc; i++) {
		desc = dp_dal_sim_desc_list_dequeue(
					&sim_ctx->tx_cpl_desc_list[ring_id]);
		if (!desc) {
			/* No more descriptors available */
			break;
		}

		/* Call vendor TX completion callback */
		if (vendor_cb.tx_cpl_cb) {
			vendor_cb.tx_cpl_cb(dal_ctx, desc, ring_num);
			sim_ctx->stats.tx_completed[ring_id]++;
			desc_count++;
		} else {
			dp_warn("TX completion callback not registered");
			break;
		}
	}

	/* End accessing descriptor list and release lock */
	dp_dal_sim_desc_list_access_end(&sim_ctx->tx_cpl_desc_list[ring_id]);

	*cnt = desc_count;

	dp_debug("Processed %u TX compl desc for ring_num %u (ring_id %d)",
		 desc_count, ring_num, ring_id);

	/* Decrement active TX descriptor processing counter */
	qdf_atomic_dec(&sim_ctx->active_tx_desc_list_cnt);

	/* Return true if any descriptors were processed */
	return (desc_count > 0);
}

static int dp_dal_sim_tx_queue_active(void *priv, u16 flowid, bool enable)
{
	dp_debug("TX queue active operation (stub)");
	return 0;
}

/**
 * dp_dal_sim_sta_active() - Enable/disable station activity
 * @priv: Pointer to private data (DAL context)
 * @info: Pointer to station information
 * @enable: True to enable, false to disable
 *
 * This function enables or disables station activity based on the provided
 * station information. Currently implemented as a stub function.
 *
 * Return: 0 on success
 */
static int dp_dal_sim_sta_active(void *priv, struct sta_info *info, bool enable)
{
	dp_debug("STA active operation (stub)");
	return 0;
}

/**
 * dp_dal_sim_notify_suspend() - Handle suspend notification
 * @priv: Pointer to private data (DAL context)
 *
 * This function handles system suspend notifications for the DAL simulator.
 * Currently implemented as a stub function.
 *
 * Return: 0 on success
 */
static int dp_dal_sim_notify_suspend(void *priv)
{
	dp_info("Suspend notification (stub)");
	return 0;
}

/**
 * dp_dal_sim_notify_resume() - Handle resume notification
 * @priv: Pointer to private data (DAL context)
 *
 * This function handles system resume notifications for the DAL simulator.
 * Currently implemented as a stub function.
 *
 * Return: 0 on success
 */
static int dp_dal_sim_notify_resume(void *priv)
{
	dp_info("Resume notification (stub)");
	return 0;
}

/**
 * dp_dal_sim_ssr_dump() - Dump SSR (SubSystem Restart) information
 * @segment: Pointer to memory segment for dump
 *
 * This function dumps SSR information to the provided memory segment.
 * Currently implemented as a stub function.
 */
static void dp_dal_sim_ssr_dump(void *segment)
{
	dp_info("SSR dump (stub)");
}

/**
 * dp_dal_sim_intf_init() - Initialize interface in DAL simulator
 * @priv: Pointer to private data (DAL context)
 * @intf_info: Pointer to interface information
 *
 * This function is called when an interface is added. It copies the interface
 * information to the DAL simulator context based on the interface type.
 *
 * Return: 0 on success, negative error code on failure
 */
static int dp_dal_sim_intf_init(void *priv, void *intf_info)
{
	struct dp_dal_ctx *dal_ctx = (struct dp_dal_ctx *)priv;
	struct dp_dal_sim_ctx *sim_ctx;
	struct dal_intf_info *info = (struct dal_intf_info *)intf_info;

	if (!dal_ctx) {
		dp_err("NULL DAL context in intf_init");
		return -EINVAL;
	}

	sim_ctx = (struct dp_dal_sim_ctx *)dal_ctx->dal_sim_ctx;
	if (!sim_ctx) {
		dp_err("NULL simulator context in intf_init");
		return -EINVAL;
	}

	if (!info) {
		dp_err("NULL interface info in intf_init");
		return -EINVAL;
	}

	if (info->type >= DAL_INTF_TYPE_MAX) {
		dp_err("Invalid interface type %d", info->type);
		return -EINVAL;
	}

	/* Copy interface information to simulator context.
	 * interface info contains vdev_id, tcl_bank_id which will
	 * used in enqueueing packets to TCL. Apart from these
	 * bss_idx, mac_addr information is also present.
	 */
	qdf_mem_copy(&sim_ctx->intf_info[info->type], info,
		     sizeof(struct dal_intf_info));

	dp_info("Interface initialized: type=%d, vdev_id=%u, tcl_bank_id=%u",
		info->type, info->vdev_id, info->tcl_bank_id);

	return 0;
}

/**
 * dp_dal_sim_intf_deinit() - De-initialize interface in DAL simulator
 * @priv: Pointer to private data (DAL context)
 * @vdev_id: VDEV ID of the interface to remove
 *
 * This function is called when an interface is removed. It clears the interface
 * information from the DAL simulator context.
 *
 * Return: 0 on success, negative error code on failure
 */
static int dp_dal_sim_intf_deinit(void *priv, uint16_t vdev_id)
{
	struct dp_dal_ctx *dal_ctx = (struct dp_dal_ctx *)priv;
	struct dp_dal_sim_ctx *sim_ctx;
	int i;

	if (!dal_ctx) {
		dp_err("NULL DAL context in intf_deinit");
		return -EINVAL;
	}

	sim_ctx = (struct dp_dal_sim_ctx *)dal_ctx->dal_sim_ctx;
	if (!sim_ctx) {
		dp_err("NULL simulator context in intf_deinit");
		return -EINVAL;
	}

	/* Find and clear the interface info matching the vdev_id */
	for (i = 0; i < DAL_INTF_TYPE_MAX; i++) {
		if (sim_ctx->intf_info[i].vdev_id == vdev_id) {
			dp_info("Interface deinitialized: type=%d, vdev_id=%u",
				sim_ctx->intf_info[i].type, vdev_id);

			/* Clear the interface information */
			qdf_mem_zero(&sim_ctx->intf_info[i],
				     sizeof(struct dal_intf_info));
			return 0;
		}
	}

	dp_err("Interface with vdev_id %u not found", vdev_id);
	return -ENOENT;
}

/**
 * dp_dal_sim_fetch_current_hp_tp() - Fetch current HP and TP values for a ring
 * @priv: Pointer to private data (DAL context)
 * @ring_num: Ring number
 * @ring_type: Ring type
 * @hp: Pointer to store head pointer value
 * @tp: Pointer to store tail pointer value
 *
 * This function fetches the current head pointer (HP) and tail pointer (TP)
 * values for a specified ring. It maps the ring_num and ring_type to the
 * appropriate ring_id in the simulator context and calls the offload
 * simulation API to get the current HP/TP values.
 *
 * Return: 0 on success, negative error code on failure
 */
static int dp_dal_sim_fetch_current_hp_tp(
	void *priv, int ring_num,
	int ring_type, uint32_t *hp, uint32_t *tp)
{
	struct dp_dal_ctx *dal_ctx = (struct dp_dal_ctx *)priv;
	struct dp_dal_sim_ctx *sim_ctx;
	int ring_id = -1;
	int offload_ring_type;
	int i;

	if (!dal_ctx) {
		dp_err("NULL DAL context");
		return -EINVAL;
	}

	sim_ctx = (struct dp_dal_sim_ctx *)dal_ctx->dal_sim_ctx;
	if (!sim_ctx) {
		dp_err("NULL dal sim context");
		return -EINVAL;
	}

	if (!hp || !tp) {
		dp_err("NULL hp/tp");
		return -EINVAL;
	}

	/* Map ring_type to offload simulation ring type */
	switch (ring_type) {
	case REO_DST:
		offload_ring_type = OFFLOAD_SIM_RING_TYPE_RX;
		/* Find the ring array index based on ring_num */
		for (i = 0; i < DAL_SIM_NUM_RX_RINGS; i++) {
			if (sim_ctx->rx_ring[i].ring_num == ring_num) {
				ring_id = i;
				break;
			}
		}
		break;
	case COMP_RING_TYPE:
		offload_ring_type = OFFLOAD_SIM_RING_TYPE_TX_CPL;
		/* Find the ring array index based on ring_num */
		for (i = 0; i < DAL_SIM_NUM_TX_RINGS; i++) {
			if (sim_ctx->tx_cmpl_ring[i].ring_num == ring_num) {
				ring_id = i;
				break;
			}
		}
		break;
	default:
		dp_err("Unsupported ring type %d", ring_type);
		return -EINVAL;
	}

	if (ring_id < 0) {
		dp_err("No ring found with ring_num %d and type %d",
		       ring_num, ring_type);
		return -EINVAL;
	}

	/* Call offload simulation API to fetch current HP/TP values */
	return dp_dal_offload_sim_fetch_current_hp_tp(sim_ctx, hp, tp,
						      offload_ring_type,
						      ring_id);
}

/*
 * This structure contains all platform bus operations for the DAL simulator.
 * It is assigned to global plat ops if mode of operation is offload mode.
 */
static struct platform_bus_ops dp_dal_sim_plat_ops = {
	.init = dp_dal_sim_init,
	.exit = dp_dal_sim_exit,
	.start = dp_dal_sim_start,
	.stop = dp_dal_sim_stop,
	.request_irq = dp_dal_sim_request_irq,
	.rx = dp_dal_sim_rx,
	.rx_replenish = dp_dal_sim_rx_replenish,
	.rxbm_sync = dp_dal_sim_rxbm_sync,
	.tx = dp_dal_sim_tx,
	.tx_cpl = dp_dal_sim_tx_cpl,
	.tx_queue_active = dp_dal_sim_tx_queue_active,
	.sta_active = dp_dal_sim_sta_active,
	.notify_suspend = dp_dal_sim_notify_suspend,
	.notify_resume = dp_dal_sim_notify_resume,
	.ssr_dump = dp_dal_sim_ssr_dump,
	.intf_init = dp_dal_sim_intf_init,
	.intf_deinit = dp_dal_sim_intf_deinit,
	.fetch_current_hp_tp = dp_dal_sim_fetch_current_hp_tp,
};

/**
 * dp_dal_sim_schedule_work() - Schedule work for interrupt processing
 * @arg: Pointer to offload_sim_irq_ctx structure
 *
 * This function is called by the offload simulation interrupt handler
 * to queue work for processing the interrupt. It checks if work is
 * already scheduled and queues work on the appropriate work queue.
 */
void dp_dal_sim_schedule_work(void *arg)
{
	struct offload_sim_irq_ctx *irq_ctx = (struct offload_sim_irq_ctx *)arg;
	struct dp_dal_sim_ctx *sim_ctx;
	int ring_id;
	bool already_scheduled;

	if (!irq_ctx) {
		dp_err("NULL IRQ context in schedule_work");
		return;
	}

	sim_ctx = irq_ctx->dal_sim_ctx;
	if (!sim_ctx) {
		dp_err("NULL sim context in schedule_work");
		return;
	}

	ring_id = irq_ctx->ring_id;

	dp_debug("Scheduling work for ring_type=%d, ring_id=%d",
		 irq_ctx->ring_type, ring_id);

	/* Handle interrupt based on ring type */
	if (irq_ctx->ring_type == OFFLOAD_SIM_RING_TYPE_RX) {
		if (ring_id < 0 || ring_id >= DAL_SIM_NUM_RX_RINGS) {
			dp_err_rl("Invalid ring_id %d", ring_id);
			return;
		}
		/* RX interrupt - increment ISR count every time */
		sim_ctx->stats.rx_isr_count[ring_id]++;

		/* RX interrupt - schedule RX work */
		already_scheduled =
			qdf_atomic_read(&sim_ctx->rx_work_scheduled[ring_id]);

		if (!already_scheduled) {
			qdf_atomic_set(&sim_ctx->rx_work_scheduled[ring_id], 1);
			qdf_queue_work(0, sim_ctx->rx_work_queue[ring_id],
				       &sim_ctx->rx_process_work[ring_id]);
			sim_ctx->stats.rx_work_queued[ring_id]++;
			dp_debug("RX work scheduled for ring_id=%d", ring_id);
		}
	} else if (irq_ctx->ring_type == OFFLOAD_SIM_RING_TYPE_TX_CPL) {
		if (ring_id < 0 || ring_id >= DAL_SIM_NUM_TX_RINGS) {
			dp_err_rl("Invalid ring_id %d", ring_id);
			return;
		}
		/* TX completion interrupt - increment ISR count every time */
		sim_ctx->stats.tx_cpl_isr_count[ring_id]++;

		/* TX completion interrupt - schedule TX completion work */
		already_scheduled = qdf_atomic_read(
				&sim_ctx->tx_compl_work_scheduled[ring_id]);

		if (!already_scheduled) {
			qdf_atomic_set(
				&sim_ctx->tx_compl_work_scheduled[ring_id],
				1);
			qdf_queue_work(
				0,
				sim_ctx->tx_compl_work_queue[ring_id],
				&sim_ctx->tx_compl_process_work[ring_id]);
			sim_ctx->stats.tx_cpl_work_queued[ring_id]++;
			dp_debug("TX compl work scheduled for ring_id=%d",
				 ring_id);
		}
	} else {
		dp_err("Invalid ring type %d", irq_ctx->ring_type);
	}
}
#endif /* FEATURE_DP_DAL_SIM */

#if defined(FEATURE_DAL_DP_SUPPORT) && defined(FEATURE_DP_DAL_SIM)
/* Global variable to track current DAL simulation mode
 * 0 = bypass mode, 1 = offload mode
 */
unsigned int g_dal_sim_curr_mode;

static void dp_dal_sim_platform_bus_ops_attach(struct dp_dal_ctx *dal_ctx)
{
	struct dp_soc *soc;

	if (!dal_ctx || !dal_ctx->soc) {
		g_dal_sim_curr_mode = DAL_DP_BYPASS_MODE;
		global_plat_ops = &plat_ops_bypass_mode;
		dp_err("Null soc context, attaching default bypass ops");
		return;
	}

	soc = dal_ctx->soc;

	if (!soc->ctrl_psoc) {
		g_dal_sim_curr_mode = DAL_DP_BYPASS_MODE;
		dp_err("Null ctrl_psoc, attaching default bypass ops");
		global_plat_ops = &plat_ops_bypass_mode;
		return;
	}
	g_dal_sim_curr_mode = cfg_get(soc->ctrl_psoc, CFG_DP_DAL_SIM_MODE);
	if (g_dal_sim_curr_mode == DAL_DP_BYPASS_MODE) {
		/* Bypass mode */
		global_plat_ops = &plat_ops_bypass_mode;
		dp_info("Platform bus operations attached - Bypass mode");
	} else if (g_dal_sim_curr_mode == DAL_DP_OFFLOAD_MODE) {
		/* Offload mode */
		global_plat_ops = &dp_dal_sim_plat_ops;
		dp_info("Platform bus operations attached - Offload mode");
	} else {
		dp_err("Invalid mode %u, defaulting to bypass mode",
		       g_dal_sim_curr_mode);
		g_dal_sim_curr_mode = DAL_DP_BYPASS_MODE;
		global_plat_ops = &plat_ops_bypass_mode;
	}
}

void dp_dal_sim_detach(void *priv)
{
	struct dp_dal_ctx *dal_ctx = (struct dp_dal_ctx *)priv;
	struct dp_dal_sim_ctx *sim_ctx;

	if (!dal_ctx) {
		dp_init_err("DAL CTX is NULL");
		return;
	}

	sim_ctx = dal_ctx->dal_sim_ctx;
	if (!sim_ctx) {
		dp_init_err("DAL SIM CTX is NULL");
		return;
	}

	sim_ctx->dp_dal_ctx = NULL;
	qdf_mem_free(sim_ctx);
	dal_ctx->dal_sim_ctx = NULL;
	dp_err("sim ctx is freed");
}

int dp_dal_sim_attach(void *priv)
{
	struct dp_dal_ctx *dal_ctx = (struct dp_dal_ctx *)priv;
	struct dp_dal_sim_ctx *sim_ctx;

	if (!dal_ctx) {
		dp_init_err("DAL CTX is NULL");
		return -EINVAL;
	}

	dp_dal_sim_platform_bus_ops_attach(dal_ctx);

	sim_ctx = qdf_mem_malloc(sizeof(*sim_ctx));
	if (!sim_ctx) {
		dp_init_err("failed to alloc mem for sim ctx");
		return -EINVAL;
	}

	dal_ctx->dal_sim_ctx = sim_ctx;
	sim_ctx->dp_dal_ctx = dal_ctx;

	/* Initialize mode switch control */
	qdf_atomic_init(&sim_ctx->sim_mode_switch_in_progress);

	dp_err("sim context allocated and attached successfully");
	return 0;
}

/**
 * dp_dal_sim_active_desc_processing() - Wait for active descriptor processing
 * to complete
 * @sim_ctx: Pointer to DAL sim context
 *
 * This function waits for all active RX and TX descriptor processing to
 * complete before allowing mode switch. It checks atomic counters with
 * retries and timeout.
 *
 * Return: 0 on success (all descriptors processed), -ETIMEDOUT on timeout
 */
static int dp_dal_sim_active_desc_processing(struct dp_dal_sim_ctx *sim_ctx)
{
	int retry = 0;
	int active_tx_desc_list_cnt, active_rx_desc_list_cnt;

	if (!sim_ctx) {
		dp_err("NULL sim context");
		return -EINVAL;
	}

	/* Wait for all active descriptor processing to complete */
	for (retry = 0; retry < DP_DAL_SIM_MODE_SWITCH_MAX_RETRIES; retry++) {
		active_tx_desc_list_cnt =
			qdf_atomic_read(&sim_ctx->active_tx_desc_list_cnt);
		active_rx_desc_list_cnt =
			qdf_atomic_read(&sim_ctx->active_rx_desc_list_cnt);

		if (active_tx_desc_list_cnt == 0 &&
		    active_rx_desc_list_cnt == 0) {
			dp_info("desc list processed (retry=%d)", retry);
			return 0;
		}

		dp_debug("desc list pending process: tx=%d rx=%d (retry=%d/%d)",
			 active_tx_desc_list_cnt, active_rx_desc_list_cnt,
			 retry, DP_DAL_SIM_MODE_SWITCH_MAX_RETRIES);

		/* Sleep for timeout milliseconds */
		msleep(DP_DAL_SIM_MODE_SWITCH_TIMEOUT_MS);
	}

	/* Timeout - descriptors still being processed */
	active_tx_desc_list_cnt =
			qdf_atomic_read(&sim_ctx->active_tx_desc_list_cnt);
	active_rx_desc_list_cnt =
			qdf_atomic_read(&sim_ctx->active_rx_desc_list_cnt);
	dp_err("Timeout waiting for active desc list processing: tx=%d, rx=%d",
	       active_tx_desc_list_cnt, active_rx_desc_list_cnt);

	return -ETIMEDOUT;
}

/**
 * dp_dal_sim_mode_bypass_switch() - Switch to bypass mode
 * @sim_ctx: pointer to dal sim context
 *
 * This function performs the necessary steps to switch the dal sim mode to
 * bypass mode.
 *
 * Return: None
 */
static inline void dp_dal_sim_mode_bypass_switch(
	struct dp_dal_sim_ctx *sim_ctx)
{
	int status;

	if (!sim_ctx) {
		dp_err("Null sim ctx");
		return;
	}

	if (!vendor_cb.mode_switch_ind) {
		dp_err("mode_switch_ind callback not registered");
		return;
	}
	/* disable irqs */
	dp_dal_offload_sim_disable_irq(sim_ctx);
	/* Set mode switch in progress flag to true */
	qdf_atomic_set(&sim_ctx->sim_mode_switch_in_progress, 1);
	/* Take spinlock for rxbm sync to ensure ongoing sync is completed */
	qdf_spin_lock_bh(&sim_ctx->rxbm_sync_lock);
	qdf_spin_unlock_bh(&sim_ctx->rxbm_sync_lock);
	/* Flush and cancel work*/
	dp_dal_sim_destroy_work(sim_ctx);
	/* Add logic to wait for desc list to be empty */
	status = dp_dal_sim_active_desc_processing(sim_ctx);
	if (status) {
		dp_err("forced mode switch");
		sim_ctx->stats.error_stats.mode_switch_desc_list_timeout++;
	}

	dp_dal_offload_sim_sync_refill_ring_hp_to_ddr(sim_ctx);

	/* Update the global plat ops with offload mode ops */
	*global_plat_ops = plat_ops_bypass_mode;

	status = vendor_cb.mode_switch_ind(
		sim_ctx->dp_dal_ctx, g_dal_sim_curr_mode, DAL_DP_BYPASS_MODE);
	if (status) {
		sim_ctx->stats.error_stats.bypass_mode_switch_ind_fail++;
		/* If mode switch fails then bypass mode ops is now only present
		 * since the intention is that offload engine is now unavailable
		 * This scenario is not expected.
		 */
		dp_err("forced mode switch: ind fail status %d", status);
	}
	/*Deinit offload_ctx*/
	dp_info("Switched to bypass mode");
	g_dal_sim_curr_mode = DAL_DP_BYPASS_MODE;
	dp_dal_offload_sim_deinit(sim_ctx);
	qdf_atomic_set(&sim_ctx->sim_mode_switch_in_progress, 0);
}

/**
 * dp_dal_sim_mode_offload_switch() - Switch to offload mode
 * @sim_ctx: pointer to dal sim context
 *
 * This function performs the necessary steps to switch the dal sim mode to
 * offload mode.
 *
 * Return: None
 */
static inline void dp_dal_sim_mode_offload_switch(
	struct dp_dal_sim_ctx *sim_ctx)
{
	int status;

	if (!sim_ctx) {
		dp_err("Null sim ctx");
		return;
	}

	if (!vendor_cb.mode_switch_ind) {
		dp_err("mode_switch_ind callback not registered");
		return;
	}
	/* Set mode switch in progress flag to true */
	qdf_atomic_set(&sim_ctx->sim_mode_switch_in_progress, 1);
	/* Update the global plat ops with offload mode ops */
	*global_plat_ops = dp_dal_sim_plat_ops;

	status = vendor_cb.mode_switch_ind(sim_ctx->dp_dal_ctx,
					   g_dal_sim_curr_mode,
					   DAL_DP_OFFLOAD_MODE);
	if (status) {
		sim_ctx->stats.error_stats.offload_mode_switch_ind_fail++;
		*global_plat_ops = plat_ops_bypass_mode;
		dp_err("mode switch ind fail status %d. Restored bypass ops",
		       status);
		goto exit;
	}

	dp_dal_offload_sim_sync_refill_ring_hp(sim_ctx);

	g_dal_sim_curr_mode = DAL_DP_OFFLOAD_MODE;
	dp_info("Switched to offload mode");
exit:
	qdf_atomic_set(&sim_ctx->sim_mode_switch_in_progress, 0);
}

void dp_dal_sim_trigger_mode_switch(
	struct cdp_soc_t *soc_hdl, uint8_t mode_requested)
{
	struct dp_soc *dp_soc;
	struct dp_dal_ctx *dal_ctx;
	struct dp_dal_sim_ctx *sim_ctx;

	dp_soc = cdp_soc_t_to_dp_soc(soc_hdl);

	if (!dp_soc) {
		dp_err("Null soc context");
		return;
	}

	dal_ctx = dp_soc->dal_ctx;
	if (!dal_ctx) {
		dp_err("DAL context is NULL");
		return;
	}

	sim_ctx = (struct dp_dal_sim_ctx *)dal_ctx->dal_sim_ctx;
	if (!sim_ctx) {
		dp_err("DAL sim context is NULL");
		return;
	}

	if (g_dal_sim_curr_mode == mode_requested) {
		dp_err_rl("curr_mode %d req_mode %d same",
			  g_dal_sim_curr_mode, mode_requested);
		return;
	}

	dp_info("mode switch request: curr_mode:%d, new mode %d",
		g_dal_sim_curr_mode, mode_requested);
	if (mode_requested == DAL_DP_BYPASS_MODE &&
	    g_dal_sim_curr_mode == DAL_DP_OFFLOAD_MODE)
		dp_dal_sim_mode_bypass_switch(sim_ctx);
	else if (mode_requested == DAL_DP_OFFLOAD_MODE &&
		 g_dal_sim_curr_mode == DAL_DP_BYPASS_MODE)
		dp_dal_sim_mode_offload_switch(sim_ctx);
	else
		dp_err_rl("Invalid mode requested %d", mode_requested);
}

uint8_t dp_dal_sim_get_curr_mode(void)
{
	return g_dal_sim_curr_mode;
}
#endif /* FEATURE_DAL_DP_SUPPORT && FEATURE_DP_DAL_SIM */
