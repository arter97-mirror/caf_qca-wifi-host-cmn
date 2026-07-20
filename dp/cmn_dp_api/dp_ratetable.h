/*
 * Copyright (c) 2016-2021 The Linux Foundation. All rights reserved.
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * Permission to use, copy, modify, and/or distribute this software for
 * any purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all
 * copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _DP_RATES_H_
#define _DP_RATES_H_

#define CMN_DP_ASSERT(__bool)

/*
 * Modes Types
 */
enum CMN_MODE_TYPES {
	CMN_IEEE80211_MODE_INVALID = 0,
	CMN_IEEE80211_MODE_A,
	CMN_IEEE80211_MODE_B,
	CMN_IEEE80211_MODE_G,
	CMN_IEEE80211_MODE_TURBO,
	CMN_IEEE80211_MODE_NA,
	CMN_IEEE80211_MODE_NG,
	CMN_IEEE80211_MODE_N,
	CMN_IEEE80211_MODE_AC,
	CMN_IEEE80211_MODE_AXA,
	CMN_IEEE80211_MODE_AXG,
	CMN_IEEE80211_MODE_AX,
#ifdef WLAN_FEATURE_11BE
	CMN_IEEE80211_MODE_BEA,
	CMN_IEEE80211_MODE_BEG,
#endif
#ifdef WLAN_FEATURE_11BN
	CMN_IEEE80211_MODE_BNA,
	CMN_IEEE80211_MODE_BNG,
#endif
	CMN_IEEE80211_MODE_MAX
};

#define NUM_SPATIAL_STREAMS 8
#define MAX_SPATIAL_STREAMS_SUPPORTED_AT_160MHZ 4
#define VHT_EXTRA_MCS_SUPPORT
#define CONFIG_160MHZ_SUPPORT 1
#define NUM_HT_MCS 8
#define NUM_VHT_MCS 12
#define RT_INVALID_INDEX (0xff)

#define NUM_HE_MCS 14
#if defined(WLAN_FEATURE_11BE) || defined(WLAN_FEATURE_11BN)
#define NUM_EHT_MCS 16
#ifdef WLAN_FEATURE_11BN
#define NUM_UHR_MCS 20
/*
 * Max wire MCS value for UHR (Intermediate MCS 23 is the highest).
 * Used to size the mcs-to-table-offset mapping array.
 */
#define UHR_MCS_MAX 24
/*
 * uhr_mcs_to_tbl_idx[] maps a wire MCS index to its offset within one
 * NSS block of the UHR rate table.  The table is laid out as:
 *   [0]=MCS14 [1]=MCS15 [2]=MCS0 [3]=MCS1 [4]=IMCS17 [5]=MCS2 [6]=MCS3
 *   [7]=IMCS19 [8]=MCS4 [9]=IMCS20 [10]=MCS5 [11]=MCS6 [12]=MCS7
 *   [13]=IMCS23 [14]=MCS8 [15]=MCS9 [16]=MCS10 [17]=MCS11 [18]=MCS12
 *   [19]=MCS13
 * RT_INVALID_INDEX (0xff) marks wire MCS values that have no table entry.
 */
#define UHR_MCS_TBL_IDX_INVALID RT_INVALID_INDEX
static const uint8_t uhr_mcs_to_tbl_idx[UHR_MCS_MAX] = {
	2,  /* MCS  0 */
	3,  /* MCS  1 */
	5,  /* MCS  2 */
	6,  /* MCS  3 */
	8,  /* MCS  4 */
	10, /* MCS  5 */
	11, /* MCS  6 */
	12, /* MCS  7 */
	14, /* MCS  8 */
	15, /* MCS  9 */
	16, /* MCS 10 */
	17, /* MCS 11 */
	18, /* MCS 12 */
	19, /* MCS 13 */
	0,  /* MCS 14 */
	1,  /* MCS 15 */
	UHR_MCS_TBL_IDX_INVALID, /* MCS 16 - unused */
	4,  /* MCS 17 (IMCS) */
	UHR_MCS_TBL_IDX_INVALID, /* MCS 18 - unused */
	7,  /* MCS 19 (IMCS) */
	9,  /* MCS 20 (IMCS) */
	UHR_MCS_TBL_IDX_INVALID, /* MCS 21 - unused */
	UHR_MCS_TBL_IDX_INVALID, /* MCS 22 - unused */
	13, /* MCS 23 (IMCS) */
};
#else
#define NUM_UHR_MCS 16
#endif
#endif

