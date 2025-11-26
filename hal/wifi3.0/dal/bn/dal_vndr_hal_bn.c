// SPDX-License-Identifier: ISC
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */
#include "dal_vndr_hal_bn.h"
#include "dal_vndr_hal_be.h"
#include "dal_vndr_hal_defines_be.h"
#include "dal_vndr_hal_defines_bn.h"
#include "dal_vndr_hal_internal.h"

/**
 * dal_vndr_hal_tx_desc_set_buf_addr_bn() - Fill Buffer Address information
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
dal_vndr_hal_tx_desc_set_buf_addr_bn(void *desc, dma_addr_t paddr,
				     uint8_t rbm_id, uint32_t desc_id,
				     uint8_t type)
{
	/* Set buffer_addr_info.buffer_addr_31_0 */
	HAL_SET_FLD(desc, TCL_ASSIST_CMD, BUF_ADDR_INFO_BUFFER_ADDR_31_0) =
	      HAL_TX_SM(TCL_ASSIST_CMD, BUF_ADDR_INFO_BUFFER_ADDR_31_0, paddr);

	/* Set buffer_addr_info.buffer_addr_39_32 */
	HAL_SET_FLD(desc, TCL_ASSIST_CMD, BUF_ADDR_INFO_BUFFER_ADDR_39_32) |=
		HAL_TX_SM(TCL_ASSIST_CMD, BUF_ADDR_INFO_BUFFER_ADDR_39_32,
			  (((uint64_t)paddr) >> 32));

	/* Set buffer_addr_info.return_buffer_manager = rbm id */
	HAL_SET_FLD(desc, TCL_ASSIST_CMD,
		    BUF_ADDR_INFO_RETURN_BUFFER_MANAGER) |=
			HAL_TX_SM(TCL_ASSIST_CMD,
				  BUF_ADDR_INFO_RETURN_BUFFER_MANAGER, rbm_id);

	/* Set buffer_addr_info.sw_buffer_cookie = desc_id */
	HAL_SET_FLD(desc, TCL_ASSIST_CMD, BUF_ADDR_INFO_SW_BUFFER_COOKIE) |=
	    HAL_TX_SM(TCL_ASSIST_CMD, BUF_ADDR_INFO_SW_BUFFER_COOKIE, desc_id);

	/* Set  Buffer or Ext Descriptor Type */
	HAL_SET_FLD(desc, TCL_ASSIST_CMD, BUF_OR_EXT_DESC_TYPE) |=
		HAL_TX_SM(TCL_ASSIST_CMD, BUF_OR_EXT_DESC_TYPE, type);
}

/**
 * dal_vndr_hal_tx_desc_set_buf_length_bn() - Set Data length in bytes
 * in Tx Descriptor
 * @desc: Handle to Tx Descriptor
 * @data_length: MSDU length in case of direct descriptor.
 *              Length of link extension descriptor in case of Link extension
 *              descriptor.Includes the length of Metadata
 * Return: None
 */
static inline void dal_vndr_hal_tx_desc_set_buf_length_bn(void *desc,
							  uint16_t data_length)
{
	/* TODO: add generic macro which can be used for other fields */
	if (qdf_unlikely(data_length & ~(TCL_ASSIST_CMD_DATA_LENGTH_MASK >>
					TCL_ASSIST_CMD_DATA_LENGTH_LSB))) {
		hal_err("data_length overflow %d", data_length);
		qdf_assert_always(0);
	}
	HAL_SET_FLD(desc, TCL_ASSIST_CMD, DATA_LENGTH) |=
		HAL_TX_SM(TCL_ASSIST_CMD, DATA_LENGTH, data_length);
}

/**
 * dal_vndr_hal_tx_desc_set_buf_offset_bn() - Sets Packet Offset field in
 * Tx descriptor
 * @desc: Handle to Tx Descriptor
 * @offset: Packet offset from Metadata in case of direct buffer descriptor.
 *
 * Return: void
 */
static inline void dal_vndr_hal_tx_desc_set_buf_offset_bn(void *desc,
							  uint8_t offset)
{
	HAL_SET_FLD(desc, TCL_ASSIST_CMD, METADATA_LENGTH) |=
		HAL_TX_SM(TCL_ASSIST_CMD, METADATA_LENGTH, offset);
}

