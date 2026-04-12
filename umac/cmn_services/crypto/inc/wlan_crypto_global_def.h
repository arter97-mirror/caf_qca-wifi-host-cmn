/*
 * Copyright (c) 2017-2021 The Linux Foundation. All rights reserved.
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

/**
 * DOC: Public definitions  for crypto service
 */

#ifndef _WLAN_CRYPTO_GLOBAL_DEF_H_
#define _WLAN_CRYPTO_GLOBAL_DEF_H_

#include <wlan_cmn.h>
#ifdef WLAN_CRYPTO_SUPPORT_FILS
#include "wlan_crypto_fils_def.h"
#endif
#include <wlan_objmgr_cmn.h>
#include <wlan_cmn_ieee80211.h>

#define WLAN_CRYPTO_TID_SIZE         (17)
#define WLAN_CRYPTO_RSC_SIZE         (16)
#define WLAN_CRYPTO_KEYBUF_SIZE      (32)
#define WLAN_CRYPTO_MICBUF_SIZE      (16)
#define WLAN_CRYPTO_MIC_LEN          (8)
#define WLAN_CRYPTO_IV_SIZE          (16)
#define WLAN_CRYPTO_MIC256_LEN       (16)
#define WLAN_CRYPTO_TXMIC_OFFSET     (0)
#define WLAN_CRYPTO_RXMIC_OFFSET     (WLAN_CRYPTO_TXMIC_OFFSET + \
					WLAN_CRYPTO_MIC_LEN)
#define WLAN_CRYPTO_WAPI_IV_SIZE     (16)
#define WLAN_CRYPTO_CRC_LEN          (4)
#define WLAN_CRYPTO_IV_LEN           (3)
#define WLAN_CRYPTO_KEYID_LEN        (1)
#define WLAN_CRYPTO_EXT_IV_LEN       (4)
#define WLAN_CRYPTO_EXT_IV_BIT       (0x20)
#define WLAN_CRYPTO_KEYIX_NONE       ((uint16_t)-1)
#define WLAN_CRYPTO_MAXKEYIDX        (4)
#define WLAN_CRYPTO_MAXIGTKKEYIDX    (2)
#define WLAN_CRYPTO_MAXBIGTKKEYIDX   (2)
#ifndef WLAN_CRYPTO_MAX_VLANKEYIX
#define WLAN_CRYPTO_MAX_VLANKEYIX    WLAN_CRYPTO_MAXKEYIDX
#endif
#define WLAN_CRYPTO_MAX_PMKID        (16)
#define WLAN_CRYPTO_TOTAL_KEYIDX     (WLAN_CRYPTO_MAXKEYIDX + \
					WLAN_CRYPTO_MAXIGTKKEYIDX + \
					WLAN_CRYPTO_MAXBIGTKKEYIDX)
/* 40 bit wep key len */
#define WLAN_CRYPTO_KEY_WEP40_LEN    (5)
/* 104 bit wep key len */
#define WLAN_CRYPTO_KEY_WEP104_LEN   (13)
/* 128 bit wep key len */
#define WLAN_CRYPTO_KEY_WEP128_LEN   (16)

#define WLAN_CRYPTO_KEY_TKIP_LEN     (32)
#define WLAN_CRYPTO_KEY_CCMP_LEN     (16)
#define WLAN_CRYPTO_KEY_CCMP_256_LEN (32)
#define WLAN_CRYPTO_KEY_GCMP_LEN     (16)
#define WLAN_CRYPTO_KEY_GCMP_256_LEN (32)
#define WLAN_CRYPTO_KEY_WAPI_LEN     (32)
#define WLAN_CRYPTO_KEY_GMAC_LEN     (16)
#define WLAN_CRYPTO_KEY_GMAC_256_LEN (32)
#define WLAN_CRYPTO_WPI_SMS4_IVLEN   (16)
#define WLAN_CRYPTO_WPI_SMS4_KIDLEN  (1)
#define WLAN_CRYPTO_WPI_SMS4_PADLEN  (1)
#define WLAN_CRYPTO_WPI_SMS4_MICLEN  (16)

/* FILS definitions */
#define WLAN_CRYPTO_FILS_OPTIONAL_DATA_LEN 3
#define WLAN_CRYPTO_FILS_RIK_LABEL "Re-authentication Integrity Key@ietf.org"

/* key used for xmit */
#define WLAN_CRYPTO_KEY_XMIT         (0x01)
/* key used for recv */
#define WLAN_CRYPTO_KEY_RECV         (0x02)
/* key used for WPA group operation */
#define WLAN_CRYPTO_KEY_GROUP        (0x04)
/* key also used for management frames */
#define WLAN_CRYPTO_KEY_MFP          (0x08)
/* host-based encryption */
#define WLAN_CRYPTO_KEY_SWENCRYPT    (0x10)
/* host-based enmic */
#define WLAN_CRYPTO_KEY_SWENMIC      (0x20)
/* do not remove unless OS commands us to do so */
#define WLAN_CRYPTO_KEY_PERSISTENT   (0x40)
/* per STA default key */
#define WLAN_CRYPTO_KEY_DEFAULT      (0x80)
/* host-based decryption */
#define WLAN_CRYPTO_KEY_SWDECRYPT    (0x100)
/* host-based demic */
#define WLAN_CRYPTO_KEY_SWDEMIC      (0x200)
/* get pn from fw for key */
#define WLAN_CRYPTO_KEY_GET_PN       (0x400)

#define WLAN_CRYPTO_KEY_SWCRYPT      (WLAN_CRYPTO_KEY_SWENCRYPT \
						| WLAN_CRYPTO_KEY_SWDECRYPT)

#define WLAN_CRYPTO_KEY_SWMIC        (WLAN_CRYPTO_KEY_SWENMIC \
						| WLAN_CRYPTO_KEY_SWDEMIC)

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 12, 0)
#define WLAN_AKM_SUITE_FT_8021X         0x000FAC03
#define WLAN_AKM_SUITE_FT_PSK           0x000FAC04
#endif

/* Maximum lifetime for a PMKID entry - 12 Hrs */
#define WLAN_CRYPTO_MAX_PMKID_LIFETIME 43200
#define WLAN_CRYPTO_MAX_PMKID_LIFETIME_THRESHOLD 100

/*
 * Cipher types
 */