#define NUM_SPATIAL_STREAM 4
#define NUM_SPATIAL_STREAMS 8
#define WHAL_160MHZ_SUPPORT 1
#define MAX_SPATIAL_STREAMS_SUPPORTED_AT_160MHZ 4
#define RT_GET_RT(_rt)		    ((const struct DP_CMN_RATE_TABLE *)(_rt))
#define RT_GET_INFO(_rt, _index)	    RT_GET_RT(_rt)->info[(_index)]
#define RT_GET_RAW_KBPS(_rt, _index) \
	(RT_GET_INFO(_rt, (_index)).ratekbps)
#define RT_GET_SGI_KBPS(_rt, _index) \
	(RT_GET_INFO(_rt, (_index)).ratekbpssgi)

#define HW_RATECODE_CCK_SHORT_PREAM_MASK  0x4
/* pow2 to optimize out * and / */
#define DP_ATH_RATE_EP_MULTIPLIER     BIT(7)
#define DP_ATH_EP_MUL(a, b)	      ((a) * (b))
#define DP_ATH_RATE_LPF_LEN	      10	  /* Low pass filter length
						   * for averaging rates
						   */
#define DUMMY_MARKER	  0
#define DP_ATH_RATE_IN(c)  (DP_ATH_EP_MUL((c), DP_ATH_RATE_EP_MULTIPLIER))

static inline int dp_ath_rate_lpf(uint64_t _d, int _e)
{
	_e = DP_ATH_RATE_IN((_e));
	return (((_d) != DUMMY_MARKER) ? ((((_d) << 3) + (_e) - (_d)) >> 3) :
			(_e));
}

static inline int dp_ath_rate_out(uint64_t _i)
{
	int _mul = DP_ATH_RATE_EP_MULTIPLIER;

	return (((_i) != DUMMY_MARKER) ?
			((((_i) % (_mul)) >= ((_mul) / 2)) ?
			((_i) + ((_mul) - 1)) / (_mul) : (_i) / (_mul)) :
				DUMMY_MARKER);
}

#define RXDESC_GET_DATA_LEN(rx_desc) \
	(txrx_pdev->htt_pdev->ar_rx_ops->msdu_desc_msdu_length(rx_desc))
#define ASSEMBLE_HW_RATECODE(_rate, _nss, _pream)     \
	(((_pream) << 6) | ((_nss) << 4) | (_rate))
#define GET_HW_RATECODE_PREAM(_rcode)     (((_rcode) >> 6) & 0x3)
#define GET_HW_RATECODE_NSS(_rcode)       (((_rcode) >> 4) & 0x3)
#define GET_HW_RATECODE_RATE(_rcode)      (((_rcode) >> 0) & 0xF)

#define VHT_INVALID_MCS    (0xFF)  /* Certain MCSs are not valid in VHT mode */
#define VHT_INVALID_BCC_RATE  0
#define NUM_HT_SPATIAL_STREAM 4

#define NUM_HT_RIX_PER_BW (NUM_HT_MCS * NUM_HT_SPATIAL_STREAM)
#define NUM_VHT_RIX_PER_BW (NUM_VHT_MCS * NUM_SPATIAL_STREAMS)
#define NUM_HE_RIX_PER_BW (NUM_HE_MCS * NUM_SPATIAL_STREAMS)

#define NUM_VHT_RIX_FOR_160MHZ (NUM_VHT_MCS * \
		MAX_SPATIAL_STREAMS_SUPPORTED_AT_160MHZ)
#define NUM_HE_RIX_FOR_160MHZ (NUM_HE_MCS * \
		MAX_SPATIAL_STREAMS_SUPPORTED_AT_160MHZ)

#define CCK_RATE_TABLE_INDEX 0
#define CCK_RATE_TABLE_END_INDEX 3
#define CCK_RATE_11M_INDEX 0
#define CCK_FALLBACK_MIN_RATE 0x3 /** 1 Mbps */
#define CCK_FALLBACK_MAX_RATE 0x2 /** 2 Mbps */

