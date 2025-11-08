/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: ISC
 */

#include "dp_dal.h"
#include "dp_dal_rx.h"
#include "dp_dal_tx.h"
#include <wlan_cfg.h>
#include <qdf_types.h>
#include "qdf_mem.h"

/**
 * dp_dal_bus_init_bypass_mode() - Skeleton for platform bus init
 * in bypass mode
 *
 * @priv: private data
 *
 * Return: 0 on success
 */
static int dp_dal_bus_init_bypass_mode(void *priv)
{
	return 0;
}

/**
 * dp_dal_bus_exit_bypass_mode() - Skeleton for platform bus exit in bypass mode
 *
 * @priv: private data
 */
static void dp_dal_bus_exit_bypass_mode(void *priv)
{
}

/**
 * dp_dal_bus_start_bypass_mode() - Skeleton for platform bus start
 * in bypass mode
 *
 * @priv: private data
 *
 * Return: 0 on success
 */
static int dp_dal_bus_start_bypass_mode(void *priv)
{
	return 0;
}

/**
 * dp_dal_bus_stop_bypass_mode() - Skeleton for platform bus stop in bypass mode
 *
 * @priv: private data
 */
static void dp_dal_bus_stop_bypass_mode(void *priv)
{
}

/**
 * dp_dal_request_irq_bypass_mode() - Skeleton for platform bus request irq in
 * bypass mode
 *
 * @priv: private data
 *
 * Return: 0 on success
 */
static int dp_dal_request_irq_bypass_mode(void *priv)
{
	return 0;
}

/**
 * dp_dal_sta_active_bypass_mode() - Skeleton for platform bus sta active
 * in bypass mode
 *
 * @priv: private data
 * @info: sta info
 * @enable: enable
 *
 * Return: 0 on success
 */
static int dp_dal_sta_active_bypass_mode(void *priv,
					 struct sta_info *info, bool enable)
{
	return 0;
}

/**
 * dp_dal_notify_suspend_bypass_mode() - Skeleton for platform bus notify
 * suspend in bypass mode
 *
 * @priv: private data
 *
 * Return: 0 on success
 */
static int dp_dal_notify_suspend_bypass_mode(void *priv)
{
	return 0;
}

/**
 * dp_dal_notify_resume_bypass_mode() - Skeleton for platform bus notify resume
 * in bypass mode
 *
 * @priv: private data
 *
 * Return: 0 on success
 */
static int dp_dal_notify_resume_bypass_mode(void *priv)
{
	return 0;
}

/**
 * dp_dal_ssr_dump_bypass_mode() - Skeleton for platform bus ssr dump
 * in bypass mode
 *
 * @segment: segment
 */
static void dp_dal_ssr_dump_bypass_mode(void *segment)
{
}

/**
 * dp_dal_intf_init_bypass_mode() - interface initialization in bypass mode
 * @priv: pointer to dal context
 * @intf_info: interface info
 *
 * Return: 0 on success
 */
static int dp_dal_intf_init_bypass_mode(void *priv, void *intf_info)
{
	return 0;
}

/**
 * dp_dal_intf_deinit_bypass_mode() - interface deinitialization in bypass mode
 * @priv: pointer to dal context
 * @vdev_id: vdev id corresponds to interface
 *
 * Return: 0 on success
 */
static int dp_dal_intf_deinit_bypass_mode(void *priv, uint16_t vdev_id)
{
	return 0;
}

