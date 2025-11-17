/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: ISC
 */
#include "dal_vndr_hal_be.h"
#include "dal_vndr_hal_defines_be.h"
#include "dal_vndr_hal_internal.h"

/**
 * dal_vndr_hal_tx_desc_set_buf_addr_be() - Fill Buffer Address information
 * in Tx Desc
 * @desc: Handle to Tx Descriptor
 * @paddr: Physical Address
 * @rbm_id: Return Buffer Manager ID
 * @desc_id: Descriptor ID
 * @type: 0 - Address points to a MSDU buffer
 *        1 - Address points to MSDU extension descriptor
 *
 * Return: None
 */
static inline void
dal_vndr_hal_tx_desc_set_buf_addr_be(void *desc,
				     dma_addr_t paddr, uint8_t rbm_id,
				     uint32_t desc_id, uint8_t type)
{
	/* Set buffer_addr_info.buffer_addr_31_0 */
	HAL_SET_FLD(desc, TCL_DATA_CMD,
		    BUF_ADDR_INFO_BUFFER_ADDR_31_0) =
		HAL_TX_SM(TCL_DATA_CMD, BUF_ADDR_INFO_BUFFER_ADDR_31_0, paddr);

	/* Set buffer_addr_info.buffer_addr_39_32 */
	HAL_SET_FLD(desc, TCL_DATA_CMD,
		    BUF_ADDR_INFO_BUFFER_ADDR_39_32) |=
		HAL_TX_SM(TCL_DATA_CMD, BUF_ADDR_INFO_BUFFER_ADDR_39_32,
			  (((uint64_t)paddr) >> 32));

	/* Set buffer_addr_info.return_buffer_manager = rbm id */
	HAL_SET_FLD(desc, TCL_DATA_CMD,
		    BUF_ADDR_INFO_RETURN_BUFFER_MANAGER) |=
		HAL_TX_SM(TCL_DATA_CMD,
			  BUF_ADDR_INFO_RETURN_BUFFER_MANAGER, rbm_id);

	/* Set buffer_addr_info.sw_buffer_cookie = desc_id */
	HAL_SET_FLD(desc, TCL_DATA_CMD,
		    BUF_ADDR_INFO_SW_BUFFER_COOKIE) |=
		HAL_TX_SM(TCL_DATA_CMD, BUF_ADDR_INFO_SW_BUFFER_COOKIE,
			  desc_id);

	/* Set  Buffer or Ext Descriptor Type */
	HAL_SET_FLD(desc, TCL_DATA_CMD,
		    BUF_OR_EXT_DESC_TYPE) |=
		HAL_TX_SM(TCL_DATA_CMD, BUF_OR_EXT_DESC_TYPE, type);
}

/**
 * dal_vndr_hal_tx_desc_set_lmac_id_be- set lmac id to the descriptor
 * @desc: Cached descriptor that software maintains
 * @lmac_id: lmac id
 */
static inline void
dal_vndr_hal_tx_desc_set_lmac_id_be(void *desc, uint8_t lmac_id)
{
	HAL_SET_FLD(desc, TCL_DATA_CMD, PMAC_ID) |=
		HAL_TX_SM(TCL_DATA_CMD, PMAC_ID, lmac_id);
}

/**
 * dal_vndr_hal_tx_desc_set_buf_length_be() - Set Data length in bytes in tx
 * descriptor
 * @desc: Handle to Tx Descriptor
 * @data_length: MSDU length in case of direct descriptor.
 * Length of link extension descriptor in case of Link extension
 * descriptor.Includes the length of Metadata
 * Return: None
 */
static inline void  dal_vndr_hal_tx_desc_set_buf_length_be(void *desc,
							   uint16_t data_length)
{
	HAL_SET_FLD(desc, TCL_DATA_CMD, DATA_LENGTH) |=
		HAL_TX_SM(TCL_DATA_CMD, DATA_LENGTH, data_length);
}

/**
 * dal_vndr_hal_tx_desc_set_buf_offset_be() - Sets Packet Offset field in Tx
 * descriptor
 * @desc: Handle to Tx Descriptor
 * @offset: Packet offset from Metadata in case of direct buffer descriptor.
 *
 * Return: void
 */
static inline void dal_vndr_hal_tx_desc_set_buf_offset_be(void *desc,
							  uint8_t offset)
{
	HAL_SET_FLD(desc, TCL_DATA_CMD, PACKET_OFFSET) |=
		HAL_TX_SM(TCL_DATA_CMD, PACKET_OFFSET, offset);
}

/**
 * dal_vndr_hal_tx_desc_set_l3_checksum_en_be() -  Set IPv4 checksum enable flag
 * in  Tx Descriptor for MSDU_buffer type
 * @desc: Handle to Tx Descriptor
 * @en: ipv4 checksum enable flags
 *
 * Return: void
 */
