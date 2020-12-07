/*
 * cipher.c
 *
 * cipher meta-functions
 *
 * David A. McGrew
 * Cisco Systems, Inc.
 *
 */

/*
 *
 * Copyright (c) 2001-2017 Cisco Systems, Inc.
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

#include "cipher.h"
#include "crypto_types.h"
#include "crypto_err.h"                /* for srtp_debug */
#include "alloc.h"              /* for crypto_alloc(), crypto_free()  */

srtp_err_status_t srtp_cipher_type_alloc (const srtp_cipher_type_t *ct, srtp_cipher_t **c, int key_len, int tlen)
{
    if (!ct || !ct->alloc) {
	return (srtp_err_status_bad_param);
    }
    return ((ct)->alloc((c), (key_len), (tlen)));
}

srtp_err_status_t srtp_cipher_dealloc (srtp_cipher_t *c)
{
    if (!c || !c->type) {
	return (srtp_err_status_bad_param);
    }
    return (((c)->type)->dealloc(c));
}

srtp_err_status_t srtp_cipher_init (srtp_cipher_t *c, const uint8_t *key)
{
    if (!c || !c->type || !c->state) {
	return (srtp_err_status_bad_param);
    }
    return (((c)->type)->init(((c)->state), (key)));
}


srtp_err_status_t srtp_cipher_set_iv (srtp_cipher_t *c, uint8_t *iv, int direction)
{
    if (!c || !c->type || !c->state) {
	return (srtp_err_status_bad_param);
    }

    return (((c)->type)->set_iv(((c)->state), iv, direction)); 
}

srtp_err_status_t srtp_cipher_output (srtp_cipher_t *c, uint8_t *buffer, uint32_t *num_octets_to_output)
{

    /* zeroize the buffer */
    octet_string_set_to_zero(buffer, *num_octets_to_output);

    /* exor keystream into buffer */
    return (((c)->type)->encrypt(((c)->state), buffer, num_octets_to_output));
}

srtp_err_status_t srtp_cipher_encrypt (srtp_cipher_t *c, uint8_t *buffer, uint32_t *num_octets_to_output)
{
    if (!c || !c->type || !c->state) {
	return (srtp_err_status_bad_param);
    }

    return (((c)->type)->encrypt(((c)->state), buffer, num_octets_to_output));
}

srtp_err_status_t srtp_cipher_decrypt (srtp_cipher_t *c, uint8_t *buffer, uint32_t *num_octets_to_output)
{
    if (!c || !c->type || !c->state) {
	return (srtp_err_status_bad_param);
    }

    return (((c)->type)->decrypt(((c)->state), buffer, num_octets_to_output));
}

srtp_err_status_t srtp_cipher_get_tag (srtp_cipher_t *c, uint8_t *buffer, uint32_t *tag_len)
{
    if (!c || !c->type || !c->state) {
	return (srtp_err_status_bad_param);
    }
    if (!((c)->type)->get_tag) {
	return (srtp_err_status_no_such_op);
    }

    return (((c)->type)->get_tag(((c)->state), buffer, tag_len));
}

srtp_err_status_t srtp_cipher_set_aad (srtp_cipher_t *c, const uint8_t *aad, uint32_t aad_len)
{
    if (!c || !c->type || !c->state) {
	return (srtp_err_status_bad_param);
    }
    if (!((c)->type)->set_aad) {
	return (srtp_err_status_no_such_op);
    }

    return (((c)->type)->set_aad(((c)->state), aad, aad_len));
}

/* some bookkeeping functions */

int srtp_cipher_get_key_length (const srtp_cipher_t *c)
{
    return c->key_len;
}


/*
 * A trivial platform independent random source.  The random
 * data is used for some of the cipher self-tests.
 */
static srtp_err_status_t srtp_cipher_rand (void *dest, uint32_t len) 
{
#if defined(HAVE_RAND_S)
  uint8_t *dst = (uint8_t *)dest;
  while (len)
  {
    unsigned int val;
    errno_t err = rand_s(&val);

    if (err != 0)
      return srtp_err_status_fail;
  
    *dst++ = val & 0xff;
    len--;
  }
#else
  /* Generic C-library (rand()) version */
  /* This is a random source of last resort */
  uint8_t *dst = (uint8_t *)dest;
  while (len)
  {
	  int val = rand();
	  /* rand() returns 0-32767 (ugh) */
	  /* Is this a good enough way to get random bytes?
	     It is if it passes FIPS-140... */
	  *dst++ = val & 0xff;
	  len--;
  }
#endif
  return srtp_err_status_ok;
}
 
#define SELF_TEST_BUF_OCTETS 128
#define NUM_RAND_TESTS       128
#define MAX_KEY_LEN          64

/*
 * cipher_bits_per_second(c, l, t) computes (an estimate of) the
 * number of bits that a cipher implementation can encrypt in a second
 *
 * c is a cipher (which MUST be allocated and initialized already), l
 * is the length in octets of the test data to be encrypted, and t is
 * the number of trials
 *
 * if an error is encountered, the value 0 is returned
 */
uint64_t srtp_cipher_bits_per_second (srtp_cipher_t *c, int octets_in_buffer, int num_trials)
{
    int i;
    v128_t nonce;
    clock_t timer;
    unsigned char *enc_buf;
    unsigned int len = octets_in_buffer;

    enc_buf = (unsigned char*)srtp_crypto_alloc(octets_in_buffer);
    if (enc_buf == NULL) {
        return 0; /* indicate bad parameters by returning null */

    }
    /* time repeated trials */
    v128_set_to_zero(&nonce);
    timer = clock();
    for (i = 0; i < num_trials; i++, nonce.v32[3] = i) {
        if (srtp_cipher_set_iv(c, (uint8_t*)&nonce, srtp_direction_encrypt) != srtp_err_status_ok) {
            srtp_crypto_free(enc_buf);
            return 0;
        }
        if (srtp_cipher_encrypt(c, enc_buf, &len) != srtp_err_status_ok) {
            srtp_crypto_free(enc_buf);
            return 0;
        }
    }
    timer = clock() - timer;

    srtp_crypto_free(enc_buf);

    if (timer == 0) {
        /* Too fast! */
        return 0;
    }

    return (uint64_t)CLOCKS_PER_SEC * num_trials * 8 * octets_in_buffer / timer;
}