struct platform_bus_ops plat_ops_bypass_mode = {
	.init = dp_dal_bus_init_bypass_mode,
	.exit = dp_dal_bus_exit_bypass_mode,
	.start = dp_dal_bus_start_bypass_mode,
	.stop = dp_dal_bus_stop_bypass_mode,
	.request_irq = dp_dal_request_irq_bypass_mode,
	.rx = dp_dal_rx_bypass_mode,
	.rx_replenish = dp_dal_rx_replenish_bypass_mode,
	.rxbm_sync = dp_dal_rx_rxbm_sync_bypass_mode,
	.tx = dp_dal_tx_bypass_mode,
	.tx_cpl = dp_dal_tx_cpl_bypass_mode,
	.tx_queue_active = dp_dal_tx_queue_active_bypass_mode,
	.sta_active = dp_dal_sta_active_bypass_mode,
	.notify_suspend = dp_dal_notify_suspend_bypass_mode,
	.notify_resume = dp_dal_notify_resume_bypass_mode,
	.ssr_dump = dp_dal_ssr_dump_bypass_mode,
	.intf_init = dp_dal_intf_init_bypass_mode,
	.intf_deinit = dp_dal_intf_deinit_bypass_mode,
};

struct platform_bus_ops *global_plat_ops = &plat_ops_bypass_mode;

struct vendor_cb_ops vendor_cb = {
	.rx_isr_cb = dp_dal_rx_isr_vendor_cb,
	.rx_replenish_alloc_cb = dp_dal_rx_replenish_alloc_vendor_cb,
	.rx_cpl_cb = dp_dal_rx_desc_cb,
	.tx_isr_cb = dp_dal_tx_cmp_isr_vendor_cb,
	.tx_cpl_cb = dp_dal_tx_cpl_cb,
};

/**
 * dp_dal_soc_detach - detach DP DAL to SOC
 * @soc: pointer to dp_soc structure
 *
 * Return: None.
 */
void dp_dal_soc_detach(struct dp_soc *soc)
{
	qdf_mem_common_free(soc->dal_ctx);
	soc->dal_ctx = NULL;
	dp_info("DAL context destroyed");
}

/**
 * dp_dal_soc_deinit - De-initialize DP DAL for SOC
 * @soc: pointer to dp_soc structure
 *
 * Return: None.
 */
void dp_dal_soc_deinit(struct dp_soc *soc)
{
	if (!soc || !soc->dal_ctx)
		return;

	dp_dal_rx_desc_list_cleanup(dal_ctx);

	qdf_spinlock_destroy(&soc->dal_ctx->dal_rx_desc_lock);
	qdf_spinlock_destroy(&soc->dal_ctx->dal_tx_cpl_lock);

	dp_dal_bus_stop(soc);
	dp_dal_bus_exit(soc);
}

/**
 * dp_dal_soc_attach - Attach DP DAL to SOC
 * @soc: pointer to dp_soc structure
 *
 * Return: QDF_STATUS_SUCCESS on success, error code on failure.
 */