#define OFDM_RATE_TABLE_INDEX 4
#define OFDMA_RATE_54M_INDEX 8
#define OFDMA_RATE_TABLE_END_INDEX 11

#define HT_20_RATE_TABLE_INDEX 12
#define HT_40_RATE_TABLE_INDEX (HT_20_RATE_TABLE_INDEX + NUM_HT_RIX_PER_BW)

#define VHT_20_RATE_TABLE_INDEX (HT_40_RATE_TABLE_INDEX + NUM_HT_RIX_PER_BW)
#define VHT_40_RATE_TABLE_INDEX (VHT_20_RATE_TABLE_INDEX + NUM_VHT_RIX_PER_BW)
#define VHT_80_RATE_TABLE_INDEX (VHT_40_RATE_TABLE_INDEX + NUM_VHT_RIX_PER_BW)

#define VHT_160_RATE_TABLE_INDEX (VHT_80_RATE_TABLE_INDEX + NUM_VHT_RIX_PER_BW)
#define VHT_LAST_RIX_PLUS_ONE (VHT_160_RATE_TABLE_INDEX + \
		NUM_VHT_RIX_FOR_160MHZ)

#define HE_20_RATE_TABLE_INDEX VHT_LAST_RIX_PLUS_ONE
#define HE_40_RATE_TABLE_INDEX (HE_20_RATE_TABLE_INDEX + NUM_HE_RIX_PER_BW)
#define HE_80_RATE_TABLE_INDEX (HE_40_RATE_TABLE_INDEX + NUM_HE_RIX_PER_BW)

#define HE_160_RATE_TABLE_INDEX (HE_80_RATE_TABLE_INDEX + NUM_HE_RIX_PER_BW)
#define HE_LAST_RIX_PLUS_ONE (HE_160_RATE_TABLE_INDEX + NUM_HE_RIX_FOR_160MHZ)

#ifdef WLAN_FEATURE_11BE
#define NUM_EHT_SPATIAL_STREAM 4
#define NUM_EHT_RIX_PER_BW (NUM_EHT_MCS * NUM_EHT_SPATIAL_STREAM)

#define EHT_20_RATE_TABLE_INDEX HE_LAST_RIX_PLUS_ONE
#define EHT_40_RATE_TABLE_INDEX (EHT_20_RATE_TABLE_INDEX + NUM_EHT_RIX_PER_BW)
#define EHT_60_RATE_TABLE_INDEX (EHT_40_RATE_TABLE_INDEX + NUM_EHT_RIX_PER_BW)
#define EHT_80_RATE_TABLE_INDEX (EHT_60_RATE_TABLE_INDEX + NUM_EHT_RIX_PER_BW)
#define EHT_120_RATE_TABLE_INDEX (EHT_80_RATE_TABLE_INDEX + NUM_EHT_RIX_PER_BW)
#define EHT_140_RATE_TABLE_INDEX (EHT_120_RATE_TABLE_INDEX + NUM_EHT_RIX_PER_BW)
#define EHT_160_RATE_TABLE_INDEX (EHT_140_RATE_TABLE_INDEX + NUM_EHT_RIX_PER_BW)
#define EHT_200_RATE_TABLE_INDEX (EHT_160_RATE_TABLE_INDEX + NUM_EHT_RIX_PER_BW)
#define EHT_240_RATE_TABLE_INDEX (EHT_200_RATE_TABLE_INDEX + NUM_EHT_RIX_PER_BW)
#define EHT_280_RATE_TABLE_INDEX (EHT_240_RATE_TABLE_INDEX + NUM_EHT_RIX_PER_BW)
#define EHT_320_RATE_TABLE_INDEX (EHT_280_RATE_TABLE_INDEX + NUM_EHT_RIX_PER_BW)
#define EHT_LAST_RIX_PLUS_ONE (EHT_320_RATE_TABLE_INDEX + NUM_EHT_RIX_PER_BW)
#endif

#ifdef WLAN_FEATURE_11BN
#define NUM_UHR_SPATIAL_STREAM 4
#define NUM_UHR_RIX_PER_BW (NUM_UHR_MCS * NUM_UHR_SPATIAL_STREAM)

