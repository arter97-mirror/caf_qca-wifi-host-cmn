/*
 * Copyright (c) 2017-2021 The Linux Foundation. All rights reserved.
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
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
 * DOC: qdf_crypto.c
 *
 * This source file contains linux specific definitions for QDF crypto APIs
 */

/* Include Files */
#include "qdf_crypto.h"
#include <linux/export.h>
#include <crypto/hash.h>
#include <crypto/aes.h>
#include <crypto/skcipher.h>
#include <crypto/aead.h>
#ifdef WLAN_FEATURE_11BN_SMD
#include <crypto/kpp.h>
#include <crypto/ecdh.h>
#include <linux/random.h>
#endif /* WLAN_FEATURE_11BN_SMD */
#include <linux/ieee80211.h>
#include <qdf_module.h>

/* Function Definitions and Documentation */
#define MAX_HMAC_ELEMENT_CNT 10

/*
 * xor: API to calculate xor
 * @a: first variable
 * @b: second variable
 * @len: length of variables
 */
static void xor(uint8_t *a, const uint8_t *b, size_t len)
{
	unsigned int i;

	for (i = 0; i < len; i++)
	a[i] ^= b[i];
}

int qdf_get_hash(uint8_t *type,
		uint8_t element_cnt, uint8_t *addr[], uint32_t *addr_len,
		int8_t *hash)
{
	return qdf_get_hmac_hash(type, NULL, 0, element_cnt,
				 addr, addr_len, hash);
}

int qdf_get_hmac_hash(uint8_t *type, uint8_t *key,
		uint32_t keylen,
		uint8_t element_cnt, uint8_t *addr[], uint32_t *addr_len,
		int8_t *hash)
{
	int i;
	size_t src_len[MAX_HMAC_ELEMENT_CNT];

	if (element_cnt > MAX_HMAC_ELEMENT_CNT) {
		QDF_TRACE(QDF_MODULE_ID_QDF, QDF_TRACE_LEVEL_ERROR,
			  FL("Invalid element count %d"), element_cnt);
		return -EINVAL;
	}

	for (i = 0; i < element_cnt; i++)
		src_len[i] = addr_len[i];

	return qdf_get_keyed_hash(type, key, keylen, (const uint8_t **)addr,
				  src_len, element_cnt,  hash);
}

QDF_STATUS
qdf_default_hmac_sha256_kdf(uint8_t *secret, uint32_t secret_len,
			    uint8_t *label, uint8_t *optional_data,
			    uint32_t optional_data_len, uint8_t *key,
			    uint32_t keylen)
{
	uint8_t tmp_hash[SHA256_DIGEST_SIZE] = {0};
	uint8_t count = 1;
	uint8_t *addr[4];
	uint32_t len[4];
	uint32_t current_position = 0, remaining_data = SHA256_DIGEST_SIZE;

	addr[0] = tmp_hash;
	len[0] = SHA256_DIGEST_SIZE;
	addr[1] = label;
	len[1] = strlen(label) + 1;
	addr[2] = optional_data;
	len[2] = optional_data_len;
	addr[3] = &count;
	len[3] = 1;

	if (keylen == 0 ||
	    (keylen > (WLAN_MAX_PRF_INTERATIONS_COUNT * SHA256_DIGEST_SIZE))) {
		qdf_err("invalid key length %d", keylen);
		return QDF_STATUS_E_FAILURE;
	}

	/* Create T1 */
	if (qdf_get_hmac_hash(HMAC_SHA256_CRYPTO_TYPE, secret, secret_len, 3,
			      &addr[1], &len[1], tmp_hash) < 0) {
		qdf_err("failed to get hmac hash");
		return QDF_STATUS_E_FAILURE;
	}

	/* Update hash from tmp_hash */
	qdf_mem_copy(key + current_position, tmp_hash, remaining_data);
	current_position += remaining_data;

	for (count = 2; current_position < keylen; count++) {
		remaining_data = keylen - current_position;
		if (remaining_data > SHA256_DIGEST_SIZE)
			remaining_data = SHA256_DIGEST_SIZE;

		/* Create T-n */
		if (qdf_get_hmac_hash(HMAC_SHA256_CRYPTO_TYPE, secret,
				      secret_len, 4, addr, len, tmp_hash) < 0) {
			qdf_err("failed to get hmac hash");
			return QDF_STATUS_E_FAILURE;
		}
		/* Update hash from tmp_hash */
		qdf_mem_copy(key + current_position, tmp_hash, remaining_data);
		current_position += remaining_data;
	}

	return QDF_STATUS_SUCCESS;
}

/* qdf_update_dbl from RFC 5297. Length of d is AES_BLOCK_SIZE (128 bits) */
void qdf_update_dbl(uint8_t *d)
{
	int i;
	uint8_t msb, msb_prev = 0;

	/* left shift by 1 */
	for (i = AES_BLOCK_SIZE - 1; i >= 0; i--) {
		msb = d[i] & 0x80;
		d[i] = d[i] << 1;
		d[i] += msb_prev ? 1 : 0;
		msb_prev = msb;
	}

	if (msb)
		d[AES_BLOCK_SIZE - 1] ^= 0x87;
}

static inline void xor_128(const uint8_t *a, const uint8_t *b, uint8_t *out)
{
	uint8_t i;

	for (i = 0; i < AES_BLOCK_SIZE; i++)
		out[i] = a[i] ^ b[i];
}

