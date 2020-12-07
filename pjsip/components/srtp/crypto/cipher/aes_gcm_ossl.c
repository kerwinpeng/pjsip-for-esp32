/*
 * aes_gcm_ossl.c
 *
 * AES Galois Counter Mode
 *
 * John A. Foley
 * Cisco Systems, Inc.
 *
 */

/*
 *
 * Copyright (c) 2013-2017, Cisco Systems, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 *   Redistributions in binary form must reproduce the above
 *   copyright notice, this list of conditions and the following
 *   disclaimer in the documentation and/or other materials provided
 *   with the distribution.
 *
 *   Neither the name of the Cisco Systems, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifdef HAVE_CONFIG_H
    #include <srtp_config.h>
#endif

#include "aes_gcm_ossl.h"
#include "alloc.h"
#include "crypto_err.h"                /* for srtp_debug */
#include "crypto_types.h"

/*
 * The following are the global singleton instances for the
 * 128-bit and 256-bit GCM ciphers.
 */
extern const srtp_cipher_type_t srtp_aes_gcm_128_openssl;
extern const srtp_cipher_type_t srtp_aes_gcm_256_openssl;

/*
 * For now we only support 8 and 16 octet tags.  The spec allows for
 * optional 12 byte tag, which may be supported in the future.
 */
#define GCM_AUTH_TAG_LEN    16
#define GCM_AUTH_TAG_LEN_8  8


/*
 * This function allocates a new instance of this crypto engine.
 * The key_len parameter should be one of 28 or 44 for
 * AES-128-GCM or AES-256-GCM respectively.  Note that the
 * key length includes the 14 byte salt value that is used when
 * initializing the KDF.
 */
static srtp_err_status_t srtp_aes_gcm_openssl_alloc (srtp_cipher_t **c, int key_len, int tlen)
{
    srtp_aes_gcm_ctx_t *gcm;
    srtp_debug_module_t srtp_mod_aes_gcm = {
        0,               /* debugging is off by default */
        "aes gcm"        /* printable module name       */
    };

    debug_print(srtp_mod_aes_gcm, "allocating cipher with key length %d", key_len);
    debug_print(srtp_mod_aes_gcm, "allocating cipher with tag length %d", tlen);

    /*
     * Verify the key_len is valid for one of: AES-128/256
     */
    if (key_len != SRTP_AES_GCM_128_KEY_LEN_WSALT &&
        key_len != SRTP_AES_GCM_256_KEY_LEN_WSALT) {
        return (srtp_err_status_bad_param);
    }

    if (tlen != GCM_AUTH_TAG_LEN &&
        tlen != GCM_AUTH_TAG_LEN_8) {
        return (srtp_err_status_bad_param);
    }

    /* allocate memory a cipher of type aes_gcm */
    *c = (srtp_cipher_t *)srtp_crypto_alloc(sizeof(srtp_cipher_t));
    if (*c == NULL) {
        return (srtp_err_status_alloc_fail);
    }
    memset(*c, 0x0, sizeof(srtp_cipher_t));

    gcm = (srtp_aes_gcm_ctx_t *)srtp_crypto_alloc(sizeof(srtp_aes_gcm_ctx_t));
    if (gcm == NULL) {
	srtp_crypto_free(*c);	
	*c = NULL;
        return (srtp_err_status_alloc_fail);
    }
    memset(gcm, 0x0, sizeof(srtp_aes_gcm_ctx_t));

    /* set pointers */
    (*c)->state = gcm;

    /* setup cipher attributes */
    switch (key_len) {
    case SRTP_AES_GCM_128_KEY_LEN_WSALT:
        (*c)->type = &srtp_aes_gcm_128_openssl;
        (*c)->algorithm = SRTP_AES_GCM_128;
        gcm->key_size = SRTP_AES_128_KEY_LEN;
        gcm->tag_len = tlen;
        break;
    case SRTP_AES_GCM_256_KEY_LEN_WSALT:
        (*c)->type = &srtp_aes_gcm_256_openssl;
        (*c)->algorithm = SRTP_AES_GCM_256;
        gcm->key_size = SRTP_AES_256_KEY_LEN;
        gcm->tag_len = tlen;
        break;
    }

    /* set key size        */
    (*c)->key_len = key_len;

    return (srtp_err_status_ok);
}


/*
 * This function deallocates a GCM session
 */
static srtp_err_status_t srtp_aes_gcm_openssl_dealloc (srtp_cipher_t *c)
{
    srtp_aes_gcm_ctx_t *ctx;

    ctx = (srtp_aes_gcm_ctx_t*)c->state;
    if (ctx) {

        /* zeroize the key material */
        octet_string_set_to_zero(ctx, sizeof(srtp_aes_gcm_ctx_t));
        srtp_crypto_free(ctx);
    }

    /* free memory */
    srtp_crypto_free(c);

    return (srtp_err_status_ok);
}

/*
 * aes_gcm_openssl_context_init(...) initializes the aes_gcm_context
 * using the value in key[].
 *
 * the key is the secret key
 */
static srtp_err_status_t srtp_aes_gcm_openssl_context_init (void* cv, const uint8_t *key)
{
    srtp_aes_gcm_ctx_t *c = (srtp_aes_gcm_ctx_t *)cv;
    srtp_debug_module_t srtp_mod_aes_gcm = {
        0,               /* debugging is off by default */
        "aes gcm"        /* printable module name       */
    };

    c->dir = srtp_direction_any;

    debug_print(srtp_mod_aes_gcm, "key:  %s", srtp_octet_string_hex_string(key, c->key_size));


    return (srtp_err_status_ok);
}


