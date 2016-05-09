/* Copyright (c) 2016 Vladimir Makarov <vmakarov@gcc.gnu.org>

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

   The generation of a new number takes about 6 CPU cycles on x86_64
   (Intel 4.2GHz i7-4790K), or speed of the generation is about 700M
   numbers per sec.  So it is very fast.  */

#ifndef __MUM_PRNG__
#define __MUM_PRNG__

#include "mum.h"

static struct {
  /* MUM state */
  uint64_t state; 
} mum_prng_state;

static inline void
init_mum_prng (void) { mum_prng_state.state = 1; }

static inline void
set_mum_seed (uint32_t seed) { mum_prng_state.state = seed; }

static inline uint64_t
get_mum_prn (void) {
  uint64_t r;
#ifdef MUM_PRNG_STANDARD_INTERFACE
  /* mum_hash64 provides the same hash for the same key.  Don't use
     mum_hash here.  */
  r = mum_hash64 (mum_prng_state.state, 0x746addee2093e7e1ULL);
#else
  r = _mum (mum_prng_state.state, _mum_key_step_prime);
#endif
  mum_prng_state.state ^= r;
  return r;
}

static inline void
finish_mum_prng (void) {
}


#endif