static inline void leftshift_onebit(const uint8_t *input, uint8_t *output)
{
	int i, overflow = 0;

	for (i = (AES_BLOCK_SIZE - 1); i >= 0; i--) {
		output[i] = input[i] << 1;
		output[i] |= overflow;
		overflow = (input[i] & 0x80) ? 1 : 0;
	}
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
static void
generate_subkey(struct crypto_aes_ctx *aes_ctx, uint8_t *k1, uint8_t *k2)
{
	uint8_t l[AES_BLOCK_SIZE] = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};
	uint8_t tmp[AES_BLOCK_SIZE];
	const uint8_t const_rb[AES_BLOCK_SIZE] = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x87
	};
	const uint8_t const_zero[AES_BLOCK_SIZE] = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};

	aes_encrypt(aes_ctx, l, const_zero);

	if ((l[0] & 0x80) == 0) {       /* If MSB(l) = 0, then k1 = l << 1 */
		leftshift_onebit(l, k1);
	} else {                /* Else k1 = ( l << 1 ) (+) Rb */
		leftshift_onebit(l, tmp);
		xor_128(tmp, const_rb, k1);
	}

	if ((k1[0] & 0x80) == 0) {
		leftshift_onebit(k1, k2);
	} else {
		leftshift_onebit(k1, tmp);
		xor_128(tmp, const_rb, k2);
	}
}
#else
static void
generate_subkey(struct crypto_cipher *tfm, uint8_t *k1, uint8_t *k2)
{
	uint8_t l[AES_BLOCK_SIZE], tmp[AES_BLOCK_SIZE];
	const uint8_t const_rb[AES_BLOCK_SIZE] = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x87
	};
	const uint8_t const_zero[AES_BLOCK_SIZE] = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};

	crypto_cipher_encrypt_one(tfm, l, const_zero);

	if ((l[0] & 0x80) == 0) {       /* If MSB(l) = 0, then k1 = l << 1 */
		leftshift_onebit(l, k1);
	} else {                /* Else k1 = ( l << 1 ) (+) Rb */
		leftshift_onebit(l, tmp);
		xor_128(tmp, const_rb, k1);
	}

	if ((k1[0] & 0x80) == 0) {
		leftshift_onebit(k1, k2);
	} else {
		leftshift_onebit(k1, tmp);
		xor_128(tmp, const_rb, k2);
	}
}
#endif

static inline void padding(const uint8_t *lastb, uint8_t *pad, uint16_t length)
{
	uint8_t j;

	/* original last block */
	for (j = 0; j < AES_BLOCK_SIZE; j++) {
		if (j < length)
			pad[j] = lastb[j];
		else if (j == length)
			pad[j] = 0x80;
		else
			pad[j] = 0x00;
	}
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
int qdf_crypto_aes_128_cmac(const uint8_t *key, const uint8_t *data,
			    uint16_t len, uint8_t *mic)
{
	uint8_t x[AES_BLOCK_SIZE], y[AES_BLOCK_SIZE];
	uint8_t m_last[AES_BLOCK_SIZE], padded[AES_BLOCK_SIZE];
	uint8_t k1[AES_KEYSIZE_128], k2[AES_KEYSIZE_128];
	int cmp_blk;
	int i, num_block = (len + 15) / AES_BLOCK_SIZE;
	struct crypto_aes_ctx aes_ctx;
	int ret;

	/*
	 * Calculate MIC and then copy
	 */
	ret = aes_expandkey(&aes_ctx, key, AES_KEYSIZE_128);
	if (ret) {
		qdf_err("aes_expandkey failed (%d)", ret);
		return ret;
	}

	generate_subkey(&aes_ctx, k1, k2);

	if (num_block == 0) {
		num_block = 1;
		cmp_blk = 0;
	} else {
		cmp_blk = ((len % AES_BLOCK_SIZE) == 0) ? 1 : 0;
	}

	if (cmp_blk) {
		/* Last block is complete block */
		xor_128(&data[AES_BLOCK_SIZE * (num_block - 1)], k1, m_last);
	} else {
		/* Last block is not complete block */
		padding(&data[AES_BLOCK_SIZE * (num_block - 1)], padded,
			len % AES_BLOCK_SIZE);
		xor_128(padded, k2, m_last);
	}

	for (i = 0; i < AES_BLOCK_SIZE; i++)
		x[i] = 0;

	for (i = 0; i < (num_block - 1); i++) {
		/* y = Mi (+) x */
		xor_128(x, &data[AES_BLOCK_SIZE * i], y);
		/* x = AES-128(KEY, y) */
		aes_encrypt(&aes_ctx, x, y);
	}

	xor_128(x, m_last, y);
	aes_encrypt(&aes_ctx, x, y);
	memzero_explicit(&aes_ctx, sizeof(aes_ctx));

	memcpy(mic, x, CMAC_TLEN);

	return 0;
}
#else
int qdf_crypto_aes_128_cmac(const uint8_t *key, const uint8_t *data,
			    uint16_t len, uint8_t *mic)
{
	uint8_t x[AES_BLOCK_SIZE], y[AES_BLOCK_SIZE];
	uint8_t m_last[AES_BLOCK_SIZE], padded[AES_BLOCK_SIZE];
	uint8_t k1[AES_KEYSIZE_128], k2[AES_KEYSIZE_128];
	int cmp_blk;
	int i, num_block = (len + 15) / AES_BLOCK_SIZE;
	struct crypto_cipher *tfm;
	int ret;

	/*
	 * Calculate MIC and then copy
	 */
	tfm = crypto_alloc_cipher("aes", 0, CRYPTO_ALG_ASYNC);
	if (IS_ERR(tfm)) {
		ret = PTR_ERR(tfm);
		qdf_err("crypto_alloc_cipher failed (%d)", ret);
		return ret;
	}

	ret = crypto_cipher_setkey(tfm, key, AES_KEYSIZE_128);
	if (ret) {
		qdf_err("crypto_cipher_setkey failed (%d)", ret);
		crypto_free_cipher(tfm);
		return ret;
	}

	generate_subkey(tfm, k1, k2);

	if (num_block == 0) {
		num_block = 1;
		cmp_blk = 0;
	} else {
		cmp_blk = ((len % AES_BLOCK_SIZE) == 0) ? 1 : 0;
	}

	if (cmp_blk) {
		/* Last block is complete block */
		xor_128(&data[AES_BLOCK_SIZE * (num_block - 1)], k1, m_last);
	} else {
		/* Last block is not complete block */
		padding(&data[AES_BLOCK_SIZE * (num_block - 1)], padded,
			len % AES_BLOCK_SIZE);
		xor_128(padded, k2, m_last);
	}

	for (i = 0; i < AES_BLOCK_SIZE; i++)
		x[i] = 0;

	for (i = 0; i < (num_block - 1); i++) {
		/* y = Mi (+) x */
		xor_128(x, &data[AES_BLOCK_SIZE * i], y);
		/* x = AES-128(KEY, y) */
		crypto_cipher_encrypt_one(tfm, x, y);
	}

	xor_128(x, m_last, y);
	crypto_cipher_encrypt_one(tfm, x, y);

	crypto_free_cipher(tfm);

	memcpy(mic, x, CMAC_TLEN);

	return 0;
}
#endif

/**
 * set_desc_flags() - set flags variable in the shash_desc struct
 * @desc: pointer to shash_desc struct
 * @tfm: pointer to crypto_shash struct
 *
 * Set the flags variable in the shash_desc struct by getting the flag
 * from the crypto_hash struct. The flag is not actually used, prompting
 * its removal from kernel code in versions 5.2 and above. Thus, for
 * versions 5.2 and above, do not set the flag variable of shash_desc.
 */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 2, 0))
