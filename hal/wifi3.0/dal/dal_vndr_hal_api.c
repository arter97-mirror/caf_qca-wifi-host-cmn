/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: ISC
 */

#include "dal_vndr_hal_api.h"
#include "dal_vndr_hal_internal.h"
#include "dal_vndr_hal_be.h"
#include "dal_vndr_hal_defines_be.h"
#include "dal_vndr_hal_bn.h"

#define DAL_VNDR_HAL_WBM2SW_RELEASE_SRC_GET(wbm_desc) (((*(((uint32_t *)wbm_desc) + \
		(WBM_RELEASE_RING_TX_RELEASE_SOURCE_MODULE_OFFSET >> 2))) & \
		 WBM_RELEASE_RING_TX_RELEASE_SOURCE_MODULE_MASK) >> \
		 WBM_RELEASE_RING_TX_RELEASE_SOURCE_MODULE_LSB)

/* sw_cookie is 20-bit field, MSB is bit 19 */
#define DAL_VNDR_HAL_TX_DESC_COOKIE_MSB_BIT	\
	(TCL_DATA_CMD_BUF_ADDR_INFO_SW_BUFFER_COOKIE_MSB - \
		TCL_DATA_CMD_BUF_ADDR_INFO_SW_BUFFER_COOKIE_LSB)
#define DAL_VNDR_HAL_TX_DESC_COOKIE_MSB_MASK \
	(1 << DAL_VNDR_HAL_TX_DESC_COOKIE_MSB_BIT)

/* Macro to set MSB bit of sw_cookie for buffer tracking */
#define DAL_VNDR_HAL_TX_DESC_COOKIE_SET_MSB(cookie) \
	((cookie) | DAL_VNDR_HAL_TX_DESC_COOKIE_MSB_MASK)

/* Macro to check if MSB bit is set in sw_cookie */
#define DAL_VNDR_HAL_TX_COMP_COOKIE_MSB_IS_SET(cookie) \
	((cookie) & DAL_VNDR_HAL_TX_DESC_COOKIE_MSB_MASK)

/**
 * dal_vndr_hal_tx_comp_get_desc_id_generic() - Get descriptor ID from Tx
 * completion descriptor
 * @hal_desc: Tx completion descriptor pointer
 *
 * This function extracts the descriptor ID (sw_buffer_cookie) from the
 * hardware completion descriptor.
 *
 * Return: Descriptor ID
 */
static inline uint32_t dal_vndr_hal_tx_comp_get_desc_id_generic(void *hal_desc)
{
	uint32_t comp_desc =
			*(uint32_t *)(((uint8_t *)hal_desc) +
				BUFFER_ADDR_INFO_SW_BUFFER_COOKIE_OFFSET);

	/* Cookie is placed on 2nd word */
	return (comp_desc & BUFFER_ADDR_INFO_SW_BUFFER_COOKIE_MASK) >>
			BUFFER_ADDR_INFO_SW_BUFFER_COOKIE_LSB;
}

/**
 * dal_vndr_hal_tx_comp_get_buffer_source_generic() - Get buffer release source
 * from Tx completion descriptor
 * @hal_desc: Tx completion descriptor pointer
 *
 * This function extracts the buffer release source module from the
 * hardware completion descriptor.
 *
 * Return: Buffer release source module
 */
static inline uint32_t dal_vndr_hal_tx_comp_get_buffer_source_generic(
	void *hal_desc)
{
	return DAL_VNDR_HAL_WBM2SW_RELEASE_SRC_GET(hal_desc);
}

/**
 * dal_vndr_hal_tx_comp_get_tx_status_generic() - Get Tx transmission status
 * @hal_desc: Tx completion descriptor pointer
 *
 * This function extracts the transmit status value from the Tx completion
 * descriptor (TQM release reason).
 *
 * Return: Transmit status value
 */
static inline uint8_t dal_vndr_hal_tx_comp_get_tx_status_generic(void *hal_desc)
{
	uint32_t comp_desc =
		*(uint32_t *)(((uint8_t *)hal_desc) +
			WBM_RELEASE_RING_TX_TQM_RELEASE_REASON_OFFSET);

	return (comp_desc & WBM_RELEASE_RING_TX_TQM_RELEASE_REASON_MASK) >>
			WBM_RELEASE_RING_TX_TQM_RELEASE_REASON_LSB;
}

/**
 * dal_vndr_hal_attach_common_ops() - Attach common HAL operations
 * @hal_soc: HAL SoC handle
 *
 * This function attaches common HAL operations that are shared across
 * all target types.
 *
 * Return: void
 */
static void dal_vndr_hal_attach_common_ops(struct dal_vndr_hal_soc *hal_soc)
{
	if (!hal_soc || !hal_soc->ops)
		return;

	/* Attach common ops that are target-independent */
	hal_soc->ops->dal_vndr_hal_tx_comp_get_desc_id =
				dal_vndr_hal_tx_comp_get_desc_id_generic;
	hal_soc->ops->dal_vndr_hal_tx_comp_get_buffer_source =
				dal_vndr_hal_tx_comp_get_buffer_source_generic;
	hal_soc->ops->dal_vndr_hal_tx_comp_get_tx_status =
				dal_vndr_hal_tx_comp_get_tx_status_generic;
}

/**
 * dal_vndr_hal_ops_attach() - Attach HAL operations
 * @hal_soc: HAL SoC handle
 *
 * This function attaches the default HAL operations.
 *
 * Return: void
 */
#if defined(CONFIG_BORON) && defined(DAL_VNDR_HAL_BN)
void dal_vndr_hal_ops_attach(void *hal_soc)
{
	struct dal_vndr_hal_soc *hal_soc_hdl =
		(struct dal_vndr_hal_soc *)hal_soc;

	if (!hal_soc_hdl)
		return;
	/* BORON-specific ops attach path */
	dal_vndr_hal_default_ops_attach_bn(hal_soc_hdl);
}

#else /* Non-BORON (default/BE) */
void dal_vndr_hal_ops_attach(void *hal_soc)
{
	struct dal_vndr_hal_soc *hal_soc_hdl =
		(struct dal_vndr_hal_soc *)hal_soc;

	if (!hal_soc_hdl)
		return;

	/* Attach common ops */
	dal_vndr_hal_attach_common_ops(hal_soc_hdl);
	/*
	 * Currently directly calling BE ops attach
	 * In future, this will be based on target type
	 * and call appropriate ops attach function
	 * based on target type
	 * For now, only BE is supported, so directly calling
	 * BE ops attach function
	 */
	dal_vndr_hal_default_ops_attach_be(hal_soc_hdl);
}
#endif

