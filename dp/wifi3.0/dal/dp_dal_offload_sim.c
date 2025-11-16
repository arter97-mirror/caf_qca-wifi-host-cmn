/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: ISC
 */

#include "dp_dal_offload_sim.h"
#include "dp_dal_sim.h"
#include <qdf_mem.h>
#include "dal_vndr_hal_be.h"
/* Maximum length for IRQ name strings */
#define OFFLOAD_SIM_IRQ_NAME_LEN 40
#ifdef FEATURE_DP_DAL_SIM

#ifdef DAL_OFFLOAD_SIM
/**
 * dp_dal_offload_sim_hal_addrs_params_init() - Form hal_srng address parameters
 * for offload simulation.
 * @offload_sim_ctx: Pointer to offload simulation context
 * @hal_srng: Pointer to destination dal_vndr_hal_srng structure
 * @sim_srng: Pointer to source dal_sim_srng structure
 *
 * hp/tp address are physical address for lmac rings and for umac rings
 * they are offset from BAR. For HP/TP addr, we need to add base address of
 * device to get the correct virtual address. Google offload engine has to
 * use BAR address to form correct physical address. Vendor HAL APIS are
 * agnostic of the address being used in the dal_vndr_hal_srng structure.
 * Care must be taken to assign correct address based on simulation mode or
 * real use case scenario.
 *
 * Return: None
 *
 */
static inline void dp_dal_offload_sim_hal_addrs_params_init(
	struct dp_dal_offload_sim_ctx *offload_sim_ctx,
	struct dal_vndr_hal_srng *hal_srng,
	struct dal_sim_srng *sim_srng)
{
	/* Here in simulation mode ring_base_addr is filled with virtual address
	 * so that vendor hal apis can use directly ring_base_addr to access
	 * descriptors. In real hardware, ring_base_addr will be filled with
	 * physical address.
	 */
	hal_srng->ring_base_addr = sim_srng->ring_base_vaddr;
	hal_srng->ring_base_paddr = sim_srng->ring_base_paddr;

	if (sim_srng->ring_dir == DAL_VNDR_HAL_SRNG_SRC_RING) {
		hal_srng->u.src_ring.hp = sim_srng->u.src_ring.hp;

		if (sim_srng->lmac_ring)
			/* In offload simulation we are filling virtual address
			 * in  hp_addr and tp_addr field. Usage for offload
			 * engine can vary.
			 *
			 * In case of lmac rings, hp_addr is physical address
			 * which needs to be converted to virtual address before
			 * passing to vendor HAL APIs in case of offload engine
			 * simulation. Real offload engine can use physical
			 * address.
			 */
			hal_srng->u.src_ring.hp_addr =
				phys_to_virt(sim_srng->u.src_ring.hp_addr);
		else
			hal_srng->u.src_ring.hp_addr =
				sim_srng->u.src_ring.hp_addr +
				offload_sim_ctx->dev_base_addr;

		hal_srng->u.src_ring.tp_addr =
			phys_to_virt(sim_srng->u.src_ring.tp_addr);
	} else {
		hal_srng->u.dst_ring.tp = sim_srng->u.dst_ring.tp;

		if (sim_srng->lmac_ring)
			hal_srng->u.dst_ring.tp_addr =
				phys_to_virt(sim_srng->u.dst_ring.tp_addr);
		else
			hal_srng->u.dst_ring.tp_addr =
					sim_srng->u.dst_ring.tp_addr +
					offload_sim_ctx->dev_base_addr;

		hal_srng->u.dst_ring.hp_addr =
				phys_to_virt(sim_srng->u.dst_ring.hp_addr);
	}
}
#else
static inline void dp_dal_offload_sim_hal_addrs_params_init(
				struct dp_dal_offload_sim_ctx *offload_sim_ctx,
				struct dal_vndr_hal_srng *hal_srng,
				struct dal_sim_srng *sim_srng)
{
}
#endif
/**
 * dp_dal_offload_sim_hal_ring_init() - Init dal_vndr_hal_srng structure.
 * @offload_sim_ctx: offload sim ctx
 * @hal_srng: Pointer to destination dal_vndr_hal_srng structure
 * @sim_srng: Pointer to source dal_sim_srng structure
 *
 * Copies ring information field by field from dal_sim_srng to dal_vndr_hal_srng
 */