typedef enum wlan_crypto_cipher_type {
	WLAN_CRYPTO_CIPHER_WEP             = 0,
	WLAN_CRYPTO_CIPHER_TKIP            = 1,
	WLAN_CRYPTO_CIPHER_AES_OCB         = 2,
	WLAN_CRYPTO_CIPHER_AES_CCM         = 3,
	WLAN_CRYPTO_CIPHER_WAPI_SMS4       = 4,
	WLAN_CRYPTO_CIPHER_CKIP            = 5,
	WLAN_CRYPTO_CIPHER_AES_CMAC        = 6,
	WLAN_CRYPTO_CIPHER_AES_CCM_256     = 7,
	WLAN_CRYPTO_CIPHER_AES_CMAC_256    = 8,
	WLAN_CRYPTO_CIPHER_AES_GCM         = 9,
	WLAN_CRYPTO_CIPHER_AES_GCM_256     = 10,
	WLAN_CRYPTO_CIPHER_AES_GMAC        = 11,
	WLAN_CRYPTO_CIPHER_AES_GMAC_256    = 12,
	WLAN_CRYPTO_CIPHER_WAPI_GCM4       = 13,
	WLAN_CRYPTO_CIPHER_FILS_AEAD       = 14,
	WLAN_CRYPTO_CIPHER_WEP_40          = 15,
	WLAN_CRYPTO_CIPHER_WEP_104         = 16,
	WLAN_CRYPTO_CIPHER_NONE            = 17,
	WLAN_CRYPTO_CIPHER_MAX             = (WLAN_CRYPTO_CIPHER_NONE + 1),
	WLAN_CRYPTO_CIPHER_INVALID,
} wlan_crypto_cipher_type;

/* Auth types */
typedef enum wlan_crypto_auth_mode {
	WLAN_CRYPTO_AUTH_NONE          = 0,
	WLAN_CRYPTO_AUTH_OPEN          = 1,
	WLAN_CRYPTO_AUTH_SHARED        = 2,
	WLAN_CRYPTO_AUTH_8021X         = 3,
	WLAN_CRYPTO_AUTH_AUTO          = 4,
	WLAN_CRYPTO_AUTH_WPA           = 5,
	WLAN_CRYPTO_AUTH_RSNA          = 6,
	WLAN_CRYPTO_AUTH_CCKM          = 7,
	WLAN_CRYPTO_AUTH_WAPI          = 8,
	WLAN_CRYPTO_AUTH_SAE           = 9,
	WLAN_CRYPTO_AUTH_FILS_SK       = 10,
	WLAN_CRYPTO_AUTH_EPPKE         = 11,
	WLAN_CRYPTO_AUTH_8021X_IN_AUTH = 12,
	/** Keep WLAN_CRYPTO_AUTH_MAX at the end. */
	WLAN_CRYPTO_AUTH_MAX,
} wlan_crypto_auth_mode;

/* crypto capabilities */
typedef enum wlan_crypto_cap {
	WLAN_CRYPTO_CAP_PRIVACY          = 0,
	WLAN_CRYPTO_CAP_WPA1             = 1,
	WLAN_CRYPTO_CAP_WPA2             = 2,
	WLAN_CRYPTO_CAP_WPA              = 3,
	WLAN_CRYPTO_CAP_AES              = 4,
	WLAN_CRYPTO_CAP_WEP              = 5,
	WLAN_CRYPTO_CAP_CKIP             = 6,
	WLAN_CRYPTO_CAP_TKIP_MIC         = 7,
	WLAN_CRYPTO_CAP_CCM256           = 8,
	WLAN_CRYPTO_CAP_GCM              = 9,
	WLAN_CRYPTO_CAP_GCM_256          = 10,
	WLAN_CRYPTO_CAP_WAPI_SMS4        = 11,
	WLAN_CRYPTO_CAP_WAPI_GCM4        = 12,
	WLAN_CRYPTO_CAP_KEY_MGMT_OFFLOAD = 13,
	WLAN_CRYPTO_CAP_PMF_OFFLOAD      = 14,
	WLAN_CRYPTO_CAP_PN_TID_BASED     = 15,
	WLAN_CRYPTO_CAP_FILS_AEAD        = 16,
} wlan_crypto_cap;

typedef enum wlan_crypto_rsn_cap {
	WLAN_CRYPTO_RSN_CAP_PREAUTH       = 0x01,
	WLAN_CRYPTO_RSN_CAP_MFP_ENABLED   = 0x80,
	WLAN_CRYPTO_RSN_CAP_MFP_REQUIRED  = 0x40,
	WLAN_CRYPTO_RSN_CAP_OCV_SUPPORTED  = 0x4000,
} wlan_crypto_rsn_cap;

