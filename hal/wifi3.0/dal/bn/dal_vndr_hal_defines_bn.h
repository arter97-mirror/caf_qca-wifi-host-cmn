/* SPDX-License-Identifier: ISC */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef _DAL_VNDR_HAL_DEFINES_BN_H_
#define _DAL_VNDR_HAL_DEFINES_BN_H_
#include <linux/types.h>
#include "dal_vndr_hal_internal.h"
#include "dal_vndr_hal_defines_be.h"

#define NUM_OF_DWORDS_REO_DESTINATION_RING 8

struct dal_reo_destination_ring {
#ifndef WIFI_BIT_ORDER_BIG_ENDIAN
	struct   dal_buffer_addr_info  buf_or_link_desc_virt_addr_or_addr_info;
		struct   dal_rx_mpdu_desc_info rx_mpdu_desc_info_details;
		struct   dal_rx_msdu_desc_info rx_msdu_desc_info_details;
		uint32_t release_source_module :  3,
			cookie_conversion_status  :  1,
			rxdma_push_reason :  2,
			rxdma_error_code  :  5,
			reo_delink_error  :  1,
			sw_buffer_cookie  : 20;
		uint32_t msdu_link_desc_index  :  4,
			reserved_6a   : 28;
			uint32_t reo_dest_buffer_type  :  1,
			reo_push_reason   :  2,
			reo_error_code    :  5,
			captured_msdu_data_size   :  4,
			sw_exception  :  1,
			src_link_id   :  3,
			reo_destination_struct_signature  :  4,
			ring_id   :  8,
			looping_count :  4;
#else
	struct   dal_buffer_addr_info  buf_or_link_desc_virt_addr_or_addr_info;
		struct   dal_rx_mpdu_desc_info rx_mpdu_desc_info_details;
		struct   dal_rx_msdu_desc_info rx_msdu_desc_info_details;
		uint32_t sw_buffer_cookie  : 20,
			reo_delink_error  :  1,
			rxdma_error_code  :  5,
			rxdma_push_reason :  2,
			cookie_conversion_status  :  1,
			release_source_module :  3;
		uint32_t reserved_6a   : 28,
			msdu_link_desc_index  :  4;
		uint32_t looping_count :  4,
			ring_id   :  8,
			reo_destination_struct_signature  :  4,
			src_link_id       :  3,
			sw_exception      :  1,
			captured_msdu_data_size   :  4,
			reo_error_code        :  5,
			reo_push_reason       :  2,
			reo_dest_buffer_type      :  1;
#endif
};

#define REO_DESTINATION_RING_BUF_OR_LINK_DESC_VIRT_ADDR_OR_ADDR_INFO_BUFFER_ADDR_31_0_OFFSET 0x00000000
#define REO_DESTINATION_RING_BUF_OR_LINK_DESC_VIRT_ADDR_OR_ADDR_INFO_BUFFER_ADDR_31_0_LSB 0
#define REO_DESTINATION_RING_BUF_OR_LINK_DESC_VIRT_ADDR_OR_ADDR_INFO_BUFFER_ADDR_31_0_MSB 31
#define REO_DESTINATION_RING_BUF_OR_LINK_DESC_VIRT_ADDR_OR_ADDR_INFO_BUFFER_ADDR_31_0_MASK 0xffffffff

#define REO_DESTINATION_RING_BUF_OR_LINK_DESC_VIRT_ADDR_OR_ADDR_INFO_BUFFER_ADDR_39_32_OFFSET 0x00000004
#define REO_DESTINATION_RING_BUF_OR_LINK_DESC_VIRT_ADDR_OR_ADDR_INFO_BUFFER_ADDR_39_32_LSB 0
#define REO_DESTINATION_RING_BUF_OR_LINK_DESC_VIRT_ADDR_OR_ADDR_INFO_BUFFER_ADDR_39_32_MSB 7
#define REO_DESTINATION_RING_BUF_OR_LINK_DESC_VIRT_ADDR_OR_ADDR_INFO_BUFFER_ADDR_39_32_MASK 0x000000ff

#define REO_DESTINATION_RING_BUF_OR_LINK_DESC_VIRT_ADDR_OR_ADDR_INFO_RETURN_BUFFER_MANAGER_OFFSET 0x00000004
#define REO_DESTINATION_RING_BUF_OR_LINK_DESC_VIRT_ADDR_OR_ADDR_INFO_RETURN_BUFFER_MANAGER_LSB 8
#define REO_DESTINATION_RING_BUF_OR_LINK_DESC_VIRT_ADDR_OR_ADDR_INFO_RETURN_BUFFER_MANAGER_MSB 11
#define REO_DESTINATION_RING_BUF_OR_LINK_DESC_VIRT_ADDR_OR_ADDR_INFO_RETURN_BUFFER_MANAGER_MASK 0x00000f00

#define REO_DESTINATION_RING_BUF_OR_LINK_DESC_VIRT_ADDR_OR_ADDR_INFO_SW_BUFFER_COOKIE_OFFSET 0x00000004
#define REO_DESTINATION_RING_BUF_OR_LINK_DESC_VIRT_ADDR_OR_ADDR_INFO_SW_BUFFER_COOKIE_LSB 12
#define REO_DESTINATION_RING_BUF_OR_LINK_DESC_VIRT_ADDR_OR_ADDR_INFO_SW_BUFFER_COOKIE_MSB 31
#define REO_DESTINATION_RING_BUF_OR_LINK_DESC_VIRT_ADDR_OR_ADDR_INFO_SW_BUFFER_COOKIE_MASK 0xfffff000

#define REO_DESTINATION_RING_RX_MPDU_DESC_INFO_DETAILS_MSDU_COUNT_OFFSET    0x00000008
#define REO_DESTINATION_RING_RX_MPDU_DESC_INFO_DETAILS_MSDU_COUNT_LSB   0
#define REO_DESTINATION_RING_RX_MPDU_DESC_INFO_DETAILS_MSDU_COUNT_MSB   7
#define REO_DESTINATION_RING_RX_MPDU_DESC_INFO_DETAILS_MSDU_COUNT_MASK  0x000000ff

#define REO_DESTINATION_RING_RX_MPDU_DESC_INFO_DETAILS_FRAGMENT_FLAG_OFFSET 0x00000008
#define REO_DESTINATION_RING_RX_MPDU_DESC_INFO_DETAILS_FRAGMENT_FLAG_LSB    8
#define REO_DESTINATION_RING_RX_MPDU_DESC_INFO_DETAILS_FRAGMENT_FLAG_MSB    8
#define REO_DESTINATION_RING_RX_MPDU_DESC_INFO_DETAILS_FRAGMENT_FLAG_MASK   0x00000100

#define REO_DESTINATION_RING_RX_MPDU_DESC_INFO_DETAILS_MPDU_RETRY_BIT_OFFSET    0x00000008
#define REO_DESTINATION_RING_RX_MPDU_DESC_INFO_DETAILS_MPDU_RETRY_BIT_LSB   9
#define REO_DESTINATION_RING_RX_MPDU_DESC_INFO_DETAILS_MPDU_RETRY_BIT_MSB   9
#define REO_DESTINATION_RING_RX_MPDU_DESC_INFO_DETAILS_MPDU_RETRY_BIT_MASK  0x00000200

#define REO_DESTINATION_RING_RX_MPDU_DESC_INFO_DETAILS_AMPDU_FLAG_OFFSET    0x00000008
#define REO_DESTINATION_RING_RX_MPDU_DESC_INFO_DETAILS_AMPDU_FLAG_LSB   10
#define REO_DESTINATION_RING_RX_MPDU_DESC_INFO_DETAILS_AMPDU_FLAG_MSB   10
#define REO_DESTINATION_RING_RX_MPDU_DESC_INFO_DETAILS_AMPDU_FLAG_MASK  0x00000400

#define REO_DESTINATION_RING_RX_MPDU_DESC_INFO_DETAILS_BAR_FRAME_OFFSET 0x00000008
#define REO_DESTINATION_RING_RX_MPDU_DESC_INFO_DETAILS_BAR_FRAME_LSB    11
#define REO_DESTINATION_RING_RX_MPDU_DESC_INFO_DETAILS_BAR_FRAME_MSB    11
#define REO_DESTINATION_RING_RX_MPDU_DESC_INFO_DETAILS_BAR_FRAME_MASK   0x00000800

#define REO_DESTINATION_RING_RX_MPDU_DESC_INFO_DETAILS_PN_FIELDS_CONTAIN_VALID_INFO_OFFSET 0x00000008
#define REO_DESTINATION_RING_RX_MPDU_DESC_INFO_DETAILS_PN_FIELDS_CONTAIN_VALID_INFO_LSB 12
#define REO_DESTINATION_RING_RX_MPDU_DESC_INFO_DETAILS_PN_FIELDS_CONTAIN_VALID_INFO_MSB 12
#define REO_DESTINATION_RING_RX_MPDU_DESC_INFO_DETAILS_PN_FIELDS_CONTAIN_VALID_INFO_MASK 0x00001000

#define REO_DESTINATION_RING_RX_MPDU_DESC_INFO_DETAILS_RAW_MPDU_OFFSET  0x00000008
#define REO_DESTINATION_RING_RX_MPDU_DESC_INFO_DETAILS_RAW_MPDU_LSB 13
#define REO_DESTINATION_RING_RX_MPDU_DESC_INFO_DETAILS_RAW_MPDU_MSB 13
#define REO_DESTINATION_RING_RX_MPDU_DESC_INFO_DETAILS_RAW_MPDU_MASK    0x00002000

#define REO_DESTINATION_RING_RX_MPDU_DESC_INFO_DETAILS_MORE_FRAGMENT_FLAG_OFFSET    0x00000008
#define REO_DESTINATION_RING_RX_MPDU_DESC_INFO_DETAILS_MORE_FRAGMENT_FLAG_LSB       14
#define REO_DESTINATION_RING_RX_MPDU_DESC_INFO_DETAILS_MORE_FRAGMENT_FLAG_MSB       14
#define REO_DESTINATION_RING_RX_MPDU_DESC_INFO_DETAILS_MORE_FRAGMENT_FLAG_MASK      0x00004000

#define REO_DESTINATION_RING_RX_MPDU_DESC_INFO_DETAILS_SRC_INFO_OFFSET  0x00000008
#define REO_DESTINATION_RING_RX_MPDU_DESC_INFO_DETAILS_SRC_INFO_LSB 15
#define REO_DESTINATION_RING_RX_MPDU_DESC_INFO_DETAILS_SRC_INFO_MSB 26
#define REO_DESTINATION_RING_RX_MPDU_DESC_INFO_DETAILS_SRC_INFO_MASK    0x07ff8000