static void set_desc_flags(struct shash_desc *desc, struct crypto_shash *tfm)
{
	desc->flags = crypto_shash_get_flags(tfm);
}
#else
static void set_desc_flags(struct shash_desc *desc, struct crypto_shash *tfm)
{
}
#endif

int qdf_get_keyed_hash(const char *alg, const uint8_t *key,
			unsigned int key_len, const uint8_t *src[],
			size_t *src_len, size_t num_elements, uint8_t *out)
{
	struct crypto_shash *tfm;
	int ret;
	size_t i;

	tfm = crypto_alloc_shash(alg, 0, CRYPTO_ALG_ASYNC);
	if (IS_ERR(tfm)) {
		QDF_TRACE(QDF_MODULE_ID_QDF, QDF_TRACE_LEVEL_ERROR,
			  FL("Failed to allocate transformation for %s: %ld"),
			  alg, PTR_ERR(tfm));
		return -EINVAL;
	}

	if (key && key_len) {
		ret = crypto_shash_setkey(tfm, key, key_len);
		if (ret) {
			QDF_TRACE(QDF_MODULE_ID_QDF, QDF_TRACE_LEVEL_ERROR,
				  FL("Set key failed for %s, ret:%d"),
				  alg, -ret);
			goto error;
		}
	}

	do {
		SHASH_DESC_ON_STACK(desc, tfm);
		desc->tfm = tfm;
		set_desc_flags(desc, tfm);

		ret = crypto_shash_init(desc);
		if (ret) {
			QDF_TRACE(QDF_MODULE_ID_QDF, QDF_TRACE_LEVEL_ERROR,
				  FL("Failed to init hash for %s, ret:%d"),
				  alg, -ret);
			goto error;
		}

		for (i = 0; i < num_elements; i++) {
			ret = crypto_shash_update(desc, src[i], src_len[i]);
			if (ret) {
				QDF_TRACE(QDF_MODULE_ID_QDF,
					  QDF_TRACE_LEVEL_ERROR,
					  FL("Failed to update hash for %s, ret:%d"),
					  alg, -ret);
				goto error;
			}
		}

		ret = crypto_shash_final(desc, out);
		if (ret)
			QDF_TRACE(QDF_MODULE_ID_QDF, QDF_TRACE_LEVEL_ERROR,
				  FL("Failed to get digest for %s, ret:%d"),
				  alg, -ret);
	} while (0);

error:
	crypto_free_shash(tfm);
	return ret;
}

qdf_export_symbol(qdf_get_keyed_hash);

/* AES String to Vector from RFC 5297, 'out' should be of length AES_BLOCK_SIZE
 */