/**
 * enum wlan_crypto_rsnx_cap - The RSNX Capabilities field encodes optional
 * security capabilities. Bits 0–3 hold a 4-bit length subfield (n - 1).
 * Remaining bits are flags.
 *
 * @WLAN_CRYPTO_RSNX_CAP_LEN_MASK: Bits 0–3 mask for (n - 1) length subfield.
 * @WLAN_CRYPTO_RSNX_CAP_LEN_SHIFT: Shift for the length subfield (0).
 *
 * @WLAN_CRYPTO_RSNX_CAP_TWT_PROT: Protected TWT Operations Support.
 *   Set if STA supports protected TWT operations (per 10.46.1).
 *
 * @WLAN_CRYPTO_RSNX_CAP_SAE_H2E: SAE Hash-to-Element.
 *   Set if STA supports SAE hash-to-element method (12.4.4.2.3/12.4.4.3.3).
 *
 * @WLAN_CRYPTO_RSNX_CAP_SAE_PK: SAE PK - Wi-Fi Alliance allocated.
 *
 * @WLAN_CRYPTO_RSNX_CAP_WUR_PROT: Protected WUR Frame Support.
 *   Set when 11RSNAWURFrameProtectionActivated is true (11.21.6.3.1).
 *
 * @WLAN_CRYPTO_RSNX_CAP_SECURE_LTF: Secure HE-LTF Support.
 *   Set when dot11SecureLTFImplemented is true (11.21.6.4.5).
 *
 * @WLAN_CRYPTO_RSNX_CAP_SECURE_RTT: Secure RTT Support.
 *   Set if STA supports Secure RTT Measurement exchange (11.21.6.4.2.7).
 *
 * @WLAN_CRYPTO_RSNX_CAP_URNM_MFPR_X20: Unassociated Range Negotiation and
 *   Measurement MFP Required Exempt 20 MHz.
 *   Set when dot11RSTARequiresPMFActivated is set to 1 for 20 MHz exempt case
 *   (11.21.6.3.1, Annex C.3).
 *
 * @WLAN_CRYPTO_RSNX_CAP_PROT_ANNOUNCE: Protected Announce Support.
 *   Non-EDMG STA sets when dot11ProtectedAnnounceImplemented is true (12.6.18).
 *
 * @WLAN_CRYPTO_RSNX_CAP_PBAC: PBAC (Policy-Based Admission Control).
 *   Set when PBAC is enabled to protect block ack agreements (11.16).
 *
 * @WLAN_CRYPTO_RSNX_CAP_EXT_SIG_ACT_PROT: Extended SIG Action Protection.
 *   Set when Extended SIG Action Protection operations are implemented.
 *
 * @WLAN_CRYPTO_RSNX_CAP_SPP_AMSDU: SPP AMSDU Capable.
 *   Non‑DMG STA sets when dot11SPPAMSDUCapable is true (10.11).
 *
 * @WLAN_CRYPTO_RSNX_CAP_URNM_MFPR: Unassociated Range Negotiation and
 *   Measurement MFP Required.
 *   Set when dot11RSTARequiresPMFActivated is 2 (11.21.6.3.1).
 *
 * @WLAN_CRYPTO_RSNX_CAP_RSVD_16: Reserved (bit 16).
 * @WLAN_CRYPTO_RSNX_CAP_RSVD_17: Reserved (bit 17).
 * @WLAN_CRYPTO_RSNX_CAP_RSVD_18: Reserved (bit 18).
 * @WLAN_CRYPTO_RSNX_CAP_RSVD_19: Reserved (bit 19).
 * @WLAN_CRYPTO_RSNX_CAP_RSVD_20: Reserved (bit 20).
 *
 * @WLAN_CRYPTO_RSNX_CAP_SSID_PROT: SSID Protection.
 *   Set to indicate support for protected SSID exchange during 4‑way HS.
 *
 * @WLAN_CRYPTO_RSNX_CAP_QMF_ACI_UNMASK: QMF ACI Subfield Unmask Support.
 *   Set if STA supports unmasking ACI subfield during AAD construction.
 *
 * @WLAN_CRYPTO_RSNX_CAP_EDP_MGMT_IADDR: EDP Robust Individually Addressed
 *   Management Frame Support.
 *   Set by EDP STA if robust individually addressed mgmt frames are supported
 *   (12.16.3).
 *
 * @WLAN_CRYPTO_RSNX_CAP_EDP_BF_CSI_TB_TX: EDP Robust Individually Addressed
 *   Beamforming/CSI/CQI Frame TB Tx Support.
 *   Set if TB Tx is supported for individually addressed BF/CSI/CQI frames.
 *
 * @WLAN_CRYPTO_RSNX_CAP_EDP_BF_CSI_RX: EDP Robust Individually Addressed
 *   Beamforming/CSI/CQI Frame Rx Support.
 *   Set if Rx is supported for individually addressed BF/CSI/CQI frames.
 *
 * @WLAN_CRYPTO_RSNX_CAP_EDP_CAPS_REQRES: EDP Capabilities & Operation
 *   Parameters Request/Response Support.
 *   Set if EDP Caps & Operation Params Req/Resp is supported (12.16.4).
 *
 * @WLAN_CRYPTO_RSNX_CAP_ASSOC_FRM_ENCRYPTION: (Re)Association Frame
 *   Encryption Support.
 *   Set if STA supports encrypting (Re)Association Request/Response frames.
 *
 * @WLAN_CRYPTO_RSNX_CAP_DOT1X_OVER_AUTH_FRM: IEEE 802.1X Authentication
 *  Utilizing Authentication Frame Support.
 *   Set when 802.1X authentication via Authentication frames is supported.
 *
 * @WLAN_CRYPTO_RSNX_CAP_PMKSA_PRIVACY: PMKSA Caching Privacy Support.
 *   Set when PMKSA caching privacy is activated (12.16.7).
 *
 * @WLAN_CRYPTO_RSNX_CAP_GRP_EDP_EPOCH: Group EDP Epoch Supported.
 *   Non-AP MLD sets when dot11EDPGroupEpochActivated is true.
 *
 * @WLAN_CRYPTO_RSNX_CAP_DS_MAC_ADDR: DS MAC Address Support.
 *   Set when dot11DSMACAddressActivated is true.
 *
 * @WLAN_CRYPTO_RSNX_CAP_EDP_BF_CSI_NTB_TX: EDP Robust Individually Addressed
 *   Beamforming/CSI/CQI Frame Non-TB Tx Support.
 *   Set if Non-TB Tx is supported for individually addressed BF/CSI/CQI frames.
 *
 * Definition: (IEEE Std 802.11-2024, 9.4.2.240, Table 9-373)
 */
enum wlan_crypto_rsnx_cap {
	/* 0–3: length subfield (n − 1), 4 bits */
	WLAN_CRYPTO_RSNX_CAP_LEN_MASK        = 0x0F,
	WLAN_CRYPTO_RSNX_CAP_LEN_SHIFT       = 0,

	/* single bit flags */
	WLAN_CRYPTO_RSNX_CAP_TWT_PROT          = BIT(4),
	WLAN_CRYPTO_RSNX_CAP_SAE_H2E           = BIT(5),
	WLAN_CRYPTO_RSNX_CAP_SAE_PK            = BIT(6),
	WLAN_CRYPTO_RSNX_CAP_WUR_PROT          = BIT(7),
	WLAN_CRYPTO_RSNX_CAP_SECURE_LTF        = BIT(8),
	WLAN_CRYPTO_RSNX_CAP_SECURE_RTT        = BIT(9),
	WLAN_CRYPTO_RSNX_CAP_URNM_MFPR_X20     = BIT(10),
	WLAN_CRYPTO_RSNX_CAP_PROT_ANNOUNCE     = BIT(11),
	WLAN_CRYPTO_RSNX_CAP_PBAC              = BIT(12),
	WLAN_CRYPTO_RSNX_CAP_EXT_SIG_ACT_PROT  = BIT(13),
	WLAN_CRYPTO_RSNX_CAP_SPP_AMSDU         = BIT(14),
	WLAN_CRYPTO_RSNX_CAP_URNM_MFPR         = BIT(15),

	WLAN_CRYPTO_RSNX_CAP_RSVD_16         = BIT(16),
	WLAN_CRYPTO_RSNX_CAP_RSVD_17         = BIT(17),
	WLAN_CRYPTO_RSNX_CAP_RSVD_18         = BIT(18),
	WLAN_CRYPTO_RSNX_CAP_RSVD_19         = BIT(19),
	WLAN_CRYPTO_RSNX_CAP_RSVD_20         = BIT(20),

	WLAN_CRYPTO_RSNX_CAP_SSID_PROT             = BIT(21),
	WLAN_CRYPTO_RSNX_CAP_QMF_ACI_UNMASK        = BIT(22),
	WLAN_CRYPTO_RSNX_CAP_EDP_MGMT_IADDR        = BIT(23),
	WLAN_CRYPTO_RSNX_CAP_EDP_BF_CSI_TB_TX      = BIT(24),
	WLAN_CRYPTO_RSNX_CAP_EDP_BF_CSI_RX         = BIT(25),
	WLAN_CRYPTO_RSNX_CAP_EDP_CAPS_REQRES       = BIT(26),
	WLAN_CRYPTO_RSNX_CAP_ASSOC_FRM_ENCRYPTION  = BIT(27),
	WLAN_CRYPTO_RSNX_CAP_DOT1X_OVER_AUTH_FRM   = BIT(28),
	WLAN_CRYPTO_RSNX_CAP_PMKSA_PRIVACY         = BIT(29),
	WLAN_CRYPTO_RSNX_CAP_GRP_EDP_EPOCH         = BIT(30),
	WLAN_CRYPTO_RSNX_CAP_DS_MAC_ADDR           = BIT(31),