static inline void dal_vndr_hal_tx_desc_set_l3_checksum_en_be(void *desc,
							      uint8_t en)
{
	HAL_SET_FLD(desc, TCL_DATA_CMD, IPV4_CHECKSUM_EN) |=
		HAL_TX_SM(TCL_DATA_CMD, IPV4_CHECKSUM_EN, en);
}

/**
 * dal_vndr_hal_tx_desc_set_l4_checksum_en_be() - Set TCP/IP checksum enable
 * flags in Tx Descriptor for MSDU_buffer type
 * @desc: Handle to Tx Descriptor
 * @en: UDP/TCP over ipv4/ipv6 checksum enable flags (5 bits)
 *
 * Return: void
 */
static inline void dal_vndr_hal_tx_desc_set_l4_checksum_en_be(void *desc,
							      uint8_t en)
{
	HAL_SET_FLD(desc, TCL_DATA_CMD, IPV4_CHECKSUM_EN) |=
		(HAL_TX_SM(TCL_DATA_CMD, UDP_OVER_IPV4_CHECKSUM_EN, en) |
		 HAL_TX_SM(TCL_DATA_CMD, UDP_OVER_IPV6_CHECKSUM_EN, en) |
		 HAL_TX_SM(TCL_DATA_CMD, TCP_OVER_IPV4_CHECKSUM_EN, en) |
		 HAL_TX_SM(TCL_DATA_CMD, TCP_OVER_IPV6_CHECKSUM_EN, en));
}

/**
 * dal_vndr_hal_tx_desc_set_bank_id_be() - set bank id to the descriptor
 * @desc: Cached descriptor that software maintains
 * @bank_id: bank id
 */
static inline void dal_vndr_hal_tx_desc_set_bank_id_be(void *desc,
						       uint8_t bank_id)
{
	HAL_SET_FLD(desc, TCL_DATA_CMD, BANK_ID) |=
		HAL_TX_SM(TCL_DATA_CMD, BANK_ID, bank_id);
}

/**
 * dal_vndr_hal_tx_desc_set_vdev_id_be() - set vdev id to the tx  descriptor
 * @desc: Cached descriptor that software maintains
 * @vdev_id: vdev id
 */
static inline void dal_vndr_hal_tx_desc_set_vdev_id_be(void *desc,
						       uint8_t vdev_id)
{
	HAL_SET_FLD(desc, TCL_DATA_CMD, VDEV_ID) |=
		HAL_TX_SM(TCL_DATA_CMD, VDEV_ID, vdev_id);
}

/**
 * dal_vndr_hal_tx_desc_set_hlos_tid_be() - Set the TID value
 * (override DSCP/PCP fields in frame) to be used for Tx Frame
 * @desc: Handle to Tx Descriptor
 * @hlos_tid: HLOS TID
 *
 * Return: void
 */
static inline void dal_vndr_hal_tx_desc_set_hlos_tid_be(void *desc,
							uint8_t hlos_tid)
{
	HAL_SET_FLD(desc, TCL_DATA_CMD, HLOS_TID) |=
		HAL_TX_SM(TCL_DATA_CMD, HLOS_TID, hlos_tid);

	HAL_SET_FLD(desc, TCL_DATA_CMD, HLOS_TID_OVERWRITE) |=
	   HAL_TX_SM(TCL_DATA_CMD, HLOS_TID_OVERWRITE, 1);
}

/**
 * dal_vndr_hal_tx_desc_set_flow_override_enable_be() - Set flow_override_enable
 * @desc: Handle to Tx Descriptor
 * @val: Value to be set
 *
 * Return: None
 */
static inline void  dal_vndr_hal_tx_desc_set_flow_override_enable_be(
								void *desc,
								uint8_t val)
{
	HAL_SET_FLD(desc, TCL_DATA_CMD, FLOW_OVERRIDE_ENABLE) |=
		HAL_TX_SM(TCL_DATA_CMD, FLOW_OVERRIDE_ENABLE, val);
}

/**
 * dal_vndr_hal_tx_desc_set_flow_override_be() - Set flow_override field in
 * TX desc
 * @desc: Handle to Tx Descriptor
 * @val: Value to be set
 *
 * Return: None
 */
static inline void  dal_vndr_hal_tx_desc_set_flow_override_be(void *desc,
							      uint8_t val)
{
	HAL_SET_FLD(desc, TCL_DATA_CMD, FLOW_OVERRIDE) |=
		HAL_TX_SM(TCL_DATA_CMD, FLOW_OVERRIDE, val);
}

/**
 * dal_vndr_hal_tx_desc_set_who_classify_info_sel_be() - Set
 * who_classify_info_sel field
 * @desc: Handle to Tx Descriptor
 * @val: Value to be set
 *
 * Return: None
 */
static inline void  dal_vndr_hal_tx_desc_set_who_classify_info_sel_be(
							void *desc,
							uint8_t val)
{
	HAL_SET_FLD(desc, TCL_DATA_CMD, WHO_CLASSIFY_INFO_SEL) |=
		HAL_TX_SM(TCL_DATA_CMD, WHO_CLASSIFY_INFO_SEL, val);
}