static void dp_dal_offload_sim_hal_ring_init(
	struct dp_dal_offload_sim_ctx *offload_sim_ctx,
	struct dal_vndr_hal_srng *hal_srng,
	struct dal_sim_srng *sim_srng)
{
	/* Copy basic ring information */
	hal_srng->ring_id = sim_srng->hal_ring_id;
	hal_srng->num_entries = sim_srng->num_entries;
	hal_srng->ring_size = sim_srng->ring_size;
	hal_srng->ring_size_mask = sim_srng->ring_size_mask;
	hal_srng->lmac_ring = sim_srng->lmac_ring;
	hal_srng->entry_size = sim_srng->entry_size;
	hal_srng->msi_addr = sim_srng->msi_addr;
	hal_srng->msi_data = sim_srng->msi_data;
	hal_srng->ring_type = sim_srng->ring_type;
	hal_srng->ring_dir = sim_srng->ring_dir;
	hal_srng->irq_num = sim_srng->irq_num;
	hal_srng->hal_soc = &offload_sim_ctx->hal_soc;
	/* copy ring address information from sim ring to hal ring */
	dp_dal_offload_sim_hal_addrs_params_init(
		offload_sim_ctx, hal_srng, sim_srng);
	DAL_VNDR_SRNG_LOCK_INIT(&hal_srng->lock);
}
int dp_dal_offload_sim_init(struct dp_dal_sim_ctx *dal_sim_ctx)
{
	struct dp_dal_offload_sim_ctx *offload_ctx;
	int i;
	int status = 0;

	if (!dal_sim_ctx) {
		dp_err("NULL DAL sim context");
		return -EINVAL;
	}

	/* Allocate offload simulation context */
	offload_ctx = qdf_mem_malloc(sizeof(*offload_ctx));
	if (!offload_ctx) {
		dp_err("Failed to allocate offload sim context");
		return -ENOMEM;
	}
	/* Zero out the structure */
	qdf_mem_zero(offload_ctx, sizeof(*offload_ctx));

	offload_ctx->hal_soc.ops = qdf_mem_malloc(
					sizeof(*offload_ctx->hal_soc.ops));
	if (!offload_ctx->hal_soc.ops) {
		status = -ENOMEM;
		goto free_offload_ctx;
	}

	/* Store the dev base addr here from dal_sim_ctx.
	 * Google DAL has to assign BAR address here.
	 */
	offload_ctx->hal_soc.dev_base_addr = dal_sim_ctx->dev_base_addr;
	/* Copy dal_vndr_hal_srng from dal_sim_srng structures */
	/* RX rings */
	for (i = 0; i < DAL_RX_RINGS_MAX; i++) {
		dp_dal_offload_sim_hal_ring_init(
					offload_ctx,
					&offload_ctx->rx_ring_hal_srng[i],
					&dal_sim_ctx->rx_ring[i]);
	}

	/* TX completion rings */
	for (i = 0; i < DAL_TX_RINGS_MAX; i++) {
		dp_dal_offload_sim_hal_ring_init(
					offload_ctx,
					&offload_ctx->tx_cmpl_ring_hal_srng[i],
					&dal_sim_ctx->tx_cmpl_ring[i]);
	}

	/* TX rings */
	for (i = 0; i < DAL_TX_RINGS_MAX; i++) {
		dp_dal_offload_sim_hal_ring_init(
					offload_ctx,
					&offload_ctx->tx_ring_hal_srng[i],
					&dal_sim_ctx->tx_ring[i]);
	}

	/* RX refill ring */
	dp_dal_offload_sim_hal_ring_init(offload_ctx,
					 &offload_ctx->rx_refill_ring_hal_srng,
					 &dal_sim_ctx->rx_refill_ring);

	/* Vendor HAL ops can be overridden here if needed with target_type*/
	dal_vndr_hal_default_ops_attach_be(&offload_ctx->hal_soc);

	/* Mark as offload_sim_ctx_initialized */
	offload_ctx->offload_sim_ctx_initialized = true;

	/* Assign to dal_sim_ctx */
	dal_sim_ctx->offload_sim_ctx = offload_ctx;

	return 0;
free_offload_ctx:
	qdf_mem_free(offload_ctx);
	return status;
}