/**
 * dal_vndr_hal_tx_desc_set_l3_checksum_en_bn() -  Set IPv4 checksum enable
 * flag in Tx Descriptor for MSDU_buffer type
 * @desc: Handle to Tx Descriptor
 * @en: ipv4 checksum enable flags
 *
 * Return: void
 */
static inline void dal_vndr_hal_tx_desc_set_l3_checksum_en_bn(void *desc,
							      uint8_t en)
{
	HAL_SET_FLD(desc, TCL_ASSIST_CMD, L3_CHECKSUM_ENABLE) |=
		HAL_TX_SM(TCL_ASSIST_CMD, L3_CHECKSUM_ENABLE, en);
}

/**
 * dal_vndr_hal_tx_desc_set_l4_checksum_en_bn() -  Set TCP/IP checksum enable
 * flags Tx Descriptor for MSDU_buffer type
 * @desc: Handle to Tx Descriptor
 * @en: UDP/TCP over ipv4/ipv6 checksum enable flags (5 bits)
 *
 * Return: void
 */
static inline void dal_vndr_hal_tx_desc_set_l4_checksum_en_bn(void *desc,
							      uint8_t en)
{
	HAL_SET_FLD(desc, TCL_ASSIST_CMD, L4_CHECKSUM_ENABLE) |=
		 HAL_TX_SM(TCL_ASSIST_CMD, L4_CHECKSUM_ENABLE, en);
}

/**
 * dal_vndr_hal_tx_desc_set_bank_id_bn() - set bank id to the descriptor
 * to Hardware
 * @desc: Cached descriptor that software maintains
 * @bank_id: bank id
 */
static inline void dal_vndr_hal_tx_desc_set_bank_id_bn(void *desc,
						       uint8_t bank_id)
{
	HAL_SET_FLD(desc, TCL_ASSIST_CMD, BANK_ID) |=
		HAL_TX_SM(TCL_ASSIST_CMD, BANK_ID, bank_id);
}

/**
 * dal_vndr_hal_tx_desc_set_fw_metadata_bn() - Sets the metadata that is part
 * of TCL descriptor
 * @desc: Handle to Tx Descriptor
 * @metadata: Metadata to be sent to Firmware
 *
 * Return: void
 */
static inline void dal_vndr_hal_tx_desc_set_fw_metadata_bn(void *desc,
							   uint16_t metadata)
{
	HAL_SET_FLD(desc, TCL_ASSIST_CMD, TCL_CMD_NUMBER) |=
		HAL_TX_SM(TCL_ASSIST_CMD, TCL_CMD_NUMBER, metadata);
}

/**
 * dal_vndr_hal_tx_desc_set_vdev_id_bn() - set vdev id to the desc to Hardware
 * @desc: Cached descriptor that software maintains
 * @vdev_id: vdev id
 */
static inline void dal_vndr_hal_tx_desc_set_vdev_id_bn(void *desc,
						       uint8_t vdev_id)
{
	HAL_SET_FLD(desc, TCL_ASSIST_CMD, VDEV_ID) |=
		HAL_TX_SM(TCL_ASSIST_CMD, VDEV_ID, vdev_id);
}

/**
 * dal_vndr_hal_tx_desc_set_hlos_tid_bn() - Set the TID value (override
 * DSCP/PCP fields in frame) to be used for Tx Frame
 * @desc: Handle to Tx Descriptor
 * @hlos_tid: HLOS TID
 *
 * Return: void
 */
static inline void dal_vndr_hal_tx_desc_set_hlos_tid_bn(void *desc,
							uint8_t hlos_tid)
{
	HAL_SET_FLD(desc, TCL_ASSIST_CMD, HLOS_TID) |=
		HAL_TX_SM(TCL_ASSIST_CMD, HLOS_TID, hlos_tid);

	HAL_SET_FLD(desc, TCL_ASSIST_CMD, HLOS_TID_OVERWRITE) |=
	   HAL_TX_SM(TCL_ASSIST_CMD, HLOS_TID_OVERWRITE, 1);
}

/**
 * dal_vndr_hal_tx_desc_set_tx_notify_frame_bn() - Set TX notify_frame field
 * in Tx desc
 * @desc: Handle to Tx Descriptor
 * @val: Value to be set
 *
 * Return: None
 */
static inline void dal_vndr_hal_tx_desc_set_tx_notify_frame_bn(void *desc,
							       uint8_t val)
{
	HAL_SET_FLD(desc, TCL_ASSIST_CMD, TX_NOTIFY_FRAME) |=
		HAL_TX_SM(TCL_ASSIST_CMD, TX_NOTIFY_FRAME, val);
}

