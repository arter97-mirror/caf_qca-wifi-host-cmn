/*
 * Copyright (c) 2017-2018 The Linux Foundation. All rights reserved.
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.
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
 * DOC: Public API initialization of crypto service with object manager
 */

#ifndef __WLAN_CRYPTO_OBJ_MGR_I_
#define __WLAN_CRYPTO_OBJ_MGR_I_

#ifdef WLAN_CRYPTO_WEP_OS_DERIVATIVE
static inline const struct wlan_crypto_cipher *wep_register(void)
{
	return NULL;
}
#else
const struct wlan_crypto_cipher *wep_register(void);
#endif

#ifdef WLAN_CRYPTO_TKIP_OS_DERIVATIVE
static inline const struct wlan_crypto_cipher *tkip_register(void)
{
	return NULL;
}
#else
const struct wlan_crypto_cipher *tkip_register(void);
#endif

#ifdef WLAN_CRYPTO_CCMP_OS_DERIVATIVE
static inline const struct wlan_crypto_cipher *ccmp_register(void)
{
	return NULL;
}

static inline const struct wlan_crypto_cipher *ccmp256_register(void)
{
	return NULL;
}
#else
const struct wlan_crypto_cipher *ccmp_register(void);
const struct wlan_crypto_cipher *ccmp256_register(void);
#endif

#ifdef WLAN_CRYPTO_GCMP_OS_DERIVATIVE
static inline const struct wlan_crypto_cipher *gcmp_register(void)
{
	return NULL;
}

static inline const struct wlan_crypto_cipher *gcmp256_register(void)
{
	return NULL;
}
#else
const struct wlan_crypto_cipher *gcmp_register(void);
const struct wlan_crypto_cipher *gcmp256_register(void);
#endif

#ifdef WLAN_CRYPTO_WAPI_OS_DERIVATIVE
static inline const struct wlan_crypto_cipher *wapi_register(void)
{
	return NULL;
}
#else
const struct wlan_crypto_cipher *wapi_register(void);
#endif

#ifdef WLAN_CRYPTO_FILS_OS_DERIVATIVE
static inline const struct wlan_crypto_cipher *fils_register(void)
{
	return NULL;
}
#else
/**
 * fils_register() - Register all callback functions to Crypto manager
 *
 * This function is invoked from crypto object manager to register
 * FILS specific callbacks.
 *
 * Return: Pointer to wlan_crypto_cipher Object
 */
const struct wlan_crypto_cipher *fils_register(void);
#endif


static inline void *wlan_get_vdev_crypto_obj(struct wlan_objmgr_vdev *vdev)
{
	void *crypto_priv;
	crypto_priv = wlan_objmgr_vdev_get_comp_private_obj(vdev,
							WLAN_UMAC_COMP_CRYPTO);

	return crypto_priv;
}

static inline void *wlan_get_peer_crypto_obj(struct wlan_objmgr_peer *peer)
{
	void *crypto_priv;
	crypto_priv = wlan_objmgr_peer_get_comp_private_obj(peer,
							WLAN_UMAC_COMP_CRYPTO);

	return crypto_priv;
}

/**
 * wlan_crypto_pmksa_aquire_lock() - Acquire the PMKSA cache lock
 *
 * Serializes all access to the per-vdev PMKSA cache
 * (crypto_params->pmksa[]) within the crypto module. Not to be used
 * outside wlan_crypto_global_api.c / wlan_crypto_obj_mgr.c - external
 * modules must use the wlan_crypto_get_*_pmksa_copy() APIs instead of
 * taking this lock themselves.
 *
 * Separate from the crypto key lock (wlan_crypto_aquire_lock()) and not
 * gated by any feature flag - used by SAE, roam-offload and FILS paths.
 */
void wlan_crypto_pmksa_aquire_lock(void);

/**
 * wlan_crypto_pmksa_release_lock() - Release the PMKSA cache lock
 */
void wlan_crypto_pmksa_release_lock(void);

#endif /* end of __WLAN_CRYPTO_OBJ_MGR_I_*/