#define REO_DESTINATION_RING_RX_MPDU_DESC_INFO_DETAILS_MPDU_QOS_CONTROL_VALID_OFFSET 0x00000008
#define REO_DESTINATION_RING_RX_MPDU_DESC_INFO_DETAILS_MPDU_QOS_CONTROL_VALID_LSB   27
#define REO_DESTINATION_RING_RX_MPDU_DESC_INFO_DETAILS_MPDU_QOS_CONTROL_VALID_MSB   27
#define REO_DESTINATION_RING_RX_MPDU_DESC_INFO_DETAILS_MPDU_QOS_CONTROL_VALID_MASK  0x08000000

#define REO_DESTINATION_RING_RX_MPDU_DESC_INFO_DETAILS_TID_OFFSET   0x00000008
#define REO_DESTINATION_RING_RX_MPDU_DESC_INFO_DETAILS_TID_LSB  28
#define REO_DESTINATION_RING_RX_MPDU_DESC_INFO_DETAILS_TID_MSB  31
#define REO_DESTINATION_RING_RX_MPDU_DESC_INFO_DETAILS_TID_MASK 0xf0000000

#define REO_DESTINATION_RING_RX_MPDU_DESC_INFO_DETAILS_PEER_META_DATA_OFFSET    0x0000000c
#define REO_DESTINATION_RING_RX_MPDU_DESC_INFO_DETAILS_PEER_META_DATA_LSB   0
#define REO_DESTINATION_RING_RX_MPDU_DESC_INFO_DETAILS_PEER_META_DATA_MSB   31
#define REO_DESTINATION_RING_RX_MPDU_DESC_INFO_DETAILS_PEER_META_DATA_MASK  0xffffffff

#define REO_DESTINATION_RING_RX_MSDU_DESC_INFO_DETAILS_FIRST_MSDU_IN_MPDU_FLAG_OFFSET 0x00000010
#define REO_DESTINATION_RING_RX_MSDU_DESC_INFO_DETAILS_FIRST_MSDU_IN_MPDU_FLAG_LSB  0
#define REO_DESTINATION_RING_RX_MSDU_DESC_INFO_DETAILS_FIRST_MSDU_IN_MPDU_FLAG_MSB  0
#define REO_DESTINATION_RING_RX_MSDU_DESC_INFO_DETAILS_FIRST_MSDU_IN_MPDU_FLAG_MASK 0x00000001

#define REO_DESTINATION_RING_RX_MSDU_DESC_INFO_DETAILS_LAST_MSDU_IN_MPDU_FLAG_OFFSET 0x00000010
#define REO_DESTINATION_RING_RX_MSDU_DESC_INFO_DETAILS_LAST_MSDU_IN_MPDU_FLAG_LSB   1
#define REO_DESTINATION_RING_RX_MSDU_DESC_INFO_DETAILS_LAST_MSDU_IN_MPDU_FLAG_MSB   1
#define REO_DESTINATION_RING_RX_MSDU_DESC_INFO_DETAILS_LAST_MSDU_IN_MPDU_FLAG_MASK  0x00000002

#define REO_DESTINATION_RING_RX_MSDU_DESC_INFO_DETAILS_MSDU_CONTINUATION_OFFSET     0x00000010
#define REO_DESTINATION_RING_RX_MSDU_DESC_INFO_DETAILS_MSDU_CONTINUATION_LSB        2
#define REO_DESTINATION_RING_RX_MSDU_DESC_INFO_DETAILS_MSDU_CONTINUATION_MSB        2
#define REO_DESTINATION_RING_RX_MSDU_DESC_INFO_DETAILS_MSDU_CONTINUATION_MASK       0x00000004

#define REO_DESTINATION_RING_RX_MSDU_DESC_INFO_DETAILS_MSDU_LENGTH_OFFSET   0x00000010
#define REO_DESTINATION_RING_RX_MSDU_DESC_INFO_DETAILS_MSDU_LENGTH_LSB  3
#define REO_DESTINATION_RING_RX_MSDU_DESC_INFO_DETAILS_MSDU_LENGTH_MSB  16
#define REO_DESTINATION_RING_RX_MSDU_DESC_INFO_DETAILS_MSDU_LENGTH_MASK 0x0001fff8

#define REO_DESTINATION_RING_RX_MSDU_DESC_INFO_DETAILS_MSDU_DROP_OFFSET 0x00000010
#define REO_DESTINATION_RING_RX_MSDU_DESC_INFO_DETAILS_MSDU_DROP_LSB    17
#define REO_DESTINATION_RING_RX_MSDU_DESC_INFO_DETAILS_MSDU_DROP_MSB    17
#define REO_DESTINATION_RING_RX_MSDU_DESC_INFO_DETAILS_MSDU_DROP_MASK   0x00020000

#define REO_DESTINATION_RING_RX_MSDU_DESC_INFO_DETAILS_SA_IS_VALID_OFFSET   0x00000010
#define REO_DESTINATION_RING_RX_MSDU_DESC_INFO_DETAILS_SA_IS_VALID_LSB  18
#define REO_DESTINATION_RING_RX_MSDU_DESC_INFO_DETAILS_SA_IS_VALID_MSB  18
#define REO_DESTINATION_RING_RX_MSDU_DESC_INFO_DETAILS_SA_IS_VALID_MASK 0x00040000

#define REO_DESTINATION_RING_RX_MSDU_DESC_INFO_DETAILS_DA_IS_VALID_OFFSET   0x00000010
#define REO_DESTINATION_RING_RX_MSDU_DESC_INFO_DETAILS_DA_IS_VALID_LSB  19
#define REO_DESTINATION_RING_RX_MSDU_DESC_INFO_DETAILS_DA_IS_VALID_MSB  19
#define REO_DESTINATION_RING_RX_MSDU_DESC_INFO_DETAILS_DA_IS_VALID_MASK 0x00080000

#define REO_DESTINATION_RING_RX_MSDU_DESC_INFO_DETAILS_DA_IS_MCBC_OFFSET    0x00000010
#define REO_DESTINATION_RING_RX_MSDU_DESC_INFO_DETAILS_DA_IS_MCBC_LSB   20
#define REO_DESTINATION_RING_RX_MSDU_DESC_INFO_DETAILS_DA_IS_MCBC_MSB   20
#define REO_DESTINATION_RING_RX_MSDU_DESC_INFO_DETAILS_DA_IS_MCBC_MASK  0x00100000

#define REO_DESTINATION_RING_RX_MSDU_DESC_INFO_DETAILS_L3_HEADER_PADDING_MSB_OFFSET 0x00000010
#define REO_DESTINATION_RING_RX_MSDU_DESC_INFO_DETAILS_L3_HEADER_PADDING_MSB_LSB    21
#define REO_DESTINATION_RING_RX_MSDU_DESC_INFO_DETAILS_L3_HEADER_PADDING_MSB_MSB    21
#define REO_DESTINATION_RING_RX_MSDU_DESC_INFO_DETAILS_L3_HEADER_PADDING_MSB_MASK   0x00200000

#define REO_DESTINATION_RING_RX_MSDU_DESC_INFO_DETAILS_TCP_UDP_CHKSUM_FAIL_OFFSET   0x00000010
#define REO_DESTINATION_RING_RX_MSDU_DESC_INFO_DETAILS_TCP_UDP_CHKSUM_FAIL_LSB      22
#define REO_DESTINATION_RING_RX_MSDU_DESC_INFO_DETAILS_TCP_UDP_CHKSUM_FAIL_MSB      22
#define REO_DESTINATION_RING_RX_MSDU_DESC_INFO_DETAILS_TCP_UDP_CHKSUM_FAIL_MASK     0x00400000

#define REO_DESTINATION_RING_RX_MSDU_DESC_INFO_DETAILS_IP_CHKSUM_FAIL_OFFSET    0x00000010
#define REO_DESTINATION_RING_RX_MSDU_DESC_INFO_DETAILS_IP_CHKSUM_FAIL_LSB   23
#define REO_DESTINATION_RING_RX_MSDU_DESC_INFO_DETAILS_IP_CHKSUM_FAIL_MSB   23
#define REO_DESTINATION_RING_RX_MSDU_DESC_INFO_DETAILS_IP_CHKSUM_FAIL_MASK  0x00800000

#define REO_DESTINATION_RING_RX_MSDU_DESC_INFO_DETAILS_FR_DS_OFFSET 0x00000010
#define REO_DESTINATION_RING_RX_MSDU_DESC_INFO_DETAILS_FR_DS_LSB    24
#define REO_DESTINATION_RING_RX_MSDU_DESC_INFO_DETAILS_FR_DS_MSB    24
#define REO_DESTINATION_RING_RX_MSDU_DESC_INFO_DETAILS_FR_DS_MASK   0x01000000

#define REO_DESTINATION_RING_RX_MSDU_DESC_INFO_DETAILS_TO_DS_OFFSET 0x00000010
#define REO_DESTINATION_RING_RX_MSDU_DESC_INFO_DETAILS_TO_DS_LSB    25
#define REO_DESTINATION_RING_RX_MSDU_DESC_INFO_DETAILS_TO_DS_MSB    25
#define REO_DESTINATION_RING_RX_MSDU_DESC_INFO_DETAILS_TO_DS_MASK   0x02000000

#define REO_DESTINATION_RING_RX_MSDU_DESC_INFO_DETAILS_INTRA_BSS_OFFSET 0x00000010
#define REO_DESTINATION_RING_RX_MSDU_DESC_INFO_DETAILS_INTRA_BSS_LSB    26
#define REO_DESTINATION_RING_RX_MSDU_DESC_INFO_DETAILS_INTRA_BSS_MSB    26
#define REO_DESTINATION_RING_RX_MSDU_DESC_INFO_DETAILS_INTRA_BSS_MASK   0x04000000

#define REO_DESTINATION_RING_RX_MSDU_DESC_INFO_DETAILS_DEST_CHIP_ID_OFFSET  0x00000010
#define REO_DESTINATION_RING_RX_MSDU_DESC_INFO_DETAILS_DEST_CHIP_ID_LSB 27
#define REO_DESTINATION_RING_RX_MSDU_DESC_INFO_DETAILS_DEST_CHIP_ID_MSB 28
#define REO_DESTINATION_RING_RX_MSDU_DESC_INFO_DETAILS_DEST_CHIP_ID_MASK    0x18000000