/**
 * dal_vndr_hal_tx_desc_set_to_fw_bn() - Set To_FW bit in Tx Descriptor.
 * @desc: Handle to Tx Descriptor
 * @to_fw: if set, Forward packet to FW along with classification result
 *
 * Return: void
 */
static inline void dal_vndr_hal_tx_desc_set_to_fw_bn(void *desc, uint8_t to_fw)
{
	HAL_SET_FLD(desc, TCL_ASSIST_CMD, TO_FW_TQM) |=
		HAL_TX_SM(TCL_ASSIST_CMD, TO_FW_TQM, to_fw);
}

/**
 * dal_vndr_hal_tx_desc_set_peer_txpt_ci_index_bn() - Set txpt_classify_info
 *
 * @desc: Handle to Tx Descriptor
 * @peer_txpt_ci_index: peer txpt_classify_info index
 *
 * Return: void
 */
static inline void
dal_vndr_hal_tx_desc_set_peer_txpt_ci_index_bn(void *desc,
					       uint8_t peer_txpt_ci_index)
{
	HAL_SET_FLD(desc, TCL_ASSIST_CMD, TXPT_CLASSIFY_INFO_INDEX) |=
		HAL_TX_SM(TCL_ASSIST_CMD, TXPT_CLASSIFY_INFO_INDEX,
			  peer_txpt_ci_index);
}

/**
 * dal_vndr_hal_tx_desc_set_peer_txpt_ci_tos_tc_val_bn() - Program dscp value
 * to select TID based txpt_ci entry selection
 *
 * @desc: Handle to Tx Descriptor
 * @tos_tc_val: DSCP value
 *
 * Return: void
 */
static inline void
dal_vndr_hal_tx_desc_set_peer_txpt_ci_tos_tc_val_bn(void *desc,
						    uint8_t tos_tc_val)
{
	HAL_SET_FLD(desc, TCL_ASSIST_CMD, TOS_TC_VALUE) |=
		HAL_TX_SM(TCL_ASSIST_CMD, TOS_TC_VALUE, tos_tc_val);
}

/**
 * dal_vndr_hal_tx_desc_set_da_is_bcast_mcast_bn() - Set da is bcast or mcast
 *
 * @desc: Handle to Tx Descriptor
 * @is_bcast: is broadcast
 * @is_mcast: is multicast
 *
 * Return: void
 */
static inline void
dal_vndr_hal_tx_desc_set_da_is_bcast_mcast_bn(void *desc, uint8_t is_bcast,
					      uint8_t is_mcast)
{
	/* It is more optimal to do this check at caller, but this is
	 * HAL specific, so checking here.
	 */
	if (qdf_unlikely(is_bcast || is_mcast))
		HAL_SET_FLD(desc, TCL_ASSIST_CMD, DA_IS_BCAST_MCAST) |=
			HAL_TX_SM(TCL_ASSIST_CMD, DA_IS_BCAST_MCAST, 1);

	HAL_SET_FLD(desc, TCL_ASSIST_CMD, DA_IS_BCAST) |=
		HAL_TX_SM(TCL_ASSIST_CMD, DA_IS_BCAST, is_bcast);
}

/**
 * dal_vndr_hal_tx_desc_set_l3_type_bn() - Set l3_type, IPV4/IPV6...
 *
 * @desc: Handle to Tx Descriptor
 * @l3_type: L3 type
 *
 * Return: void
 */
static inline void dal_vndr_hal_tx_desc_set_l3_type_bn(void *desc,
						       uint16_t l3_type)
{
	HAL_SET_FLD(desc, TCL_ASSIST_CMD, L3_TYPE) |=
		HAL_TX_SM(TCL_ASSIST_CMD, L3_TYPE, l3_type);
}

/**
 * dal_vndr_hal_tx_desc_set_l4_protocol_bn() - Set l4_protocol
 *
 * @desc: Handle to Tx Descriptor
 * @l4_protocol: l4_protocol type
 *
 * Return: void
 */
static inline void dal_vndr_hal_tx_desc_set_l4_protocol_bn(void *desc,
							   uint8_t l4_protocol)
{
	HAL_SET_FLD(desc, TCL_ASSIST_CMD, L4_PROTOCOL) |=
		HAL_TX_SM(TCL_ASSIST_CMD, L4_PROTOCOL, l4_protocol);
}

