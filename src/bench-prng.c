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
static void init_prng (void) { init_mum_prng (); }
static uint64_t get_prn (void) { return get_mum_prn (); }
static void finish_prng (void) { finish_mum_prng (); }
#elif defined(MUM512)
#include "mum512-prng.h"
#define N2 2000
static void init_prng (void) { init_mum512_prng (); }
static uint64_t get_prn (void) { return get_mum512_prn (); }
static void finish_prng (void) { finish_mum512_prng (); }
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