int qdf_aes_s2v(const uint8_t *key, unsigned int key_len, const uint8_t *s[],
		   size_t s_len[], size_t num_s, uint8_t *out)
{
	const char *alg = "cmac(aes)";
	uint8_t d[AES_BLOCK_SIZE];
	uint8_t buf[AES_BLOCK_SIZE] = { 0 };
	size_t buf_len = AES_BLOCK_SIZE;
	const uint8_t *a[1];
	unsigned int i;
	uint8_t *t = NULL;
	size_t t_len;
	int ret;

	if (num_s == 0) {
		/* V = AES-CMAC(K, <one>) */
		buf[0] = 0x01;
		a[0] = buf;
		ret = qdf_get_keyed_hash(alg, key, key_len, a, &buf_len, 1,
					 out);
		return ret;
	}

	/* D = AES-CMAC(K, <zero>) */
	a[0] = buf;
	ret = qdf_get_keyed_hash(alg, key, key_len, a, &buf_len, 1, d);
	if (ret)
		goto error;

	for (i = 0; i < num_s - 1; i++) {
		/* D = qdf_update_dbl(D) xor AES-CMAC(K, Si) */
		qdf_update_dbl(d);
		ret = qdf_get_keyed_hash(alg, key, key_len, &s[i], &s_len[i], 1,
					 buf);
		if (ret)
			goto error;
		xor(d, buf, AES_BLOCK_SIZE);
	}

	if (s_len[i] >= AES_BLOCK_SIZE) {
		/* len(Sn) >= 128 */
		/* T = Sn xorend D */
		t = qdf_mem_malloc(s_len[i]);
		if (!t)
			return -EINVAL;
		qdf_mem_copy(t, s[i], s_len[i]);
		xor(t + s_len[i] - AES_BLOCK_SIZE, d, AES_BLOCK_SIZE);
		t_len = s_len[i];
	} else {
		/* len(Sn) < 128 */
		/* T = qdf_update_dbl(D) xor pad(Sn) */
		qdf_update_dbl(d);
		qdf_mem_zero(buf, AES_BLOCK_SIZE);
		qdf_mem_copy(buf, s[i], s_len[i]);
		buf[s_len[i]] = 0x80;
		xor(d, s[i], AES_BLOCK_SIZE);
		t = d;
		t_len = AES_BLOCK_SIZE;
	}

	/* V = AES-CMAC(K, T) */
	a[0] = t;
	ret = qdf_get_keyed_hash(alg, key, key_len, a, &t_len, 1, out);

error:
	if (t && t != d)
		qdf_mem_free(t);
	return ret;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 3, 0))