void dp_dal_offload_sim_deinit(struct dp_dal_sim_ctx *dal_sim_ctx)
{
	struct dp_dal_offload_sim_ctx *offload_ctx;

	if (!dal_sim_ctx) {
		dp_warn("NULL DAL sim context in deinit");
		return;
	}

	offload_ctx =
		(struct dp_dal_offload_sim_ctx *)dal_sim_ctx->offload_sim_ctx;
	if (!offload_ctx) {
		dp_warn("NULL offload sim context in deinit");
		return;
	}

	dp_info("Deinitializing offload simulation context");

	/* Free IRQs before deinitializing */
	dp_dal_offload_sim_free_irq(dal_sim_ctx);

	/* Mark as not initialized */
	offload_ctx->offload_sim_ctx_initialized = false;
	/* Free hal_soc ops memory allocated in init */
	qdf_mem_free(offload_ctx->hal_soc.ops);
	/* Free the context */
	qdf_mem_free(offload_ctx);

	/* Nullify the pointer in dal_sim_ctx */
	dal_sim_ctx->offload_sim_ctx = NULL;
	dp_info("Offload simulation context deinitialized successfully");
}

/**
 * dp_dal_offload_sim_interrupt_handler() - Interrupt handler for offload sim
 * @irq: IRQ number
 * @arg: Pointer to offload_sim_irq_ctx structure
 *
 * This function is called when an interrupt is received. It calls
 * dp_dal_sim_schedule_work to queue work for processing the interrupt.
 *
 * Return: IRQ_HANDLED
 */
static irqreturn_t dp_dal_offload_sim_interrupt_handler(int irq, void *arg)
{
	struct offload_sim_irq_ctx *irq_ctx = (struct offload_sim_irq_ctx *)arg;
	struct dp_dal_sim_ctx *sim_ctx;

	if (!irq_ctx) {
		dp_err("NULL IRQ context in interrupt handler");
		return IRQ_HANDLED;
	}

	sim_ctx = irq_ctx->dal_sim_ctx;
	if (!sim_ctx) {
		dp_err("NULL sim context in interrupt handler");
		return IRQ_HANDLED;
	}

	dp_debug("Interrupt %d received for ring_type=%d, ring_id=%d",
		 irq, irq_ctx->ring_type, irq_ctx->ring_id);
	/* Call dal sim api to queue work for processing interrupt */
	dp_dal_sim_schedule_work(arg);

	return IRQ_HANDLED;
}

