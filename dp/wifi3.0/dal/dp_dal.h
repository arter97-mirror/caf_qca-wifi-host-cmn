/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: ISC
 */

#ifndef DP_DAL_H
#define DP_DAL_H

#include <qdf_status.h>
#include "dp_types.h"

#define DP_REO_DST_REMAP_REMOVE_DAL(_reo_config) \
	((_reo_config) &= ~(DAL_DP_REO_RING_MASK))

#ifdef FEATURE_DAL_DP_SUPPORT
#define DAL_DP_TCL_RING_MASK 0x3
#define DAL_DP_REO_RING_MASK 0xc

#define PRIORITY_CLASS 8
#define MAC_ADDR_LEN 6

#define DAL_RX_RINGS_MAX 2
#define DAL_TX_RINGS_MAX 2

#define DAL_TX_RING_ID0_STA 0
#define DAL_TX_RING_ID0_SAP 1

extern struct platform_bus_ops *global_plat_ops;

/**
 * struct sta_info - STA information
 * @bss_idx: BSS index
 * @qos_txq_map: QoS TX queue map
 * @addr: MAC address
 */
struct sta_info {
	u16 bss_idx;
	u16 qos_txq_map[PRIORITY_CLASS];
	u8 addr[MAC_ADDR_LEN];
};

/**
 * struct platform_bus_ops - Platform bus operations
 * @init: Initialize the bus
 * @exit: Exit the bus
 * @start: Start the bus
 * @stop: Stop the bus
 * @request_irq: Request an interrupt
 * @rx: RX operation
 * @rx_replenish: Replenish RX buffers
 * @rxbm_sync: Sync RX buffer manager
 * @tx: TX operation
 * @tx_cpl: TX completion
 * @tx_queue_active: Check if TX queue is active
 * @sta_active: Check if STA is active
 * @notify_suspend: Notify suspend
 * @notify_resume: Notify resume
 * @ssr_dump: Dump SSR
 * @intf_init: Interface init
 * @intf_deinit: Interface deinit
 */
struct platform_bus_ops {
	int (*init)(void *priv);
	void (*exit)(void *priv);
	int (*start)(void *priv);
	void (*stop)(void *priv);
	int (*request_irq)(void *priv);
	bool (*rx)(void *priv, u32 *cnt, u16 ring_id);
	int (*rx_replenish)(void *priv, u32 cnt, bool use_rsv_pktid);
	int (*rxbm_sync)(void *priv, u32 cnt, void **rxbm);
	int (*tx)(void *priv, u8 ring_id, u32 ifidx, void *desc,
		  void *tx_metadata);
	bool (*tx_cpl)(void *priv, u32 *cnt, u16 ring_id);
	int (*tx_queue_active)(void *priv, u16 flowid, bool enable);
	int (*sta_active)(void *priv, struct sta_info *info, bool enable);
	int (*notify_suspend)(void *priv);
	int (*notify_resume)(void *priv);
	void (*ssr_dump)(void *segment);
	int (*intf_init)(void *priv, void *intf_info);
	int (*intf_deinit)(void *priv, uint16_t vdev_id);
};

/**
 * struct vendor_cb_ops - Vendor callback operations
 * @rx_isr_cb: RX ISR callback
 * @rx_cpl_cb: RX completion callback
 * @rx_replenish_alloc_cb: RX replenish allocation callback
 * @tx_cpl_cb: TX completion callback
 * @tx_isr_cb: Tx ISR callback
 */
struct vendor_cb_ops {
	int (*rx_isr_cb)(int ring_num, void *priv);
	int (*rx_cpl_cb)(void *priv, void *desc);
	int (*rx_replenish_alloc_cb)(void *priv, u16 count);
	int (*tx_cpl_cb)(void *priv, void *desc, u16 ring_id);
	int (*tx_isr_cb)(int rint_num, void *priv);
};

/**
 * struct dal_srng - Ring information for DAL rings
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
 * @u: union for src ring or dest ring
 * @tp: tail pointer value
 * @hp_addr: UMAC ring it is HP offset form BAR
 *		LMAC ring it is HP physical addr
 * @tp_addr: physical address of TP
 * @hp: head pointer value
 * @tp_addr: UMAC ring it is TP offset form BAR
 *		LMAC ring it is TP physical addr
 * @hp_addr: physical address of HP
 * @ring_num: ring number used to indicate isr indication
 * @grp_id: ext group context id associated with the ring
 *
 * This structure has all the ring information that will be passed to DAL
 * during init.
 */
struct dal_srng {
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
	enum hal_srng_dir ring_dir;
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
	uint8_t grp_id;
};

/**
 * enum dal_intf_type - interface type
 * @DAL_INTF_TYPE_STA: station interface
 * @DAL_INTF_TYPE_SAP: SAP interface
 *
 * @DAL_INTF_TYPE_MAX: max supported interface type
 */
