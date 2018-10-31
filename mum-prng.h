/* Copyright (c) 2016, 2017, 2018
   Vladimir Makarov <vmakarov@gcc.gnu.org>

   Permission is hereby granted, free of charge, to any person
   obtaining a copy of this software and associated documentation
   files (the "Software"), to deal in the Software without
   restriction, including without limitation the rights to use, copy,
   modify, merge, publish, distribute, sublicense, and/or sell copies
   of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
   BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
*/

/* Pseudo Random Number Generator (PRNG) based on MUM hash function.
   It is not a crypto level PRNG.

   To use a generator call `init_mum_prng` first, then call
   `get_mum_prn` as much as you want to get a new PRN.  At the end of
   the PRNG use, call `finish_mum_prng`.  You can change the default
   seed by calling set_mum_seed.

   The PRNG passes NIST Statistical Test Suite for Random and
   Pseudorandom Number Generators for Cryptographic Applications
   (version 2.2.1) with 1000 bitstreams each containing 1M bits.

   The generation of a new number takes about 3.5 CPU cycles on x86_64
   (Intel 4.2GHz i7-4790K), or speed of the generation is about 1120M
   numbers per sec.  So it is very fast.  */

#ifndef __MUM_PRNG__
#define __MUM_PRNG__

#include "mum.h"

#ifndef MUM_PRNG_UNROLL
#define MUM_PRNG_UNROLL 16
#endif

#if MUM_PRNG_UNROLL < 1 || MUM_PRNG_UNROLL > 16
#error "wrong MUM_PRNG_UNROLL value"
#endif

#ifdef __GNUC__
#define EXPECT(cond, v) __builtin_expect((cond), (v))
#else
#define EXPECT(cond, v) (cond)
#endif

#if defined(__GNUC__) && ((__GNUC__ == 4) &&  (__GNUC_MINOR__ >= 9) || (__GNUC__ > 4))
#define _MUM_PRNG_FRESH_GCC
#endif

static struct {
  int count; 
#if defined(__x86_64__) && defined(_MUM_PRNG_FRESH_GCC)
  int avx2_support;
#endif
  /* MUM PRNG state */
  uint64_t state[MUM_PRNG_UNROLL]; 
} _mum_prng_state;

#if defined(__x86_64__) && defined(_MUM_PRNG_FRESH_GCC)
static inline void
_mum_prng_setup_avx2 (void) {
  __builtin_cpu_init ();
  _mum_prng_state.avx2_support = __builtin_cpu_supports ("avx2");
}
#endif

static inline void
_start_mum_prng (uint32_t seed) {
  int i;

  _mum_prng_state.count = MUM_PRNG_UNROLL;
  for (i = 0; i < MUM_PRNG_UNROLL; i++)
    _mum_prng_state.state[i] = seed + 1;
#if defined(__x86_64__) && defined(_MUM_PRNG_FRESH_GCC)
  _mum_prng_setup_avx2 ();
#endif
}

static inline void
init_mum_prng (void) {
  _start_mum_prng (0);
}

static inline void
set_mum_prng_seed (uint32_t seed) {
  _start_mum_prng (seed);
}

#if defined(__x86_64__) && defined(_MUM_PRNG_FRESH_GCC)
/* This code specialized for Haswell generates MULX insns. */
static inline uint64_t _MUM_TARGET("arch=haswell")
_mum_avx2 (uint64_t v, uint64_t p) {
  uint64_t hi, lo;
  __uint128_t r = (__uint128_t) v * (__uint128_t) p;
  hi = (uint64_t) (r >> 64);
  lo = (uint64_t) r;
  return hi + lo;
}

static void _MUM_TARGET("arch=haswell")
_mum_prng_update_avx2 (void) {
  int i;

  _mum_prng_state.count = 0;
  for (i = 0; i < MUM_PRNG_UNROLL; i++)
    _mum_prng_state.state[i] ^= _mum_avx2 (_mum_prng_state.state[i], _mum_primes[i]);
}
#endif

static void __attribute__ ((noinline))
_mum_prng_update (void) {
  int i;
  _mum_prng_state.count = 0;
  for (i = 0; i < MUM_PRNG_UNROLL; i++)
    _mum_prng_state.state[i] ^= _mum (_mum_prng_state.state[i], _mum_primes[i]);
}

static inline uint64_t
get_mum_prn (void) {
  if (EXPECT (_mum_prng_state.count == MUM_PRNG_UNROLL, 0)) {
#if defined(__x86_64__) && defined(_MUM_PRNG_FRESH_GCC)
    if (EXPECT (_mum_prng_state.avx2_support, 1)) {
      _mum_prng_update_avx2 ();
      return _mum_prng_state.state[_mum_prng_state.count++];
    }
#endif
    _mum_prng_update ();
  }
  return _mum_prng_state.state[_mum_prng_state.count++];
}

static inline void
finish_mum_prng (void) {
}

#endif
