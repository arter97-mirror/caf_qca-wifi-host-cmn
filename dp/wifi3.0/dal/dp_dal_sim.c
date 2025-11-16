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

	ret = pld_get_user_msi_assignment(soc->osdev->dev, "DAL",
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

	int status = 0;
	struct dp_dal_ctx *dal_ctx = (struct dp_dal_ctx *)priv;
	struct dp_dal_sim_ctx *sim_ctx = NULL;

	if (!dal_ctx) {
		dp_err("NULL DAL context in offload_mode_init");
		return -EINVAL;
	}

	/* Allocate dal_sim_ctx */
	sim_ctx = qdf_mem_malloc(sizeof(*sim_ctx));
	if (!sim_ctx) {
		dp_err("Failed to allocate simulator context");
		return -ENOMEM;
	}

	/* Set dp_dal_ctx pointer */
	sim_ctx->dp_dal_ctx = dal_ctx;

	/* Get the device base address*/
	struct hal_soc *hal_soc = (struct hal_soc *)dal_ctx->soc->hal_soc;

	sim_ctx->dev_base_addr = hal_soc->dev_base_addr;

	/* Get the device ptr */
	sim_ctx->dev = dal_ctx->soc->osdev->dev;
	/* Parse ring information from driver */
	status = dp_dal_sim_parse_ring_info(sim_ctx, priv);
	if (status) {
		dp_err("Failed to parse ring information");
		goto free_sim_ctx;
	}

	/* Initialize offload_sim_ctx */
	status = dp_dal_offload_sim_init(sim_ctx);
	if (status) {
		dp_err("Failed to initialize offload sim context");
		goto free_sim_ctx;
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

	/* Assign dal_sim_ctx to dp_dal_ctx */
	dal_ctx->dal_sim_ctx = sim_ctx;

	/* Mark as initialized */
	sim_ctx->sim_ctx_initialized = true;

	dp_info("dal sim init complete");
	return status;
free_offload_ctx:
	dp_dal_offload_sim_deinit(sim_ctx);
	/* Destroy any work queues that were successfully created */
	dp_dal_sim_destroy_work(sim_ctx);
free_sim_ctx:
	dal_ctx->dal_sim_ctx = NULL;
	qdf_mem_free(sim_ctx);
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

	/* Destroy all work queues and work items */
	dp_dal_sim_destroy_work(sim_ctx);

	/* Call offload mode exit api to free offload simulation resources */
	dp_dal_offload_sim_deinit(sim_ctx);

	/* Clear all fields */
	sim_ctx->dp_dal_ctx = NULL;
	sim_ctx->sim_ctx_initialized = false;

	/* Free context */
	qdf_mem_free(sim_ctx);
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
	dp_dal_ctx->dal_sim_ctx = NULL;
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
 * dp_dal_sim_request_irq() - Register IRQs for DAL simulator
 * @priv: Pointer to private data (DAL simulator context)
 *
 * Registers interrupt handlers for RX and TX completion rings.
 *
 * Return: 0 on success, negative error code on failure
 */
static int dp_dal_sim_request_irq(void *priv)
{
	struct dp_dal_ctx *dp_dal_ctx = (struct dp_dal_ctx *)priv;
	int status = 0;

	if (!dp_dal_ctx) {
		dp_err("NULL context in offload_mode_request_irq");
		return -EINVAL;
	}

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
	void *desc_list[DP_DAL_SIM_RX_BUDGET];
	u32 desc_count = 0;
	u32 budget = DP_DAL_SIM_RX_BUDGET;
	int ret;
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

	dp_debug("Processing RX ring_num %u (array index %d) with budget %u",
		 ring_num, ring_id, budget);

	/* Get REO descriptors from the ring using offload sim wrapper */
	ret = dp_dal_offload_sim_get_reo_desc(sim_ctx, ring_id,
					      desc_list, &desc_count, budget);
	if (ret) {
		dp_err("Failed to get REO descriptors, ret=%d", ret);
		*cnt = 0;
		return false;
	}

	/* Process all retrieved descriptors */
	for (i = 0; i < desc_count; i++) {
		/* Call vendor RX completion callback with ring_num from srng */
		if (vendor_cb.rx_cpl_cb) {
			vendor_cb.rx_cpl_cb(dal_ctx, desc_list[i], ring_num);
			sim_ctx->stats.rx_received[ring_id]++;
		} else {
			dp_warn("RX completion callback not registered");
			break;
		}
	}

	*cnt = desc_count;

	dp_debug("Processed %u RX descriptors for ring_num %u (array index %d)",
		 desc_count, ring_num, ring_id);

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
	int synced_cnt = 0;

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
	void *desc_list[DP_DAL_SIM_TX_BUDGET];
	u32 desc_count = 0;
	u32 budget = DP_DAL_SIM_TX_BUDGET;
	int ret;
	u32 i;
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

	dp_debug("Processing TX compl ring_num %u (ring_id %d) with budget %u",
		 ring_num, ring_id, budget);

	/* Get TX completion descriptors */
	ret = dp_dal_offload_sim_get_tx_compl_desc(sim_ctx,
						   ring_id,
						   desc_list, &desc_count,
						   budget);
	if (ret) {
		dp_err("Failed to get TX completion descriptors, ret=%d", ret);
		*cnt = 0;
		return false;
	}

	/* Process all retrieved descriptors */
	for (i = 0; i < desc_count; i++) {
		/* Call vendor TX completion callback with ring_num from srng */
		if (vendor_cb.tx_cpl_cb) {
			vendor_cb.tx_cpl_cb(dal_ctx, desc_list[i], ring_num);
			sim_ctx->stats.tx_completed[ring_id]++;
		} else {
			dp_warn("TX completion callback not registered");
			break;
		}
	}

	*cnt = desc_count;

	dp_debug("Processed %u TX compl desc for ring_num %u (ring_id %d)",
		 desc_count, ring_num, ring_id);

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
	return 0;
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
#if defined(FEATURE_DP_DAL_SIM)
void dp_dal_sim_platform_bus_ops_attach(void)
{
	*global_plat_ops = dp_dal_sim_plat_ops;
}
#else
void dp_dal_sim_platform_bus_ops_attach(void)
{
}
#endif /* FEATURE_DP_DAL_SIM */