/*
 * aes_gcm_openssl_set_iv(c, iv) sets the counter value to the exor of iv with
 * the offset
 */
static srtp_err_status_t srtp_aes_gcm_openssl_set_iv (void *cv, uint8_t *iv, srtp_cipher_direction_t direction)
{
    srtp_aes_gcm_ctx_t *c = (srtp_aes_gcm_ctx_t *)cv;
    srtp_debug_module_t srtp_mod_aes_gcm = {
        0,               /* debugging is off by default */
        "aes gcm"        /* printable module name       */
    };

    if (direction != srtp_direction_encrypt && direction != srtp_direction_decrypt) {
        return (srtp_err_status_bad_param);
    }
    c->dir = direction;

    debug_print(srtp_mod_aes_gcm, "setting iv: %s", v128_hex_string((v128_t*)iv));

    return (srtp_err_status_ok);
}

/*
 * This function processes the AAD
 *
 * Parameters:
 *	c	Crypto context
 *	aad	Additional data to process for AEAD cipher suites
 *	aad_len	length of aad buffer
 */
static srtp_err_status_t srtp_aes_gcm_openssl_set_aad (void *cv, const uint8_t *aad, uint32_t aad_len)
{
    srtp_aes_gcm_ctx_t *c = (srtp_aes_gcm_ctx_t *)cv;
    int rv;

    /*
     * Set dummy tag, OpenSSL requires the Tag to be set before
     * processing AAD
     */

    /*
     * OpenSSL never write to address pointed by the last parameter of
     * EVP_CIPHER_CTX_ctrl while EVP_CTRL_GCM_SET_TAG (in reality, 
     * OpenSSL copy its content to the context), so we can make 
     * aad read-only in this function and all its wrappers.
     */
    unsigned char dummy_tag[GCM_AUTH_TAG_LEN];
    memset(dummy_tag, 0x0, GCM_AUTH_TAG_LEN);

    return (srtp_err_status_ok);
}

/*
 * This function encrypts a buffer using AES GCM mode
 *
 * Parameters:
 *	c	Crypto context
 *	buf	data to encrypt
 *	enc_len	length of encrypt buffer
 */
static srtp_err_status_t srtp_aes_gcm_openssl_encrypt (void *cv, unsigned char *buf, unsigned int *enc_len)
{
    srtp_aes_gcm_ctx_t *c = (srtp_aes_gcm_ctx_t *)cv;
    if (c->dir != srtp_direction_encrypt && c->dir != srtp_direction_decrypt) {
        return (srtp_err_status_bad_param);
    }

    return (srtp_err_status_ok);
}

/*
 * This function calculates and returns the GCM tag for a given context.
 * This should be called after encrypting the data.  The *len value
 * is increased by the tag size.  The caller must ensure that *buf has
 * enough room to accept the appended tag.
 *
 * Parameters:
 *	c	Crypto context
 *	buf	data to encrypt
 *	len	length of encrypt buffer
 */
static srtp_err_status_t srtp_aes_gcm_openssl_get_tag (void *cv, uint8_t *buf, uint32_t *len)
{
    srtp_aes_gcm_ctx_t *c = (srtp_aes_gcm_ctx_t *)cv;

    /*
     * Increase encryption length by desired tag size
     */
    *len = c->tag_len;

    return (srtp_err_status_ok);
}


/*
 * This function decrypts a buffer using AES GCM mode
 *
 * Parameters:
 *	c	Crypto context
 *	buf	data to encrypt
 *	enc_len	length of encrypt buffer
 */
static srtp_err_status_t srtp_aes_gcm_openssl_decrypt (void *cv, unsigned char *buf, unsigned int *enc_len)
{
    srtp_aes_gcm_ctx_t *c = (srtp_aes_gcm_ctx_t *)cv;
    if (c->dir != srtp_direction_encrypt && c->dir != srtp_direction_decrypt) {
        return (srtp_err_status_bad_param);
    }

    /*
     * Reduce the buffer size by the tag length since the tag
     * is not part of the original payload
     */
    *enc_len -= c->tag_len;

    return (srtp_err_status_ok);
}

/*
 * This is the vector function table for this crypto engine.
 */
const srtp_cipher_type_t srtp_aes_gcm_128_openssl = {
    srtp_aes_gcm_openssl_alloc,
    srtp_aes_gcm_openssl_dealloc,
    srtp_aes_gcm_openssl_context_init,
    srtp_aes_gcm_openssl_set_aad,
    srtp_aes_gcm_openssl_encrypt,
    srtp_aes_gcm_openssl_decrypt,
    srtp_aes_gcm_openssl_set_iv,
    srtp_aes_gcm_openssl_get_tag,
    SRTP_AES_GCM_128
};

/*
 * This is the vector function table for this crypto engine.
 */
const srtp_cipher_type_t srtp_aes_gcm_256_openssl = {
    srtp_aes_gcm_openssl_alloc,
    srtp_aes_gcm_openssl_dealloc,
    srtp_aes_gcm_openssl_context_init,
    srtp_aes_gcm_openssl_set_aad,
    srtp_aes_gcm_openssl_encrypt,
    srtp_aes_gcm_openssl_decrypt,
    srtp_aes_gcm_openssl_set_iv,
    srtp_aes_gcm_openssl_get_tag,
    SRTP_AES_GCM_256
};