int qdf_aes_ctr(const uint8_t *key, unsigned int key_len, uint8_t *siv,
		const uint8_t *src, size_t src_len, uint8_t *dest, bool enc)
{
	struct crypto_skcipher *tfm;
	struct skcipher_request *req = NULL;
	struct scatterlist sg_in, sg_out;
	int ret;

	if (!IS_VALID_CTR_KEY_LEN(key_len)) {
		QDF_TRACE(QDF_MODULE_ID_QDF, QDF_TRACE_LEVEL_ERROR,
			  FL("Invalid key length: %u"), key_len);
		return -EINVAL;
	}

	tfm = crypto_alloc_skcipher("ctr(aes)", 0, CRYPTO_ALG_ASYNC);
	if (IS_ERR(tfm)) {
		QDF_TRACE(QDF_MODULE_ID_QDF, QDF_TRACE_LEVEL_ERROR,
			  FL("Failed to alloc transformation for ctr(aes):%ld"),
			  PTR_ERR(tfm));
		return -EAGAIN;
	}

	req = skcipher_request_alloc(tfm, GFP_KERNEL);
	if (!req) {
		QDF_TRACE(QDF_MODULE_ID_QDF, QDF_TRACE_LEVEL_ERROR,
			  FL("Failed to allocate request for ctr(aes)"));
		crypto_free_skcipher(tfm);
		return -EAGAIN;
	}

	ret = crypto_skcipher_setkey(tfm, key, key_len);
	if (ret) {
		QDF_TRACE(QDF_MODULE_ID_QDF, QDF_TRACE_LEVEL_ERROR,
			  FL("Set key failed for ctr(aes), ret:%d"), -ret);
		skcipher_request_free(req);
		crypto_free_skcipher(tfm);
		return ret;
	}

	sg_init_one(&sg_in, src, src_len);
	sg_init_one(&sg_out, dest, src_len);
	skcipher_request_set_crypt(req, &sg_in, &sg_out, src_len, siv);

	if (enc)
		ret = crypto_skcipher_encrypt(req);
	else
		ret = crypto_skcipher_decrypt(req);

	if (ret) {
		QDF_TRACE(QDF_MODULE_ID_QDF, QDF_TRACE_LEVEL_ERROR,
			  FL("%s failed for ctr(aes), ret:%d"),
			  enc ? "Encryption" : "Decryption", -ret);
	}

	skcipher_request_free(req);
	crypto_free_skcipher(tfm);
	return ret;
}
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 18, 0))
int qdf_aes_ctr(const uint8_t *key, unsigned int key_len, uint8_t *siv,
		const uint8_t *src, size_t src_len, uint8_t *dest, bool enc)
{
	struct crypto_ablkcipher *tfm;
	struct ablkcipher_request *req = NULL;
	struct scatterlist sg_in, sg_out;
	int ret;

	if (!IS_VALID_CTR_KEY_LEN(key_len)) {
		QDF_TRACE(QDF_MODULE_ID_QDF, QDF_TRACE_LEVEL_ERROR,
			  FL("Invalid key length: %u"), key_len);
		return -EINVAL;
	}

	tfm = crypto_alloc_ablkcipher("ctr(aes)", 0, CRYPTO_ALG_ASYNC);
	if (IS_ERR(tfm)) {
		QDF_TRACE(QDF_MODULE_ID_QDF, QDF_TRACE_LEVEL_ERROR,
			  FL("Failed to alloc transformation for ctr(aes):%ld"),
			  PTR_ERR(tfm));
		return -EAGAIN;
	}

	req = ablkcipher_request_alloc(tfm, GFP_KERNEL);
	if (!req) {
		QDF_TRACE(QDF_MODULE_ID_QDF, QDF_TRACE_LEVEL_ERROR,
			  FL("Failed to allocate request for ctr(aes)"));
		crypto_free_ablkcipher(tfm);
		return -EAGAIN;
	}

	ret = crypto_ablkcipher_setkey(tfm, key, key_len);
	if (ret) {
		QDF_TRACE(QDF_MODULE_ID_QDF, QDF_TRACE_LEVEL_ERROR,
			  FL("Set key failed for ctr(aes), ret:%d"), -ret);
		ablkcipher_request_free(req);
		crypto_free_ablkcipher(tfm);
		return ret;
	}

	sg_init_one(&sg_in, src, src_len);
	sg_init_one(&sg_out, dest, src_len);
	ablkcipher_request_set_crypt(req, &sg_in, &sg_out, src_len, siv);

	if (enc)
		ret = crypto_ablkcipher_encrypt(req);
	else
		ret = crypto_ablkcipher_decrypt(req);

	if (ret) {
		QDF_TRACE(QDF_MODULE_ID_QDF, QDF_TRACE_LEVEL_ERROR,
			  FL("%s failed for ctr(aes), ret:%d"),
			  enc ? "Encryption" : "Decryption", -ret);
	}

	ablkcipher_request_free(req);
	crypto_free_ablkcipher(tfm);

	return ret;
}
#else
int qdf_aes_ctr(const uint8_t *key, unsigned int key_len, uint8_t *siv,
		const uint8_t *src, size_t src_len, uint8_t *dest, bool enc)
{
	return -EINVAL;
}
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 0))
int qdf_crypto_aes_gmac(const uint8_t *key, uint16_t key_length,
			uint8_t *iv, const uint8_t *aad,
			const uint8_t *data, uint16_t data_len, uint8_t *mic)
{
	struct crypto_aead *tfm;
	int ret = 0;
	struct scatterlist sg[4];
	uint16_t req_size;
	struct aead_request *req = NULL;
	uint8_t *aad_ptr, *input;

	tfm = crypto_alloc_aead("gcm(aes)", 0, CRYPTO_ALG_ASYNC);
	if (IS_ERR(tfm)) {
		ret = PTR_ERR(tfm);
		tfm = NULL;
		QDF_TRACE(QDF_MODULE_ID_QDF, QDF_TRACE_LEVEL_ERROR,
			  "%s: crypto_alloc_aead failed (%d)", __func__, ret);
		goto err_tfm;
	}

	ret = crypto_aead_setkey(tfm, key, key_length);
	if (ret) {
		QDF_TRACE(QDF_MODULE_ID_QDF, QDF_TRACE_LEVEL_ERROR,
			  "crypto_aead_setkey failed (%d)", ret);
		goto err_tfm;
	}

	ret = crypto_aead_setauthsize(tfm, IEEE80211_MMIE_GMAC_MICLEN);
	if (ret) {
		QDF_TRACE(QDF_MODULE_ID_QDF, QDF_TRACE_LEVEL_ERROR,
			  "crypto_aead_setauthsize failed (%d)", ret);
		goto err_tfm;
	}

	/* Prepare aead request */
	req_size = sizeof(*req) + crypto_aead_reqsize(tfm) +
			IEEE80211_MMIE_GMAC_MICLEN + AAD_LEN;
	req = qdf_mem_malloc(req_size);
	if (!req) {
		ret = -ENOMEM;
		goto err_tfm;
	}

	input = (uint8_t *)req + sizeof(*req) + crypto_aead_reqsize(tfm);
	aad_ptr = input + IEEE80211_MMIE_GMAC_MICLEN;
	qdf_mem_copy(aad_ptr, aad, AAD_LEN);

	/* Scatter list operations */
	sg_init_table(sg, 4);
	sg_set_buf(&sg[0], aad_ptr, AAD_LEN);
	sg_set_buf(&sg[1], data, data_len);
	sg_set_buf(&sg[2], input, IEEE80211_MMIE_GMAC_MICLEN);
	sg_set_buf(&sg[3], mic, IEEE80211_MMIE_GMAC_MICLEN);

	aead_request_set_tfm(req, tfm);
	aead_request_set_crypt(req, sg, sg, 0, iv);
	aead_request_set_ad(req,
			    AAD_LEN + data_len + IEEE80211_MMIE_GMAC_MICLEN);
	crypto_aead_encrypt(req);

err_tfm:
	if (tfm)
		crypto_free_aead(tfm);

	if (req)
		qdf_mem_free(req);

	return ret;
}
#else
int qdf_crypto_aes_gmac(uint8_t *key, uint16_t key_length,
			uint8_t *iv, uint8_t *aad, uint8_t *data,
			uint16_t data_len, uint8_t *mic)
{
	return -EINVAL;
}
#endif

