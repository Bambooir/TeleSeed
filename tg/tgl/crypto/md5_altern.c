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

#include <gcrypt.h>

#include "md5.h"

void TGLC_md5 (const unsigned char *d, size_t n, unsigned char *md) {
  gcry_md_hash_buffer (GCRY_MD_MD5, md, d, n);
}

#endif