int dp_dal_offload_sim_request_irq(struct dp_dal_sim_ctx *dal_sim_ctx)
{
	struct dp_dal_offload_sim_ctx *offload_ctx;
	int i, ret;
	char irq_name[OFFLOAD_SIM_IRQ_NAME_LEN];

	if (!dal_sim_ctx) {
		dp_err("NULL DAL sim context in request_irq");
		return -EINVAL;
	}

	offload_ctx =
		(struct dp_dal_offload_sim_ctx *)dal_sim_ctx->offload_sim_ctx;
	if (!offload_ctx) {
		dp_err("NULL offload sim context in request_irq");
		return -EINVAL;
	}

	dp_info("Registering IRQs for offload simulation");

	/* Register RX ring IRQs */
	for (i = 0; i < DAL_RX_RINGS_MAX; i++) {
		/* Initialize IRQ context */
		offload_ctx->rx_irq_ctx[i].dal_sim_ctx = dal_sim_ctx;
		offload_ctx->rx_irq_ctx[i].ring_id = i;
		offload_ctx->rx_irq_ctx[i].ring_type = OFFLOAD_SIM_RING_TYPE_RX;
		offload_ctx->rx_irq_ctx[i].irq_configured = false;
		/* Format IRQ name with group ID */
		qdf_scnprintf(irq_name, OFFLOAD_SIM_IRQ_NAME_LEN,
			      "offload_sim_wlan_grp_id_%d",
			      dal_sim_ctx->rx_ring[i].grp_id);
		/* Register IRQ using platform-specific function */
		ret = pfrm_request_irq(
			dal_sim_ctx->dev,
			dal_sim_ctx->rx_ring[i].irq_num,
			dp_dal_offload_sim_interrupt_handler,
			IRQF_SHARED | IRQF_NO_SUSPEND,
			irq_name,
			&offload_ctx->rx_irq_ctx[i]);
		if (ret) {
			dp_err("Failed irq register RX ring %d, ret=%d",
			       i, ret);
			goto free_irqs;
		}
		/* Mark IRQ as configured on successful registration */
		offload_ctx->rx_irq_ctx[i].irq_configured = true;
		dp_info("Registered RX IRQ %d for ring %d",
			dal_sim_ctx->rx_ring[i].irq_num, i);
	}

	/* Register TX completion ring IRQs */
	for (i = 0; i < DAL_TX_RINGS_MAX; i++) {
		/* Initialize IRQ context */
		offload_ctx->tx_cpl_irq_ctx[i].dal_sim_ctx = dal_sim_ctx;
		offload_ctx->tx_cpl_irq_ctx[i].ring_id = i;
		offload_ctx->tx_cpl_irq_ctx[i].ring_type =
						OFFLOAD_SIM_RING_TYPE_TX_CPL;
		offload_ctx->tx_cpl_irq_ctx[i].irq_configured = false;
		/* Format IRQ name with group ID */
		qdf_scnprintf(irq_name, OFFLOAD_SIM_IRQ_NAME_LEN,
			      "offload_sim_wlan_grp_id_%d",
			      dal_sim_ctx->tx_cmpl_ring[i].grp_id);
		/* Register IRQ using platform-specific function */
		ret = pfrm_request_irq(
				dal_sim_ctx->dev,
				dal_sim_ctx->tx_cmpl_ring[i].irq_num,
				dp_dal_offload_sim_interrupt_handler,
				IRQF_SHARED | IRQF_NO_SUSPEND,
				irq_name,
				&offload_ctx->tx_cpl_irq_ctx[i]);
		if (ret) {
			dp_err("Failed irq register TX compl ring %d, ret=%d",
			       i, ret);
			goto free_irqs;
		}
		/* Mark IRQ as configured on successful registration */
		offload_ctx->tx_cpl_irq_ctx[i].irq_configured = true;
		dp_info("Registered TX completion IRQ %d for ring %d",
			dal_sim_ctx->tx_cmpl_ring[i].irq_num, i);
	}

	dp_info("IRQ registration complete");
	return 0;

free_irqs:
	/* Free any IRQs that were successfully registered */
	dp_dal_offload_sim_free_irq(dal_sim_ctx);
	return ret;
}

void dp_dal_offload_sim_free_irq(struct dp_dal_sim_ctx *dal_sim_ctx)
{
	struct dp_dal_offload_sim_ctx *offload_ctx;
	int i;

	if (!dal_sim_ctx) {
		dp_warn("NULL DAL sim context in free_irq");
		return;
	}

	offload_ctx =
		(struct dp_dal_offload_sim_ctx *)dal_sim_ctx->offload_sim_ctx;
	if (!offload_ctx) {
		dp_warn("NULL offload sim context in free_irq");
		return;
	}

	/* Free RX ring IRQs */
	for (i = 0; i < DAL_RX_RINGS_MAX; i++) {
		/* Only free IRQ if it was successfully configured */
		if (offload_ctx->rx_irq_ctx[i].irq_configured) {
			pfrm_free_irq(dal_sim_ctx->dev,
				      dal_sim_ctx->rx_ring[i].irq_num,
				      &offload_ctx->rx_irq_ctx[i]);
			offload_ctx->rx_irq_ctx[i].irq_configured = false;
			dp_debug("Freed RX IRQ %d for ring %d",
				 dal_sim_ctx->rx_ring[i].irq_num, i);
		}
	}

	/* Free TX completion ring IRQs */
	for (i = 0; i < DAL_TX_RINGS_MAX; i++) {
		/* Only free IRQ if it was successfully configured */
		if (offload_ctx->tx_cpl_irq_ctx[i].irq_configured) {
			pfrm_free_irq(dal_sim_ctx->dev,
				      dal_sim_ctx->tx_cmpl_ring[i].irq_num,
				       &offload_ctx->tx_cpl_irq_ctx[i]);
			offload_ctx->tx_cpl_irq_ctx[i].irq_configured = false;
			dp_debug("Freed TX completion IRQ %d for ring %d",
				 dal_sim_ctx->tx_cmpl_ring[i].irq_num, i);
		}
	}
}