#define REO_DESTINATION_RING_RX_MSDU_DESC_INFO_DETAILS_DECAP_FORMAT_OFFSET  0x00000010
#define REO_DESTINATION_RING_RX_MSDU_DESC_INFO_DETAILS_DECAP_FORMAT_LSB 29
#define REO_DESTINATION_RING_RX_MSDU_DESC_INFO_DETAILS_DECAP_FORMAT_MSB 30
#define REO_DESTINATION_RING_RX_MSDU_DESC_INFO_DETAILS_DECAP_FORMAT_MASK    0x60000000

#define REO_DESTINATION_RING_RELEASE_SOURCE_MODULE_OFFSET   0x00000014
#define REO_DESTINATION_RING_RELEASE_SOURCE_MODULE_LSB  0
#define REO_DESTINATION_RING_RELEASE_SOURCE_MODULE_MSB  2
#define REO_DESTINATION_RING_RELEASE_SOURCE_MODULE_MASK 0x00000007

#define REO_DESTINATION_RING_COOKIE_CONVERSION_STATUS_OFFSET    0x00000014
#define REO_DESTINATION_RING_COOKIE_CONVERSION_STATUS_LSB   3
#define REO_DESTINATION_RING_COOKIE_CONVERSION_STATUS_MSB   3
#define REO_DESTINATION_RING_COOKIE_CONVERSION_STATUS_MASK  0x00000008

#define REO_DESTINATION_RING_RXDMA_PUSH_REASON_OFFSET   0x00000014
#define REO_DESTINATION_RING_RXDMA_PUSH_REASON_LSB      4
#define REO_DESTINATION_RING_RXDMA_PUSH_REASON_MSB      5
#define REO_DESTINATION_RING_RXDMA_PUSH_REASON_MASK     0x00000030

#define REO_DESTINATION_RING_RXDMA_ERROR_CODE_OFFSET    0x00000014
#define REO_DESTINATION_RING_RXDMA_ERROR_CODE_LSB       6
#define REO_DESTINATION_RING_RXDMA_ERROR_CODE_MSB       10
#define REO_DESTINATION_RING_RXDMA_ERROR_CODE_MASK      0x000007c0

#define REO_DESTINATION_RING_REO_DELINK_ERROR_OFFSET    0x00000014
#define REO_DESTINATION_RING_REO_DELINK_ERROR_LSB       11
#define REO_DESTINATION_RING_REO_DELINK_ERROR_MSB       11
#define REO_DESTINATION_RING_REO_DELINK_ERROR_MASK      0x00000800

#define REO_DESTINATION_RING_SW_BUFFER_COOKIE_OFFSET    0x00000014
#define REO_DESTINATION_RING_SW_BUFFER_COOKIE_LSB   12
#define REO_DESTINATION_RING_SW_BUFFER_COOKIE_MSB   31
#define REO_DESTINATION_RING_SW_BUFFER_COOKIE_MASK  0xfffff000

#define REO_DESTINATION_RING_MSDU_LINK_DESC_INDEX_OFFSET    0x00000018
#define REO_DESTINATION_RING_MSDU_LINK_DESC_INDEX_LSB   0
#define REO_DESTINATION_RING_MSDU_LINK_DESC_INDEX_MSB   3
#define REO_DESTINATION_RING_MSDU_LINK_DESC_INDEX_MASK  0x0000000f

#define REO_DESTINATION_RING_RESERVED_6A_OFFSET 0x00000018
#define REO_DESTINATION_RING_RESERVED_6A_LSB    4
#define REO_DESTINATION_RING_RESERVED_6A_MSB    31
#define REO_DESTINATION_RING_RESERVED_6A_MASK   0xfffffff0

#define REO_DESTINATION_RING_REO_DEST_BUFFER_TYPE_OFFSET    0x0000001c
#define REO_DESTINATION_RING_REO_DEST_BUFFER_TYPE_LSB   0
#define REO_DESTINATION_RING_REO_DEST_BUFFER_TYPE_MSB   0
#define REO_DESTINATION_RING_REO_DEST_BUFFER_TYPE_MASK  0x00000001

#define REO_DESTINATION_RING_REO_PUSH_REASON_OFFSET 0x0000001c
#define REO_DESTINATION_RING_REO_PUSH_REASON_LSB    1
#define REO_DESTINATION_RING_REO_PUSH_REASON_MSB    2
#define REO_DESTINATION_RING_REO_PUSH_REASON_MASK   0x00000006

#define REO_DESTINATION_RING_REO_ERROR_CODE_OFFSET  0x0000001c
#define REO_DESTINATION_RING_REO_ERROR_CODE_LSB     3
#define REO_DESTINATION_RING_REO_ERROR_CODE_MSB     7
#define REO_DESTINATION_RING_REO_ERROR_CODE_MASK    0x000000f8

#define REO_DESTINATION_RING_CAPTURED_MSDU_DATA_SIZE_OFFSET 0x0000001c
#define REO_DESTINATION_RING_CAPTURED_MSDU_DATA_SIZE_LSB    8
#define REO_DESTINATION_RING_CAPTURED_MSDU_DATA_SIZE_MSB    11
#define REO_DESTINATION_RING_CAPTURED_MSDU_DATA_SIZE_MASK   0x00000f00

#define REO_DESTINATION_RING_SW_EXCEPTION_OFFSET    0x0000001c
#define REO_DESTINATION_RING_SW_EXCEPTION_LSB       12
#define REO_DESTINATION_RING_SW_EXCEPTION_MSB       12
#define REO_DESTINATION_RING_SW_EXCEPTION_MASK      0x00001000

#define REO_DESTINATION_RING_SRC_LINK_ID_OFFSET     0x0000001c
#define REO_DESTINATION_RING_SRC_LINK_ID_LSB        13
#define REO_DESTINATION_RING_SRC_LINK_ID_MSB        15
#define REO_DESTINATION_RING_SRC_LINK_ID_MASK       0x0000e000

#define REO_DESTINATION_RING_REO_DESTINATION_STRUCT_SIGNATURE_OFFSET 0x0000001c
#define REO_DESTINATION_RING_REO_DESTINATION_STRUCT_SIGNATURE_LSB   16
#define REO_DESTINATION_RING_REO_DESTINATION_STRUCT_SIGNATURE_MSB   19
#define REO_DESTINATION_RING_REO_DESTINATION_STRUCT_SIGNATURE_MASK  0x000f0000

#define REO_DESTINATION_RING_RING_ID_OFFSET         0x0000001c
#define REO_DESTINATION_RING_RING_ID_LSB            20
#define REO_DESTINATION_RING_RING_ID_MSB            27
#define REO_DESTINATION_RING_RING_ID_MASK           0x0ff00000

#define REO_DESTINATION_RING_LOOPING_COUNT_OFFSET   0x0000001c
#define REO_DESTINATION_RING_LOOPING_COUNT_LSB      28
#define REO_DESTINATION_RING_LOOPING_COUNT_MSB      31
#define REO_DESTINATION_RING_LOOPING_COUNT_MASK     0xf0000000

#define NUM_OF_DWORDS_TCL_ASSIST_CMD 16

struct dal_tcl_assist_cmd {
#ifndef WIFI_BIT_ORDER_BIG_ENDIAN
	struct   dal_buffer_addr_info                           buf_addr_info;
	uint32_t buf_or_ext_desc_type                               :  1,
		bank_id                                                 :  6,
		vdev_id                                                 :  8,
		data_length                                             : 14,
		to_fw_tqm                                               :  2,
		reserved_2a                                             :  1;
	uint32_t reserved_3a                                        : 25,
		metadata_length                                         :  7;
	uint32_t txpt_classify_info_index                           :  8,
		txpt_classify_info_sel                                :  5,
		txpt_classify_info_override                             :  1,
		flow_override_enable                                    :  1,
		flow_select                                             :  1,
		hlos_tid                                                :  4,
		hlos_tid_overwrite                                      :  1,
		tos_tc_value                                            :  8,
		reserved_4a                                            :  3;
	uint32_t reserved_5a                                        : 16,
		l3_type                                                 : 16;
	uint32_t l4_protocol                                        :  8,
		l4_port                                                 : 16,
		l4_port_type                                            :  1,
		da_is_bcast_mcast                                       :  1,
		da_is_bcast                                             :  1,
		reserved_6a                                             :  5;
	uint32_t ip_address_31_0                                    : 32;
	uint32_t ip_address_63_32                                   : 32;
	uint32_t ip_address_95_64                                  : 32;
	uint32_t ip_address_127_96                                 : 32;
	uint32_t ip_da_or_sa                                        :  1,
		encap_length_change                                     :  6,
		encap_length_decrease                                   :  1,
		encap_length_override                                   :  1,
		type_or_length                                          :  1,
		snap_oui_zero_or_f8                                     :  1,
		snap_oui_not_zero_and_not_f8                            :  1,
		msdu_color                                              :  2,
		tx_notify_frame                                         :  3,
		tqm_no_drop                                             :  1,
		reserved_11a                                            : 12,
		l3_checksum_enable                                      :  1,
		l4_checksum_enable                                      :  1;
	uint32_t buffer_timestamp                                   : 19,
		buffer_timestamp_valid                                  :  1,
		s_vlan_tag_present                                      :  1,
		c_vlan_tag_present                                      :  1,
		wmac_hdr_len                                            :  7,
		reserved_12a                                            :  3;
	uint32_t tcl_cmd_number                                     : 32;
	uint32_t reserved_14a                                      : 32;
	uint32_t reserved_15a                                       : 20,
		ring_id                                                 :  8,
		looping_count                                           :  4;
#else
	struct   dal_buffer_addr_info                         buf_addr_info;
	uint32_t reserved_2a                                       :  1,
		to_fw_tqm                                               :  2,
		data_length                                             : 14,
		vdev_id                                                 :  8,
		bank_id                                                 :  6,
		buf_or_ext_desc_type                                    :  1;
	uint32_t metadata_length                                    :  7,
		reserved_3a                                             : 25;
	uint32_t reserved_4a                                       :  3,
		tos_tc_value                                          :  8,
		hlos_tid_overwrite                                      :  1,
		hlos_tid                                                :  4,
		flow_select                                             :  1,
		flow_override_enable                                    :  1,
		txpt_classify_info_override                             :  1,
		txpt_classify_info_sel                                  :  5,
		txpt_classify_info_index                                :  8;
	uint32_t l3_type                                            : 16,
		reserved_5a                                             : 16;
	uint32_t reserved_6a                                        :  5,
		da_is_bcast                                             :  1,
		da_is_bcast_mcast                                       :  1,
		l4_port_type                                            :  1,
		l4_port                                                 : 16,
		l4_protocol                                             :  8;
	uint32_t ip_address_31_0                                    : 32;
	uint32_t ip_address_63_32                                   : 32;
	uint32_t ip_address_95_64                                   : 32;
	uint32_t ip_address_127_96                                  : 32;
	uint32_t l4_checksum_enable                                 :  1,
		l3_checksum_enable                                      :  1,
		reserved_11a                                            : 12,
		tqm_no_drop                                             :  1,
		tx_notify_frame                                         :  3,
		msdu_color                                              :  2,
		snap_oui_not_zero_and_not_f8                            :  1,
		snap_oui_zero_or_f8                                     :  1,
		type_or_length                                          :  1,
		encap_length_override                                   :  1,
		encap_length_decrease                                   :  1,
		encap_length_change                                     :  6,
		ip_da_or_sa                                             :  1;
	uint32_t reserved_12a                                       :  3,
		wmac_hdr_len                                            :  7,
		c_vlan_tag_present                                      :  1,
		s_vlan_tag_present                                      :  1,
		buffer_timestamp_valid                                  :  1,
		buffer_timestamp                                        : 19;
	uint32_t tcl_cmd_number                                     : 32;
	uint32_t reserved_14a                                       : 32;
	uint32_t looping_count                                      :  4,
		ring_id                                                 :  8,
		reserved_15a                                            : 20;
#endif
};

