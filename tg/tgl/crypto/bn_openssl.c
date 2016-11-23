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

#ifndef TGL_AVOID_OPENSSL

#include <assert.h>

#include <openssl/bn.h>

#include "bn.h"
#include "meta.h"

TGLC_WRAPPER_ASSOC(bn_ctx,BN_CTX)
TGLC_WRAPPER_ASSOC(bn,BIGNUM)

TGLC_bn_ctx *TGLC_bn_ctx_new (void) {
  return wrap_bn_ctx (BN_CTX_new ());
}

void TGLC_bn_ctx_free (TGLC_bn_ctx* ctx) {
  BN_CTX_free (unwrap_bn_ctx (ctx));
}

TGLC_bn *TGLC_bn_new (void) {
  return wrap_bn (BN_new ());
}

void TGLC_bn_free (TGLC_bn *a) {
  BN_free (unwrap_bn (a));
}

void TGLC_bn_clear_free (TGLC_bn *a) {
  BN_clear_free (unwrap_bn (a));
}

int TGLC_bn_cmp (const TGLC_bn *a, const TGLC_bn *b) {
  return BN_cmp (unwrap_bn (a), unwrap_bn (b));
}

int TGLC_bn_is_prime (const TGLC_bn *a, int checks, void (*callback) (int, int, void *), TGLC_bn_ctx *ctx, void *cb_arg) {
  return BN_is_prime (unwrap_bn (a), checks, callback, unwrap_bn_ctx (ctx), cb_arg);
}

int TGLC_bn_bn2bin (const TGLC_bn *a, unsigned char *to) {
  return BN_bn2bin (unwrap_bn (a), to);
}

TGLC_bn * TGLC_bn_bin2bn(const unsigned char *s, int len, TGLC_bn *ret) {
  return wrap_bn (BN_bin2bn (s, len, unwrap_bn (ret)));
}

int TGLC_bn_set_word (TGLC_bn *a, unsigned long w) {
  return BN_set_word (unwrap_bn (a), w);
}

unsigned long TGLC_bn_get_word (const TGLC_bn *a) {
  return BN_get_word (unwrap_bn (a));
}

int TGLC_bn_num_bits (const TGLC_bn *a) {
  return BN_num_bits (unwrap_bn (a));
}

void TGLC_bn_sub (TGLC_bn *r, const TGLC_bn *a, const TGLC_bn *b) {
  int res = BN_sub (unwrap_bn (r), unwrap_bn (a), unwrap_bn (b));
  assert (res);
}

int TGLC_bn_div (TGLC_bn *dv, TGLC_bn *rem, const TGLC_bn *a, const TGLC_bn *d, TGLC_bn_ctx *ctx) {
  return BN_div (unwrap_bn (dv), unwrap_bn (rem), unwrap_bn (a), unwrap_bn (d), unwrap_bn_ctx (ctx));
}

int TGLC_bn_mod_exp (TGLC_bn *r, const TGLC_bn *a, const TGLC_bn *p, const TGLC_bn *m, TGLC_bn_ctx *ctx) {
  return BN_mod_exp (unwrap_bn (r), unwrap_bn (a), unwrap_bn (p), unwrap_bn (m), unwrap_bn_ctx (ctx));
}

#endif
