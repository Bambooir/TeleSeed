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

/* Fail-fast when something becomes deprecated. */
#define GCRYPT_NO_DEPRECATED

#include <assert.h>
#include <gcrypt.h>

#include "bn.h"
#include "meta.h"

// There is no "_ctx" equivalent in gcrypt.
TGLC_WRAPPER_ASSOC(bn_ctx,int)
TGLC_WRAPPER_ASSOC(bn,struct gcry_mpi)

TGLC_bn_ctx *TGLC_bn_ctx_new (void) {
  /* Must not be the null pointer, but must never be dereferenced. Assume that "1" is an invalid address. */
  return ((void *)1);
}

void TGLC_bn_ctx_free (TGLC_bn_ctx* ctx) {
  (void) ctx;
}

TGLC_bn *TGLC_bn_new (void) {
  // TODO: Determine how many bits should be pre-allocated.
  TGLC_bn *ret = wrap_bn (gcry_mpi_new (2048));
  assert (ret);
  return ret;
}

void TGLC_bn_free (TGLC_bn *a) {
  gcry_mpi_release (unwrap_bn (a));
}

void TGLC_bn_clear_free (TGLC_bn *a) {
  /* This only protects against accidental use-after-free. I don't see the point of clearing, since we're not operating in protected memory AND the most crucial fields are never cleared anyway. */
  gcry_mpi_set_ui (unwrap_bn (a), 0);
  gcry_mpi_release (unwrap_bn (a));
}

int TGLC_bn_cmp (const TGLC_bn *a, const TGLC_bn *b) {
  return gcry_mpi_cmp (unwrap_bn (a), unwrap_bn (b));
}

int TGLC_bn_is_prime (const TGLC_bn *a, int checks, void (*callback) (int, int, void *), TGLC_bn_ctx *ctx, void *cb_arg) {
  assert (0 == checks);
  assert (NULL == callback);
  (void) ctx;
  assert (NULL == cb_arg);
  /* Second argument is ignored.
   * No really, here's what libgcrypt's source does:
   * (void)flags; */
  gcry_error_t err = gcry_prime_check (unwrap_bn (a), 0);
  /* This is nasty. In essence, gcry returns a bool whether it is NOT a prime. */
  return !err;
}

int TGLC_bn_bn2bin (const TGLC_bn *a, unsigned char *to) {
  const unsigned long num_bytes = TGLC_bn_num_bytes (a);
  gcry_error_t gcry_error = gcry_mpi_print (GCRYMPI_FMT_USG, to, num_bytes, NULL, unwrap_bn (a));
  assert (!gcry_error);
  return num_bytes;
}

TGLC_bn *TGLC_bn_bin2bn(const unsigned char *s, int len, TGLC_bn *ret) {
  gcry_mpi_t ret_ptr = NULL;
  gcry_error_t gcry_error = gcry_mpi_scan (&ret_ptr, GCRYMPI_FMT_USG, s, len, NULL);
  assert (!gcry_error);
  assert (ret_ptr);

  if (!ret) {
    return wrap_bn (ret_ptr);
  }

  gcry_mpi_snatch (unwrap_bn (ret), ret_ptr);
  return ret;
}

int TGLC_bn_set_word (TGLC_bn *a, unsigned long w) {
  assert (a);
  gcry_mpi_set_ui (unwrap_bn (a), w);
  return 1;
}

unsigned long TGLC_bn_get_word (const TGLC_bn *a) {
  const unsigned long num_bytes = TGLC_bn_num_bytes (a);
  assert (sizeof (unsigned long) >= num_bytes);
  unsigned char tmp[sizeof (unsigned long)];
  memset (tmp, 0, sizeof (unsigned long));
  TGLC_bn_bn2bin (a, tmp + sizeof (unsigned long) - num_bytes);

  /* Inefficient, but runs a total of three times per connection. */
  /* TODO: Optimize? */
  unsigned long ret = 0;
  unsigned int i;
  for (i = 0; i < sizeof (unsigned long); ++i) {
      ret <<= 8;
      ret |= tmp[i];
  }
  return ret;
}

int TGLC_bn_num_bits (const TGLC_bn *a) {
  return gcry_mpi_get_nbits (unwrap_bn (a));
}

void TGLC_bn_sub (TGLC_bn *r, const TGLC_bn *a, const TGLC_bn *b) {
  gcry_mpi_sub (unwrap_bn (r), unwrap_bn (a), unwrap_bn (b));
}

int TGLC_bn_div (TGLC_bn *dv, TGLC_bn *rem, const TGLC_bn *a, const TGLC_bn *d, TGLC_bn_ctx *ctx) {
  (void) ctx;
  gcry_mpi_div (unwrap_bn (dv), unwrap_bn (rem), unwrap_bn (a), unwrap_bn (d), 0);
  return 1;
}

int TGLC_bn_mod_exp (TGLC_bn *r, const TGLC_bn *a, const TGLC_bn *p, const TGLC_bn *m, TGLC_bn_ctx *ctx) {
  (void) ctx;
  gcry_mpi_powm (unwrap_bn (r), unwrap_bn (a), unwrap_bn (p), unwrap_bn (m));
  return 1;
}

#endif