#ifdef WLAN_FEATURE_11BN_SMD
QDF_STATUS qdf_crypto_ecdh_init(void)
{
	struct crypto_kpp *test_tfm;

	/* Verify ECDH support is available */
	test_tfm = crypto_alloc_kpp("ecdh-nist-p256", 0, 0);
	if (IS_ERR(test_tfm)) {
		QDF_TRACE(QDF_MODULE_ID_QDF, QDF_TRACE_LEVEL_ERROR,
			  "ECDH not supported: %ld", PTR_ERR(test_tfm));
		return QDF_STATUS_E_FAILURE;
	}

	crypto_free_kpp(test_tfm);
	QDF_TRACE(QDF_MODULE_ID_QDF, QDF_TRACE_LEVEL_INFO,
		  "ECDH crypto initialized successfully");
	return QDF_STATUS_SUCCESS;
}

qdf_export_symbol(qdf_crypto_ecdh_init);

void qdf_crypto_ecdh_deinit(void)
{
	QDF_TRACE(QDF_MODULE_ID_QDF, QDF_TRACE_LEVEL_INFO,
		  "ECDH crypto deinitialized");
}

qdf_export_symbol(qdf_crypto_ecdh_deinit);

/**
 * qdf_crypto_ecdh_set_secret() - Set private key in transform
 * @tfm: KPP transform
 * @private_key: Private key buffer
 * @key_size: Size of private key
 *
 * Internal helper to encode and set private key in crypto transform.
 *
 * Return: QDF_STATUS_SUCCESS on success, error code otherwise
 */
static QDF_STATUS qdf_crypto_ecdh_set_secret(struct crypto_kpp *tfm,
					     const uint8_t *private_key,
					     size_t key_size)
{
	char *key_buf = NULL;
	unsigned int buf_size;
	int ret;
	QDF_STATUS status = QDF_STATUS_E_FAILURE;
	struct ecdh params = {0};

	params.key = (char *)private_key;
	params.key_size = key_size;

	buf_size = crypto_ecdh_key_len(&params);
	if (buf_size == 0) {
		QDF_TRACE(QDF_MODULE_ID_QDF, QDF_TRACE_LEVEL_ERROR,
			  "Invalid ECDH key buffer size");
		return QDF_STATUS_E_INVAL;
	}

	key_buf = qdf_mem_malloc(buf_size);
	if (!key_buf)
		return QDF_STATUS_E_NOMEM;

	ret = crypto_ecdh_encode_key(key_buf, buf_size, &params);
	if (ret < 0) {
		QDF_TRACE(QDF_MODULE_ID_QDF, QDF_TRACE_LEVEL_ERROR,
			  "Failed to encode ECDH key: %d", ret);
		goto out;
	}

	ret = crypto_kpp_set_secret(tfm, key_buf, buf_size);
	if (ret < 0) {
		QDF_TRACE(QDF_MODULE_ID_QDF, QDF_TRACE_LEVEL_ERROR,
			  "Failed to set ECDH secret: %d", ret);
		goto out;
	}

	status = QDF_STATUS_SUCCESS;

out:
	if (key_buf) {
		qdf_mem_zero(key_buf, buf_size);
		qdf_mem_free(key_buf);
	}
	return status;
}

QDF_STATUS qdf_crypto_ecdh_generate_private_key(uint8_t *private_key,
						size_t key_size)
{
	int attempts = 0;
	int ret;
	char *key_buf = NULL;
	unsigned int buf_size;
	struct ecdh params = {0};
	struct crypto_kpp *test_tfm = NULL;
	const int MAX_ATTEMPTS = 10;

	if (!private_key || key_size != QDF_ECDH_PRIVATE_KEY_SIZE) {
		QDF_TRACE(QDF_MODULE_ID_QDF, QDF_TRACE_LEVEL_ERROR,
			  "Invalid parameters for private key generation");
		return QDF_STATUS_E_INVAL;
	}

	test_tfm = crypto_alloc_kpp("ecdh-nist-p256", 0, 0);
	if (IS_ERR(test_tfm)) {
		QDF_TRACE(QDF_MODULE_ID_QDF, QDF_TRACE_LEVEL_ERROR,
			  "Failed to allocate test transform");
		return QDF_STATUS_E_FAILURE;
	}

	params.key_size = key_size;
	buf_size = crypto_ecdh_key_len(&params);

	key_buf = qdf_mem_malloc(buf_size);
	if (!key_buf) {
		crypto_free_kpp(test_tfm);
		return QDF_STATUS_E_NOMEM;
	}

	/* Generate and validate private key */
	do {
		get_random_bytes(private_key, key_size);
		attempts++;

		params.key = (char *)private_key;
		params.key_size = key_size;

		ret = crypto_ecdh_encode_key(key_buf, buf_size, &params);
		if (ret < 0)
			continue;

		ret = crypto_kpp_set_secret(test_tfm, key_buf, buf_size);
		if (ret == 0)
			break;  /* Valid key found */

	} while (attempts < MAX_ATTEMPTS);

	qdf_mem_zero(key_buf, buf_size);
	qdf_mem_free(key_buf);
	crypto_free_kpp(test_tfm);

	if (ret < 0) {
		QDF_TRACE(QDF_MODULE_ID_QDF, QDF_TRACE_LEVEL_ERROR,
			  "Failed to generate valid private key after %d attempts",
			  attempts);
		return QDF_STATUS_E_FAILURE;
	}

	QDF_TRACE(QDF_MODULE_ID_QDF, QDF_TRACE_LEVEL_INFO,
		  "Generated ECDH private key (%zu bytes) in %d attempt(s)",
		  key_size, attempts);
	return QDF_STATUS_SUCCESS;
}