/**
 * dp_dal_offload_sim_tx_hw_enqueue() - Enqueue TX descriptor to hardware ring
 * @dal_sim_ctx: Pointer to DAL simulation context
 * @ring_id: Ring ID for TCL descriptor enqueue
 * @desc: Pointer to the cached TCL descriptor
 * @tx_metadata: Pointer to TX metadata
 *
 * This wrapper function handles ring access start/end and syncs the TX
 * descriptor content to hardware. It encapsulates the ring access logic
 * for TX descriptor enqueue operations.
 *
 * Return: 0 on success, negative error code on failure
 */
int dp_dal_offload_sim_tx_hw_enqueue(
			struct dp_dal_sim_ctx *dal_sim_ctx,
			u8 ring_id,
			void *desc,
			void *tx_metadata)
{
	struct dp_dal_ctx *dal_ctx;
	struct dp_dal_offload_sim_ctx *offload_ctx;
	struct dal_vndr_hal_srng *tcl_ring;
	void *hal_tx_desc;
	int ret = 0;

	if (!dal_sim_ctx) {
		dp_err("NULL simulator context in tx_hw_enqueue");
		return -EINVAL;
	}

	dal_ctx = (struct dp_dal_ctx *)dal_sim_ctx->dp_dal_ctx;
	if (!dal_ctx) {
		dp_err("NULL DAL context in tx_hw_enqueue");
		return -EINVAL;
	}

	offload_ctx =
		(struct dp_dal_offload_sim_ctx *)dal_sim_ctx->offload_sim_ctx;
	if (!offload_ctx) {
		dp_err("NULL offload context in tx_hw_enqueue");
		return -EINVAL;
	}

	/* Get TCL ring for the specified ring_id from offload context */
	tcl_ring = &offload_ctx->tx_ring_hal_srng[ring_id];

	dp_debug("Enqueuing TX descriptor for ring_id %u", ring_id);
	DAL_VNDR_SRNG_LOCK(&tcl_ring->lock);
	/* Begin ring access */
	dal_vndr_hal_srng_access_start(&offload_ctx->hal_soc, tcl_ring);

	/* Get next available descriptor slot in TCL ring */
	hal_tx_desc = dal_vndr_hal_srng_src_get_next(&offload_ctx->hal_soc,
						     tcl_ring);
	if (!hal_tx_desc) {
		dp_verbose_debug("TCL ring full for ring_id %u", ring_id);
		ret = -ENOSPC;
		goto exit;
	}

	/* Sync cached descriptor content to HW descriptor */
	dal_vndr_hal_tx_desc_sync(desc, hal_tx_desc,
				  DAL_VNDR_HAL_TX_DESC_LEN_BYTES);

	dp_debug("TX descriptor enqueued successfully for ring_id %u", ring_id);
exit:
	/* End ring access and update the HP */
	dal_vndr_hal_srng_access_end(&offload_ctx->hal_soc, tcl_ring);
	DAL_VNDR_SRNG_UNLOCK(&tcl_ring->lock);

	return ret;
}

