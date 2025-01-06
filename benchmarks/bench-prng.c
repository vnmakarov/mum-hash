#define N1 100000
#if defined(BBS)
#include "bbs-prng.h"
#define N2 2
static void init_prng (void) { init_bbs_prng (); }
static uint64_t get_prn (void) { return get_bbs_prn (); }
static void finish_prng (void) { finish_bbs_prng (); }
#elif defined(CHACHA)
#include "chacha-prng.h"
#define N2 5000
static void init_prng (void) { init_chacha_prng (); }
static uint64_t get_prn (void) { return get_chacha_prn (); }
static void finish_prng (void) { finish_chacha_prng (); }
#elif defined(SIP24)
#include "sip24-prng.h"
#define N2 5000
static void init_prng (void) { init_sip24_prng (); }
static uint64_t get_prn (void) { return get_sip24_prn (); }
static void finish_prng (void) { finish_sip24_prng (); }
#elif defined(MUM)
#include "mum-prng.h"
#define N2 30000
static void init_prng (void) { mum_hash_randomize (0); init_mum_prng (); }
static uint64_t get_prn (void) { return get_mum_prn (); }
static void finish_prng (void) { finish_mum_prng (); }
#elif defined(MUM512)
#include "mum512-prng.h"
#define N2 2000
static void init_prng (void) { init_mum512_prng (); }
static uint64_t get_prn (void) { return get_mum512_prn (); }
static void finish_prng (void) { finish_mum512_prng (); }
#elif defined(XOROSHIRO128P)
#include "xoroshiro128plus.c"
#define N2 30000
static void init_prng (void) { s[0] = 0xe220a8397b1dcdaf; s[1] = 0x6e789e6aa1b965f4; }
static uint64_t get_prn (void) { return next (); }
static void finish_prng (void) { }
#elif defined(XOROSHIRO128STARSTAR)
#include "xoroshiro128starstar.c"
#define N2 30000
static void init_prng (void) { s[0] = 0xe220a8397b1dcdaf; s[1] = 0x6e789e6aa1b965f4; }
static uint64_t get_prn (void) { return next (); }
static void finish_prng (void) { }
#elif defined(XOSHIRO256P)
#include "xoshiro256plus.c"
#define N2 30000
static void init_prng (void) { s[0] = 0xe220a8397b1dcdaf; s[1] = 0x6e789e6aa1b965f4; s[2] = 0x6c45d188009454f; s[3] = 0xf88bb8a8724c81ec; }
static uint64_t get_prn (void) { return next (); }
static void finish_prng (void) { }
#elif defined(XOSHIRO256STARSTAR)
#include "xoshiro256starstar.c"
#define N2 30000
static void init_prng (void) { s[0] = 0xe220a8397b1dcdaf; s[1] = 0x6e789e6aa1b965f4; s[2] = 0x6c45d188009454f; s[3] = 0xf88bb8a8724c81ec; }
static uint64_t get_prn (void) { return next (); }
static void finish_prng (void) { }
#elif defined(XOSHIRO512P)
#include "xoshiro512plus.c"
#define N2 30000
static void init_prng (void) { s[0] = 0xe220a8397b1dcdaf; s[1] = 0x6e789e6aa1b965f4; s[2] = 0x6c45d188009454f; s[3] = 0xf88bb8a8724c81ec; s[4] = 0x1b39896a51a8749b; s[5] = 0x53cb9f0c747ea2ea; s[6] = 0x2c829abe1f4532e1; s[7] = 0xc584133ac916ab3c; }
static uint64_t get_prn (void) { return next (); }
static void finish_prng (void) { }
#elif defined(XOSHIRO512STARSTAR)
#include "xoshiro512starstar.c"
#define N2 30000
static void init_prng (void) { s[0] = 0xe220a8397b1dcdaf; s[1] = 0x6e789e6aa1b965f4; s[2] = 0x6c45d188009454f; s[3] = 0xf88bb8a8724c81ec; s[4] = 0x1b39896a51a8749b; s[5] = 0x53cb9f0c747ea2ea; s[6] = 0x2c829abe1f4532e1; s[7] = 0xc584133ac916ab3c; }
static uint64_t get_prn (void) { return next (); }
static void finish_prng (void) { }
#elif defined(RAND)
#include <stdlib.h>
#include <stdint.h>
#define N2 7000
static void init_prng (void) { }
static uint64_t get_prn (void) { return rand (); }
static void finish_prng (void) { }
#endif

uint64_t dummy;
#include <stdio.h>
#include <time.h>

#ifdef OUTPUT
#include <stdint.h>

int main(void)
{
    init_prng ();
    while (1) {
        uint64_t value = get_prn();
        fwrite((void*) &value, sizeof(value), 1, stdout);
    }
}

#else
int main (void) {
  int i, j; double d; uint64_t res = 0;
  clock_t t = clock ();
  
  init_prng ();
  for (i = 0; i < N2; i++)
    for (j = 0; j < N1; j++)
      res ^= get_prn ();
  finish_prng ();
  t = clock () - t;
  d = (N1 + 0.0) * N2 * CLOCKS_PER_SEC / t / 1000;
  if (d > 1000)
    printf ("%7.2fM prns/sec\n", d / 1000);
  else
    printf ("%7.2fK prns/sec\n", d);
  dummy = res;
  return 0;
}
#endif