#define TCL_ASSIST_CMD_BUF_ADDR_INFO_BUFFER_ADDR_31_0_OFFSET        0x00000000
#define TCL_ASSIST_CMD_BUF_ADDR_INFO_BUFFER_ADDR_31_0_LSB           0
#define TCL_ASSIST_CMD_BUF_ADDR_INFO_BUFFER_ADDR_31_0_MSB           31
#define TCL_ASSIST_CMD_BUF_ADDR_INFO_BUFFER_ADDR_31_0_MASK         0xffffffff

#define TCL_ASSIST_CMD_BUF_ADDR_INFO_BUFFER_ADDR_39_32_OFFSET       0x00000004
#define TCL_ASSIST_CMD_BUF_ADDR_INFO_BUFFER_ADDR_39_32_LSB          0
#define TCL_ASSIST_CMD_BUF_ADDR_INFO_BUFFER_ADDR_39_32_MSB          7
#define TCL_ASSIST_CMD_BUF_ADDR_INFO_BUFFER_ADDR_39_32_MASK         0x000000ff

#define TCL_ASSIST_CMD_BUF_ADDR_INFO_RETURN_BUFFER_MANAGER_OFFSET   0x00000004
#define TCL_ASSIST_CMD_BUF_ADDR_INFO_RETURN_BUFFER_MANAGER_LSB      8
#define TCL_ASSIST_CMD_BUF_ADDR_INFO_RETURN_BUFFER_MANAGER_MSB     11
#define TCL_ASSIST_CMD_BUF_ADDR_INFO_RETURN_BUFFER_MANAGER_MASK     0x00000f00

#define TCL_ASSIST_CMD_BUF_ADDR_INFO_SW_BUFFER_COOKIE_OFFSET        0x00000004
#define TCL_ASSIST_CMD_BUF_ADDR_INFO_SW_BUFFER_COOKIE_LSB           12
#define TCL_ASSIST_CMD_BUF_ADDR_INFO_SW_BUFFER_COOKIE_MSB           31
#define TCL_ASSIST_CMD_BUF_ADDR_INFO_SW_BUFFER_COOKIE_MASK          0xfffff000

#define TCL_ASSIST_CMD_BUF_OR_EXT_DESC_TYPE_OFFSET                  0x00000008
#define TCL_ASSIST_CMD_BUF_OR_EXT_DESC_TYPE_LSB                     0
#define TCL_ASSIST_CMD_BUF_OR_EXT_DESC_TYPE_MSB                     0
#define TCL_ASSIST_CMD_BUF_OR_EXT_DESC_TYPE_MASK                   0x00000001

#define TCL_ASSIST_CMD_BANK_ID_OFFSET                               0x00000008
#define TCL_ASSIST_CMD_BANK_ID_LSB                                  1
#define TCL_ASSIST_CMD_BANK_ID_MSB                                  6
#define TCL_ASSIST_CMD_BANK_ID_MASK                               0x0000007e

#define TCL_ASSIST_CMD_VDEV_ID_OFFSET                              0x00000008
#define TCL_ASSIST_CMD_VDEV_ID_LSB                                 7
#define TCL_ASSIST_CMD_VDEV_ID_MSB                                 14
#define TCL_ASSIST_CMD_VDEV_ID_MASK                                0x00007f80

#define TCL_ASSIST_CMD_DATA_LENGTH_OFFSET                         0x00000008
#define TCL_ASSIST_CMD_DATA_LENGTH_LSB                            15
#define TCL_ASSIST_CMD_DATA_LENGTH_MSB                            28
#define TCL_ASSIST_CMD_DATA_LENGTH_MASK                         0x1fff8000

#define TCL_ASSIST_CMD_TO_FW_TQM_OFFSET                  0x00000008
#define TCL_ASSIST_CMD_TO_FW_TQM_LSB                     29
#define TCL_ASSIST_CMD_TO_FW_TQM_MSB                     30
#define TCL_ASSIST_CMD_TO_FW_TQM_MASK                    0x60000000

#define TCL_ASSIST_CMD_RESERVED_2A_OFFSET                0x00000008
#define TCL_ASSIST_CMD_RESERVED_2A_LSB                   31
#define TCL_ASSIST_CMD_RESERVED_2A_MSB                   31
#define TCL_ASSIST_CMD_RESERVED_2A_MASK                  0x80000000

#define TCL_ASSIST_CMD_RESERVED_3A_OFFSET                0x0000000c
#define TCL_ASSIST_CMD_RESERVED_3A_LSB                   0
#define TCL_ASSIST_CMD_RESERVED_3A_MSB                   24
#define TCL_ASSIST_CMD_RESERVED_3A_MASK                  0x01ffffff

#define TCL_ASSIST_CMD_METADATA_LENGTH_OFFSET            0x0000000c
#define TCL_ASSIST_CMD_METADATA_LENGTH_LSB               25
#define TCL_ASSIST_CMD_METADATA_LENGTH_MSB               31
#define TCL_ASSIST_CMD_METADATA_LENGTH_MASK              0xfe000000

#define TCL_ASSIST_CMD_TXPT_CLASSIFY_INFO_INDEX_OFFSET          0x00000010
#define TCL_ASSIST_CMD_TXPT_CLASSIFY_INFO_INDEX_LSB             0
#define TCL_ASSIST_CMD_TXPT_CLASSIFY_INFO_INDEX_MSB             7
#define TCL_ASSIST_CMD_TXPT_CLASSIFY_INFO_INDEX_MASK            0x000000ff

#define TCL_ASSIST_CMD_TXPT_CLASSIFY_INFO_SEL_OFFSET            0x00000010
#define TCL_ASSIST_CMD_TXPT_CLASSIFY_INFO_SEL_LSB               8
#define TCL_ASSIST_CMD_TXPT_CLASSIFY_INFO_SEL_MSB               12
#define TCL_ASSIST_CMD_TXPT_CLASSIFY_INFO_SEL_MASK              0x00001f00

#define TCL_ASSIST_CMD_TXPT_CLASSIFY_INFO_OVERRIDE_OFFSET       0x00000010
#define TCL_ASSIST_CMD_TXPT_CLASSIFY_INFO_OVERRIDE_LSB          13
#define TCL_ASSIST_CMD_TXPT_CLASSIFY_INFO_OVERRIDE_MSB          13
#define TCL_ASSIST_CMD_TXPT_CLASSIFY_INFO_OVERRIDE_MASK         0x00002000

#define TCL_ASSIST_CMD_FLOW_OVERRIDE_ENABLE_OFFSET              0x00000010
#define TCL_ASSIST_CMD_FLOW_OVERRIDE_ENABLE_LSB                 14
#define TCL_ASSIST_CMD_FLOW_OVERRIDE_ENABLE_MSB                 14
#define TCL_ASSIST_CMD_FLOW_OVERRIDE_ENABLE_MASK                0x00004000

#define TCL_ASSIST_CMD_FLOW_SELECT_OFFSET                       0x00000010
#define TCL_ASSIST_CMD_FLOW_SELECT_LSB                          15
#define TCL_ASSIST_CMD_FLOW_SELECT_MSB                          15
#define TCL_ASSIST_CMD_FLOW_SELECT_MASK                         0x00008000

#define TCL_ASSIST_CMD_HLOS_TID_OFFSET                          0x00000010
#define TCL_ASSIST_CMD_HLOS_TID_LSB                             16
#define TCL_ASSIST_CMD_HLOS_TID_MSB                             19
#define TCL_ASSIST_CMD_HLOS_TID_MASK                            0x000f0000

#define TCL_ASSIST_CMD_HLOS_TID_OVERWRITE_OFFSET              0x00000010
#define TCL_ASSIST_CMD_HLOS_TID_OVERWRITE_LSB                 20
#define TCL_ASSIST_CMD_HLOS_TID_OVERWRITE_MSB                 20
#define TCL_ASSIST_CMD_HLOS_TID_OVERWRITE_MASK                0x00100000

#define TCL_ASSIST_CMD_TOS_TC_VALUE_OFFSET                    0x00000010
#define TCL_ASSIST_CMD_TOS_TC_VALUE_LSB                       21
#define TCL_ASSIST_CMD_TOS_TC_VALUE_MSB                       28
#define TCL_ASSIST_CMD_TOS_TC_VALUE_MASK                      0x1fe00000

#define TCL_ASSIST_CMD_RESERVED_4A_OFFSET                  0x00000010
#define TCL_ASSIST_CMD_RESERVED_4A_LSB                     29
#define TCL_ASSIST_CMD_RESERVED_4A_MSB                     31
#define TCL_ASSIST_CMD_RESERVED_4A_MASK                    0xe0000000