#define UHR_20_RATE_TABLE_INDEX EHT_LAST_RIX_PLUS_ONE
#define UHR_40_RATE_TABLE_INDEX (UHR_20_RATE_TABLE_INDEX + NUM_UHR_RIX_PER_BW)
#define UHR_60_RATE_TABLE_INDEX (UHR_40_RATE_TABLE_INDEX + NUM_UHR_RIX_PER_BW)
#define UHR_80_RATE_TABLE_INDEX (UHR_60_RATE_TABLE_INDEX + NUM_UHR_RIX_PER_BW)
#define UHR_120_RATE_TABLE_INDEX (UHR_80_RATE_TABLE_INDEX + NUM_UHR_RIX_PER_BW)
#define UHR_140_RATE_TABLE_INDEX (UHR_120_RATE_TABLE_INDEX + NUM_UHR_RIX_PER_BW)
#define UHR_160_RATE_TABLE_INDEX (UHR_140_RATE_TABLE_INDEX + NUM_UHR_RIX_PER_BW)
#define UHR_200_RATE_TABLE_INDEX (UHR_160_RATE_TABLE_INDEX + NUM_UHR_RIX_PER_BW)
#define UHR_240_RATE_TABLE_INDEX (UHR_200_RATE_TABLE_INDEX + NUM_UHR_RIX_PER_BW)
#define UHR_280_RATE_TABLE_INDEX (UHR_240_RATE_TABLE_INDEX + NUM_UHR_RIX_PER_BW)
#define UHR_320_RATE_TABLE_INDEX (UHR_280_RATE_TABLE_INDEX + NUM_UHR_RIX_PER_BW)
#define UHR_LAST_RIX_PLUS_ONE (UHR_320_RATE_TABLE_INDEX + NUM_UHR_RIX_PER_BW)
#endif

#ifdef WLAN_FEATURE_11BN
#define DP_RATE_TABLE_SIZE UHR_LAST_RIX_PLUS_ONE
#elif defined(WLAN_FEATURE_11BE)
#define DP_RATE_TABLE_SIZE EHT_LAST_RIX_PLUS_ONE
#else
#define DP_RATE_TABLE_SIZE HE_LAST_RIX_PLUS_ONE
#endif

#define INVALID_RATE_ERR -1
#define NUM_LEGACY_MCS 1

/*
 * The order of the rate types are jumbled below since the current code
 * implementation is mapped in such way already.
 *
 * @DP_HT_RATE: HT Ratetype
 * @DP_VHT_RATE: VHT Ratetype
 * @DP_11B_CCK_RATE: 11B CCK Ratetype
 * @DP_11A_OFDM_RATE: 11A OFDM Ratetype
 * @DP_11G_CCK_OFDM_RATE: 11G CCK + OFDM Ratetype
 * @DP_HE_RATE: HE Ratetype
 */
enum DP_CMN_RATE_TYPE {
	DP_HT_RATE = 2,
	DP_VHT_RATE,
	DP_11B_CCK_RATE,
	DP_11A_OFDM_RATE,
	DP_11G_CCK_OFDM_RATE,
	DP_HE_RATE
};

#define DP_RATEKBPS_SGI(i) (dp_11abgnratetable.info[i].ratekbpssgi)
#define DP_RATEKBPS(i) (dp_11abgnratetable.info[i].ratekbps)
#define RATE_ROUNDOUT(rate) (((rate) / 1000) * 1000)

/* The following would span more than one octet
 * when 160MHz BW defined for VHT
 * Also it's important to maintain the ordering of
 * this enum else it would break other rate adaptation functions.
 */
