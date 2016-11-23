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

#ifndef CRYPTO_META_H_
#define CRYPTO_META_H_

#include <assert.h>

/* All this wrapping/unwrapping solves a fundamental problem:
 * - The libtgl implementation wants access to the TGLC_* types.
 * - The libtgl implementation should not see any other definitions from any
 *   external crypto library (to make sure no symbol slips "through" during the
 *   OpenSSL-to-gcrypt transition).
 * - Most TGLC_* types (3 out of 4 types) are exclusively used as pointers
 *   throughout the libtgl source, in a way to allow for incomplete types.
 * - The CORE type (e.g. BIGNUM from <openssl/bn.h>) may be incomplete,
 *   even for tglc.
 * This means the standard approaches don't work:
 * - TGLC_NAME can't be a typedef to CORE, since CORE shouldn't even be visible
 *   from libtgl.
 * - TGLC_NAME can't be a pointer to CORE, same reason. Also, it would require
 *   a significant amount of rewriting (error-prone work).
 * - TGLC_NAME can't be a void pointer. Retain type checking!
 * - So TGLC_NAME *must* be an incomplete custom struct.
 * However, this ensues the following ugliness.
 *
 * The standard doesn't explicitly allow it, but there's a pretty good argument
 * that casting ptr-to-some-struct to ptr-to-other-struct is *probably* okay for
 * most compilers: https://stackoverflow.com/a/8702750/3070326
 */
#define TGLC_WRAPPER_ASSOC(NAME,CORE)                                          \
  static TGLC_ ## NAME *wrap_ ## NAME (const CORE *p)                          \
                              __attribute__ ((unused));                        \
  static CORE *unwrap_ ## NAME (const TGLC_ ## NAME *p)                        \
                              __attribute__ ((unused));                        \
  static CORE *unwrap_ ## NAME (const TGLC_ ## NAME *p) {                      \
    return (CORE *)p;                                                          \
  }                                                                            \
  static TGLC_ ## NAME *wrap_ ## NAME (const CORE *p) {                        \
    return (TGLC_ ## NAME *)p;                                                 \
  }

#endif /* CRYPTO_META_H_ */