QDF_STATUS dp_dal_soc_attach(struct dp_soc *soc)
{
	struct dp_dal_ctx *ctx;

	ctx = qdf_mem_common_alloc(sizeof(*ctx));
	if (!ctx) {
		dp_init_err("Failed to allocate memory for DAL context");
		return QDF_STATUS_E_FAILURE;
	}

	dp_info("DAL context allocated");

	ctx->soc = soc;
	soc->dal_ctx = ctx;

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS
dp_dal_update_ring_grp_id(struct dp_soc *soc,
			  struct dal_srng *dal_ring, enum hal_ring_type type)
{
	struct dp_intr *intr_ctx;
	uint8_t grp_id;
	uint8_t dal_tx_mask;
	uint8_t dal_rx_mask;
	uint8_t ring_idx;
	int i;

	for (i = 0; i < WLAN_CFG_INT_NUM_CONTEXTS; i++) {
		dal_tx_mask = soc->intr_ctx[i].dal_tx_ring_mask;
		dal_rx_mask = soc->intr_ctx[i].dal_rx_ring_mask;

		if (type == REO_DST && dal_rx_mask) {
			if (!(dal_rx_mask & (1 << dal_ring->ring_num)))
				continue;

			intr_ctx = &soc->intr_ctx[i];
			goto get_grp_id;
		}

		if (type == COMP_RING_TYPE && dal_tx_mask) {
			ring_idx = dal_ring->ring_num;

			if (!(1 << wlan_cfg_get_wbm_ring_num_for_index(soc->wlan_cfg_ctx,
								       ring_idx) &
			      dal_tx_mask))
				continue;

			intr_ctx = &soc->intr_ctx[i];
			goto get_grp_id;
		}
	}

	dp_err("Failed to get grp id for the DAL ring %d type %d",
	       dal_ring->ring_num, type);

	return QDF_STATUS_E_FAILURE;

get_grp_id:
	grp_id = hif_get_ext_grp_id(soc->hif_handle, intr_ctx);
	if (grp_id >= HIF_MAX_GROUP) {
		dp_err("failed to get grp id for the DAL ring %d type %d",
		       dal_ring->ring_num, type);
		return QDF_STATUS_E_FAILURE;
	}

	dal_ring->grp_id = grp_id;

	return QDF_STATUS_SUCCESS;
}

static QDF_STATUS dp_dal_create_ring_to_grp_mapping(struct dp_soc *soc)
{
	struct dp_dal_ctx *dal_ctx = soc->dal_ctx;
	struct dal_srng *dal_ring;
	int i;

	for (i = 0; i < DAL_RX_RINGS_MAX; i++) {
		dal_ring = &dal_ctx->rx_ring[i];

		if (!dal_ring->initialized)
			continue;

		if (dp_dal_update_ring_grp_id(soc, dal_ring, REO_DST) !=
		    QDF_STATUS_SUCCESS) {
			dp_err("Failed to update grp_id for RX ring %d", i);
			return QDF_STATUS_E_FAILURE;
		}
	}

	for (i = 0; i < DAL_TX_RINGS_MAX; i++) {
		dal_ring = &dal_ctx->tx_cmpl_ring[i];

		if (!dal_ring->initialized)
			continue;

		if (dp_dal_update_ring_grp_id(soc, dal_ring, COMP_RING_TYPE) !=
		    QDF_STATUS_SUCCESS) {
			dp_err("Failed to update grp_id for Tx cmp ring %d", i);
			return QDF_STATUS_E_FAILURE;
		}
	}

	return QDF_STATUS_SUCCESS;
}

/**
 * dp_dal_attach_rx_buffers - attach rx buffers to RXDMA_BUF ring
 * @soc: pointer to dp_soc structure
 *
 * Return: 0 on success
 */
static int dp_dal_attach_rx_buffers(struct dp_soc *soc)
{
	struct rx_desc_pool *rx_desc_pool;
	struct dp_dal_ctx *dal_ctx = soc->dal_ctx;

	if (!dal_ctx) {
		dp_err("DAL context is NULL");
		return -EINVAL;
	}

	rx_desc_pool = &soc->rx_desc_buf[0];

	if (global_plat_ops->rx_replenish) {
		return global_plat_ops->rx_replenish(dal_ctx,
						     rx_desc_pool->pool_size,
						     false);
	} else {
		dp_err("rx_replenish plat op is not registered");
		QDF_BUG(0);
	}

	return -EOPNOTSUPP;
}

/**
 * dp_dal_soc_init - Initialize DP DAL for SOC
 * @soc: pointer to dp_soc structure
 *
 * Return: QDF_STATUS_SUCCESS on success, error code on failure.
 */
QDF_STATUS dp_dal_soc_init(struct dp_soc *soc)
{
	struct dp_dal_ctx *dal_ctx;
	QDF_STATUS status;

	if (!soc || !soc->dal_ctx)
		return QDF_STATUS_E_INVAL;

	dal_ctx = soc->dal_ctx;

	qdf_spinlock_create(&dal_ctx->dal_tx_cpl_lock);
	qdf_spinlock_create(&dal_ctx->dal_rx_desc_lock);

	status = dp_dal_create_ring_to_grp_mapping(soc);
	if (status != QDF_STATUS_SUCCESS) {
		dp_err("failed to create DAL ring to grp mapping %d", status);
		goto destroy_lock;
	}

	status = dp_dal_bus_init(soc);
	if (status) {
		dp_err("DAL platform bus init failed %d", status);
		goto destroy_lock;
	}

	status = dp_dal_bus_request_irq(soc);
	if (status) {
		dp_err("DAL platform bus request IRQ failed %d", status);
		goto bus_deinit;
	}

	status = dp_dal_attach_rx_buffers(soc);
	if (status) {
		dp_err("DAL rx buffer attach failed %d", status);
		goto bus_deinit;
	}

	status = dp_dal_bus_start(soc);
	if (status) {
		dp_err("DAL platform bus start failed %d", status);
		goto bus_deinit;
	}

	dp_info("DAL SOC init completed successfully");

	return QDF_STATUS_SUCCESS;

bus_deinit:
	dp_info("DAL SOC init failed");
	dp_dal_bus_exit(soc);
destroy_lock:
	qdf_spinlock_destroy(&dal_ctx->dal_rx_desc_lock);
	qdf_spinlock_destroy(&dal_ctx->dal_tx_cpl_lock);
	return status;
}

/**
 * dp_dal_bus_exit() - DAL bus exit
 * @soc: pointer to DP SoC
 *
 * Called during driver deinit dp_pdev_deinit(), this function will release all
 * allocated resources in the offload engine and stops the Offload Engine.
 *
 * Return: void
 */
void dp_dal_bus_exit(struct dp_soc *soc)
{
	struct dp_dal_ctx *dal_ctx = soc->dal_ctx;

	if (!dal_ctx)
		return;

	if (global_plat_ops->exit)
		global_plat_ops->exit(dal_ctx);
}

/**
 * dp_dal_bus_init() - DAL bus initialization function
 * @soc: pointer to DP SoC
 *
 * Called during cdp_soc_attach_target(), this function sync TXBM information
 * to the offload engine.
 *
 * Return: int
 */
int dp_dal_bus_init(struct dp_soc *soc)
{
	struct dp_dal_ctx *dal_ctx = soc->dal_ctx;

	if (!dal_ctx)
		return -EINVAL;

	if (global_plat_ops->init)
		return global_plat_ops->init(dal_ctx);

	return 0;
}

/**
 * dp_dal_bus_stop - Stop DP DAL bus
 * @soc: pointer to dp_soc structure
 *
 * This function stops the DP DAL bus associated with the given SOC.
 */
void dp_dal_bus_stop(struct dp_soc *soc)
{
	struct dp_dal_ctx *dal_ctx = soc->dal_ctx;

	if (!dal_ctx)
		return;

	if (global_plat_ops->stop)
		global_plat_ops->stop(dal_ctx);
}

/**
 * dp_dal_bus_start() - DAL bus start function
 * @soc: pointer to DP SoC
 *
 * Called during cdp_soc_attach_target(), this function sync ring information
 * to the offload engine.
 *
 * Return: int
 */
int dp_dal_bus_start(struct dp_soc *soc)
{
	struct dp_dal_ctx *dal_ctx = soc->dal_ctx;

	if (!dal_ctx)
		return -EINVAL;

	if (global_plat_ops->start)
		return global_plat_ops->start(dal_ctx);

	return 0;
}

/**
 * dp_dal_bus_request_irq() - DAL IRQ registration function
 * @soc: pointer to DP SoC
 *
 * Called during cdp_soc_attach_target(), this function sync IRQ info to OE,
 * OE will register Tx & Rx interrupts.
 *
 * Return: int
 */
int dp_dal_bus_request_irq(struct dp_soc *soc)
{
	struct dp_dal_ctx *dal_ctx = soc->dal_ctx;

	if (!dal_ctx)
		return -EINVAL;

	if (global_plat_ops->request_irq)
		return global_plat_ops->request_irq(dal_ctx);

	return 0;
}

/**
 * dp_dal_bus_rx_buffer_enqueue() - DAL RX buffer enqueue function
 * @soc: pointer to DP SoC
 * @cnt: Number of RX buffers to replenish
 *
 * Called during cdp_soc_attach_target() and during RX replenish, this function
 * enqueues RX buffers to DAL, DAL/OE will in turn update the buffers into
 * SW2FW ring.
 *
 * Return: int
 */
int dp_dal_bus_rx_buffer_enqueue(struct dp_soc *soc, uint32_t cnt)
{
	struct dp_dal_ctx *dal_ctx = soc->dal_ctx;

	if (!dal_ctx)
		return -EINVAL;

	if (global_plat_ops->rx_replenish)
		return global_plat_ops->rx_replenish(dal_ctx, cnt, false);

	return 0;
}

static enum dal_intf_type
qdf_opmode_to_dal_intf_type(enum QDF_OPMODE mode)
{
	switch (mode) {
	case QDF_STA_MODE:
		return DAL_INTF_TYPE_STA;
	case QDF_SAP_MODE:
		return DAL_INTF_TYPE_SAP;
	default:
		return DAL_INTF_TYPE_MAX;
	}
}

/**
 * dp_dal_interface_add() - DAL interface add
 * @soc: pointer to DP SoC
 * @vdev: DP vdev structure
 *
 * Called during dp_vdev_attach_wifi3(), this function will add interface
 * details to offload engine.
 *
 * Return: int
 */
int dp_dal_interface_add(struct dp_soc *soc, struct dp_vdev *vdev)
{
	struct dp_dal_ctx *dal_ctx = soc->dal_ctx;
	struct dal_intf_info intf_info = {0};
	enum dal_intf_type type;
	int status;

	if (!dal_ctx) {
		dp_err("DAL context is NULL, cannot add interface");
		return QDF_STATUS_E_FAILURE;
	}

	type = qdf_opmode_to_dal_intf_type(vdev->qdf_opmode);
	if (type >= DAL_INTF_TYPE_MAX)
		return 0;

	intf_info.type = type;
	intf_info.vdev_id = vdev->vdev_id;
	intf_info.tcl_bank_id = vdev->bank_id;

	if (type == DAL_INTF_TYPE_STA)
		intf_info.tx_ring_id = DAL_TX_RING_ID0_STA;
	else if (type == DAL_TX_RING_ID0_SAP)
		intf_info.tx_ring_id = DAL_TX_RING_ID0_SAP;
	intf_info.tx_rbm_id =
		wlan_cfg_get_rbm_id_for_index(soc->wlan_cfg_ctx,
					      intf_info.tx_ring_id);

	qdf_mem_copy(&intf_info.mac_address[0],
		     &vdev->mac_addr.raw[0], QDF_MAC_ADDR_SIZE);

	if (global_plat_ops->intf_init) {
		status = global_plat_ops->intf_init(dal_ctx,
						    &intf_info);
		if (status) {
			dp_err("dal interface add failed vdev_id:%d status %d",
			       vdev->vdev_id, status);
			return status;
		}
	}

	if (global_plat_ops->tx_queue_active) {
		status = global_plat_ops->tx_queue_active(dal_ctx,
							  vdev->vdev_id, true);
		if (status) {
			dp_err("dal tx queue active failed vdev_id:%d status %d",
			       vdev->vdev_id, status);
			/* Cleanup the interface that was just initialized */
			if (global_plat_ops->intf_deinit)
				global_plat_ops->intf_deinit(dal_ctx,
							     vdev->vdev_id);
			return status;
		}
	}

	return 0;
}

/**
 * dp_dal_interface_remove() - DAL interface remove
 * @soc: pointer to DP SoC
 * @vdev_id: vdev ID of the interface
 *
 * Called during dp_vdev_detach_wifi3(), this function will remove interface
 * details from the offload engine.
 *
 * Return: None
 */
void dp_dal_interface_remove(struct dp_soc *soc, uint16_t vdev_id)
{
	struct dp_dal_ctx *dal_ctx = soc->dal_ctx;
	int status;

	if (!dal_ctx) {
		dp_err("DAL context is NULL, cannot remove interface");
		return;
	}

	if (global_plat_ops->tx_queue_active) {
		status = global_plat_ops->tx_queue_active(dal_ctx,
							  vdev_id, false);
		if (status)
			dp_err("dal txq deactivate failed vdev_id:%d status %d",
			       vdev_id, status);
		/* Continue to intf_deinit despite error */
	}

	if (global_plat_ops->intf_deinit) {
		status = global_plat_ops->intf_deinit(dal_ctx, vdev_id);
		if (status)
			dp_err("dal intf remove failed vdev_id:%d status %d",
			       vdev_id, status);
	}
}

/**
 * dp_dal_sta_active() - DAL API to send STA information
 * @soc: pointer to DP SoC
 * @info: station information
 * @enable: 0: disconnect, 1: connect
 *
 * Called during STA connect/disconnect, this function will share station
 * information to the offload engine.
 *
 * Return: int
 */
int dp_dal_sta_active(struct dp_soc *soc, struct sta_info *info, bool enable)
{
	struct dp_dal_ctx *dal_ctx = soc->dal_ctx;

	if (!dal_ctx || !info)
		return -EINVAL;

	if (global_plat_ops->sta_active)
		return global_plat_ops->sta_active(dal_ctx, info, enable);

	return 0;
}

static void dp_dal_update_ring_params(struct dp_soc *soc,
				      struct hal_srng *srng,
				      struct dal_srng *dal_ring)
{
	struct hal_soc *hal = (struct hal_soc *)soc->hal_soc;

	dal_ring->hal_ring_id = srng->ring_id;
	dal_ring->ring_base_paddr = srng->ring_base_paddr;
	dal_ring->ring_base_vaddr = srng->ring_base_vaddr;
	dal_ring->num_entries = srng->num_entries;
	dal_ring->ring_size = srng->ring_size;
	dal_ring->ring_size_mask = srng->ring_size_mask;
	dal_ring->entry_size = srng->entry_size;
	dal_ring->ring_type = srng->ring_type;
	dal_ring->ring_dir = srng->ring_dir;
	dal_ring->lmac_ring = srng->flags & HAL_SRNG_LMAC_RING ? true : false;

	if (srng->ring_dir == HAL_SRNG_SRC_RING) {
		dal_ring->u.src_ring.hp = srng->u.src_ring.hp;

		if (dal_ring->lmac_ring)
			dal_ring->u.src_ring.hp_addr =
				virt_to_phys(srng->u.src_ring.hp_addr);
		else
			dal_ring->u.src_ring.hp_addr =
					srng->u.src_ring.hp_addr -
					(uint32_t *)(hal->dev_base_addr);

		dal_ring->u.src_ring.tp_addr =
			virt_to_phys(srng->u.src_ring.tp_addr);
	} else {
		dal_ring->u.dst_ring.tp = srng->u.dst_ring.tp;

		if (dal_ring->lmac_ring)
			dal_ring->u.dst_ring.tp_addr =
				virt_to_phys(srng->u.src_ring.tp_addr);
		else
			dal_ring->u.dst_ring.tp_addr =
					srng->u.dst_ring.tp_addr -
					(uint32_t *)(hal->dev_base_addr);

		dal_ring->u.dst_ring.hp_addr =
				virt_to_phys(srng->u.dst_ring.hp_addr);
	}
}

void dp_dal_save_srng_info(struct dp_soc *soc, struct dp_srng *srng,
			   enum hal_ring_type type, int ring_num)
{
	struct dp_dal_ctx *dal_ctx = soc->dal_ctx;
	struct dal_srng *dal_ring;
	int ring_info_cnt;

	if (!dp_srng_check_dal_owned_ring(srng))
		return;

	switch (type) {
	case REO_DST:
		ring_info_cnt = dal_ctx->num_rx_ring_info;
		if (ring_info_cnt >= DAL_RX_RINGS_MAX) {
			dp_err("Max rx ring info limit:%d reached",
			       ring_info_cnt);
			return;
		}

		dal_ring = &dal_ctx->rx_ring[ring_info_cnt];
		dp_dal_update_ring_params(soc,
					  (struct hal_srng *)srng->hal_srng,
					  dal_ring);
		dal_ring->ring_num = ring_num;
		dal_ring->initialized = true;
		dal_ctx->num_rx_ring_info++;
		break;
	case TCL_DATA:
		ring_info_cnt = dal_ctx->num_tx_ring_info;
		if (ring_info_cnt >= DAL_TX_RINGS_MAX) {
			dp_err("Max tx ring info limit:%d reached",
			       ring_info_cnt);
			return;
		}

		dal_ring = &dal_ctx->tx_ring[ring_info_cnt];
		dp_dal_update_ring_params(soc,
					  (struct hal_srng *)srng->hal_srng,
					  dal_ring);
		dal_ring->ring_num = ring_num;
		dal_ring->initialized = true;
		dal_ctx->num_tx_ring_info++;
		break;
	case COMP_RING_TYPE:
		ring_info_cnt = dal_ctx->num_tx_cmpl_ring_info;
		if (ring_info_cnt >= DAL_TX_RINGS_MAX) {
			dp_err("Max tx cmpl ring info limit:%d reached",
			       ring_info_cnt);
			return;
		}

		dal_ring = &dal_ctx->tx_cmpl_ring[ring_info_cnt];
		dp_dal_update_ring_params(soc,
					  (struct hal_srng *)srng->hal_srng,
					  dal_ring);
		dal_ring->ring_num = ring_num;
		dal_ring->initialized = true;
		dal_ctx->num_tx_cmpl_ring_info++;
		break;
	case RXDMA_BUF:
		dal_ring = &dal_ctx->rx_refill_ring;
		dp_dal_update_ring_params(soc,
					  (struct hal_srng *)srng->hal_srng,
					  dal_ring);
		dal_ring->initialized = true;
		break;
	default:
		dp_err("Invalid ring info rcvd srng %pK type %d ring_num %d",
		       srng, type, ring_num);
	}
}

int dp_dal_get_ext_grp_id(struct dp_dal_ctx *dal_ctx,
			  int ring_num, enum hal_ring_type type)
{
	struct dal_srng *dal_ring;
	int grp_id = 0xFF;
	int i;

	if (type == REO_DST) {
		for (i = 0; i < DAL_RX_RINGS_MAX; i++) {
			dal_ring = &dal_ctx->rx_ring[i];
			if (dal_ring->ring_num == ring_num)
				return dal_ring->grp_id;
		}
	} else if (type == COMP_RING_TYPE) {
		for (i = 0; i < DAL_TX_RINGS_MAX; i++) {
			dal_ring = &dal_ctx->tx_cmpl_ring[i];
			if (dal_ring->ring_num == ring_num)
				return dal_ring->grp_id;
		}
	} else {
		dp_err("invalid ring_type:%d received", type);
	}

	return grp_id;
}

uint32_t dp_service_dal_srngs(void *dp_ctx, uint32_t dp_budget, int cpu)
{
	struct dp_intr *int_ctx = (struct dp_intr *)dp_ctx;
	struct dp_soc *soc = int_ctx->soc;
	int dal_tx_mask = 0;
	int dal_rx_mask = 0;
	int i;

	dal_tx_mask = int_ctx->dal_tx_ring_mask;
	dal_rx_mask = int_ctx->dal_rx_ring_mask;

	if (dal_rx_mask) {
		for (i = 0; i < soc->num_reo_dest_rings; i++) {
			if (!(dal_rx_mask & (1 << i)))
				continue;

			dp_dal_rx_handler(soc, i, dp_budget);
		}
	}

	if (dal_tx_mask) {
		for (i = 0; i < soc->num_tx_comp_rings; i++) {
			if (!(1 <<  wlan_cfg_get_wbm_ring_num_for_index(soc->wlan_cfg_ctx, i) &
			      dal_tx_mask))
				continue;

			dp_dal_tx_comp_handler(soc, i, dp_budget);
		}
	}

	return 0;
}