/**
 * dal_vndr_hal_tx_desc_set_type_or_length_bn() - Set type_or_length
 *
 * @desc: Handle to Tx Descriptor
 * @type_or_length: type_or_length
 *
 * Return: void
 */
static inline void
dal_vndr_hal_tx_desc_set_type_or_length_bn(void *desc, uint8_t type_or_length)
{
	HAL_SET_FLD(desc, TCL_ASSIST_CMD, TYPE_OR_LENGTH) |=
		HAL_TX_SM(TCL_ASSIST_CMD, TYPE_OR_LENGTH, type_or_length);
}

/**
 * dal_vndr_hal_tx_desc_set_dport_bn() - Set dport in Tx Descriptor.
 * @desc: Handle to Tx Descriptor
 * @l4_port: port
 *
 * Return: void
 */
static inline void dal_vndr_hal_tx_desc_set_dport_bn(void *desc,
						     uint16_t l4_port)
{
	HAL_SET_FLD(desc, TCL_ASSIST_CMD, L4_PORT) |=
		HAL_TX_SM(TCL_ASSIST_CMD, L4_PORT, l4_port);

	HAL_SET_FLD(desc, TCL_ASSIST_CMD, L4_PORT_TYPE) |=
		HAL_TX_SM(TCL_ASSIST_CMD, L4_PORT_TYPE, 1);
}

/**
 * dal_vndr_hal_tx_desc_set_snap_oui_zero_or_f8_bn() - Set snap_oui_zero_or_f8
 *
 * @desc: Handle to Tx Descriptor
 * @is_snap_oui_zero_or_f8: is snap_oui_zero_or_f8 present
 *
 * Return: void
 */
static inline void
dal_vndr_hal_tx_desc_set_snap_oui_zero_or_f8_bn(void *desc,
						uint8_t is_snap_oui_zero_or_f8)
{
	HAL_SET_FLD(desc, TCL_ASSIST_CMD, SNAP_OUI_ZERO_OR_F8) |=
	HAL_TX_SM(TCL_ASSIST_CMD, SNAP_OUI_ZERO_OR_F8, is_snap_oui_zero_or_f8);
}

/**
 * dal_vndr_hal_tx_desc_set_snap_oui_not_zero_or_not_f8_bn() - Set
 *						snap_oui_not_zero_or_not_f8
 *
 * @desc: Handle to Tx Descriptor
 * @is_snap_oui_not_zero_or_not_f8: is snap_oui_not_zero_or_f8 present
 *
 * Return: void
 */
static inline void
dal_vndr_hal_tx_desc_set_snap_oui_not_zero_or_not_f8_bn(void *desc,
							uint8_t is_snap_oui_not_zero_or_not_f8)
{
	HAL_SET_FLD(desc, TCL_ASSIST_CMD, SNAP_OUI_NOT_ZERO_AND_NOT_F8) |=
	 HAL_TX_SM(TCL_ASSIST_CMD, SNAP_OUI_NOT_ZERO_AND_NOT_F8,
		   is_snap_oui_not_zero_or_not_f8);
}

/**
 * dal_vndr_hal_tx_desc_set_s_vlan_tag_bn() - Set S_VLAN_TAG_PRESENT
 *
 * @desc: Handle to Tx Descriptor
 * @s_vlan_present: is s_vlan present
 *
 * Return: void
 */
static inline void
dal_vndr_hal_tx_desc_set_s_vlan_tag_bn(void *desc, uint8_t s_vlan_present)
{
	HAL_SET_FLD(desc, TCL_ASSIST_CMD, S_VLAN_TAG_PRESENT) |=
		HAL_TX_SM(TCL_ASSIST_CMD, S_VLAN_TAG_PRESENT, s_vlan_present);
}

/**
 * dal_vndr_hal_tx_desc_set_c_vlan_tag_bn() - Set C_VLAN_TAG_PRESENT
 *
 * @desc: Handle to Tx Descriptor
 * @c_vlan_present: is c_vlan present
 *
 * Return: void
 */
static inline void
dal_vndr_hal_tx_desc_set_c_vlan_tag_bn(void *desc, uint8_t c_vlan_present)
{
	HAL_SET_FLD(desc, TCL_ASSIST_CMD, C_VLAN_TAG_PRESENT) |=
		HAL_TX_SM(TCL_ASSIST_CMD, C_VLAN_TAG_PRESENT, c_vlan_present);
}

