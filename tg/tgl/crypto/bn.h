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

#ifndef __TGL_CRYPTO_BN_H__
#define __TGL_CRYPTO_BN_H__

typedef struct TGLC_bn_ctx TGLC_bn_ctx;
typedef struct TGLC_bn TGLC_bn;

TGLC_bn_ctx *TGLC_bn_ctx_new (void);
void TGLC_bn_ctx_free (TGLC_bn_ctx* ctx);

TGLC_bn *TGLC_bn_new (void);
void TGLC_bn_free (TGLC_bn *a);
void TGLC_bn_clear_free (TGLC_bn *a);
int TGLC_bn_cmp (const TGLC_bn *a, const TGLC_bn *b);
int TGLC_bn_is_prime (const TGLC_bn *a, int checks, void (*callback) (int, int, void *), TGLC_bn_ctx *ctx, void *cb_arg);
int TGLC_bn_bn2bin (const TGLC_bn *a, unsigned char *to);
TGLC_bn * TGLC_bn_bin2bn(const unsigned char *s, int len, TGLC_bn *ret);
int TGLC_bn_set_word (TGLC_bn *a, unsigned long w);
unsigned long TGLC_bn_get_word (const TGLC_bn *a);
int TGLC_bn_num_bits (const TGLC_bn *a);
void TGLC_bn_sub (TGLC_bn *r, const TGLC_bn *a, const TGLC_bn *b);
int TGLC_bn_div (TGLC_bn *dv, TGLC_bn *rem, const TGLC_bn *a, const TGLC_bn *d, TGLC_bn_ctx *ctx);
int TGLC_bn_mod_exp (TGLC_bn *r, const TGLC_bn *a, const TGLC_bn *p, const TGLC_bn *m, TGLC_bn_ctx *ctx);

#define TGLC_bn_num_bytes(a) ((TGLC_bn_num_bits(a)+7)/8)
#define TGLC_bn_mod(rem,m,d,ctx) TGLC_bn_div(NULL,(rem),(m),(d),(ctx))

#endif