#define TCL_ASSIST_CMD_RESERVED_5A_OFFSET                  0x00000014
#define TCL_ASSIST_CMD_RESERVED_5A_LSB                     0
#define TCL_ASSIST_CMD_RESERVED_5A_MSB                     15
#define TCL_ASSIST_CMD_RESERVED_5A_MASK                    0x0000ffff

#define TCL_ASSIST_CMD_L3_TYPE_OFFSET                           0x00000014
#define TCL_ASSIST_CMD_L3_TYPE_LSB                              16
#define TCL_ASSIST_CMD_L3_TYPE_MSB                              31
#define TCL_ASSIST_CMD_L3_TYPE_MASK                             0xffff0000

#define TCL_ASSIST_CMD_L4_PROTOCOL_OFFSET                       0x00000018
#define TCL_ASSIST_CMD_L4_PROTOCOL_LSB                          0
#define TCL_ASSIST_CMD_L4_PROTOCOL_MSB                          7
#define TCL_ASSIST_CMD_L4_PROTOCOL_MASK                         0x000000ff

#define TCL_ASSIST_CMD_L4_PORT_OFFSET                               0x00000018
#define TCL_ASSIST_CMD_L4_PORT_LSB                                  8
#define TCL_ASSIST_CMD_L4_PORT_MSB                                  23
#define TCL_ASSIST_CMD_L4_PORT_MASK                                 0x00ffff00

#define TCL_ASSIST_CMD_L4_PORT_TYPE_OFFSET                          0x00000018
#define TCL_ASSIST_CMD_L4_PORT_TYPE_LSB                             24
#define TCL_ASSIST_CMD_L4_PORT_TYPE_MSB                             24
#define TCL_ASSIST_CMD_L4_PORT_TYPE_MASK                            0x01000000

#define TCL_ASSIST_CMD_DA_IS_BCAST_MCAST_OFFSET                    0x00000018
#define TCL_ASSIST_CMD_DA_IS_BCAST_MCAST_LSB                       25
#define TCL_ASSIST_CMD_DA_IS_BCAST_MCAST_MSB                       25
#define TCL_ASSIST_CMD_DA_IS_BCAST_MCAST_MASK                      0x02000000

#define TCL_ASSIST_CMD_DA_IS_BCAST_OFFSET                         0x00000018
#define TCL_ASSIST_CMD_DA_IS_BCAST_LSB                            26
#define TCL_ASSIST_CMD_DA_IS_BCAST_MSB                            26
#define TCL_ASSIST_CMD_DA_IS_BCAST_MASK                           0x04000000

#define TCL_ASSIST_CMD_RESERVED_6A_OFFSET                       0x00000018
#define TCL_ASSIST_CMD_RESERVED_6A_LSB                          27
#define TCL_ASSIST_CMD_RESERVED_6A_MSB                          31
#define TCL_ASSIST_CMD_RESERVED_6A_MASK                         0xf8000000

#define TCL_ASSIST_CMD_IP_ADDRESS_31_0_OFFSET                      0x0000001c
#define TCL_ASSIST_CMD_IP_ADDRESS_31_0_LSB                         0
#define TCL_ASSIST_CMD_IP_ADDRESS_31_0_MSB                         31
#define TCL_ASSIST_CMD_IP_ADDRESS_31_0_MASK                        0xffffffff

#define TCL_ASSIST_CMD_IP_ADDRESS_63_32_OFFSET                      0x00000020
#define TCL_ASSIST_CMD_IP_ADDRESS_63_32_LSB                         0
#define TCL_ASSIST_CMD_IP_ADDRESS_63_32_MSB                         31
#define TCL_ASSIST_CMD_IP_ADDRESS_63_32_MASK                        0xffffffff

#define TCL_ASSIST_CMD_IP_ADDRESS_95_64_OFFSET                   0x00000024
#define TCL_ASSIST_CMD_IP_ADDRESS_95_64_LSB                      0
#define TCL_ASSIST_CMD_IP_ADDRESS_95_64_MSB                      31
#define TCL_ASSIST_CMD_IP_ADDRESS_95_64_MASK                     0xffffffff

#define TCL_ASSIST_CMD_IP_ADDRESS_127_96_OFFSET                   0x00000028
#define TCL_ASSIST_CMD_IP_ADDRESS_127_96_LSB                      0
#define TCL_ASSIST_CMD_IP_ADDRESS_127_96_MSB                      31
#define TCL_ASSIST_CMD_IP_ADDRESS_127_96_MASK                     0xffffffff

#define TCL_ASSIST_CMD_IP_DA_OR_SA_OFFSET                          0x0000002c
#define TCL_ASSIST_CMD_IP_DA_OR_SA_LSB                             0
#define TCL_ASSIST_CMD_IP_DA_OR_SA_MSB                             0
#define TCL_ASSIST_CMD_IP_DA_OR_SA_MASK                            0x00000001

#define TCL_ASSIST_CMD_ENCAP_LENGTH_CHANGE_OFFSET               0x0000002c
#define TCL_ASSIST_CMD_ENCAP_LENGTH_CHANGE_LSB                  1
#define TCL_ASSIST_CMD_ENCAP_LENGTH_CHANGE_MSB                  6
#define TCL_ASSIST_CMD_ENCAP_LENGTH_CHANGE_MASK                 0x0000007e

#define TCL_ASSIST_CMD_ENCAP_LENGTH_DECREASE_OFFSET              0x0000002c
#define TCL_ASSIST_CMD_ENCAP_LENGTH_DECREASE_LSB                 7
#define TCL_ASSIST_CMD_ENCAP_LENGTH_DECREASE_MSB                 7
#define TCL_ASSIST_CMD_ENCAP_LENGTH_DECREASE_MASK                0x00000080

#define TCL_ASSIST_CMD_ENCAP_LENGTH_OVERRIDE_OFFSET              0x0000002c
#define TCL_ASSIST_CMD_ENCAP_LENGTH_OVERRIDE_LSB                 8
#define TCL_ASSIST_CMD_ENCAP_LENGTH_OVERRIDE_MSB                 8
#define TCL_ASSIST_CMD_ENCAP_LENGTH_OVERRIDE_MASK                0x00000100

#define TCL_ASSIST_CMD_TYPE_OR_LENGTH_OFFSET                      0x0000002c
#define TCL_ASSIST_CMD_TYPE_OR_LENGTH_LSB                         9
#define TCL_ASSIST_CMD_TYPE_OR_LENGTH_MSB                         9
#define TCL_ASSIST_CMD_TYPE_OR_LENGTH_MASK                        0x00000200

#define TCL_ASSIST_CMD_SNAP_OUI_ZERO_OR_F8_OFFSET                 0x0000002c
#define TCL_ASSIST_CMD_SNAP_OUI_ZERO_OR_F8_LSB                    10
#define TCL_ASSIST_CMD_SNAP_OUI_ZERO_OR_F8_MSB                    10
#define TCL_ASSIST_CMD_SNAP_OUI_ZERO_OR_F8_MASK                   0x00000400

#define TCL_ASSIST_CMD_SNAP_OUI_NOT_ZERO_AND_NOT_F8_OFFSET         0x0000002c
#define TCL_ASSIST_CMD_SNAP_OUI_NOT_ZERO_AND_NOT_F8_LSB            11
#define TCL_ASSIST_CMD_SNAP_OUI_NOT_ZERO_AND_NOT_F8_MSB            11
#define TCL_ASSIST_CMD_SNAP_OUI_NOT_ZERO_AND_NOT_F8_MASK           0x00000800

#define TCL_ASSIST_CMD_MSDU_COLOR_OFFSET                            0x0000002c
#define TCL_ASSIST_CMD_MSDU_COLOR_LSB                               12
#define TCL_ASSIST_CMD_MSDU_COLOR_MSB                               13
#define TCL_ASSIST_CMD_MSDU_COLOR_MASK                              0x00003000

#define TCL_ASSIST_CMD_TX_NOTIFY_FRAME_OFFSET                       0x0000002c
#define TCL_ASSIST_CMD_TX_NOTIFY_FRAME_LSB                          14
#define TCL_ASSIST_CMD_TX_NOTIFY_FRAME_MSB                          16
#define TCL_ASSIST_CMD_TX_NOTIFY_FRAME_MASK                         0x0001c000

#define TCL_ASSIST_CMD_TQM_NO_DROP_OFFSET                           0x0000002c
#define TCL_ASSIST_CMD_TQM_NO_DROP_LSB                              17
#define TCL_ASSIST_CMD_TQM_NO_DROP_MSB                              17
#define TCL_ASSIST_CMD_TQM_NO_DROP_MASK                             0x00020000

#define TCL_ASSIST_CMD_RESERVED_11A_OFFSET                       0x0000002c
#define TCL_ASSIST_CMD_RESERVED_11A_LSB                          18
#define TCL_ASSIST_CMD_RESERVED_11A_MSB                          29
#define TCL_ASSIST_CMD_RESERVED_11A_MASK                         0x3ffc0000

#define TCL_ASSIST_CMD_L3_CHECKSUM_ENABLE_OFFSET                    0x0000002c
#define TCL_ASSIST_CMD_L3_CHECKSUM_ENABLE_LSB                       30
#define TCL_ASSIST_CMD_L3_CHECKSUM_ENABLE_MSB                       30
#define TCL_ASSIST_CMD_L3_CHECKSUM_ENABLE_MASK                     0x40000000

#define TCL_ASSIST_CMD_L4_CHECKSUM_ENABLE_OFFSET                    0x0000002c
#define TCL_ASSIST_CMD_L4_CHECKSUM_ENABLE_LSB                       31
#define TCL_ASSIST_CMD_L4_CHECKSUM_ENABLE_MSB                       31
#define TCL_ASSIST_CMD_L4_CHECKSUM_ENABLE_MASK                      0x80000000

#define TCL_ASSIST_CMD_BUFFER_TIMESTAMP_OFFSET                 0x00000030
#define TCL_ASSIST_CMD_BUFFER_TIMESTAMP_LSB                    0
#define TCL_ASSIST_CMD_BUFFER_TIMESTAMP_MSB                    18
#define TCL_ASSIST_CMD_BUFFER_TIMESTAMP_MASK                   0x0007ffff

#define TCL_ASSIST_CMD_BUFFER_TIMESTAMP_VALID_OFFSET              0x00000030
#define TCL_ASSIST_CMD_BUFFER_TIMESTAMP_VALID_LSB                 19
#define TCL_ASSIST_CMD_BUFFER_TIMESTAMP_VALID_MSB                 19
#define TCL_ASSIST_CMD_BUFFER_TIMESTAMP_VALID_MASK                0x00080000

