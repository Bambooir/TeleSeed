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

#include <gcrypt.h>

#include "meta.h"
#include "rsa_pem.h"
#include "../tools.h" /* talloc */

struct TGLC_rsa {
  TGLC_bn *n;
  TGLC_bn *e;
};

TGLC_rsa *TGLC_rsa_new (unsigned long e, int n_bytes, const unsigned char *n) {
  assert (n_bytes > 0 && n_bytes < 5000);
  TGLC_rsa *ret = talloc (sizeof (TGLC_rsa));
  ret->e = TGLC_bn_new ();
  TGLC_bn_set_word (ret->e, e);
  ret->n = TGLC_bn_bin2bn (n, n_bytes, NULL);
  assert (n_bytes == TGLC_bn_num_bytes (ret->n));
  return ret;
}

#define RSA_GETTER(M)                                                          \
  TGLC_bn *TGLC_rsa_ ## M (TGLC_rsa *key) {                                    \
    return key->M;                                                             \
  }                                                                            \

RSA_GETTER(n);
RSA_GETTER(e);

void TGLC_rsa_free (TGLC_rsa *key) {
  if (key->e) {
    TGLC_bn_free (key->e);
  }
  if (key->n) {
    TGLC_bn_free (key->n);
  }
  tfree (key, sizeof (TGLC_rsa));
}

TGLC_rsa *TGLC_pem_read_RSAPublicKey (FILE *fp) {
  /*
   * Reading PEM format involves ASN.1 and is hard. libgcrypt doesn't support it.
   * The dependency on oh-so-freaking-much code just to parse static data that
   * will never change is not justified. Let the caller figure out how to resolve
   * this (telegram-purple does so by using libpurple's built-in functions), and
   * ignore any PEM files.
   */
  (void) fp;
  return NULL;
}

#endif
