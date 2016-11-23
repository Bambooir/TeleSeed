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

    Copyright Nikolay Durov, Andrey Lopatin 2012-2013
              Vitaly Valtman 2013-2015
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define	_FILE_OFFSET_BITS	64

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#ifdef WIN32
#include <winsock2.h>
#else
#include <netdb.h>
#endif

#include "crypto/aes.h"
#include "crypto/rand.h"
#include "crypto/sha.h"

#include "mtproto-common.h"
#include "tools.h"

#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#endif

#include <errno.h>

#ifndef O_BINARY
#define O_BINARY 0
#endif

static int __packet_buffer[PACKET_BUFFER_SIZE + 16];
int *tgl_packet_ptr;
int *tgl_packet_buffer = __packet_buffer + 16;

static long long rsa_encrypted_chunks, rsa_decrypted_chunks;

//int verbosity;

#ifndef WIN32
static int get_random_bytes (struct tgl_state *TLS, unsigned char *buf, int n) {
  int r = 0, h = open ("/dev/random", O_RDONLY | O_NONBLOCK);
  if (h >= 0) {
    r = read (h, buf, n);
    if (r > 0) {
      vlogprintf (E_DEBUG, "added %d bytes of real entropy to secure random numbers seed\n", r);
    } else {
      r = 0;
    }
    close (h);
  }

  if (r < n) {
    h = open ("/dev/urandom", O_RDONLY);
    if (h < 0) {
      return r;
    }
    int s = read (h, buf + r, n - r);
    close (h);
    if (s > 0) {
      r += s;
    }
  }

  if (r >= (int) sizeof (long)) {
    *(long *)buf ^= lrand48 ();
    srand48 (*(long *)buf);
  }

  return r;
}
#else
static HCRYPTPROV hCryptoServiceProvider = 0;
static int get_random_bytes (struct tgl_state *TLS, unsigned char *buf, int n) {
	if (hCryptoServiceProvider == 0) {
		/* Crypto init */
		CryptAcquireContextA(&hCryptoServiceProvider, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
	}
	
	if (!CryptGenRandom(hCryptoServiceProvider, n, buf)) {
		return -1;
	}
    
	
	return n;
}
#endif


/* RDTSC */
#if defined(__i386__)
#define HAVE_RDTSC
static __inline__ unsigned long long rdtsc (void) {
  unsigned long long int x;
  __asm__ volatile ("rdtsc" : "=A" (x));
  return x;
}
#elif defined(__x86_64__)
#define HAVE_RDTSC
static __inline__ unsigned long long rdtsc (void) {
  unsigned hi, lo;
  __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
  return ((unsigned long long) lo) | (((unsigned long long) hi) << 32);
}
#endif

void tgl_prng_seed (struct tgl_state *TLS, const char *password_filename, int password_length) {
  struct timespec T;
  tgl_my_clock_gettime (CLOCK_REALTIME, &T);
  TGLC_rand_add (&T, sizeof (T), 4.0);
#ifdef HAVE_RDTSC
  unsigned long long r = rdtsc ();
  TGLC_rand_add (&r, 8, 4.0);
#endif
  unsigned short p = getpid ();
  TGLC_rand_add (&p, sizeof (p), 0.0);
#ifndef WIN32
  p = getppid ();
  TGLC_rand_add (&p, sizeof (p), 0.0);
#endif
  unsigned char rb[32];
  int s = get_random_bytes (TLS, rb, 32);
  if (s > 0) {
    TGLC_rand_add (rb, s, s);
  }
  memset (rb, 0, sizeof (rb));
  if (password_filename && password_length > 0) {
    int fd = open (password_filename, O_RDONLY | O_BINARY);
    if (fd < 0) {
      vlogprintf (E_WARNING, "Warning: fail to open password file - \"%s\", %s.\n", password_filename, strerror(errno));
    } else {
      unsigned char *a = talloc0 (password_length);
      int l = read (fd, a, password_length);
      if (l < 0) {
        vlogprintf (E_WARNING, "Warning: fail to read password file - \"%s\", %s.\n", password_filename, strerror(errno));
      } else {
        vlogprintf (E_DEBUG, "read %d bytes from password file.\n", l);
        TGLC_rand_add (a, l, l);
      }
      close (fd);
      tfree_secure (a, password_length);
    }
  }
  TLS->TGLC_bn_ctx = TGLC_bn_ctx_new ();
  ensure_ptr (TLS->TGLC_bn_ctx);
}

int tgl_serialize_bignum (TGLC_bn *b, char *buffer, int maxlen) {
  int itslen = TGLC_bn_num_bytes (b);
  int reqlen;
  if (itslen < 254) {
    reqlen = itslen + 1;
  } else {
    reqlen = itslen + 4;
  }
  int newlen = (reqlen + 3) & -4;
  int pad = newlen - reqlen;
  reqlen = newlen;
  if (reqlen > maxlen) {
    return -reqlen;
  }
  if (itslen < 254) {
    *buffer++ = itslen;
  } else {
    *(int *)buffer = (itslen << 8) + 0xfe;
    buffer += 4;
  }
  int l = TGLC_bn_bn2bin (b, (unsigned char *)buffer);
  assert (l == itslen);
  buffer += l;
  while (pad --> 0) {
    *buffer++ = 0;
  }
  return reqlen;
}


long long tgl_do_compute_rsa_key_fingerprint (TGLC_rsa *key) {
  static char tempbuff[4096];
  static unsigned char sha[20]; 
  assert (TGLC_rsa_n (key) && TGLC_rsa_e (key));
  int l1 = tgl_serialize_bignum (TGLC_rsa_n (key), tempbuff, 4096);
  assert (l1 > 0);
  int l2 = tgl_serialize_bignum (TGLC_rsa_e (key), tempbuff + l1, 4096 - l1);
  assert (l2 > 0 && l1 + l2 <= 4096);
  TGLC_sha1 ((unsigned char *)tempbuff, l1 + l2, sha);
  return *(long long *)(sha + 12);
}

void tgl_out_cstring (const char *str, long len) {
  assert (len >= 0 && len < (1 << 24));
  assert ((char *) packet_ptr + len + 8 < (char *) (packet_buffer + PACKET_BUFFER_SIZE));
  char *dest = (char *) packet_ptr;
  if (len < 254) {
    *dest++ = len;
  } else {
    *packet_ptr = (len << 8) + 0xfe;
    dest += 4;
  }
  memcpy (dest, str, len);
  dest += len;
  while ((long) dest & 3) {
    *dest++ = 0;
  }
  packet_ptr = (int *) dest;
}

void tgl_out_cstring_careful (const char *str, long len) {
  assert (len >= 0 && len < (1 << 24));
  assert ((char *) packet_ptr + len + 8 < (char *) (packet_buffer + PACKET_BUFFER_SIZE));
  char *dest = (char *) packet_ptr;
  if (len < 254) {
    dest++;
    if (dest != str) {
      memmove (dest, str, len);
    }
    dest[-1] = len;
  } else {
    dest += 4;
    if (dest != str) {
      memmove (dest, str, len);
    }
    *packet_ptr = (len << 8) + 0xfe;
  }
  dest += len;
  while ((long) dest & 3) {
    *dest++ = 0;
  }
  packet_ptr = (int *) dest;
}


void tgl_out_data (const void *data, long len) {
  assert (len >= 0 && len < (1 << 24) && !(len & 3));
  assert ((char *) packet_ptr + len + 8 < (char *) (packet_buffer + PACKET_BUFFER_SIZE));
  memcpy (packet_ptr, data, len);
  packet_ptr += len >> 2;
}

int *tgl_in_ptr, *tgl_in_end;

int tgl_fetch_bignum (TGLC_bn *x) {
  int l = prefetch_strlen ();
  if (l < 0) {
    return l;
  }
  char *str = fetch_str (l);
  assert (TGLC_bn_bin2bn ((unsigned char *) str, l, x) == x);
  return l;
}

int tgl_pad_rsa_encrypt (struct tgl_state *TLS, char *from, int from_len, char *to, int size, TGLC_bn *N, TGLC_bn *E) {
  int pad = (255000 - from_len - 32) % 255 + 32;
  int chunks = (from_len + pad) / 255;
  int bits = TGLC_bn_num_bits (N);
  assert (bits >= 2041 && bits <= 2048);
  assert (from_len > 0 && from_len <= 2550);
  assert (size >= chunks * 256);
  assert (TGLC_rand_pseudo_bytes ((unsigned char *) from + from_len, pad) >= 0);
  int i;
  TGLC_bn *x = TGLC_bn_new ();
  TGLC_bn *y = TGLC_bn_new ();
  assert(x);
  assert(y);
  rsa_encrypted_chunks += chunks;
  for (i = 0; i < chunks; i++) {
    TGLC_bn_bin2bn ((unsigned char *) from, 255, x);
    assert (TGLC_bn_mod_exp (y, x, E, N, TLS->TGLC_bn_ctx) == 1);
    unsigned l = 256 - TGLC_bn_num_bytes (y);
    assert (l <= 256);
    memset (to, 0, l);
    TGLC_bn_bn2bin (y, (unsigned char *) to + l);
    to += 256;
  }
  TGLC_bn_free (x);
  TGLC_bn_free (y);
  return chunks * 256;
}

int tgl_pad_rsa_decrypt (struct tgl_state *TLS, char *from, int from_len, char *to, int size, TGLC_bn *N, TGLC_bn *D) {
  if (from_len < 0 || from_len > 0x1000 || (from_len & 0xff)) {
    return -1;
  }
  int chunks = (from_len >> 8);
  int bits = TGLC_bn_num_bits (N);
  assert (bits >= 2041 && bits <= 2048);
  assert (size >= chunks * 255);
  int i;
  TGLC_bn *x = TGLC_bn_new ();
  TGLC_bn *y = TGLC_bn_new ();
  assert(x);
  assert(y);
  for (i = 0; i < chunks; i++) {
    ++rsa_decrypted_chunks;
    TGLC_bn_bin2bn ((unsigned char *) from, 256, x);
    assert (TGLC_bn_mod_exp (y, x, D, N, TLS->TGLC_bn_ctx) == 1);
    int l = TGLC_bn_num_bytes (y);
    if (l > 255) {
      TGLC_bn_free (x);
      TGLC_bn_free (y);
      return -1;
    }
    assert (l >= 0 && l <= 255);
    memset (to, 0, 255 - l);
    TGLC_bn_bn2bin (y, (unsigned char *) to + 255 - l);
    to += 255;
  }
  TGLC_bn_free (x);
  TGLC_bn_free (y);
  return chunks * 255;
}

static unsigned char aes_key_raw[32], aes_iv[32];
static TGLC_aes_key aes_key;

void tgl_init_aes_unauth (const char server_nonce[16], const char hidden_client_nonce[32], int encrypt) {
  static unsigned char buffer[64], hash[20];
  memcpy (buffer, hidden_client_nonce, 32);
  memcpy (buffer + 32, server_nonce, 16);
  TGLC_sha1 (buffer, 48, aes_key_raw);
  memcpy (buffer + 32, hidden_client_nonce, 32);
  TGLC_sha1 (buffer, 64, aes_iv + 8);
  memcpy (buffer, server_nonce, 16);
  memcpy (buffer + 16, hidden_client_nonce, 32);
  TGLC_sha1 (buffer, 48, hash);
  memcpy (aes_key_raw + 20, hash, 12);
  memcpy (aes_iv, hash + 12, 8);
  memcpy (aes_iv + 28, hidden_client_nonce, 4);
  if (encrypt) {
    TGLC_aes_set_encrypt_key (aes_key_raw, 32*8, &aes_key);
  } else {
    TGLC_aes_set_decrypt_key (aes_key_raw, 32*8, &aes_key);
  }
  memset (aes_key_raw, 0, sizeof (aes_key_raw));
}

void tgl_init_aes_auth (char auth_key[192], char msg_key[16], int encrypt) {
  static unsigned char buffer[48], hash[20];
  //  sha1_a = SHA1 (msg_key + substr (auth_key, 0, 32));
  //  sha1_b = SHA1 (substr (auth_key, 32, 16) + msg_key + substr (auth_key, 48, 16));
  //  sha1_с = SHA1 (substr (auth_key, 64, 32) + msg_key);
  //  sha1_d = SHA1 (msg_key + substr (auth_key, 96, 32));
  //  aes_key = substr (sha1_a, 0, 8) + substr (sha1_b, 8, 12) + substr (sha1_c, 4, 12);
  //  aes_iv = substr (sha1_a, 8, 12) + substr (sha1_b, 0, 8) + substr (sha1_c, 16, 4) + substr (sha1_d, 0, 8);
  memcpy (buffer, msg_key, 16);
  memcpy (buffer + 16, auth_key, 32);
  TGLC_sha1 (buffer, 48, hash);
  memcpy (aes_key_raw, hash, 8);
  memcpy (aes_iv, hash + 8, 12);

  memcpy (buffer, auth_key + 32, 16);
  memcpy (buffer + 16, msg_key, 16);
  memcpy (buffer + 32, auth_key + 48, 16);
  TGLC_sha1 (buffer, 48, hash);
  memcpy (aes_key_raw + 8, hash + 8, 12);
  memcpy (aes_iv + 12, hash, 8);

  memcpy (buffer, auth_key + 64, 32);
  memcpy (buffer + 32, msg_key, 16);
  TGLC_sha1 (buffer, 48, hash);
  memcpy (aes_key_raw + 20, hash + 4, 12);
  memcpy (aes_iv + 20, hash + 16, 4);

  memcpy (buffer, msg_key, 16);
  memcpy (buffer + 16, auth_key + 96, 32);
  TGLC_sha1 (buffer, 48, hash);
  memcpy (aes_iv + 24, hash, 8);
  
  if (encrypt) {
    TGLC_aes_set_encrypt_key (aes_key_raw, 32*8, &aes_key);
  } else {
    TGLC_aes_set_decrypt_key (aes_key_raw, 32*8, &aes_key);
  }
  memset (aes_key_raw, 0, sizeof (aes_key_raw));
}

int tgl_pad_aes_encrypt (char *from, int from_len, char *to, int size) {
  int padded_size = (from_len + 15) & -16;
  assert (from_len > 0 && padded_size <= size);
  if (from_len < padded_size) {
    assert (TGLC_rand_pseudo_bytes ((unsigned char *) from + from_len, padded_size - from_len) >= 0);
  }
  TGLC_aes_ige_encrypt ((unsigned char *) from, (unsigned char *) to, padded_size, &aes_key, aes_iv, 1);
  return padded_size;
}

int tgl_pad_aes_decrypt (char *from, int from_len, char *to, int size) {
  if (from_len <= 0 || from_len > size || (from_len & 15)) {
    return -1;
  }
  TGLC_aes_ige_encrypt ((unsigned char *) from, (unsigned char *) to, from_len, &aes_key, aes_iv, 0);
  return from_len;
}