#define TCL_ASSIST_CMD_S_VLAN_TAG_PRESENT_OFFSET                   0x00000030
#define TCL_ASSIST_CMD_S_VLAN_TAG_PRESENT_LSB                      20
#define TCL_ASSIST_CMD_S_VLAN_TAG_PRESENT_MSB                      20
#define TCL_ASSIST_CMD_S_VLAN_TAG_PRESENT_MASK                     0x00100000

#define TCL_ASSIST_CMD_C_VLAN_TAG_PRESENT_OFFSET                   0x00000030
#define TCL_ASSIST_CMD_C_VLAN_TAG_PRESENT_LSB                      21
#define TCL_ASSIST_CMD_C_VLAN_TAG_PRESENT_MSB                      21
#define TCL_ASSIST_CMD_C_VLAN_TAG_PRESENT_MASK                     0x00200000

#define TCL_ASSIST_CMD_WMAC_HDR_LEN_OFFSET                         0x00000030
#define TCL_ASSIST_CMD_WMAC_HDR_LEN_LSB                            22
#define TCL_ASSIST_CMD_WMAC_HDR_LEN_MSB                            28
#define TCL_ASSIST_CMD_WMAC_HDR_LEN_MASK                           0x1fc00000

#define TCL_ASSIST_CMD_RESERVED_12A_OFFSET                        0x00000030
#define TCL_ASSIST_CMD_RESERVED_12A_LSB                           29
#define TCL_ASSIST_CMD_RESERVED_12A_MSB                           31
#define TCL_ASSIST_CMD_RESERVED_12A_MASK                          0xe0000000

#define TCL_ASSIST_CMD_TCL_CMD_NUMBER_OFFSET                     0x00000034
#define TCL_ASSIST_CMD_TCL_CMD_NUMBER_LSB                        0
#define TCL_ASSIST_CMD_TCL_CMD_NUMBER_MSB                        31
#define TCL_ASSIST_CMD_TCL_CMD_NUMBER_MASK                       0xffffffff

#define TCL_ASSIST_CMD_RESERVED_14A_OFFSET                      0x00000038
#define TCL_ASSIST_CMD_RESERVED_14A_LSB                         0
#define TCL_ASSIST_CMD_RESERVED_14A_MSB                         31
#define TCL_ASSIST_CMD_RESERVED_14A_MASK                        0xffffffff

#define TCL_ASSIST_CMD_RESERVED_15A_OFFSET                      0x0000003c
#define TCL_ASSIST_CMD_RESERVED_15A_LSB                         0
#define TCL_ASSIST_CMD_RESERVED_15A_MSB                         19
#define TCL_ASSIST_CMD_RESERVED_15A_MASK                        0x000fffff

#define TCL_ASSIST_CMD_RING_ID_OFFSET                             0x0000003c
#define TCL_ASSIST_CMD_RING_ID_LSB                                20
#define TCL_ASSIST_CMD_RING_ID_MSB                                27
#define TCL_ASSIST_CMD_RING_ID_MASK                               0x0ff00000

#define TCL_ASSIST_CMD_LOOPING_COUNT_OFFSET                        0x0000003c
#define TCL_ASSIST_CMD_LOOPING_COUNT_LSB                           28
#define TCL_ASSIST_CMD_LOOPING_COUNT_MSB                           31
#define TCL_ASSIST_CMD_LOOPING_COUNT_MASK                          0xf0000000

#define NUM_OF_DWORDS_WBM_RELEASE_RING_TX 4

struct dal_wbm_release_ring_tx {
#ifndef WIFI_BIT_ORDER_BIG_ENDIAN
	struct   dal_buffer_addr_info         released_buff_or_desc_addr_info;
	uint32_t release_source_module                              :  3,
		buffer_or_desc_type                                     :  3,
		reserved_2a                                             : 26;
	uint32_t tqm_status_number                                  : 32;
#else
	struct   dal_buffer_addr_info         released_buff_or_desc_addr_info;
	uint32_t reserved_2a                                        : 26,
		buffer_or_desc_type                                     :  3,
		release_source_module                                   :  3;
	uint32_t tqm_status_number                                  : 32;
#endif
};

#define WBM_RELEASE_RING_TX_RELEASED_BUFF_OR_DESC_ADDR_INFO_BUFFER_ADDR_31_0_OFFSET 0x00000000
#define WBM_RELEASE_RING_TX_RELEASED_BUFF_OR_DESC_ADDR_INFO_BUFFER_ADDR_31_0_LSB    0
#define WBM_RELEASE_RING_TX_RELEASED_BUFF_OR_DESC_ADDR_INFO_BUFFER_ADDR_31_0_MSB    31
#define WBM_RELEASE_RING_TX_RELEASED_BUFF_OR_DESC_ADDR_INFO_BUFFER_ADDR_31_0_MASK   0xffffffff

#define WBM_RELEASE_RING_TX_RELEASED_BUFF_OR_DESC_ADDR_INFO_BUFFER_ADDR_39_32_OFFSET 0x00000004
#define WBM_RELEASE_RING_TX_RELEASED_BUFF_OR_DESC_ADDR_INFO_BUFFER_ADDR_39_32_LSB   0
#define WBM_RELEASE_RING_TX_RELEASED_BUFF_OR_DESC_ADDR_INFO_BUFFER_ADDR_39_32_MSB   7
#define WBM_RELEASE_RING_TX_RELEASED_BUFF_OR_DESC_ADDR_INFO_BUFFER_ADDR_39_32_MASK  0x000000ff

#define WBM_RELEASE_RING_TX_RELEASED_BUFF_OR_DESC_ADDR_INFO_RETURN_BUFFER_MANAGER_OFFSET 0x00000004
#define WBM_RELEASE_RING_TX_RELEASED_BUFF_OR_DESC_ADDR_INFO_RETURN_BUFFER_MANAGER_LSB 8
#define WBM_RELEASE_RING_TX_RELEASED_BUFF_OR_DESC_ADDR_INFO_RETURN_BUFFER_MANAGER_MSB 11
#define WBM_RELEASE_RING_TX_RELEASED_BUFF_OR_DESC_ADDR_INFO_RETURN_BUFFER_MANAGER_MASK 0x00000f00

#define WBM_RELEASE_RING_TX_RELEASED_BUFF_OR_DESC_ADDR_INFO_SW_BUFFER_COOKIE_OFFSET 0x00000004
#define WBM_RELEASE_RING_TX_RELEASED_BUFF_OR_DESC_ADDR_INFO_SW_BUFFER_COOKIE_LSB    12
#define WBM_RELEASE_RING_TX_RELEASED_BUFF_OR_DESC_ADDR_INFO_SW_BUFFER_COOKIE_MSB    31
#define WBM_RELEASE_RING_TX_RELEASED_BUFF_OR_DESC_ADDR_INFO_SW_BUFFER_COOKIE_MASK   0xfffff000

#define WBM_RELEASE_RING_TX_RELEASE_SOURCE_MODULE_OFFSET          0x00000008
#define WBM_RELEASE_RING_TX_RELEASE_SOURCE_MODULE_LSB             0
#define WBM_RELEASE_RING_TX_RELEASE_SOURCE_MODULE_MSB             2
#define WBM_RELEASE_RING_TX_RELEASE_SOURCE_MODULE_MASK            0x00000007

#define WBM_RELEASE_RING_TX_BUFFER_OR_DESC_TYPE_OFFSET            0x00000008
#define WBM_RELEASE_RING_TX_BUFFER_OR_DESC_TYPE_LSB               3
#define WBM_RELEASE_RING_TX_BUFFER_OR_DESC_TYPE_MSB               5
#define WBM_RELEASE_RING_TX_BUFFER_OR_DESC_TYPE_MASK              0x00000038

#define WBM_RELEASE_RING_TX_RESERVED_2A_OFFSET                     0x00000008
#define WBM_RELEASE_RING_TX_RESERVED_2A_LSB                        6
#define WBM_RELEASE_RING_TX_RESERVED_2A_MSB                        31
#define WBM_RELEASE_RING_TX_RESERVED_2A_MASK                       0xffffffc0

#define WBM_RELEASE_RING_TX_TQM_STATUS_NUMBER_OFFSET             0x0000000c
#define WBM_RELEASE_RING_TX_TQM_STATUS_NUMBER_LSB                0
#define WBM_RELEASE_RING_TX_TQM_STATUS_NUMBER_MSB                31
#define WBM_RELEASE_RING_TX_TQM_STATUS_NUMBER_MASK               0xffffffff

#define HAL_RX_REO_CC_STATUS_GET_BN(reo_desc) (((*(((uint32_t *)reo_desc) + \
	(REO_DESTINATION_RING_COOKIE_CONVERSION_STATUS_OFFSET >> 2))) & \
		REO_DESTINATION_RING_COOKIE_CONVERSION_STATUS_MASK) >> \
		REO_DESTINATION_RING_COOKIE_CONVERSION_STATUS_LSB)

#define HAL_RX_RXDMA_ERR_STATUS_GET_BN(reo_desc)		\
	(_HAL_MS((*_OFFSET_TO_WORD_PTR(reo_desc,		\
		REO_DESTINATION_RING_RXDMA_PUSH_REASON_OFFSET)),\
		REO_DESTINATION_RING_RXDMA_PUSH_REASON_MASK,	\
		REO_DESTINATION_RING_RXDMA_PUSH_REASON_LSB))

#define HAL_RX_RXDMA_ERR_CODE_GET_BN(reo_desc)                 \
	(_HAL_MS((*_OFFSET_TO_WORD_PTR(reo_desc,               \
		REO_DESTINATION_RING_RXDMA_ERROR_CODE_OFFSET)),\
		REO_DESTINATION_RING_RXDMA_ERROR_CODE_MASK,    \
		REO_DESTINATION_RING_RXDMA_ERROR_CODE_LSB))

#define NUM_OF_DWORDS_TQM2SW_COMPLETION_RING 8