	/* bit 32 requires 64-bit backing if stored in u32 */
	WLAN_CRYPTO_RSNX_CAP_EDP_BF_CSI_NTB_TX = BIT_ULL(32),
};

/**
 * enum wlan_crypto_vdev_11az_security_capab  - 11az related vdev
 * security capabilities
 * @WLAN_CRYPTO_RSNX_URNM_MFPR: URNM MFP required bit from RSNXE
 * @WLAN_CRYPTO_RSN_MFPC: MFP capable bit from RSN IE
 * @WLAN_CRYPTO_RSN_MFPR: MFP required bit from RSN IE
 * @WLAN_CRYPTO_RSNX_URNM_MFPR_X20: URNM_MFPR_X20 bit from RSNXE
 * @WLAN_CRYPTO_RSNX_RSTA_EXTCAP_I2R_LMR_FB: I2R LMR FB Policy from
 * Extended Capabilities
 */
enum wlan_crypto_vdev_11az_security_capab {
	WLAN_CRYPTO_RSNX_URNM_MFPR,
	WLAN_CRYPTO_RSN_MFPC,
	WLAN_CRYPTO_RSN_MFPR,
	WLAN_CRYPTO_RSNX_URNM_MFPR_X20,
	WLAN_CRYPTO_RSNX_RSTA_EXTCAP_I2R_LMR_FB,
};

/**
 * enum wlan_crypto_vdev_pasn_caps  - PASN peer related vdev
 * crypto parameters
 * @WLAN_CRYPTO_URNM_MFPR: URNM MFP required in RSNXE
 * @WLAN_CRYPTO_MFPC: MFP capable bit from RSN IE
 * @WLAN_CRYPTO_MFPR: MFP required from RSNIE
 * @WLAN_CRYPTO_URNM_MFPR_X20: URNM_MFPR_X20 in RSNXE
 * @WLAN_CRYPTO_I2R_LMR_FB: I2R LMR feedback policy in extended capabilities
 */
enum wlan_crypto_vdev_pasn_caps {
	WLAN_CRYPTO_URNM_MFPR = BIT(0),
	WLAN_CRYPTO_MFPC = BIT(1),
	WLAN_CRYPTO_MFPR = BIT(2),
	WLAN_CRYPTO_URNM_MFPR_X20 = BIT(3),
	WLAN_CRYPTO_I2R_LMR_FB = BIT(4),
};

typedef enum wlan_crypto_key_mgmt {
	WLAN_CRYPTO_KEY_MGMT_IEEE8021X             = 0,
	WLAN_CRYPTO_KEY_MGMT_PSK                   = 1,
	WLAN_CRYPTO_KEY_MGMT_NONE                  = 2,
	WLAN_CRYPTO_KEY_MGMT_IEEE8021X_NO_WPA      = 3,
	WLAN_CRYPTO_KEY_MGMT_WPA_NONE              = 4,
	WLAN_CRYPTO_KEY_MGMT_FT_IEEE8021X          = 5,
	WLAN_CRYPTO_KEY_MGMT_FT_PSK                = 6,
	WLAN_CRYPTO_KEY_MGMT_IEEE8021X_SHA256      = 7,
	WLAN_CRYPTO_KEY_MGMT_PSK_SHA256            = 8,
	WLAN_CRYPTO_KEY_MGMT_WPS                   = 9,
	WLAN_CRYPTO_KEY_MGMT_SAE                   = 10,
	WLAN_CRYPTO_KEY_MGMT_FT_SAE                = 11,
	WLAN_CRYPTO_KEY_MGMT_WAPI_PSK              = 12,
	WLAN_CRYPTO_KEY_MGMT_WAPI_CERT             = 13,
	WLAN_CRYPTO_KEY_MGMT_CCKM                  = 14,
	WLAN_CRYPTO_KEY_MGMT_OSEN                  = 15,
	WLAN_CRYPTO_KEY_MGMT_IEEE8021X_SUITE_B     = 16,
	WLAN_CRYPTO_KEY_MGMT_IEEE8021X_SUITE_B_192 = 17,
	WLAN_CRYPTO_KEY_MGMT_FILS_SHA256           = 18,
	WLAN_CRYPTO_KEY_MGMT_FILS_SHA384           = 19,
	WLAN_CRYPTO_KEY_MGMT_FT_FILS_SHA256        = 20,
	WLAN_CRYPTO_KEY_MGMT_FT_FILS_SHA384        = 21,
	WLAN_CRYPTO_KEY_MGMT_OWE                   = 22,
	WLAN_CRYPTO_KEY_MGMT_DPP                   = 23,
	WLAN_CRYPTO_KEY_MGMT_FT_IEEE8021X_SHA384   = 24,
	WLAN_CRYPTO_KEY_MGMT_FT_PSK_SHA384         = 25,
	WLAN_CRYPTO_KEY_MGMT_PSK_SHA384            = 26,
	WLAN_CRYPTO_KEY_MGMT_SAE_EXT_KEY           = 27,
	WLAN_CRYPTO_KEY_MGMT_FT_SAE_EXT_KEY        = 28,
	WLAN_CRYPTO_KEY_MGMT_PASN                  = 29,
	WLAN_CRYPTO_KEY_MGMT_EPPKE                 = 30,
	/** Keep WLAN_CRYPTO_KEY_MGMT_MAX at the end. */
	WLAN_CRYPTO_KEY_MGMT_MAX,
} wlan_crypto_key_mgmt;

enum wlan_crypto_key_type {
	WLAN_CRYPTO_KEY_TYPE_UNICAST,
	WLAN_CRYPTO_KEY_TYPE_GROUP,
};

#define IS_WEP_CIPHER(_c)      ((_c == WLAN_CRYPTO_CIPHER_WEP) || \
				(_c == WLAN_CRYPTO_CIPHER_WEP_40) || \
				(_c == WLAN_CRYPTO_CIPHER_WEP_104))

#define DEFAULT_KEYMGMT_6G_MASK 0xFFFFFFFF

/* AKM wlan_crypto_key_mgmt 1, 6, 8, 25 and 26 are not allowed. */
#define ALLOWED_KEYMGMT_6G_MASK 0x19FFFEBD