/**
 * dal_vndr_hal_tx_comp_get_release_reason_generic_bn() - TQM Release reason
 * @hal_desc: completion ring descriptor pointer
 *
 * This function will return the type of pointer - buffer or descriptor
 *
 * Return: buffer type
 */
static uint8_t
dal_vndr_hal_tx_comp_get_release_reason_generic_bn(void *hal_desc)
{
	uint32_t comp_desc = *(uint32_t *)(((uint8_t *)hal_desc) +
			TQM2SW_COMPLETION_RING_TQM_RELEASE_REASON_OFFSET);

	return (comp_desc &
		TQM2SW_COMPLETION_RING_TQM_RELEASE_REASON_MASK) >>
		TQM2SW_COMPLETION_RING_TQM_RELEASE_REASON_LSB;
}

/*
 * ---------------------------------------------------------------------------
 * WBM Descriptor accessor APIs for Tx completions
 * ---------------------------------------------------------------------------
 */
/**
 * dal_vndr_hal_tx_comp_get_buffer_type_bn() - Buffer or Descriptor type
 * @hal_desc: completion ring descriptor pointer
 *
 * This function will return the type of pointer - buffer or descriptor
 *
 * Return: buffer type
 */
static inline uint32_t dal_vndr_hal_tx_comp_get_buffer_type_bn(void *hal_desc)
{
	uint32_t comp_desc =
		*(uint32_t *)(((uint8_t *)hal_desc) +
		HAL_TX_COMP_BUFFER_OR_DESC_TYPE_OFFSET);

	return (comp_desc & HAL_TX_COMP_BUFFER_OR_DESC_TYPE_MASK) >>
		HAL_TX_COMP_BUFFER_OR_DESC_TYPE_LSB;
}

/**
 * dal_vndr_hal_rx_get_reo_desc_va_bn() - Get Desc virtual addr within REO Desc
 * @reo_desc: REO2SW ring descriptor pointer
 *
 * Return: RX descriptor virtual address
 */
static inline uintptr_t dal_vndr_hal_rx_get_reo_desc_va_bn(void *reo_desc)
{
	uint64_t va_from_desc;

	/* first 64 bits block for VA */
	va_from_desc = qdf_le64_to_cpu(*(uint64_t *)reo_desc);

	return (uintptr_t)va_from_desc;
}

/**
 * dal_vndr_hal_rx_reo_buf_type_get_bn() - Get REO buffer type
 * @rx_desc: RX descriptor
 *
 * Return: buffer type
 */
static uint8_t dal_vndr_hal_rx_reo_buf_type_get_bn(hal_ring_desc_t rx_desc)
{
	return HAL_RX_REO_BUF_TYPE_GET(rx_desc);
}