struct dal_tqm2sw_completion_ring {
#ifndef WIFI_BIT_ORDER_BIG_ENDIAN
	struct   dal_buffer_addr_info         buf_or_desc_virt_addr_or_addr_info;
	uint32_t release_source_module                                   :  3,
		cache_id                                                :  1,
		reserved_2a                                             :  2,
		buffer_or_desc_type                                     :  3,
		return_buffer_manager                                   :  4,
		tqm_release_reason                                      :  4,
		rbm_override_valid                                      :  1,
		sw_buffer_cookie_11_0                                   : 12,
		cookie_conversion_status                                :  1,
		tqm_completion_error                                    :  1;
	uint32_t tqm_status_number                                       : 24,
		transmit_count                                          :  7,
		sw_release_details_valid                                :  1;
	uint32_t ack_frame_rssi                                          :  8,
		first_msdu                                              :  1,
		last_msdu                                               :  1,
		fw_tx_notify_frame                                      :  3,
		buffer_timestamp                                        : 19;
	struct   tx_rate_stats_info                            tx_rate_stats;
	uint32_t sw_peer_id                                              : 16,
		tid                                                     :  4,
		sw_buffer_cookie_19_12                                  :  8,
		looping_count                                           :  4;
#else
	struct   dal_buffer_addr_info          buf_or_desc_virt_addr_or_addr_info;
	uint32_t tqm_completion_error                                    :  1,
		cookie_conversion_status                                :  1,
		sw_buffer_cookie_11_0                                   : 12,
		rbm_override_valid                                      :  1,
		tqm_release_reason                                      :  4,
		return_buffer_manager                                   :  4,
		buffer_or_desc_type                                     :  3,
		reserved_2a                                             :  2,
		cache_id                                                :  1,
		release_source_module                                   :  3;
	uint32_t sw_release_details_valid                                :  1,
		transmit_count                                          :  7,
		tqm_status_number                                       : 24;
	uint32_t buffer_timestamp                                        : 19,
		fw_tx_notify_frame                                      :  3,
		last_msdu                                               :  1,
		first_msdu                                              :  1,
		ack_frame_rssi                                          :  8;
	struct   tx_rate_stats_info                             tx_rate_stats;
	uint32_t looping_count                                           :  4,
		sw_buffer_cookie_19_12                                  :  8,
		tid                                                     :  4,
		sw_peer_id                                              : 16;
#endif
};

#define TQM2SW_COMPLETION_RING_BUF_OR_DESC_VIRT_ADDR_OR_ADDR_INFO_BUFFER_ADDR_31_0_OFFSET 0x00000000
#define TQM2SW_COMPLETION_RING_BUF_OR_DESC_VIRT_ADDR_OR_ADDR_INFO_BUFFER_ADDR_31_0_LSB 0
#define TQM2SW_COMPLETION_RING_BUF_OR_DESC_VIRT_ADDR_OR_ADDR_INFO_BUFFER_ADDR_31_0_MSB 31
#define TQM2SW_COMPLETION_RING_BUF_OR_DESC_VIRT_ADDR_OR_ADDR_INFO_BUFFER_ADDR_31_0_MASK 0xffffffff

#define TQM2SW_COMPLETION_RING_BUF_OR_DESC_VIRT_ADDR_OR_ADDR_INFO_BUFFER_ADDR_39_32_OFFSET 0x00000004
#define TQM2SW_COMPLETION_RING_BUF_OR_DESC_VIRT_ADDR_OR_ADDR_INFO_BUFFER_ADDR_39_32_LSB 0
#define TQM2SW_COMPLETION_RING_BUF_OR_DESC_VIRT_ADDR_OR_ADDR_INFO_BUFFER_ADDR_39_32_MSB 7
#define TQM2SW_COMPLETION_RING_BUF_OR_DESC_VIRT_ADDR_OR_ADDR_INFO_BUFFER_ADDR_39_32_MASK 0x000000ff

#define TQM2SW_COMPLETION_RING_BUF_OR_DESC_VIRT_ADDR_OR_ADDR_INFO_RETURN_BUFFER_MANAGER_OFFSET 0x00000004
#define TQM2SW_COMPLETION_RING_BUF_OR_DESC_VIRT_ADDR_OR_ADDR_INFO_RETURN_BUFFER_MANAGER_LSB 8
#define TQM2SW_COMPLETION_RING_BUF_OR_DESC_VIRT_ADDR_OR_ADDR_INFO_RETURN_BUFFER_MANAGER_MSB 11
#define TQM2SW_COMPLETION_RING_BUF_OR_DESC_VIRT_ADDR_OR_ADDR_INFO_RETURN_BUFFER_MANAGER_MASK 0x00000f00

#define TQM2SW_COMPLETION_RING_BUF_OR_DESC_VIRT_ADDR_OR_ADDR_INFO_SW_BUFFER_COOKIE_OFFSET 0x00000004
#define TQM2SW_COMPLETION_RING_BUF_OR_DESC_VIRT_ADDR_OR_ADDR_INFO_SW_BUFFER_COOKIE_LSB 12
#define TQM2SW_COMPLETION_RING_BUF_OR_DESC_VIRT_ADDR_OR_ADDR_INFO_SW_BUFFER_COOKIE_MSB 31
#define TQM2SW_COMPLETION_RING_BUF_OR_DESC_VIRT_ADDR_OR_ADDR_INFO_SW_BUFFER_COOKIE_MASK 0xfffff000

#define TQM2SW_COMPLETION_RING_RELEASE_SOURCE_MODULE_OFFSET         0x00000008
#define TQM2SW_COMPLETION_RING_RELEASE_SOURCE_MODULE_LSB            0
#define TQM2SW_COMPLETION_RING_RELEASE_SOURCE_MODULE_MSB            2
#define TQM2SW_COMPLETION_RING_RELEASE_SOURCE_MODULE_MASK           0x00000007

#define TQM2SW_COMPLETION_RING_CACHE_ID_OFFSET                     0x00000008
#define TQM2SW_COMPLETION_RING_CACHE_ID_LSB                        3
#define TQM2SW_COMPLETION_RING_CACHE_ID_MSB                        3
#define TQM2SW_COMPLETION_RING_CACHE_ID_MASK                       0x00000008

#define TQM2SW_COMPLETION_RING_RESERVED_2A_OFFSET                  0x00000008
#define TQM2SW_COMPLETION_RING_RESERVED_2A_LSB                     4
#define TQM2SW_COMPLETION_RING_RESERVED_2A_MSB                     5
#define TQM2SW_COMPLETION_RING_RESERVED_2A_MASK                    0x00000030

#define TQM2SW_COMPLETION_RING_BUFFER_OR_DESC_TYPE_OFFSET          0x00000008
#define TQM2SW_COMPLETION_RING_BUFFER_OR_DESC_TYPE_LSB             6
#define TQM2SW_COMPLETION_RING_BUFFER_OR_DESC_TYPE_MSB             8
#define TQM2SW_COMPLETION_RING_BUFFER_OR_DESC_TYPE_MASK            0x000001c0

#define TQM2SW_COMPLETION_RING_RETURN_BUFFER_MANAGER_OFFSET        0x00000008
#define TQM2SW_COMPLETION_RING_RETURN_BUFFER_MANAGER_LSB           9
#define TQM2SW_COMPLETION_RING_RETURN_BUFFER_MANAGER_MSB           12
#define TQM2SW_COMPLETION_RING_RETURN_BUFFER_MANAGER_MASK          0x00001e00

#define TQM2SW_COMPLETION_RING_TQM_RELEASE_REASON_OFFSET           0x00000008
#define TQM2SW_COMPLETION_RING_TQM_RELEASE_REASON_LSB              13
#define TQM2SW_COMPLETION_RING_TQM_RELEASE_REASON_MSB              16
#define TQM2SW_COMPLETION_RING_TQM_RELEASE_REASON_MASK             0x0001e000

#define TQM2SW_COMPLETION_RING_RBM_OVERRIDE_VALID_OFFSET           0x00000008
#define TQM2SW_COMPLETION_RING_RBM_OVERRIDE_VALID_LSB              17
#define TQM2SW_COMPLETION_RING_RBM_OVERRIDE_VALID_MSB              17
#define TQM2SW_COMPLETION_RING_RBM_OVERRIDE_VALID_MASK             0x00020000

#define TQM2SW_COMPLETION_RING_SW_BUFFER_COOKIE_11_0_OFFSET        0x00000008
#define TQM2SW_COMPLETION_RING_SW_BUFFER_COOKIE_11_0_LSB           18
#define TQM2SW_COMPLETION_RING_SW_BUFFER_COOKIE_11_0_MSB           29
#define TQM2SW_COMPLETION_RING_SW_BUFFER_COOKIE_11_0_MASK          0x3ffc0000

#define TQM2SW_COMPLETION_RING_COOKIE_CONVERSION_STATUS_OFFSET     0x00000008
#define TQM2SW_COMPLETION_RING_COOKIE_CONVERSION_STATUS_LSB        30
#define TQM2SW_COMPLETION_RING_COOKIE_CONVERSION_STATUS_MSB        30
#define TQM2SW_COMPLETION_RING_COOKIE_CONVERSION_STATUS_MASK       0x40000000

#define TQM2SW_COMPLETION_RING_TQM_COMPLETION_ERROR_OFFSET         0x00000008
#define TQM2SW_COMPLETION_RING_TQM_COMPLETION_ERROR_LSB            31
#define TQM2SW_COMPLETION_RING_TQM_COMPLETION_ERROR_MSB            31
#define TQM2SW_COMPLETION_RING_TQM_COMPLETION_ERROR_MASK           0x80000000

#define TQM2SW_COMPLETION_RING_TQM_STATUS_NUMBER_OFFSET            0x0000000c
#define TQM2SW_COMPLETION_RING_TQM_STATUS_NUMBER_LSB               0
#define TQM2SW_COMPLETION_RING_TQM_STATUS_NUMBER_MSB               23
#define TQM2SW_COMPLETION_RING_TQM_STATUS_NUMBER_MASK              0x00ffffff

#define TQM2SW_COMPLETION_RING_TRANSMIT_COUNT_OFFSET               0x0000000c
#define TQM2SW_COMPLETION_RING_TRANSMIT_COUNT_LSB                  24
#define TQM2SW_COMPLETION_RING_TRANSMIT_COUNT_MSB                  30
#define TQM2SW_COMPLETION_RING_TRANSMIT_COUNT_MASK                 0x7f000000

#define TQM2SW_COMPLETION_RING_SW_RELEASE_DETAILS_VALID_OFFSET     0x0000000c
#define TQM2SW_COMPLETION_RING_SW_RELEASE_DETAILS_VALID_LSB        31
#define TQM2SW_COMPLETION_RING_SW_RELEASE_DETAILS_VALID_MSB        31
#define TQM2SW_COMPLETION_RING_SW_RELEASE_DETAILS_VALID_MASK       0x80000000