/*
 * enum fils_erp_cryptosuite: this enum defines the cryptosuites used
 * to calculate auth tag and auth tag length as defined by RFC 6696 5.3.1
 * @HMAC_SHA256_64: sha256 with auth tag len as 64 bits
 * @HMAC_SHA256_128: sha256 with auth tag len as 128 bits
 * @HMAC_SHA256_256: sha256 with auth tag len as 256 bits
 */
enum fils_erp_cryptosuite {
	INVALID_CRYPTO = 0, /* reserved */
	HMAC_SHA256_64,
	HMAC_SHA256_128,
	HMAC_SHA256_256,
};

/*
 * enum wlan_crypto_oem_eht_mlo_config - ENUM for different OEM configurable
 * crypto params to allow EHT/MLO in WPA2/WPA3 security.
 *
 * @WLAN_HOST_CRYPTO_WPA2_ALLOW_NON_MLO_EHT: Allows connecting to WPA2 with PMF
 * capability set to false in EHT only mode. If the AP is MLO, the connection
 * will still be in EHT without MLO.
 *
 * @WLAN_HOST_CRYPTO_WPA2_ALLOW_MLO: Allows connecting to WPA2 with PMF
 * capability set to false in MLO mode.
 *    -If set along with WLAN_HOST_CRYPTO_WPA2_ALLOW_NON_MLO_EHT,
 *     this mode supersedes.
 *
 * @WLAN_HOST_CRYPTO_WPA2_ALLOW_NON_MLO_EHT_MFPC_SET: Allows connecting to WPA2
 * with PMF capability set to true in EHT only mode. If the AP is MLO,
 * the connection will still be in EHT without MLO.
 *
 * @WLAN_HOST_CRYPTO_WPA2_ALLOW_MLO_MFPC_SET: Allows connecting to WPA2 with PMF
 * capability set to true in MLO mode.
 *    -If set along with WLAN_HOST_CRYPTO_WPA2_ALLOW_NON_MLO_EHT_MFPC_SET,
 *     this mode supersedes.
 *
 * @WLAN_HOST_CRYPTO_WPA3_SAE_ALLOW_NON_MLO_EHT_HnP: Connect to non-MLO/MLO
 * WPA3-SAE without support for H2E (or no RSNXE IE in beacon) in non-MLO EHT.
 * This bit results in connecting to both H2E and HnP APs in EHT only mode.
 *
 * @WLAN_HOST_CRYPTO_WPA3_SAE_ALLOW_MLO_HnP: Connect to MLO WPA3-SAE without
 * support for H2E (or no RSNXE IE in beacon) in MLO.
 * This bit result in connecting to both H2E and HnP APs in MLO mode.
 *    -If set along with WLAN_HOST_CRYPTO_WPA3_SAE_ALLOW_NON_MLO_EHT_HnP,
 *     this mode supersedes.
 */
enum wlan_crypto_oem_eht_mlo_config {
	WLAN_HOST_CRYPTO_WPA2_ALLOW_NON_MLO_EHT           = BIT(0),
	WLAN_HOST_CRYPTO_WPA2_ALLOW_MLO                   = BIT(1),
	WLAN_HOST_CRYPTO_WPA2_ALLOW_NON_MLO_EHT_MFPC_SET  = BIT(2),
	WLAN_HOST_CRYPTO_WPA2_ALLOW_MLO_MFPC_SET          = BIT(3),
	/* Bits 4-15 are reserved for future WPA2 security configs */

	WLAN_HOST_CRYPTO_WPA3_SAE_ALLOW_NON_MLO_EHT_HnP   = BIT(16),
	WLAN_HOST_CRYPTO_WPA3_SAE_ALLOW_MLO_HnP           = BIT(17),
	/* Bits 18-31 are reserved for future WPA3 security configs */
};

#define WLAN_CRYPTO_WPA2_OEM_EHT_CFG_NO_PMF_ALLOWED(_cfg) \
	((_cfg) & WLAN_HOST_CRYPTO_WPA2_ALLOW_NON_MLO_EHT || \
	 (_cfg) & WLAN_HOST_CRYPTO_WPA2_ALLOW_MLO)

#define WLAN_CRYPTO_WPA2_OEM_EHT_CFG_PMF_ALLOWED(_cfg) \
	 ((_cfg) & WLAN_HOST_CRYPTO_WPA2_ALLOW_NON_MLO_EHT_MFPC_SET || \
	  (_cfg) & WLAN_HOST_CRYPTO_WPA2_ALLOW_MLO_MFPC_SET)

#define WLAN_CRYPTO_WPA3_SAE_OEM_EHT_CFG_IS_STRICT_H2E(_cfg) \
	(((_cfg) & WLAN_HOST_CRYPTO_WPA3_SAE_ALLOW_NON_MLO_EHT_HnP || \
	  (_cfg) & WLAN_HOST_CRYPTO_WPA3_SAE_ALLOW_MLO_HnP) == 0)

/**
 * struct mobility_domain_params - structure containing
 *				   mobility domain info
 * @mdie_present: mobility domain present or not
 * @mobility_domain: mobility domain
 */
struct mobility_domain_params {
	uint8_t mdie_present;
	uint16_t mobility_domain;
};

/**
 * struct wlan_crypto_pmksa - structure of crypto to contain pmkid
 * @bssid: bssid for which pmkid is saved
 * @pmkid: pmkid info
 * @pmk: pmk info
 * @pmk_len: pmk len
 * @ssid_len: ssid length
 * @ssid: ssid information
 * @cache_id: cache id
 * @pmk_lifetime: Duration in seconds for which the pmk is valid
 * @pmk_lifetime_threshold: Percentage of pmk lifetime within which
 * full authentication is expected to avoid disconnection.
 * @pmk_entry_ts: System timestamp at which the PMK entry was created.
 * @single_pmk_supported: SAE single pmk supported BSS
 * @mdid: structure to contain mobility domain parameters
 */
struct wlan_crypto_pmksa {
	struct qdf_mac_addr bssid;
	uint8_t    pmkid[PMKID_LEN];
	uint8_t    pmk[MAX_PMK_LEN];
	uint8_t    pmk_len;
	uint8_t    ssid_len;
	uint8_t    ssid[WLAN_SSID_MAX_LEN];
	uint8_t    cache_id[WLAN_CACHE_ID_LEN];
	uint32_t   pmk_lifetime;
	uint8_t    pmk_lifetime_threshold;
	qdf_time_t pmk_entry_ts;
#if defined(WLAN_SAE_SINGLE_PMK) && defined(WLAN_FEATURE_ROAM_OFFLOAD)
	bool       single_pmk_supported;
#endif
	struct mobility_domain_params mdid;
};

