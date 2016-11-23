#ifndef __MTPROTO_UTILS_H__
#define __MTPROTO_UTILS_H__
#include "crypto/bn.h"
int tglmp_check_DH_params (struct tgl_state *TLS, TGLC_bn *p, int g);
int tglmp_check_g_a (struct tgl_state *TLS, TGLC_bn *p, TGLC_bn *g_a);
int bn_factorize (TGLC_bn *pq, TGLC_bn *p, TGLC_bn *q);
#endif