enum dal_intf_type {
	DAL_INTF_TYPE_STA,
	DAL_INTF_TYPE_SAP,
	DAL_INTF_TYPE_MAX,
};

/**
 * struct dal_intf_info - DAL interface information
 * @type: Interface type
 * @mac_address: MAC address
 * @bss_idx: BSS index
 * @vdev_id: VDEV ID
 * @tcl_bank_id: TCL bank ID
 * @tx_ring_id: TX ring ID to use for tethered traffic
 * @tx_rbm_id: TX RBM ID to use for tethered traffic
 */
struct dal_intf_info {
	enum dal_intf_type type;
	uint8_t mac_address[MAC_ADDR_LEN];
	int bss_idx;
	uint16_t vdev_id;
	uint8_t tcl_bank_id;
	uint8_t tx_ring_id;
	uint8_t tx_rbm_id;
};

/**
 * struct dp_dal_rx_desc_node - DAL RX descriptor list node
 * @next: Next pointer for DAL queue
 * @rx_desc: Pointer to actual RX descriptor
 *
 * This structure is used to create a linked list for DAL RX
 * descriptor queuing.
 */
struct dp_dal_rx_desc_node {
	struct dp_dal_rx_desc_node *next;
	struct dp_rx_desc *rx_desc;
};

/**
 * struct dp_dal_ctx - Context structure for DAL simulation
 * @soc: Pointer to DP SoC
 * @rx_ring: Array of HAL SRNG structures for RX rings.
 * @tx_cmpl_ring: Array of HAL SRNG structures for TX completion rings.
 * @tx_ring: Array of HAL SRNG structures for TX rings.
 * @rx_refill_ring: HAL SRNG structure for RX refill ring.
 * @num_tx_ring_info: number of tx ring info saved
 * @num_rx_ring_info: number of rx ring info saved
 * @num_tx_cmpl_ring_info: number of tx completion ring info saved
 * @tx_cpl_desc_list: TX completion descriptor head list for each ring
 * @tx_cpl_desc_tail: TX completion descriptor tail list for each ring
 * @tx_cpl_desc_count: TX completion descriptor counts for each ring
 * @dal_tx_cpl_lock: Spinlock to protect TX completion descriptor
 *		     list operations
 * @rx_desc_head: RX descriptor node head list for each ring
 * @rx_desc_tail: RX descriptor node tail list for each ring
 * @rx_desc_count: RX descriptor counts for each ring
 * @dal_rx_desc_lock: Spinlock to protect RX descriptor list operations
 *
 * This structure maintains all necessary context for DAL operations,
 * including pointers to datapath context, platform operations, vendor
 * callbacks, for RX and TX operations. This structure is passed as priv
 * argument in all DAL APIs.
 */
struct dp_dal_ctx {
	struct dp_soc *soc;
	struct dal_srng rx_ring[DAL_RX_RINGS_MAX];
	struct dal_srng tx_cmpl_ring[DAL_TX_RINGS_MAX];
	struct dal_srng tx_ring[DAL_TX_RINGS_MAX];
	struct dal_srng rx_refill_ring;
	int num_tx_ring_info;
	int num_rx_ring_info;
	int num_tx_cmpl_ring_info;
	struct dp_tx_desc_s *tx_cpl_desc_list[MAX_TCL_DATA_RINGS];
	struct dp_tx_desc_s *tx_cpl_desc_tail[MAX_TCL_DATA_RINGS];
	uint32_t tx_cpl_desc_count[MAX_TCL_DATA_RINGS];
	qdf_spinlock_t dal_tx_cpl_lock;
	struct dp_dal_rx_desc_node *rx_desc_head[MAX_REO_DEST_RINGS];
	struct dp_dal_rx_desc_node *rx_desc_tail[MAX_REO_DEST_RINGS];
	uint32_t rx_desc_count[MAX_REO_DEST_RINGS];
	qdf_spinlock_t dal_rx_desc_lock;
};

/**
 * dp_dal_soc_detach - detach DP DAL to SOC
 * @soc: pointer to dp_soc structure
 *
 * Return: None.
 */
void dp_dal_soc_detach(struct dp_soc *soc);

/**
 * dp_dal_soc_deinit - De-initialize DP DAL for SOC
 * @soc: pointer to dp_soc structure
 *
 * Return: None.
 */
void dp_dal_soc_deinit(struct dp_soc *soc);

/**
 * dp_dal_soc_attach - Attach DP DAL to SOC
 * @soc: pointer to dp_soc structure
 *
 * Return: QDF_STATUS_SUCCESS on success, error code on failure.
 */
QDF_STATUS dp_dal_soc_attach(struct dp_soc *soc);

/**
 * dp_dal_soc_init - Initialize DP DAL for SOC
 * @soc: pointer to dp_soc structure
 *
 * Return: QDF_STATUS_SUCCESS on success, error code on failure.
 */
QDF_STATUS dp_dal_soc_init(struct dp_soc *soc);