qdf_export_symbol(qdf_crypto_ecdh_generate_private_key);

QDF_STATUS qdf_crypto_ecdh_generate_public_key(const uint8_t *private_key,
					       size_t private_key_size,
					       uint8_t *public_key,
					       size_t *public_key_size)
{
	struct crypto_kpp *tfm = NULL;
	struct kpp_request *req = NULL;
	struct scatterlist *sg_out = NULL;
	uint8_t *output_buf = NULL;
	unsigned int output_buf_size;
	QDF_STATUS status = QDF_STATUS_E_FAILURE;
	int ret;

	if (!private_key || !public_key || !public_key_size ||
	    private_key_size != QDF_ECDH_PRIVATE_KEY_SIZE) {
		QDF_TRACE(QDF_MODULE_ID_QDF, QDF_TRACE_LEVEL_ERROR,
			  "Invalid parameters for public key generation");
		return QDF_STATUS_E_INVAL;
	}

	/* Allocate transform */
	tfm = crypto_alloc_kpp("ecdh-nist-p256", 0, 0);
	if (IS_ERR(tfm)) {
		QDF_TRACE(QDF_MODULE_ID_QDF, QDF_TRACE_LEVEL_ERROR,
			  "Failed to allocate KPP transform");
		return QDF_STATUS_E_FAILURE;
	}

	/* Set private key */
	status = qdf_crypto_ecdh_set_secret(tfm, private_key, private_key_size);
	if (status != QDF_STATUS_SUCCESS) {
		QDF_TRACE(QDF_MODULE_ID_QDF, QDF_TRACE_LEVEL_ERROR,
			  "Failed to set private key");
		goto out;
	}

	/* Get required output buffer size */
	output_buf_size = crypto_kpp_maxsize(tfm);
	if (output_buf_size == 0) {
		QDF_TRACE(QDF_MODULE_ID_QDF, QDF_TRACE_LEVEL_ERROR,
			  "Invalid output buffer size");
		status = QDF_STATUS_E_FAILURE;
		goto out;
	}

	/* Allocate output buffer on heap (CONFIG_VMAP_STACK) */
	output_buf = qdf_mem_malloc(output_buf_size);
	if (!output_buf) {
		status = QDF_STATUS_E_NOMEM;
		goto out;
	}

	/* Allocate scatterlist on heap (CONFIG_VMAP_STACK) */
	sg_out = qdf_mem_malloc(sizeof(*sg_out));
	if (!sg_out) {
		status = QDF_STATUS_E_NOMEM;
		goto out;
	}

	sg_init_one(sg_out, output_buf, output_buf_size);

	/* Allocate request */
	req = kpp_request_alloc(tfm, GFP_KERNEL);
	if (!req) {
		QDF_TRACE(QDF_MODULE_ID_QDF, QDF_TRACE_LEVEL_ERROR,
			  "Failed to allocate KPP request");
		status = QDF_STATUS_E_NOMEM;
		goto out;
	}

	kpp_request_set_input(req, NULL, 0);
	kpp_request_set_output(req, sg_out, output_buf_size);

	/* Generate public key */
	ret = crypto_kpp_generate_public_key(req);
	if (ret < 0) {
		QDF_TRACE(QDF_MODULE_ID_QDF, QDF_TRACE_LEVEL_ERROR,
			  "Failed to generate public key: %d", ret);
		status = QDF_STATUS_E_FAILURE;
		goto out;
	}

	/* Copy public key to output buffer */
	if (output_buf_size > *public_key_size) {
		QDF_TRACE(QDF_MODULE_ID_QDF, QDF_TRACE_LEVEL_ERROR,
			  "Output buffer too small: need %u, have %zu",
			  output_buf_size, *public_key_size);
		status = QDF_STATUS_E_NOMEM;
		goto out;
	}

	qdf_mem_copy(public_key, output_buf, output_buf_size);
	*public_key_size = output_buf_size;

	QDF_TRACE(QDF_MODULE_ID_QDF, QDF_TRACE_LEVEL_INFO,
		  "Generated ECDH public key (%zu bytes)", *public_key_size);
	status = QDF_STATUS_SUCCESS;

out:
	if (req)
		kpp_request_free(req);
	if (sg_out)
		qdf_mem_free(sg_out);
	if (output_buf) {
		qdf_mem_zero(output_buf, output_buf_size);
		qdf_mem_free(output_buf);
	}
	if (tfm)
		crypto_free_kpp(tfm);

	return status;
}

qdf_export_symbol(qdf_crypto_ecdh_generate_public_key);

