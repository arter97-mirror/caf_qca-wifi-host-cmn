/*
 * Copyright (c) 2017-2018 The Linux Foundation. All rights reserved.
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
 * DOC: Public API intialization of crypto service with object manager
 */
#include <qdf_types.h>
#include <wlan_cmn.h>
#include <wlan_objmgr_cmn.h>
#include <wlan_objmgr_global_obj.h>
#include <wlan_objmgr_psoc_obj.h>
#include <wlan_objmgr_pdev_obj.h>
#include <wlan_objmgr_vdev_obj.h>
#include <wlan_objmgr_peer_obj.h>

#include "wlan_crypto_global_def.h"
#include "wlan_crypto_def_i.h"
#include "wlan_crypto_main_i.h"
#include "wlan_crypto_obj_mgr_i.h"


static QDF_STATUS wapi_setkey(struct wlan_crypto_key *key)
{
	return QDF_STATUS_SUCCESS;
}
static QDF_STATUS wapi_encap(struct wlan_crypto_key *key,
				qdf_nbuf_t wbuf,
				uint8_t encapdone,
				uint8_t hdrlen){
	uint8_t *ivp;
	struct wlan_crypto_cipher *cipher_table;
	uint8_t ivmic_len = 0;

	cipher_table = (struct wlan_crypto_cipher *)key->cipher_table;

	/*
	 * Copy down 802.11 header and add the IV, KeyID, and ExtIV.
	 */
	if (encapdone) {
		ivp = (uint8_t *)qdf_nbuf_data(wbuf);
	} else {
		ivmic_len = cipher_table->header + cipher_table->miclen;
		ivp = (uint8_t *)qdf_nbuf_push_head(wbuf, ivmic_len);
		qdf_mem_move(ivp, ivp + ivmic_len, hdrlen);

		qdf_mem_move(ivp + hdrlen + cipher_table->header,
			ivp + hdrlen + ivmic_len,
			(qdf_nbuf_len(wbuf) - hdrlen - ivmic_len));

		ivp = (uint8_t *) qdf_nbuf_data(wbuf);
	}

#if 0
	ivp += hdrlen;
	/* XXX wrap at 48 bits */
	key->keytsc++;

	ivp[0] = key->keyix;				    /* KeyID | ExtID */
	ivp[1] = 0;					    /* Reserved */
	ivp[2] = key->txiv[0];				/* PN0 */
	ivp[3] = key->txiv[1];				/* PN1 */
	ivp[4] = key->txiv[2];			       /* PN2 */
	ivp[5] = key->txiv[3];			       /* PN3 */
	ivp[6] = key->txiv[4];			       /* PN4 */
	ivp[7] = key->txiv[5];			       /* PN5 */
	ivp[8] = key->txiv[6];			       /* PN6 */
	ivp[9] = key->txiv[7];			       /* PN7 */
	ivp[10] = key->txiv[8];			/* PN8 */
	ivp[11] = key->txiv[9];			/* PN9 */
	ivp[12] = key->txiv[10];			 /* PN10 */
	ivp[13] = key->txiv[11];			 /* PN11 */
	ivp[14] = key->txiv[12];			 /* PN12 */
	ivp[15] = key->txiv[13];			 /* PN13 */
	ivp[16] = key->txiv[14];			/* PN14 */
	ivp[17] = key->txiv[15];			 /* PN15 */
#endif
	/*
	 * Finally, do software encrypt if neeed.
	 */
	if (key->flags & WLAN_CRYPTO_KEY_SWENCRYPT) {
//		if (!wlan_crypto_ccmp_encrypt(key->keyval,
//						 qdf_nbuf_data(wbuf),
//						 qdf_nbuf_len(wbuf), hdrlen)) {
			return QDF_STATUS_CRYPTO_ENCRYPT_FAILED;
//		}
	}


	return QDF_STATUS_SUCCESS;
}
static QDF_STATUS wapi_decap(struct wlan_crypto_key *key,
				qdf_nbuf_t wbuf,
				uint8_t tid,
				uint8_t hdrlen){
	uint8_t *origHdr;
	struct wlan_crypto_cipher *cipher_table;

	cipher_table = (struct wlan_crypto_cipher *)key->cipher_table;
	origHdr = (uint8_t *)qdf_nbuf_data(wbuf);
	tid = wlan_get_tid(qdf_nbuf_data(wbuf));
	/*
	* Copy up 802.11 header and strip crypto bits.
	*/
	if (!(key->flags & WLAN_CRYPTO_KEY_SWDECRYPT)) {
		qdf_mem_move(origHdr + cipher_table->header, origHdr, hdrlen);
		qdf_nbuf_pull_head(wbuf, cipher_table->header);
		qdf_nbuf_trim_tail(wbuf, cipher_table->trailer
						+ cipher_table->miclen);
	} else {
		qdf_nbuf_trim_tail(wbuf, cipher_table->header
					+ cipher_table->miclen);
	}
	return QDF_STATUS_SUCCESS;
}
static QDF_STATUS wapi_enmic(struct wlan_crypto_key *key,
				qdf_nbuf_t wbuf,
				uint8_t encapdone,
				uint8_t hdrlen){
	return QDF_STATUS_SUCCESS;
}
static QDF_STATUS wapi_demic(struct wlan_crypto_key *key,
				qdf_nbuf_t wbuf,
				uint8_t tid,
				uint8_t hdrlen){
	return QDF_STATUS_SUCCESS;
}

const struct wlan_crypto_cipher wapi_cipher_table = {
	"WPI_SMS4",
	WLAN_CRYPTO_CIPHER_WAPI_SMS4,
	WLAN_CRYPTO_WPI_SMS4_IVLEN + WLAN_CRYPTO_WPI_SMS4_KIDLEN
		+ WLAN_CRYPTO_WPI_SMS4_PADLEN,
	0,
	WLAN_CRYPTO_WPI_SMS4_MICLEN,
	256,
	wapi_setkey,
	wapi_encap,
	wapi_decap,
	wapi_enmic,
	wapi_demic,
};

const struct wlan_crypto_cipher wapi_gcm_sms4_cipher_table = {
	"WPI_GCM_SMS4",
	WLAN_CRYPTO_CIPHER_WAPI_GCM4,
	WLAN_CRYPTO_WPI_GCM_SMS4_IVLEN + WLAN_CRYPTO_WPI_GCM_SMS4_KIDLEN
		+ WLAN_CRYPTO_WPI_GCM_SMS4_PADLEN,
	0,
	WLAN_CRYPTO_WPI_GCM_SMS4_MICLEN,
	128,
	wapi_setkey,
	wapi_encap,
	wapi_decap,
	wapi_enmic,
	wapi_demic,
};

const struct wlan_crypto_cipher *wapi_register(void)
{
	return &wapi_cipher_table;
}

const struct wlan_crypto_cipher *wapi_register_gcm4(void)
{
	return &wapi_gcm_sms4_cipher_table;
}