/**
 * dp_dal_bus_stop - Stop DP DAL bus
 * @soc: pointer to dp_soc structure
 *
 * This function stops the DP DAL bus associated with the given SOC.
 */
void dp_dal_bus_stop(struct dp_soc *soc);

/**
 * dp_dal_bus_exit - Exit DP DAL bus
 * @soc: pointer to dp_soc structure
 *
 * This function performs cleanup when exiting the DP DAL bus.
 */
void dp_dal_bus_exit(struct dp_soc *soc);

/**
 * dp_dal_bus_init() - DAL bus initialization function
 * @soc: pointer to DP SoC
 *
 * Called during cdp_soc_attach_target(), this function sync TXBM information
 * to the offload engine.
 *
 * Return: int
 */
int dp_dal_bus_init(struct dp_soc *soc);

/**
 * dp_dal_bus_start() - DAL bus start function
 * @soc: pointer to DP SoC
 *
 * Called during cdp_soc_attach_target(), this function sync ring information
 * to the offload engine.
 *
 * Return: int
 */
int dp_dal_bus_start(struct dp_soc *soc);

/**
 * dp_dal_bus_request_irq() - DAL IRQ registration function
 * @soc: pointer to DP SoC
 *
 * Called during cdp_soc_attach_target(), this function sync IRQ info to OE,
 * OE will register Tx & Rx interrupts.
 *
 * Return: int
 */
int dp_dal_bus_request_irq(struct dp_soc *soc);

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
int dp_dal_bus_rx_buffer_enqueue(struct dp_soc *soc, uint32_t cnt);

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
int dp_dal_interface_add(struct dp_soc *soc, struct dp_vdev *vdev);

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
void dp_dal_interface_remove(struct dp_soc *soc, uint16_t vdev_id);

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
int dp_dal_sta_active(struct dp_soc *soc, struct sta_info *info, bool enable);

/**
 * dp_service_dal_srngs() - service DAL rings
 * @dp_ctx: dp intr context
 * @dp_budget: dp budget
 * @cpu: cpu
 *
 * Return: work done
 */
uint32_t dp_service_dal_srngs(void *dp_ctx, uint32_t dp_budget, int cpu);

static inline void
dp_srng_mark_dal_owned_ring(struct dp_srng *srng, uint8_t idx,
			    enum hal_ring_type type)
{
	int mask;

	if (type == REO_DST)
		mask = DAL_DP_REO_RING_MASK;
	else
		mask = DAL_DP_TCL_RING_MASK;

	if (BIT(idx) & mask)
		srng->dal_owned_ring = true;
}

static inline bool dp_srng_check_dal_owned_ring(struct dp_srng *srng)
{
	return srng->dal_owned_ring;
}

void dp_dal_save_srng_info(struct dp_soc *soc, struct dp_srng *srng,
			   enum hal_ring_type type, int ring_num);

int dp_dal_get_ext_grp_id(struct dp_dal_ctx *dal_ctx,
			  int ring_num, enum hal_ring_type type);
#else
#define DAL_DP_TCL_RING_MASK 0
#define DAL_DP_REO_RING_MASK 0

static inline void
dp_dal_save_srng_info(struct dp_soc *soc, struct dp_srng *srng,
		      enum hal_ring_type type, int ring_num)
{
}

static inline bool dp_srng_check_dal_owned_ring(struct dp_srng *srng)
{
	return false;
}

static inline void
dp_srng_mark_dal_owned_ring(struct dp_srng *srng, uint8_t idx,
			    enum hal_ring_type type)
{
}

static inline void dp_dal_soc_detach(struct dp_soc *soc)
{
}

static inline void dp_dal_soc_deinit(struct dp_soc *soc)
{
}

static inline QDF_STATUS dp_dal_soc_attach(struct dp_soc *soc)
{
	return QDF_STATUS_SUCCESS;
}

static inline QDF_STATUS dp_dal_soc_init(struct dp_soc *soc)
{
	return QDF_STATUS_SUCCESS;
}

static inline void dp_dal_bus_stop(struct dp_soc *soc)
{
}

static inline void dp_dal_bus_exit(struct dp_soc *soc)
{
}

static inline int dp_dal_bus_init(struct dp_soc *soc)
{
	return 0;
}

static inline int dp_dal_bus_start(struct dp_soc *soc)
{
	return 0;
}

static inline int dp_dal_bus_request_irq(struct dp_soc *soc)
{
	return 0;
}

static inline int dp_dal_bus_rx_buffer_enqueue(struct dp_soc *soc, uint32_t cnt)
{
	return 0;
}

static inline int
dp_dal_interface_add(struct dp_soc *soc, struct dp_vdev *vdev)
{
	return 0;
}

static inline void dp_dal_interface_remove(struct dp_soc *soc, uint16_t vdev_id)
{
}
#endif /* FEATURE_DAL_DP_SUPPORT */
#endif /* DP_DAL_H */
