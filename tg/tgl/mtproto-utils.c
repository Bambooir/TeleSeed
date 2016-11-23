#include "config.h"
#include "crypto/bn.h"
#include "tl-parser/portable_endian.h"
#include "tgl.h"
#include "tools.h"
#include "mtproto-utils.h"

static unsigned long long gcd (unsigned long long a, unsigned long long b) {
  return b ? gcd (b, a % b) : a;
}

static int check_prime (struct tgl_state *TLS, TGLC_bn *p) {
  int r = TGLC_bn_is_prime (p, /* "use default" */ 0, 0, TLS->TGLC_bn_ctx, 0);
  ensure (r >= 0);
  return r;
}


// Complete set of checks see at https://core.telegram.org/mtproto/security_guidelines


// Checks that (p,g) is acceptable pair for DH
int tglmp_check_DH_params (struct tgl_state *TLS, TGLC_bn *p, int g) {
  if (g < 2 || g > 7) { return -1; }
  if (TGLC_bn_num_bits (p) != 2048) { return -1; }
  
  TGLC_bn *t = TGLC_bn_new ();
  
  TGLC_bn *dh_g = TGLC_bn_new ();
  
  ensure (TGLC_bn_set_word (dh_g, 4 * g));
  ensure (TGLC_bn_mod (t, p, dh_g, TLS->TGLC_bn_ctx));
  int x = TGLC_bn_get_word (t);
  assert (x >= 0 && x < 4 * g);

  TGLC_bn_free (dh_g);

  int res = 0;
  switch (g) {
  case 2:
    if (x != 7) { res = -1; }
    break;
  case 3:
    if (x % 3 != 2) { res = -1; }
    break;
  case 4:
    break;
  case 5:
    if (x % 5 != 1 && x % 5 != 4) { res = -1; }
    break;
  case 6:
    if (x != 19 && x != 23) { res = -1; }
    break;
  case 7:
    if (x % 7 != 3 && x % 7 != 5 && x % 7 != 6) { res = -1; }
    break;
  }

  if (res < 0 || !check_prime (TLS, p)) { 
    TGLC_bn_free (t);
    return -1; 
  }

  TGLC_bn *b = TGLC_bn_new ();
  ensure (TGLC_bn_set_word (b, 2));
  ensure (TGLC_bn_div (t, 0, p, b, TLS->TGLC_bn_ctx));
  if (!check_prime (TLS, t)) { 
    res = -1;
  }
  TGLC_bn_free (b);
  TGLC_bn_free (t);
  return res;
}

// checks that g_a is acceptable for DH
int tglmp_check_g_a (struct tgl_state *TLS, TGLC_bn *p, TGLC_bn *g_a) {
  if (TGLC_bn_num_bytes (g_a) > 256) {
    return -1;
  }
  if (TGLC_bn_num_bits (g_a) < 2048 - 64) {
    return -1;
  }
  if (TGLC_bn_cmp (p, g_a) <= 0) {
    return -1;
  }
  
  TGLC_bn *dif = TGLC_bn_new ();
  TGLC_bn_sub (dif, p, g_a);
  if (TGLC_bn_num_bits (dif) < 2048 - 64) {
    TGLC_bn_free (dif);
    return -1;
  }
  TGLC_bn_free (dif);
  return 0;
}

static unsigned long long BN2ull (TGLC_bn *b) {
  if (sizeof (unsigned long) == 8) {
    return TGLC_bn_get_word (b);
  } else if (sizeof (unsigned long long) == 8) {
//    assert (0); // As long as nobody ever uses this code, assume it is broken.
    unsigned long long tmp;
    /* Here be dragons, but it should be okay due to be64toh */
    TGLC_bn_bn2bin (b, (unsigned char *) &tmp);
    return be64toh (tmp);
  } else {
    assert (0);
  }
}

static void ull2BN (TGLC_bn *b, unsigned long long val) {
  if (sizeof (unsigned long) == 8 || val < (1ll << 32)) {
    TGLC_bn_set_word (b, val);
  } else if (sizeof (unsigned long long) == 8) {
// assert (0); // As long as nobody ever uses this code, assume it is broken.
    htobe64(val);
    /* Here be dragons, but it should be okay due to htobe64 */
    TGLC_bn_bin2bn ((unsigned char *) &val, 8, b);
  } else {
    assert (0);
  }
}

int bn_factorize (TGLC_bn *pq, TGLC_bn *p, TGLC_bn *q) {
  // Should work in any case
  // Rewrite this code
  unsigned long long what = BN2ull (pq);

  int it = 0;
  
  unsigned long long g = 0;
  int i;
  for (i = 0; i < 3 || it < 1000; i++) {
    int q = ((rand() & 15) + 17) % what;
    unsigned long long x = (long long)rand () % (what - 1) + 1, y = x;
    int lim = 1 << (i + 18);
    int j;
    for (j = 1; j < lim; j++) {
      ++it;
      unsigned long long a = x, b = x, c = q;
      while (b) {
        if (b & 1) {
          c += a;
          if (c >= what) {
            c -= what;
          }
        }
        a += a;
        if (a >= what) {
          a -= what;
        }
        b >>= 1;
      }
      x = c;
      unsigned long long z = x < y ? what + x - y : x - y;
      g = gcd (z, what);
      if (g != 1) {
        break;
      }
      if (!(j & (j - 1))) {
        y = x;
      }
    }
    if (g > 1 && g < what) break;
  }

  assert (g > 1 && g < what);
  unsigned long long p1 = g;
  unsigned long long p2 = what / g;
  if (p1 > p2) {
    unsigned long long t = p1; p1 = p2; p2 = t;
  }
  ull2BN (p, p1);
  ull2BN (q, p2);
  return 0;
}