QDF_STATUS qdf_crypto_ecdh_compute_shared_secret(const uint8_t *private_key,
						 size_t private_key_size,
						 const uint8_t *peer_public_key,
						 size_t peer_public_key_size,
						 uint8_t *shared_secret,
						 size_t *shared_secret_size)
{
	struct crypto_kpp *tfm = NULL;
	struct kpp_request *req = NULL;
	struct scatterlist *sg_in = NULL;
	struct scatterlist *sg_out = NULL;
	uint8_t *input_buf = NULL;
	uint8_t *output_buf = NULL;
	unsigned int output_buf_size;
	QDF_STATUS status = QDF_STATUS_E_FAILURE;
	int ret;

	if (!private_key || !peer_public_key || !shared_secret ||
	    !shared_secret_size ||
	    private_key_size != QDF_ECDH_PRIVATE_KEY_SIZE) {
		QDF_TRACE(QDF_MODULE_ID_QDF, QDF_TRACE_LEVEL_ERROR,
			  "Invalid parameters for shared secret computation");
		return QDF_STATUS_E_INVAL;
	}

	/* Allocate transform */
	tfm = crypto_alloc_kpp("ecdh-nist-p256", 0, 0);
	if (IS_ERR(tfm)) {
		QDF_TRACE(QDF_MODULE_ID_QDF, QDF_TRACE_LEVEL_ERROR,
			  "Failed to allocate KPP transform");
		return QDF_STATUS_E_FAILURE;
	}

	/* Set private key */
	status = qdf_crypto_ecdh_set_secret(tfm, private_key, private_key_size);
	if (status != QDF_STATUS_SUCCESS) {
		QDF_TRACE(QDF_MODULE_ID_QDF, QDF_TRACE_LEVEL_ERROR,
			  "Failed to set private key");
		goto out;
	}

	/* Get required output buffer size */
	output_buf_size = crypto_kpp_maxsize(tfm);
	if (output_buf_size == 0) {
		QDF_TRACE(QDF_MODULE_ID_QDF, QDF_TRACE_LEVEL_ERROR,
			  "Invalid output buffer size");
		status = QDF_STATUS_E_FAILURE;
		goto out;
	}

	/* Allocate input buffer on heap (CONFIG_VMAP_STACK) */
	input_buf = qdf_mem_malloc(peer_public_key_size);
	if (!input_buf) {
		status = QDF_STATUS_E_NOMEM;
		goto out;
	}
	qdf_mem_copy(input_buf, peer_public_key, peer_public_key_size);

	/* Allocate output buffer on heap (CONFIG_VMAP_STACK) */
	output_buf = qdf_mem_malloc(output_buf_size);
	if (!output_buf) {
		status = QDF_STATUS_E_NOMEM;
		goto out;
	}

	/* Allocate scatterlists on heap (CONFIG_VMAP_STACK) */
	sg_in = qdf_mem_malloc(sizeof(*sg_in));
	if (!sg_in) {
		status = QDF_STATUS_E_NOMEM;
		goto out;
	}

	sg_out = qdf_mem_malloc(sizeof(*sg_out));
	if (!sg_out) {
		status = QDF_STATUS_E_NOMEM;
		goto out;
	}

	sg_init_one(sg_in, input_buf, peer_public_key_size);
	sg_init_one(sg_out, output_buf, output_buf_size);

	/* Allocate request */
	req = kpp_request_alloc(tfm, GFP_KERNEL);
	if (!req) {
		QDF_TRACE(QDF_MODULE_ID_QDF, QDF_TRACE_LEVEL_ERROR,
			  "Failed to allocate KPP request");
		status = QDF_STATUS_E_NOMEM;
		goto out;
	}

	kpp_request_set_input(req, sg_in, peer_public_key_size);
	kpp_request_set_output(req, sg_out, output_buf_size);

	/* Compute shared secret */
	ret = crypto_kpp_compute_shared_secret(req);
	if (ret < 0) {
		QDF_TRACE(QDF_MODULE_ID_QDF, QDF_TRACE_LEVEL_ERROR,
			  "Failed to compute shared secret: %d", ret);
		status = QDF_STATUS_E_FAILURE;
		goto out;
	}

	/* Copy shared secret to output buffer */
	if (output_buf_size > *shared_secret_size) {
		QDF_TRACE(QDF_MODULE_ID_QDF, QDF_TRACE_LEVEL_ERROR,
			  "Output buffer too small: need %u, have %zu",
			  output_buf_size, *shared_secret_size);
		status = QDF_STATUS_E_NOMEM;
		goto out;
	}

	qdf_mem_copy(shared_secret, output_buf, output_buf_size);
	*shared_secret_size = output_buf_size;

	QDF_TRACE(QDF_MODULE_ID_QDF, QDF_TRACE_LEVEL_INFO,
		  "Computed ECDH shared secret (%zu bytes)",
		  *shared_secret_size);
	status = QDF_STATUS_SUCCESS;

out:
	if (req)
		kpp_request_free(req);
	if (sg_out)
		qdf_mem_free(sg_out);
	if (sg_in)
		qdf_mem_free(sg_in);
	if (output_buf) {
		qdf_mem_zero(output_buf, output_buf_size);
		qdf_mem_free(output_buf);
	}
	if (input_buf) {
		qdf_mem_zero(input_buf, peer_public_key_size);
		qdf_mem_free(input_buf);
	}
	if (tfm)
		crypto_free_kpp(tfm);

	return status;
}

qdf_export_symbol(qdf_crypto_ecdh_compute_shared_secret);
#endif /* WLAN_FEATURE_11BN_SMD */
