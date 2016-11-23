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

#include "rand.h"

void TGLC_rand_add (const void *buf, int num, double entropy) {
  (void) entropy;
  // TODO: Translate half-broken "entropy" into gcry's "quality".
  gcry_random_add_bytes (buf, num, 50);
}

int TGLC_rand_bytes (unsigned char *buf, int num) {
  gcry_randomize (buf, num, GCRY_STRONG_RANDOM);
  return 1; // Don't ask why.
}

int TGLC_rand_pseudo_bytes (unsigned char *buf, int num) {
  gcry_create_nonce (buf, num);
  return 0; // Don't ask why.
}

#endif