void dal_vndr_hal_default_ops_attach_bn(struct dal_vndr_hal_soc *hal_soc)
{
	hal_soc->ops->dal_vndr_hal_tx_desc_set_buf_addr =
				dal_vndr_hal_tx_desc_set_buf_addr_bn;
	hal_soc->ops->dal_vndr_hal_tx_desc_set_buf_length =
				dal_vndr_hal_tx_desc_set_buf_length_bn;
	hal_soc->ops->dal_vndr_hal_tx_desc_set_buf_offset =
				dal_vndr_hal_tx_desc_set_buf_offset_bn;
	hal_soc->ops->dal_vndr_hal_tx_desc_set_l3_checksum_en =
				dal_vndr_hal_tx_desc_set_l3_checksum_en_bn;
	hal_soc->ops->dal_vndr_hal_tx_desc_set_l4_checksum_en =
				dal_vndr_hal_tx_desc_set_l4_checksum_en_bn;
	hal_soc->ops->dal_vndr_hal_tx_desc_set_bank_id =
				dal_vndr_hal_tx_desc_set_bank_id_bn;
	hal_soc->ops->dal_vndr_hal_tx_desc_set_fw_metadata =
				dal_vndr_hal_tx_desc_set_fw_metadata_bn;
	hal_soc->ops->dal_vndr_hal_tx_desc_set_vdev_id =
				dal_vndr_hal_tx_desc_set_vdev_id_bn;
	hal_soc->ops->dal_vndr_hal_tx_desc_set_hlos_tid =
				dal_vndr_hal_tx_desc_set_hlos_tid_bn;
	hal_soc->ops->dal_vndr_hal_tx_desc_set_tx_notify_frame =
				dal_vndr_hal_tx_desc_set_tx_notify_frame_bn;
	hal_soc->ops->dal_vndr_hal_tx_desc_set_to_fw =
				dal_vndr_hal_tx_desc_set_to_fw_bn;
	hal_soc->ops->dal_vndr_hal_tx_desc_set_peer_txpt_ci_index =
				dal_vndr_hal_tx_desc_set_peer_txpt_ci_index_bn;
	hal_soc->ops->dal_vndr_hal_tx_desc_set_peer_txpt_ci_tos_tc_val =
			dal_vndr_hal_tx_desc_set_peer_txpt_ci_tos_tc_val_bn;
	hal_soc->ops->dal_vndr_hal_tx_desc_set_da_is_bcast_mcast =
				dal_vndr_hal_tx_desc_set_da_is_bcast_mcast_bn;
	hal_soc->ops->dal_vndr_hal_tx_desc_set_l3_type =
				dal_vndr_hal_tx_desc_set_l3_type_bn;
	hal_soc->ops->dal_vndr_hal_tx_desc_set_l4_protocol =
				dal_vndr_hal_tx_desc_set_l4_protocol_bn;
	hal_soc->ops->dal_vndr_hal_tx_desc_set_type_or_length =
				dal_vndr_hal_tx_desc_set_type_or_length_bn;
	hal_soc->ops->dal_vndr_hal_tx_desc_set_dport =
				dal_vndr_hal_tx_desc_set_dport_bn;
	hal_soc->ops->dal_vndr_hal_tx_desc_set_snap_oui_zero_or_f8 =
			dal_vndr_hal_tx_desc_set_snap_oui_zero_or_f8_bn;
	hal_soc->ops->dal_vndr_hal_tx_desc_set_snap_oui_not_zero_or_not_f8 =
			dal_vndr_hal_tx_desc_set_snap_oui_not_zero_or_not_f8_bn;
	hal_soc->ops->dal_vndr_hal_tx_desc_set_s_vlan_tag =
				dal_vndr_hal_tx_desc_set_s_vlan_tag_bn;
	hal_soc->ops->dal_vndr_hal_tx_desc_set_c_vlan_tag =
				dal_vndr_hal_tx_desc_set_c_vlan_tag_bn;
	hal_soc->ops->dal_vndr_hal_tx_comp_get_status_generic =
				dal_vndr_hal_tx_comp_get_status_generic_be;
	hal_soc->ops->dal_vndr_hal_tx_comp_get_release_reason_generic =
			dal_vndr_hal_tx_comp_get_release_reason_generic_bn;
	hal_soc->ops->dal_vndr_hal_tx_comp_get_buffer_type =
				dal_vndr_hal_tx_comp_get_buffer_type_bn;
	hal_soc->ops->dal_vndr_hal_tx_comp_get_paddr =
				dal_vndr_hal_tx_comp_get_paddr_be;
	hal_soc->ops->dal_vndr_hal_rx_error_status_get =
				dal_vndr_hal_rx_error_status_get_be;
	hal_soc->ops->dal_vndr_hal_rx_reo_buf_cookie_get =
				dal_vndr_hal_rx_reo_buf_cookie_get_be;
	hal_soc->ops->dal_vndr_hal_rx_ret_buf_manager_get =
				dal_vndr_hal_rx_ret_buf_manager_get_be;
	hal_soc->ops->dal_vndr_hal_rx_reo_get_details =
				dal_vndr_hal_rx_reo_get_details_be;
	hal_soc->ops->dal_vndr_hal_rx_reo_buf_paddr_get =
				dal_vndr_hal_rx_reo_buf_paddr_get_be;
	hal_soc->ops->dal_vndr_hal_rx_tlv_msdu_done_get =
				dal_vndr_hal_rx_tlv_msdu_done_get_be;
	hal_soc->ops->dal_vndr_hal_rx_get_l3_pad_bytes =
				dal_vndr_hal_rx_get_l3_pad_bytes_be;
	hal_soc->ops->hal_rx_reo_buf_type_get =
				dal_vndr_hal_rx_reo_buf_type_get_bn;
	hal_soc->ops->hal_rx_get_reo_desc_va =
				dal_vndr_hal_rx_get_reo_desc_va_bn;
}