/**
 * dal_vndr_hal_tx_desc_set_tx_notify_frame_be() - Set TX notify_frame
 * field in Tx desc
 * @desc: Handle to Tx Descriptor
 * @val: Value to be set
 *
 * Return: None
 */
static inline void dal_vndr_hal_tx_desc_set_tx_notify_frame_be(void *desc,
							       uint8_t val)
{
	HAL_SET_FLD(desc, TCL_DATA_CMD, TX_NOTIFY_FRAME) |=
		HAL_TX_SM(TCL_DATA_CMD, TX_NOTIFY_FRAME, val);
}

/**
 * dal_vndr_hal_tx_comp_get_status_generic_be - Get generic tx completion status
 * @desc: WBM descriptor
 * @ts1: completion ring Tx status
 *
 * This function will parse the WBM completion descriptor and populate in
 * HAL structure
 *
 * Return: none
 */
static inline void
dal_vndr_hal_tx_comp_get_status_generic_be(void *desc, void *ts1)
{
	struct dal_vndr_hal_tx_completion_status *ts =
		(struct dal_vndr_hal_tx_completion_status *)ts1;

	*ts = *((struct dal_vndr_hal_tx_completion_status *)desc);

	ts->msdu_part_of_amsdu = (ts->first_msdu && ts->last_msdu) ?
				  false : true;
	ts->tid = ts->tid & 0xF;
}

/**
 * hal_tx_comp_get_paddr_be() - Get paddr within comp descriptor
 * @hal_desc: completion ring descriptor pointer
 *
 * This function will get buffer physical address within hardware completion
 * descriptor
 *
 * Return: Buffer physical address
 */
static inline dma_addr_t hal_tx_comp_get_paddr_be(void *hal_desc)
{
	uint32_t paddr_lo;
	uint32_t paddr_hi;

	paddr_lo = *(uint32_t *)(((uint8_t *)hal_desc) +
			BUFFER_ADDR_INFO_BUFFER_ADDR_31_0_OFFSET);

	paddr_hi = *(uint32_t *)(((uint8_t *)hal_desc) +
			BUFFER_ADDR_INFO_BUFFER_ADDR_39_32_OFFSET);

	paddr_hi = (paddr_hi & BUFFER_ADDR_INFO_BUFFER_ADDR_39_32_MASK) >>
		BUFFER_ADDR_INFO_BUFFER_ADDR_39_32_LSB;

	return (dma_addr_t)(paddr_lo | (((uint64_t)paddr_hi) << 32));
}

void dal_vndr_hal_default_ops_attach_be(struct dal_vndr_hal_soc *hal_soc)
{
	hal_soc->ops->dal_vndr_hal_tx_desc_set_lmac_id =
				dal_vndr_hal_tx_desc_set_lmac_id_be;
	hal_soc->ops->dal_vndr_hal_tx_desc_set_buf_addr =
				dal_vndr_hal_tx_desc_set_buf_addr_be;
	hal_soc->ops->dal_vndr_hal_tx_desc_set_buf_length =
				dal_vndr_hal_tx_desc_set_buf_length_be;
	hal_soc->ops->dal_vndr_hal_tx_desc_set_buf_offset =
				dal_vndr_hal_tx_desc_set_buf_offset_be;
	hal_soc->ops->dal_vndr_hal_tx_desc_set_l3_checksum_en =
				dal_vndr_hal_tx_desc_set_l3_checksum_en_be;
	hal_soc->ops->dal_vndr_hal_tx_desc_set_l4_checksum_en =
				dal_vndr_hal_tx_desc_set_l4_checksum_en_be;
	hal_soc->ops->dal_vndr_hal_tx_desc_set_bank_id =
				dal_vndr_hal_tx_desc_set_bank_id_be;
	hal_soc->ops->dal_vndr_hal_tx_desc_set_vdev_id =
				dal_vndr_hal_tx_desc_set_vdev_id_be;
	hal_soc->ops->dal_vndr_hal_tx_desc_set_hlos_tid =
				dal_vndr_hal_tx_desc_set_hlos_tid_be;
	hal_soc->ops->dal_vndr_hal_tx_desc_set_flow_override_enable =
			dal_vndr_hal_tx_desc_set_flow_override_enable_be;
	hal_soc->ops->dal_vndr_hal_tx_desc_set_flow_override =
				dal_vndr_hal_tx_desc_set_flow_override_be;
	hal_soc->ops->dal_vndr_hal_tx_desc_set_who_classify_info_sel =
			dal_vndr_hal_tx_desc_set_who_classify_info_sel_be;
	hal_soc->ops->dal_vndr_hal_tx_desc_set_tx_notify_frame =
				dal_vndr_hal_tx_desc_set_tx_notify_frame_be;
	hal_soc->ops->dal_vndr_hal_tx_comp_get_status =
				dal_vndr_hal_tx_comp_get_status_generic_be;
	hal_soc->ops->dal_vndr_hal_tx_comp_get_paddr =
				hal_tx_comp_get_paddr_be;
}
