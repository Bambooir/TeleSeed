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

//#include <stddef.h> /* NULL */

#include <openssl/rsa.h>
#include <openssl/pem.h>

#include "bn.h"
#include "meta.h"
#include "rsa_pem.h"

TGLC_WRAPPER_ASSOC(rsa,RSA)

// TODO: Refactor crucial struct-identity into its own header.
TGLC_WRAPPER_ASSOC(bn,BIGNUM)

TGLC_rsa *TGLC_rsa_new (unsigned long e, int n_bytes, const unsigned char *n) {
  RSA *ret = RSA_new ();
  ret->e = unwrap_bn (TGLC_bn_new ());
  TGLC_bn_set_word (wrap_bn (ret->e), e);
  ret->n = unwrap_bn (TGLC_bn_bin2bn (n, n_bytes, NULL));
  return wrap_rsa (ret);
}

#define RSA_GETTER(M)                                                          \
  TGLC_bn *TGLC_rsa_ ## M (TGLC_rsa *key) {                                    \
    return wrap_bn (unwrap_rsa (key)->M);                                      \
  }                                                                            \

RSA_GETTER(n);
RSA_GETTER(e);

void TGLC_rsa_free (TGLC_rsa *p) {
  RSA_free (unwrap_rsa (p));
}

TGLC_rsa *TGLC_pem_read_RSAPublicKey (FILE *fp) {
  return wrap_rsa (PEM_read_RSAPublicKey (fp, NULL, NULL, NULL));
}

#endif
