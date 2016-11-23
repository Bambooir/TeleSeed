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

#ifndef __TGL_CRYPTO_RSA_H__
#define __TGL_CRYPTO_RSA_H__

#include <stdio.h> /* FILE */

#include "bn.h"

typedef struct TGLC_rsa TGLC_rsa;

TGLC_rsa *TGLC_rsa_new (unsigned long e, int n_bytes, const unsigned char *n);

TGLC_bn *TGLC_rsa_n (TGLC_rsa *);
TGLC_bn *TGLC_rsa_e (TGLC_rsa *);

void TGLC_rsa_free (TGLC_rsa *);

TGLC_rsa *TGLC_pem_read_RSAPublicKey (FILE *fp);

#endif
