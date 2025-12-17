/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: ISC
 */

#ifndef _DAL_VNDR_HAL_BE_H_
#define _DAL_VNDR_HAL_BE_H_
#include <linux/types.h>
#include "dal_vndr_hal_internal.h"

/* See  dal_vndr_hal_defines_be.h fo reference
 * DAL_VNDR_HAL_TX_DESC_LEN_DWORDS is NUM_OF_DWORDS_TCL_DATA_CMD
 * DAL_VNDR_HAL_TX_DESC_LEN_BYTES is 4*NUM_OF_DWORDS_TCL_DATA_CMD
 */
#define DAL_VNDR_HAL_TX_DESC_LEN_DWORDS (8)
#define DAL_VNDR_HAL_TX_DESC_LEN_BYTES  (DAL_VNDR_HAL_TX_DESC_LEN_DWORDS * 4)
#define DAL_VNDR_HAL_TX_COMP_DESC_LEN_DWORDS (8)
#define DAL_VNDR_HAL_TX_COMP_DESC_LEN_BYTES \
	(DAL_VNDR_HAL_TX_COMP_DESC_LEN_DWORDS * 4)

/**
 * dal_vndr_hal_default_ops_attach_be() - Add default ops for BE chips
 * @soc: hal_soc handle
 *
 * Return: None
 */
void dal_vndr_hal_default_ops_attach_be(struct dal_vndr_hal_soc *soc);

/**
 * struct dal_vndr_hal_tx_completion_status - HAL Tx completion descriptor
 * contents. For different architecture/target it can vary.
 * The fields of this struct are aligned to WBM2SW TX comp Desc to populate
 * them efficiently. Do not add/removed the fields of the struct.
 * @reserved_va1: reserved for VA
 * @reserved_va2: reserved for VA
 * @release_src: release source = TQM/FW
 * @reserved1: reserved
 * @status: frame acked/failed
 * @reserved2: reserved
 * @ppdu_id: TSF, snapshot of this value when transmission of the
 *           PPDU containing the frame finished.
 * @transmit_cnt: Number of times this frame has been transmitted
 * @reserved3: reserved
 * @ack_frame_rssi: RSSI of the received ACK or BA frame
 * @first_msdu: Indicates this MSDU is the first MSDU in AMSDU
 * @last_msdu: Indicates this MSDU is the last MSDU in AMSDU
 * @msdu_part_of_amsdu : Indicates this MSDU was part of an A-MSDU in MPDU
 * @reserved4: reserved
 * @buffer_timestamp: Frame system entrance timestamp in units of 1024
 *		      microseconds
 * @valid:
 * @bw: Indicates the BW of the upcoming transmission -
 *       <enum 0 transmit_bw_20_MHz>
 *       <enum 1 transmit_bw_40_MHz>
 *       <enum 2 transmit_bw_80_MHz>
 *       <enum 3 transmit_bw_160_MHz>
 *       <enum 4 transmit_bw_320_MHz>
 *       <enum 5 transmit_bw_240_MHz>
 * @pkt_type: Transmit Packet Type
 * @stbc: When set, STBC transmission rate was used
 * @ldpc: When set, use LDPC transmission rates
 * @sgi: <enum 0     0_8_us_sgi > Legacy normal GI
 *       <enum 1     0_4_us_sgi > Legacy short GI
 *       <enum 2     1_6_us_sgi > HE related GI
 *       <enum 3     3_2_us_sgi > HE
 * @mcs: Transmit MCS Rate
 * @ofdma: Set when the transmission was an OFDMA transmission
 * @reserved_bn: reserved
 * @tones_in_ru: The number of tones in the RU used.
 * @reserved5: reserved
 * @tsf: Lower 32 bits of the TSF
 * @peer_id: Peer ID of the flow or MPDU queue
 * @tid: TID of the flow or MPDU queue
 * @reserved6: reserved
 */
struct dal_vndr_hal_tx_completion_status {
	uint32_t reserved_va1;
	uint32_t reserved_va2;
	uint32_t release_src:3,
		 reserved1:10,
		 status:4,
		 reserved2:15;
	uint32_t ppdu_id:24,
		 transmit_cnt:7,
		 reserved3:1;
	uint32_t ack_frame_rssi:8,
		 first_msdu:1,
		 last_msdu:1,
		 msdu_part_of_amsdu:1,
		 reserved4:2,
		 buffer_timestamp:19;
	uint32_t valid:1,
		 bw:3,
		 pkt_type:4,
		 stbc:1,
		 ldpc:1,
		 sgi:2,
		 mcs:4,
		 ofdma:1,
		 tones_in_ru:12,

		 reserved5:3;
	uint32_t tsf;
	uint32_t peer_id:16,
		 tid:8,
		 reserved6:8;
};
#endif /* _DAL_VNDR_HAL_BE_H_ */