#define TQM2SW_COMPLETION_RING_ACK_FRAME_RSSI_OFFSET               0x00000010
#define TQM2SW_COMPLETION_RING_ACK_FRAME_RSSI_LSB                  0
#define TQM2SW_COMPLETION_RING_ACK_FRAME_RSSI_MSB                  7
#define TQM2SW_COMPLETION_RING_ACK_FRAME_RSSI_MASK                 0x000000ff

#define TQM2SW_COMPLETION_RING_FIRST_MSDU_OFFSET                   0x00000010
#define TQM2SW_COMPLETION_RING_FIRST_MSDU_LSB                      8
#define TQM2SW_COMPLETION_RING_FIRST_MSDU_MSB                      8
#define TQM2SW_COMPLETION_RING_FIRST_MSDU_MASK                     0x00000100

#define TQM2SW_COMPLETION_RING_LAST_MSDU_OFFSET                    0x00000010
#define TQM2SW_COMPLETION_RING_LAST_MSDU_LSB                       9
#define TQM2SW_COMPLETION_RING_LAST_MSDU_MSB                       9
#define TQM2SW_COMPLETION_RING_LAST_MSDU_MASK                      0x00000200

#define TQM2SW_COMPLETION_RING_FW_TX_NOTIFY_FRAME_OFFSET           0x00000010
#define TQM2SW_COMPLETION_RING_FW_TX_NOTIFY_FRAME_LSB              10
#define TQM2SW_COMPLETION_RING_FW_TX_NOTIFY_FRAME_MSB              12
#define TQM2SW_COMPLETION_RING_FW_TX_NOTIFY_FRAME_MASK             0x00001c00

#define TQM2SW_COMPLETION_RING_BUFFER_TIMESTAMP_OFFSET             0x00000010
#define TQM2SW_COMPLETION_RING_BUFFER_TIMESTAMP_LSB                13
#define TQM2SW_COMPLETION_RING_BUFFER_TIMESTAMP_MSB                31
#define TQM2SW_COMPLETION_RING_BUFFER_TIMESTAMP_MASK               0xffffe000

#define TQM2SW_COMPLETION_RING_TX_RATE_STATS_TX_RATE_STATS_INFO_VALID_OFFSET 0x00000014
#define TQM2SW_COMPLETION_RING_TX_RATE_STATS_TX_RATE_STATS_INFO_VALID_LSB  0
#define TQM2SW_COMPLETION_RING_TX_RATE_STATS_TX_RATE_STATS_INFO_VALID_MSB  0
#define TQM2SW_COMPLETION_RING_TX_RATE_STATS_TX_RATE_STATS_INFO_VALID_MASK 0x00000001

#define TQM2SW_COMPLETION_RING_TX_RATE_STATS_TRANSMIT_BW_OFFSET      0x00000014
#define TQM2SW_COMPLETION_RING_TX_RATE_STATS_TRANSMIT_BW_LSB         1
#define TQM2SW_COMPLETION_RING_TX_RATE_STATS_TRANSMIT_BW_MSB         3
#define TQM2SW_COMPLETION_RING_TX_RATE_STATS_TRANSMIT_BW_MASK        0x0000000e

#define TQM2SW_COMPLETION_RING_TX_RATE_STATS_TRANSMIT_PKT_TYPE_OFFSET 0x00000014
#define TQM2SW_COMPLETION_RING_TX_RATE_STATS_TRANSMIT_PKT_TYPE_LSB    4
#define TQM2SW_COMPLETION_RING_TX_RATE_STATS_TRANSMIT_PKT_TYPE_MSB    7
#define TQM2SW_COMPLETION_RING_TX_RATE_STATS_TRANSMIT_PKT_TYPE_MASK   0x000000f0

#define TQM2SW_COMPLETION_RING_TX_RATE_STATS_TRANSMIT_STBC_OFFSET    0x00000014
#define TQM2SW_COMPLETION_RING_TX_RATE_STATS_TRANSMIT_STBC_LSB       8
#define TQM2SW_COMPLETION_RING_TX_RATE_STATS_TRANSMIT_STBC_MSB       8
#define TQM2SW_COMPLETION_RING_TX_RATE_STATS_TRANSMIT_STBC_MASK      0x00000100

#define TQM2SW_COMPLETION_RING_TX_RATE_STATS_TRANSMIT_LDPC_OFFSET    0x00000014
#define TQM2SW_COMPLETION_RING_TX_RATE_STATS_TRANSMIT_LDPC_LSB       9
#define TQM2SW_COMPLETION_RING_TX_RATE_STATS_TRANSMIT_LDPC_MSB       9
#define TQM2SW_COMPLETION_RING_TX_RATE_STATS_TRANSMIT_LDPC_MASK      0x00000200

#define TQM2SW_COMPLETION_RING_TX_RATE_STATS_TRANSMIT_SGI_OFFSET     0x00000014
#define TQM2SW_COMPLETION_RING_TX_RATE_STATS_TRANSMIT_SGI_LSB        10
#define TQM2SW_COMPLETION_RING_TX_RATE_STATS_TRANSMIT_SGI_MSB        11
#define TQM2SW_COMPLETION_RING_TX_RATE_STATS_TRANSMIT_SGI_MASK       0x00000c00

#define TQM2SW_COMPLETION_RING_TX_RATE_STATS_TRANSMIT_MCS_OFFSET   0x00000014
#define TQM2SW_COMPLETION_RING_TX_RATE_STATS_TRANSMIT_MCS_LSB      12
#define TQM2SW_COMPLETION_RING_TX_RATE_STATS_TRANSMIT_MCS_MSB      16
#define TQM2SW_COMPLETION_RING_TX_RATE_STATS_TRANSMIT_MCS_MASK     0x0001f000

#define TQM2SW_COMPLETION_RING_TX_RATE_STATS_UNEQUAL_MODULATION_INFO_OFFSET 0x00000014
#define TQM2SW_COMPLETION_RING_TX_RATE_STATS_UNEQUAL_MODULATION_INFO_LSB   17
#define TQM2SW_COMPLETION_RING_TX_RATE_STATS_UNEQUAL_MODULATION_INFO_MSB   19
#define TQM2SW_COMPLETION_RING_TX_RATE_STATS_UNEQUAL_MODULATION_INFO_MASK  0x000e0000

#define TQM2SW_COMPLETION_RING_TX_RATE_STATS_OFDMA_TRANSMISSION_OFFSET 0x00000014
#define TQM2SW_COMPLETION_RING_TX_RATE_STATS_OFDMA_TRANSMISSION_LSB    20
#define TQM2SW_COMPLETION_RING_TX_RATE_STATS_OFDMA_TRANSMISSION_MSB    20
#define TQM2SW_COMPLETION_RING_TX_RATE_STATS_OFDMA_TRANSMISSION_MASK 0x00100000

#define TQM2SW_COMPLETION_RING_TX_RATE_STATS_RESERVED_0A_OFFSET  0x00000014
#define TQM2SW_COMPLETION_RING_TX_RATE_STATS_RESERVED_0A_LSB     21
#define TQM2SW_COMPLETION_RING_TX_RATE_STATS_RESERVED_0A_MSB     24
#define TQM2SW_COMPLETION_RING_TX_RATE_STATS_RESERVED_0A_MASK    0x01e00000

#define TQM2SW_COMPLETION_RING_TX_RATE_STATS_TONES_IN_RU_OFFSET  0x00000014
#define TQM2SW_COMPLETION_RING_TX_RATE_STATS_TONES_IN_RU_LSB     25
#define TQM2SW_COMPLETION_RING_TX_RATE_STATS_TONES_IN_RU_MSB     28
#define TQM2SW_COMPLETION_RING_TX_RATE_STATS_TONES_IN_RU_MASK    0x1e000000

#define TQM2SW_COMPLETION_RING_TX_RATE_STATS_TRANSMIT_NSS_OFFSET 0x00000014
#define TQM2SW_COMPLETION_RING_TX_RATE_STATS_TRANSMIT_NSS_LSB  29
#define TQM2SW_COMPLETION_RING_TX_RATE_STATS_TRANSMIT_NSS_MSB  31
#define TQM2SW_COMPLETION_RING_TX_RATE_STATS_TRANSMIT_NSS_MASK 0xe0000000

#define TQM2SW_COMPLETION_RING_TX_RATE_STATS_PPDU_TRANSMISSION_TSF_OFFSET 0x00000018
#define TQM2SW_COMPLETION_RING_TX_RATE_STATS_PPDU_TRANSMISSION_TSF_LSB 0
#define TQM2SW_COMPLETION_RING_TX_RATE_STATS_PPDU_TRANSMISSION_TSF_MSB 31
#define TQM2SW_COMPLETION_RING_TX_RATE_STATS_PPDU_TRANSMISSION_TSF_MASK 0xffffffff

#define TQM2SW_COMPLETION_RING_SW_PEER_ID_OFFSET  0x0000001c
#define TQM2SW_COMPLETION_RING_SW_PEER_ID_LSB     0
#define TQM2SW_COMPLETION_RING_SW_PEER_ID_MSB     15
#define TQM2SW_COMPLETION_RING_SW_PEER_ID_MASK    0x0000ffff

#define TQM2SW_COMPLETION_RING_TID_OFFSET     0x0000001c
#define TQM2SW_COMPLETION_RING_TID_LSB        16
#define TQM2SW_COMPLETION_RING_TID_MSB        19
#define TQM2SW_COMPLETION_RING_TID_MASK       0x000f0000

#define TQM2SW_COMPLETION_RING_SW_BUFFER_COOKIE_19_12_OFFSET 0x0000001c
#define TQM2SW_COMPLETION_RING_SW_BUFFER_COOKIE_19_12_LSB    20
#define TQM2SW_COMPLETION_RING_SW_BUFFER_COOKIE_19_12_MSB    27
#define TQM2SW_COMPLETION_RING_SW_BUFFER_COOKIE_19_12_MASK   0x0ff00000

#define TQM2SW_COMPLETION_RING_LOOPING_COUNT_OFFSET  0x0000001c
#define TQM2SW_COMPLETION_RING_LOOPING_COUNT_LSB     28
#define TQM2SW_COMPLETION_RING_LOOPING_COUNT_MSB     31
#define TQM2SW_COMPLETION_RING_LOOPING_COUNT_MASK    0xf0000000

#endif /* _DAL_VNDR_HAL_DEFINES_BN_H_ */
