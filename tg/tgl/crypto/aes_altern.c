/* 
    This file is part of tgl-library

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

    Copyright Ben Wiederhake 2015
*/

#include "../config.h"

#ifdef TGL_AVOID_OPENSSL

/* Marginally speed up compilation */
#define GCRYPT_NO_MPI_MACROS
/* Fail-fast when something becomes deprecated. */
#define GCRYPT_NO_DEPRECATED

#include <assert.h>
#include <gcrypt.h>

#include "aes.h"
#include "meta.h"
#include "rand.h"

#define AES_BLOCK_BITS 128
#define AES_BLOCK_BYTES (AES_BLOCK_BITS/8)
#define AES_KEY_BITS 256
#define AES_KEY_BYTES (AES_KEY_BITS/8)

typedef char check_struct_sizes[(sizeof (TGLC_aes_key) == AES_KEY_BYTES) - 1];

void TGLC_aes_set_encrypt_key (const unsigned char *userKey, const int bits, TGLC_aes_key *key) {
  assert (bits == AES_KEY_BITS);
  memcpy (key->_dummy, userKey, AES_KEY_BYTES);
}

void TGLC_aes_set_decrypt_key (const unsigned char *userKey, const int bits, TGLC_aes_key *key) {
  TGLC_aes_set_encrypt_key (userKey, bits, key);
}

// TODO: Try to use gcrypt's internal buf_xor?
static void do_xor_block (const unsigned char *in, const unsigned char *with, unsigned char *out) {
  /* TODO: Referencing them as "size_t" (or whatever biggest numerical type available) is probably faster, but also more error-prone. */
  int i;
  for (i = 0; i < 16; ++i) {
    *out++ = *in++ ^ *with++;
  }
}

static gcry_error_t do_ige_encrypt (const unsigned char *in, unsigned char *out,
    unsigned long n_blocks, gcry_cipher_hd_t cipher, unsigned char *ivec) {
  /* The docs say, at the end of section 2:
   * "OpenSSL uses the convention that the first block of the IV is x_0
   * and the second block is y_0."
   * Well, no. This is a subtle error: FIRST comes the previous ENcrypted block,
   * THEN the DEcrypted block.
   * Also, keep a copy of the old cleartext, in case in == out. */
  unsigned char buf[2 * 16];
  unsigned char *prev_x = buf;
  unsigned char *cur_x = buf + 16;
  memcpy (prev_x, ivec + 16, 16);
  const unsigned char *prev_y = ivec;

  /* gcrypt doesn't allow overlapping buffers. Kudos to "reubensammut" for
   * noticing this. Note that this also works if in == out. */
  unsigned char tmp[16];
  unsigned long i;
  for (i = 0; i < n_blocks; ++i) {
    memcpy (cur_x, in, 16);
    /* Might overwrite 'in'. */
    do_xor_block (in, prev_y, out);
    gcry_error_t gcry_error = gcry_cipher_encrypt (cipher, tmp, 16, out, 16);
    if (gcry_error) {
      return gcry_error;
    }
    do_xor_block (tmp, prev_x, out);
    prev_y = out; // encrypted is in 'out'
    in += 16;
    out += 16;
    // swap (tmp_x, cur_x);
    unsigned char *tmp_x = cur_x;
    cur_x = prev_x;
    prev_x = tmp_x;
  }
  if (n_blocks > 0) {
    /* OpenSSL updates the IV, so we do that, too.
     * One could avoid memcpy here, as it's only 16 bytes. */
    memcpy (ivec + 16, prev_x, 16);
    memcpy (ivec, prev_y, 16);
  }
  return 0;
}

static gcry_error_t do_ige_decrypt (const unsigned char *in, unsigned char *out,
    unsigned long n_blocks, gcry_cipher_hd_t cipher, unsigned char *ivec) {
  /* Also, keep a copy of the old ciphertext, in case in == out. */
  unsigned char buf[2 * 16];
  unsigned char *prev_y = buf;
  unsigned char *cur_y = buf + 16;
  memcpy (prev_y, ivec, 16);
  const unsigned char *prev_x = ivec + 16;

  /* gcrypt doesn't allow overlapping buffers. Kudos to "reubensammut" for
   * noticing this. Note that this also works if in == out. */
  unsigned char tmp[16];
  unsigned long i;
  for (i = 0; i < n_blocks; ++i) {
    memcpy (cur_y, in, 16);
    /* Might overwrite 'in'. */
    do_xor_block (in, prev_x, out);
    gcry_error_t gcry_error = gcry_cipher_decrypt (cipher, tmp, 16, out, 16);
    if (gcry_error) {
      return gcry_error;
    }
    do_xor_block (tmp, prev_y, out);
    prev_x = out; // decrypted is in 'out'
    in += 16;
    out += 16;
    // swap (tmp_y, cur_y);
    unsigned char *tmp_y = cur_y;
    cur_y = prev_y;
    prev_y = tmp_y;
  }
  /* Do not change ivec */
  return 0;
}

/* Needs to be given an IV of length 2*AES_BLOCK_BYTES. */
void TGLC_aes_ige_encrypt (const unsigned char *in, unsigned char *out, size_t length, const TGLC_aes_key *key, unsigned char *ivec, const int enc) {
  assert (!(length % AES_BLOCK_BYTES));

  /* Set it up. */
  gcry_cipher_hd_t cipher;
  gcry_error_t gcry_error = gcry_cipher_open (&cipher, GCRY_CIPHER_AES256, GCRY_CIPHER_MODE_ECB, 0);
  assert (!gcry_error);
  gcry_cipher_setkey (cipher, key->_dummy, AES_KEY_BYTES);

  if (enc) {
    gcry_error = do_ige_encrypt(in, out, length / AES_BLOCK_BYTES, cipher, ivec);
  } else {
    gcry_error = do_ige_decrypt(in, out, length / AES_BLOCK_BYTES, cipher, ivec);
  }
  assert (!gcry_error);

  gcry_cipher_close(cipher);
}

#endif