int dp_dal_offload_sim_get_reo_desc(
				struct dp_dal_sim_ctx *dal_sim_ctx,
				u16 ring_id,
				void **desc_list,
				u32 *count,
				u32 budget)
{
	struct dp_dal_offload_sim_ctx *offload_ctx;
	struct dal_vndr_hal_srng *reo_ring;
	void *reo_desc;
	u32 retrieved = 0;

	if (!dal_sim_ctx) {
		dp_err("NULL simulator context in get_reo_desc");
		return -EINVAL;
	}

	if (!desc_list || !count) {
		dp_err("NULL desc_list or count pointer in get_reo_desc");
		return -EINVAL;
	}

	offload_ctx =
		(struct dp_dal_offload_sim_ctx *)dal_sim_ctx->offload_sim_ctx;
	if (!offload_ctx) {
		dp_err("NULL offload context in get_reo_desc");
		return -EINVAL;
	}

	/* Get REO ring for the specified ring_id */
	reo_ring = &offload_ctx->rx_ring_hal_srng[ring_id];

	dp_debug("Getting REO descriptors for ring_id %u with budget %u",
		 ring_id, budget);

	/* Lock the ring */
	DAL_VNDR_SRNG_LOCK(&reo_ring->lock);

	/* Begin ring access */
	dal_vndr_hal_srng_access_start(&offload_ctx->hal_soc, reo_ring);

	/* Reap REO descriptors until budget is reached or no more descriptor */
	while (retrieved < budget) {
		/* Get next REO descriptor from the ring */
		reo_desc = dal_vndr_hal_srng_dst_get_next(&offload_ctx->hal_soc,
							  reo_ring);
		if (!reo_desc) {
			/* No more descriptors in this ring */
			break;
		}

		/* Store descriptor in the list */
		desc_list[retrieved] = reo_desc;
		retrieved++;
	}

	/* End ring access */
	dal_vndr_hal_srng_access_end(&offload_ctx->hal_soc, reo_ring);

	/* Unlock the ring */
	DAL_VNDR_SRNG_UNLOCK(&reo_ring->lock);

	/* Update the count */
	*count = retrieved;

	dp_debug("Retrieved %u REO descriptors for ring_id %u",
		 retrieved, ring_id);

	return 0;
}

int dp_dal_offload_sim_get_tx_compl_desc(
				struct dp_dal_sim_ctx *dal_sim_ctx,
				u16 ring_id,
				void **desc_list,
				u32 *count,
				u32 budget)
{
	struct dp_dal_offload_sim_ctx *offload_ctx;
	struct dal_vndr_hal_srng *tx_compl_ring;
	void *tx_compl_desc;
	u32 retrieved = 0;

	if (!dal_sim_ctx) {
		dp_err("NULL simulator context in get_tx_compl_desc");
		return -EINVAL;
	}

	if (!desc_list || !count) {
		dp_err("NULL desc_list or count pointer in get_tx_compl_desc");
		return -EINVAL;
	}

	offload_ctx =
		(struct dp_dal_offload_sim_ctx *)dal_sim_ctx->offload_sim_ctx;
	if (!offload_ctx) {
		dp_err("NULL offload context in get_tx_compl_desc");
		return -EINVAL;
	}

	/* Get TX completion ring for the specified ring_id */
	tx_compl_ring = &offload_ctx->tx_cmpl_ring_hal_srng[ring_id];

	dp_debug("Getting TX compl descriptors for ring_id %u with budget %u",
		 ring_id, budget);

	/* Lock the ring */
	DAL_VNDR_SRNG_LOCK(&tx_compl_ring->lock);

	/* Begin ring access */
	dal_vndr_hal_srng_access_start(&offload_ctx->hal_soc, tx_compl_ring);

	/* Reap TX completion descriptors until budget is reached or
	 * no more descriptors.
	 */
	while (retrieved < budget) {
		/* Get next TX completion descriptor from the ring */
		tx_compl_desc = dal_vndr_hal_srng_dst_get_next(
						&offload_ctx->hal_soc,
						tx_compl_ring);
		if (!tx_compl_desc) {
			/* No more descriptors in this ring */
			break;
		}

		/* Store descriptor in the list */
		desc_list[retrieved] = tx_compl_desc;
		retrieved++;
	}

	/* End ring access */
	dal_vndr_hal_srng_access_end(&offload_ctx->hal_soc, tx_compl_ring);

	/* Unlock the ring */
	DAL_VNDR_SRNG_UNLOCK(&tx_compl_ring->lock);

	/* Update the count */
	*count = retrieved;

	dp_debug("Retrieved %u TX completion descriptors for ring_id %u",
		 retrieved, ring_id);

	return 0;
}