enum DP_CMN_MODULATION_TYPE {
	   DP_CMN_MOD_IEEE80211_T_DS,   /* direct sequence spread spectrum */
	   DP_CMN_MOD_IEEE80211_T_OFDM, /* frequency division multiplexing */
	   DP_CMN_MOD_IEEE80211_T_HT_20,
	   DP_CMN_MOD_IEEE80211_T_HT_40,
	   DP_CMN_MOD_IEEE80211_T_VHT_20,
	   DP_CMN_MOD_IEEE80211_T_VHT_40,
	   DP_CMN_MOD_IEEE80211_T_VHT_80,
	   DP_CMN_MOD_IEEE80211_T_VHT_160,
	   DP_CMN_MOD_IEEE80211_T_HE_20, /* 11AX support enabled */
	   DP_CMN_MOD_IEEE80211_T_HE_40,
	   DP_CMN_MOD_IEEE80211_T_HE_80,
	   DP_CMN_MOD_IEEE80211_T_HE_160,
#ifdef WLAN_FEATURE_11BE
	   DP_CMN_MOD_IEEE80211_T_EHT_20,
	   DP_CMN_MOD_IEEE80211_T_EHT_40,
	   DP_CMN_MOD_IEEE80211_T_EHT_60,
	   DP_CMN_MOD_IEEE80211_T_EHT_80,
	   DP_CMN_MOD_IEEE80211_T_EHT_120,
	   DP_CMN_MOD_IEEE80211_T_EHT_140,
	   DP_CMN_MOD_IEEE80211_T_EHT_160,
	   DP_CMN_MOD_IEEE80211_T_EHT_200,
	   DP_CMN_MOD_IEEE80211_T_EHT_240,
	   DP_CMN_MOD_IEEE80211_T_EHT_280,
	   DP_CMN_MOD_IEEE80211_T_EHT_320,
#endif

#ifdef WLAN_FEATURE_11BN
	DP_CMN_MOD_IEEE80211_T_UHR_20,
	DP_CMN_MOD_IEEE80211_T_UHR_40,
	DP_CMN_MOD_IEEE80211_T_UHR_60,
	DP_CMN_MOD_IEEE80211_T_UHR_80,
	DP_CMN_MOD_IEEE80211_T_UHR_120,
	DP_CMN_MOD_IEEE80211_T_UHR_140,
	DP_CMN_MOD_IEEE80211_T_UHR_160,
	DP_CMN_MOD_IEEE80211_T_UHR_200,
	DP_CMN_MOD_IEEE80211_T_UHR_240,
	DP_CMN_MOD_IEEE80211_T_UHR_280,
	DP_CMN_MOD_IEEE80211_T_UHR_320,
#endif /* WLAN_FEATURE_11BN */
	   DP_CMN_MOD_IEEE80211_T_MAX_PHY
};

/* more common nomenclature */
#define DP_CMN_MOD_IEEE80211_T_CCK DP_CMN_MOD_IEEE80211_T_DS

enum HW_RATECODE_PREAM_TYPE {
	HW_RATECODE_PREAM_OFDM,
	HW_RATECODE_PREAM_CCK,
	HW_RATECODE_PREAM_HT,
	HW_RATECODE_PREAM_VHT,
	HW_RATECODE_PREAM_HE,
#ifdef WLAN_FEATURE_11BE
	HW_RATECODE_PREAM_EHT,
#endif
#ifdef WLAN_FEATURE_11BN
	HW_RATECODE_PREAM_UHR,
#endif
};

#if defined(WLAN_FEATURE_11BE) || defined(WLAN_FEATURE_11BN)
enum BW_TYPES_FP {
	BW_20MHZ_F = 0,
	BW_40MHZ_F,
	BW_60MHZ_P,
	BW_80MHZ_F,
	BW_120MHZ_P,
	BW_140MHZ_P,
	BW_160MHZ_F,
	BW_200MHZ_P,
	BW_240MHZ_P,
	BW_280MHZ_P,
	BW_320MHZ_F,
	BW_FP_CNT,
	BW_FP_LAST = BW_320MHZ_F,
};
#endif

enum DP_CMN_MODULATION_TYPE dp_getmodulation(uint16_t pream_type,
					     uint8_t width,
					     uint8_t punc_mode);

uint32_t
dp_getrateindex(uint32_t gi, uint16_t mcs, uint8_t nss, uint8_t preamble,
		uint8_t bw, uint8_t punc_bw, uint32_t *rix, uint16_t *ratecode);

int dp_rate_idx_to_kbps(uint8_t rate_idx, uint8_t gintval);

#if ALL_POSSIBLE_RATES_SUPPORTED
int dp_get_supported_rates(int mode, int shortgi, int **rates);
int dp_get_kbps_to_mcs(int kbps_rate, int shortgi, int htflag);
#else
int dp_get_supported_rates(int mode, int shortgi, int nss,
			   int ch_width, int **rates);
int dp_get_kbps_to_mcs(int kbps_rate, int shortgi, int htflag,
		       int nss, int ch_width);
#endif

#endif /*_DP_RATES_H_*/