#ifdef WLAN_ADAPTIVE_11R
/**
 * struct key_mgmt_list - structure to store AKM(s) present in RSN IE of
 * Beacon/Probe response
 * @key_mgmt: AKM(s) present in RSN IE of Beacon/Probe response
 */
struct key_mgmt_list {
	uint32_t key_mgmt;
};
#endif

/**
 * struct wlan_crypto_params - holds crypto params
 * @authmodeset:        authentication mode
 * @ucastcipherset:     unicast ciphers
 * @mcastcipherset:     multicast cipher
 * @mgmtcipherset:      mgmt cipher
 * @cipher_caps:        cipher capability
 * @key_mgmt:           key mgmt
 * @pmksa:              pmksa
 * @rsn_caps:           rsn_capability
 * @rsnx_caps:          rsnx capability
 * @akm_list:           order of AKM present in RSN IE of Beacon/Probe response
 * @random_pmkid_cnt:   count of random PMKIDs to be added in assoc request
 *
 * This structure holds crypto params for peer or vdev
 */
struct wlan_crypto_params {
	uint32_t authmodeset;
	uint32_t ucastcipherset;
	uint32_t mcastcipherset;
	uint32_t mgmtcipherset;
	uint32_t cipher_caps;
	uint32_t key_mgmt;
	struct   wlan_crypto_pmksa *pmksa[WLAN_CRYPTO_MAX_PMKID];
	uint16_t rsn_caps;
	uint32_t rsnx_caps;
#ifdef WLAN_ADAPTIVE_11R
	struct key_mgmt_list akm_list[WLAN_CRYPTO_KEY_MGMT_MAX];
#endif
	uint8_t random_pmkid_cnt;
};

/**
 * struct wlan_crypto_ltf_keyseed_data - LTF keyseed parameters
 * @vdev_id: Vdev id
 * @peer_mac_addr: Peer mac address
 * @src_mac_addr: Source mac address
 * @rsn_authmode: Cipher suite
 * @key_seed: Secure LTF key seed
 * @key_seed_len: Key seed length
 */
struct wlan_crypto_ltf_keyseed_data {
	uint8_t vdev_id;
	struct qdf_mac_addr peer_mac_addr;
	struct qdf_mac_addr src_mac_addr;
	uint8_t rsn_authmode;
	uint8_t key_seed[WLAN_MAX_SECURE_LTF_KEYSEED_LEN];
	uint16_t key_seed_len;
};

typedef enum wlan_crypto_param_type {
	WLAN_CRYPTO_PARAM_AUTH_MODE,
	WLAN_CRYPTO_PARAM_UCAST_CIPHER,
	WLAN_CRYPTO_PARAM_MCAST_CIPHER,
	WLAN_CRYPTO_PARAM_MGMT_CIPHER,
	WLAN_CRYPTO_PARAM_CIPHER_CAP,
	WLAN_CRYPTO_PARAM_RSN_CAP,
	WLAN_CRYPTO_PARAM_RSNX_CAP,
	WLAN_CRYPTO_PARAM_KEY_MGMT,
	WLAN_CRYPTO_PARAM_PMKSA,
	WLAN_CRYPTO_PARAM_RANDOM_PMKID,
} wlan_crypto_param_type;

/**
 * struct wlan_crypto_key - key structure
 * @keylen:         length of the key
 * @valid:          is key valid or not
 * @flags:          key flags
 * @keyix:          key id
 * @cipher_type:    cipher type being used for this key
 * @key_type:       unicast or broadcast key
 * @macaddr:        MAC address of the peer
 * @src_addr:       Source mac address associated with the key
 * @cipher_table:   table which stores cipher related info
 * @private:        private pointer to save cipher context
 * @keylock:        spin lock
 * @recviv:         WAPI key receive sequence counter
 * @txiv:           WAPI key transmit sequence counter
 * @keytsc:         key transmit sequence counter
 * @keyrsc:         key receive sequence counter
 * @keyrsc_suspect: key receive sequence counter under
 *                  suspect when pN jump is detected
 * @keyglobal:      key receive global sequence counter used with suspect
 * @keyval:         key value buffer
 *
 * This key structure to key related details.
 */
struct wlan_crypto_key {
	uint8_t     keylen;
	bool        valid;
	uint16_t    flags;
	uint16_t    keyix;
	enum wlan_crypto_cipher_type cipher_type;
	enum wlan_crypto_key_type key_type;
	uint8_t     macaddr[QDF_MAC_ADDR_SIZE];
	struct qdf_mac_addr src_addr;
	void        *cipher_table;
	void        *private;
	qdf_spinlock_t	keylock;
	uint8_t     recviv[WLAN_CRYPTO_WAPI_IV_SIZE];
	uint8_t     txiv[WLAN_CRYPTO_WAPI_IV_SIZE];
	uint64_t    keytsc;
	uint64_t    keyrsc[WLAN_CRYPTO_TID_SIZE];
	uint64_t    keyrsc_suspect[WLAN_CRYPTO_TID_SIZE];
	uint64_t    keyglobal;
	uint8_t     keyval[WLAN_CRYPTO_KEYBUF_SIZE
				+ WLAN_CRYPTO_MICBUF_SIZE];
#define txmic    (keyval + WLAN_CRYPTO_KEYBUF_SIZE \
				+ WLAN_CRYPTO_TXMIC_OFFSET)
#define rxmic    (keyval + WLAN_CRYPTO_KEYBUF_SIZE \
				+ WLAN_CRYPTO_RXMIC_OFFSET)
};

/**
 * struct wlan_crypto_keys - crypto keys structure
 * @key:              key buffers for this peer
 * @igtk_key:         igtk key buffer for this peer
 * @bigtk_key:        bigtk key buffer for this peer
 * @ltf_key_seed:     LTF Key Seed buffer
 * @igtk_key_type:    igtk key type
 * @def_tx_keyid:     default key used for this peer
 * @def_igtk_tx_keyid: default igtk key used for this peer
 * @def_bigtk_tx_keyid: default bigtk key used for this peer
 */
struct wlan_crypto_keys {
	struct wlan_crypto_key *key[WLAN_CRYPTO_MAX_VLANKEYIX];
	struct wlan_crypto_key *igtk_key[WLAN_CRYPTO_MAXIGTKKEYIDX];
	struct wlan_crypto_key *bigtk_key[WLAN_CRYPTO_MAXBIGTKKEYIDX];
	struct wlan_crypto_ltf_keyseed_data ltf_key_seed;
	enum wlan_crypto_cipher_type igtk_key_type;
	uint8_t def_tx_keyid;
	uint8_t def_igtk_tx_keyid;
	uint8_t def_bigtk_tx_keyid;
};