uint32_t dp_dal_offload_sim_get_rx_refill_avail_entries(
					struct dp_dal_sim_ctx *dal_sim_ctx)
{
	struct dp_dal_offload_sim_ctx *offload_ctx;
	struct dal_vndr_hal_srng *rx_refill_ring;
	uint32_t num_entries_avail = 0;

	if (!dal_sim_ctx) {
		dp_err("NULL simulator context in get_rx_refill_avail_entries");
		return 0;
	}

	offload_ctx =
		(struct dp_dal_offload_sim_ctx *)dal_sim_ctx->offload_sim_ctx;
	if (!offload_ctx) {
		dp_err("NULL offload context in get_rx_refill_avail_entries");
		return 0;
	}

	/* Get RX refill ring */
	rx_refill_ring = &offload_ctx->rx_refill_ring_hal_srng;

	DAL_VNDR_SRNG_LOCK(&rx_refill_ring->lock);
	/* Get number of available entries using vendor HAL API */
	num_entries_avail = dal_vndr_hal_srng_src_num_avail(
							&offload_ctx->hal_soc,
							rx_refill_ring,
							1);

	DAL_VNDR_SRNG_UNLOCK(&rx_refill_ring->lock);
	dp_debug("RX refill ring available entries: %u", num_entries_avail);

	return num_entries_avail;
}

int dp_dal_offload_sim_rxbm_sync(
				struct dp_dal_sim_ctx *dal_sim_ctx,
				u32 cnt,
				void **rx_buff)
{
	struct dp_dal_offload_sim_ctx *offload_ctx;
	struct dal_vndr_hal_srng *rx_refill_ring;
	void *refill_desc;
	u32 i;

	if (!dal_sim_ctx) {
		dp_err("NULL simulator context in rxbm_sync");
		return 0;
	}

	if (!rx_buff) {
		dp_err("NULL rx_buff array in rxbm_sync");
		return 0;
	}

	offload_ctx =
		(struct dp_dal_offload_sim_ctx *)dal_sim_ctx->offload_sim_ctx;
	if (!offload_ctx) {
		dp_err("NULL offload context in rxbm_sync");
		return 0;
	}

	/* Get RX refill ring */
	rx_refill_ring = &offload_ctx->rx_refill_ring_hal_srng;

	/* Lock the ring */
	DAL_VNDR_SRNG_LOCK(&rx_refill_ring->lock);

	/* Begin ring access */
	dal_vndr_hal_srng_access_start(&offload_ctx->hal_soc, rx_refill_ring);

	/* Copy descriptors one by one to the refill ring entries */
	for (i = 0; i < cnt; i++) {
		/* Get next available entry in the refill ring */
		refill_desc = dal_vndr_hal_srng_src_get_next(
							&offload_ctx->hal_soc,
							rx_refill_ring);
		if (!refill_desc) {
			dp_err_rl("refill ring full synced %u/%u descriptors",
				  i, cnt);
			break;
		}

		/* Copy descriptor from rx_buff array to refill ring entry */
		dal_vndr_hal_rxbm_sync(&offload_ctx->hal_soc,
				       refill_desc, rx_buff[i]);
	}

	/* End ring access and update the HP */
	dal_vndr_hal_srng_access_end(&offload_ctx->hal_soc, rx_refill_ring);

	/* Unlock the ring */
	DAL_VNDR_SRNG_UNLOCK(&rx_refill_ring->lock);

	dp_debug("synced %u RX buffer descriptors to refill ring",
		 i);

	return i;
}

#endif /* FEATURE_DP_DAL_SIM */
