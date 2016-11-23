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

#include <openssl/aes.h>

#include "aes.h"
#include "meta.h"

typedef char check_struct_sizes[(sizeof (AES_KEY) == sizeof (TGLC_aes_key)) - 1];

TGLC_WRAPPER_ASSOC(aes_key,AES_KEY)

void TGLC_aes_set_encrypt_key (const unsigned char *userKey, const int bits, TGLC_aes_key *key) {
  int success = AES_set_encrypt_key(userKey, bits, unwrap_aes_key (key));
  assert (0 == success);
}

void TGLC_aes_set_decrypt_key (const unsigned char *userKey, const int bits, TGLC_aes_key *key) {
  int success = AES_set_decrypt_key(userKey, bits, unwrap_aes_key (key));
  assert (0 == success);
}

void TGLC_aes_ige_encrypt (const unsigned char *in, unsigned char *out, size_t length, const TGLC_aes_key *key, unsigned char *ivec, const int enc) {
  AES_ige_encrypt (in, out, length, unwrap_aes_key (key), ivec, enc);
}

#endif