union crypto_align_mac_addr {
	uint8_t raw[QDF_MAC_ADDR_SIZE];
	struct {
		uint16_t bytes_ab;
		uint16_t bytes_cd;
		uint16_t bytes_ef;
	} align2;
	struct {
		uint32_t bytes_abcd;
		uint16_t bytes_ef;
	} align4;
	struct __packed {
		uint16_t bytes_ab;
		uint32_t bytes_cdef;
	} align4_2;
};

/**
 * struct wlan_crypto_key_entry - crypto key entry structure
 * @mac_addr: mac addr
 * @is_active: active key entry
 * @link_id: link id
 * @vdev_id: vdev id
 * @keys: crypto keys
 * @hash_list_elem: hash list element
 */
struct wlan_crypto_key_entry {
	union crypto_align_mac_addr mac_addr;
	bool is_active;
	uint8_t link_id;
	uint8_t vdev_id;
	struct wlan_crypto_keys keys;

	TAILQ_ENTRY(wlan_crypto_key_entry) hash_list_elem;
};

/**
 * struct wlan_crypto_req_key - key request structure
 * @type:                       key/cipher type
 * @pad:                        padding member
 * @keyix:                      key index
 * @keylen:                     length of the key value
 * @flags:                      key flags
 * @macaddr:                    macaddr of the key
 * @keyrsc:                     key receive sequence counter
 * @keytsc:                     key transmit sequence counter
 * @keydata:                    key value
 * @txiv:                       wapi key tx iv
 * @recviv:                     wapi key rx iv
 * @filsaad:                    FILS AEAD data
 *
 * Key request structure used for setkey, getkey or delkey
 */
struct wlan_crypto_req_key {
	uint8_t    type;
	uint8_t    pad;
	uint16_t   keyix;
	uint8_t    keylen;
	uint16_t    flags;
	uint8_t    macaddr[QDF_MAC_ADDR_SIZE];
	uint64_t   keyrsc;
	uint64_t   keytsc;
	uint8_t    keydata[WLAN_CRYPTO_KEYBUF_SIZE + WLAN_CRYPTO_MICBUF_SIZE];
	uint8_t    txiv[WLAN_CRYPTO_WAPI_IV_SIZE];
	uint8_t    recviv[WLAN_CRYPTO_WAPI_IV_SIZE];
#ifdef WLAN_CRYPTO_SUPPORT_FILS
	struct     wlan_crypto_fils_aad_key   filsaad;
#endif
};

/**
 * struct wlan_lmac_if_crypto_tx_ops - structure of crypto function
 *                  pointers
 * @allockey: function pointer to alloc key in hw
 * @setkey:  function pointer to setkey in hw
 * @delkey: function pointer to delkey in hw
 * @defaultkey: function pointer to set default key
 * @set_key: converged function pointer to set key in hw
 * @getpn: function pointer to get current pn value of peer
 * @set_ltf_keyseed: Set LTF keyseed
 * @set_vdev_param: Set the vdev crypto parameter
 * @register_events: function pointer to register wmi event handler
 * @deregister_events: function pointer to deregister wmi event handler
 */
struct wlan_lmac_if_crypto_tx_ops {
	QDF_STATUS (*allockey)(struct wlan_objmgr_vdev *vdev,
			       struct wlan_crypto_key *key,
			       uint8_t *macaddr, uint32_t key_type);
	QDF_STATUS (*setkey)(struct wlan_objmgr_vdev *vdev,
			     struct wlan_crypto_key *key,
			     uint8_t *macaddr, uint32_t key_type);
	QDF_STATUS (*delkey)(struct wlan_objmgr_vdev *vdev,
			     struct wlan_crypto_key *key,
			     uint8_t *macaddr, uint32_t key_type);
	QDF_STATUS (*defaultkey)(struct wlan_objmgr_vdev *vdev,
				 uint8_t keyix, uint8_t *macaddr);
	QDF_STATUS (*set_key)(struct wlan_objmgr_vdev *vdev,
			      struct wlan_crypto_key *key,
			      enum wlan_crypto_key_type key_type);
	QDF_STATUS(*getpn)(struct wlan_objmgr_vdev *vdev,
			   uint8_t *macaddr, uint8_t keyix, uint32_t key_type);
	QDF_STATUS (*set_ltf_keyseed)(struct wlan_objmgr_psoc *psoc,
				      struct wlan_crypto_ltf_keyseed_data *ks);
	QDF_STATUS (*set_vdev_param)(struct wlan_objmgr_psoc *psoc,
				     uint32_t vdev_id, uint32_t param_id,
				     uint32_t param_value);
	QDF_STATUS (*register_events)(struct wlan_objmgr_psoc *psoc);
	QDF_STATUS (*deregister_events)(struct wlan_objmgr_psoc *psoc);
};

/**
 * struct wlan_lmac_if_crypto_rx_ops - structure of crypto rx  function
 *                  pointers
 * @crypto_encap: function pointer to encap tx frame
 * @crypto_decap:  function pointer to decap rx frame in hw
 * @crypto_enmic: function pointer to enmic tx frame
 * @crypto_demic: function pointer to demic rx frame
 * @set_peer_wep_keys: function pointer to set WEP keys
 * @get_rxpn: function pointer to get current Rx pn value of peer
 */

struct wlan_lmac_if_crypto_rx_ops {
	QDF_STATUS(*crypto_encap)(struct wlan_objmgr_vdev *vdev,
					qdf_nbuf_t wbuf, uint8_t *macaddr,
					uint8_t encapdone);
	QDF_STATUS(*crypto_decap)(struct wlan_objmgr_vdev *vdev,
					qdf_nbuf_t wbuf, uint8_t *macaddr,
					uint8_t tid);
	QDF_STATUS(*crypto_enmic)(struct wlan_objmgr_vdev *vdev,
					qdf_nbuf_t wbuf, uint8_t *macaddr,
					uint8_t encapdone);
	QDF_STATUS(*crypto_demic)(struct wlan_objmgr_vdev *vdev,
					qdf_nbuf_t wbuf, uint8_t *macaddr,
					uint8_t tid, uint8_t keyid);
	QDF_STATUS(*set_peer_wep_keys)(struct wlan_objmgr_vdev *vdev,
					struct wlan_objmgr_peer *peer);
	QDF_STATUS (*get_rxpn)(struct wlan_objmgr_vdev *vdev,
			       uint8_t *macaddr, uint16_t keyix);
};

#define WLAN_CRYPTO_RX_OPS_ENCAP(crypto_rx_ops) \
				(crypto_rx_ops->crypto_encap)
#define WLAN_CRYPTO_RX_OPS_DECAP(crypto_rx_ops) \
				(crypto_rx_ops->crypto_decap)
#define WLAN_CRYPTO_RX_OPS_ENMIC(crypto_rx_ops) \
				(crypto_rx_ops->crypto_enmic)
#define WLAN_CRYPTO_RX_OPS_DEMIC(crypto_rx_ops) \
				(crypto_rx_ops->crypto_demic)
#define WLAN_CRYPTO_RX_OPS_SET_PEER_WEP_KEYS(crypto_rx_ops) \
				(crypto_rx_ops->set_peer_wep_keys)
#define WLAN_CRYPTO_RX_OPS_GET_RXPN(crypto_rx_ops) \
				((crypto_rx_ops)->get_rxpn)

#define WLAN_CRYPTO_IS_WPA_WPA2(akm) \
	(QDF_HAS_PARAM(akm, WLAN_CRYPTO_KEY_MGMT_IEEE8021X) || \
	 QDF_HAS_PARAM(akm, WLAN_CRYPTO_KEY_MGMT_PSK) || \
	 QDF_HAS_PARAM(akm, WLAN_CRYPTO_KEY_MGMT_FT_IEEE8021X) || \
	 QDF_HAS_PARAM(akm, WLAN_CRYPTO_KEY_MGMT_FT_PSK) || \
	 QDF_HAS_PARAM(akm, WLAN_CRYPTO_KEY_MGMT_IEEE8021X_SHA256) || \
	 QDF_HAS_PARAM(akm, WLAN_CRYPTO_KEY_MGMT_PSK_SHA256) || \
	 QDF_HAS_PARAM(akm, WLAN_CRYPTO_KEY_MGMT_WPS) || \
	 QDF_HAS_PARAM(akm, WLAN_CRYPTO_KEY_MGMT_WAPI_PSK) || \
	 QDF_HAS_PARAM(akm, WLAN_CRYPTO_KEY_MGMT_WAPI_CERT) || \
	 QDF_HAS_PARAM(akm, WLAN_CRYPTO_KEY_MGMT_CCKM) || \
	 QDF_HAS_PARAM(akm, WLAN_CRYPTO_KEY_MGMT_OSEN) || \
	 QDF_HAS_PARAM(akm, WLAN_CRYPTO_KEY_MGMT_IEEE8021X_SUITE_B) || \
	 QDF_HAS_PARAM(akm, WLAN_CRYPTO_KEY_MGMT_FILS_SHA256) || \
	 QDF_HAS_PARAM(akm, WLAN_CRYPTO_KEY_MGMT_FILS_SHA384) || \
	 QDF_HAS_PARAM(akm, WLAN_CRYPTO_KEY_MGMT_FT_FILS_SHA256) || \
	 QDF_HAS_PARAM(akm, WLAN_CRYPTO_KEY_MGMT_FT_FILS_SHA384) || \
	 QDF_HAS_PARAM(akm, WLAN_CRYPTO_KEY_MGMT_FT_PSK_SHA384) || \
	 QDF_HAS_PARAM(akm, WLAN_CRYPTO_KEY_MGMT_PSK_SHA384))

#define WLAN_CRYPTO_IS_WPA2(akm) \
	(QDF_HAS_PARAM(akm, WLAN_CRYPTO_KEY_MGMT_PSK) || \
	 QDF_HAS_PARAM(akm, WLAN_CRYPTO_KEY_MGMT_FT_PSK) || \
	 QDF_HAS_PARAM(akm, WLAN_CRYPTO_KEY_MGMT_PSK_SHA256) || \
	 QDF_HAS_PARAM(akm, WLAN_CRYPTO_KEY_MGMT_FT_PSK_SHA384) || \
	 QDF_HAS_PARAM(akm, WLAN_CRYPTO_KEY_MGMT_PSK_SHA384))

#define WLAN_CRYPTO_IS_WPA3(akm) \
	(QDF_HAS_PARAM(akm, WLAN_CRYPTO_KEY_MGMT_SAE) || \
	 QDF_HAS_PARAM(akm, WLAN_CRYPTO_KEY_MGMT_FT_SAE) || \
	 QDF_HAS_PARAM(akm, WLAN_CRYPTO_KEY_MGMT_IEEE8021X_SUITE_B_192) || \
	 QDF_HAS_PARAM(akm, WLAN_CRYPTO_KEY_MGMT_OWE) || \
	 QDF_HAS_PARAM(akm, WLAN_CRYPTO_KEY_MGMT_DPP) || \
	 QDF_HAS_PARAM(akm, WLAN_CRYPTO_KEY_MGMT_FT_IEEE8021X_SHA384) || \
	 QDF_HAS_PARAM(akm, WLAN_CRYPTO_KEY_MGMT_SAE_EXT_KEY) || \
	 QDF_HAS_PARAM(akm, WLAN_CRYPTO_KEY_MGMT_FT_SAE_EXT_KEY))

#define WLAN_CRYPTO_IS_AKM_ENTERPRISE(akm) \
	(QDF_HAS_PARAM(akm, WLAN_CRYPTO_KEY_MGMT_IEEE8021X) || \
	 QDF_HAS_PARAM(akm, WLAN_CRYPTO_KEY_MGMT_IEEE8021X_SHA256) || \
	 QDF_HAS_PARAM(akm, WLAN_CRYPTO_KEY_MGMT_IEEE8021X_SUITE_B) || \
	 QDF_HAS_PARAM(akm, WLAN_CRYPTO_KEY_MGMT_FT_IEEE8021X) || \
	 QDF_HAS_PARAM(akm, WLAN_CRYPTO_KEY_MGMT_FT_IEEE8021X_SHA384) || \
	 QDF_HAS_PARAM(akm, WLAN_CRYPTO_KEY_MGMT_IEEE8021X_SUITE_B_192) || \
	 QDF_HAS_PARAM(akm, WLAN_CRYPTO_KEY_MGMT_FILS_SHA256) || \
	 QDF_HAS_PARAM(akm, WLAN_CRYPTO_KEY_MGMT_FILS_SHA384) || \
	 QDF_HAS_PARAM(akm, WLAN_CRYPTO_KEY_MGMT_FT_FILS_SHA256) || \
	 QDF_HAS_PARAM(akm, WLAN_CRYPTO_KEY_MGMT_FT_FILS_SHA384))

#define WLAN_CRYPTO_IS_AKM_SAE(akm) \
	(QDF_HAS_PARAM(akm, WLAN_CRYPTO_KEY_MGMT_SAE) || \
	 QDF_HAS_PARAM(akm, WLAN_CRYPTO_KEY_MGMT_FT_SAE) || \
	 QDF_HAS_PARAM(akm, WLAN_CRYPTO_KEY_MGMT_SAE_EXT_KEY) || \
	 QDF_HAS_PARAM(akm, WLAN_CRYPTO_KEY_MGMT_FT_SAE_EXT_KEY))
#endif /* end of _WLAN_CRYPTO_GLOBAL_DEF_H_ */
