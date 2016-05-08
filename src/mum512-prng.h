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

/* Pseudo Random Number Generator (PRNG) based on MUM512 hash
   function.  It might be a crypto level PRNG.

   To use a generator call `init_mum512_prng` first, then call
   `get_mum512_prn` as much as you want to get a new PRN.  At the end
   of the PRNG use, call `finish_mum512_prng`.  You can change the
   default seed by calling set_mum512_seed.

   The PRNG passes NIST Statistical Test Suite for Random and
   Pseudorandom Number Generators for Cryptographic Applications
   (version 2.2.1) with 1000 bitstreams each containing 1M bits.

   The generation of a new number takes about 62 CPU cycles on x86_64
   (Intel 4.2GHz i7-4790K), or speed of the generation is about 67M
   numbers per sec. */

#ifndef __MUM512_PRNG__
#define __MUM512_PRNG__

#include "mum512.h"

/* Default seed (initial state).  */
static uint64_t mum512_start_seed[4][2] = {
    {0Xc0e484a76bbdbf2bULL, 0Xea0a397c33eb3fafULL},
    {0Xf445f7c55f41d3e6ULL, 0X3bd8dc401433c0ddULL},
    {0Xf35d8101c6625902ULL, 0X4dfa7e49a5c0fbf9ULL},
    {0X69695bb2eec9914bULL, 0Xac3cb4a350e58879ULL},
};     

static struct {
  /* An index in the output used to generate the current PRN. */
  int ind;
  /* count from which the current state generated.  */
  _mc_ti count;
  /* The current MUM512 state.  */
  _mc_ti state[4];
  /* The digest used to generate the next 8 PRNs.  */
  uint64_t output[8]; 
} mum512_prng_state;

static inline void
init_mum512_prng (void) {
  int i;

  mum512_prng_state.ind = 0; mum512_prng_state.count = _mc_const (0, 0);
  for (i = 0; i < 4; i++)
    mum512_prng_state.state[i] = _mc_get (mum512_start_seed[i]);
}

/* Make the state equal to {SEED, 0, ..., 0}.  */
static inline void
set_mum512_seed (uint32_t seed) {
  int i;
  
  mum512_prng_state.state[0] = _mc_const (seed, 0);
  for (i = 1; i < 4; i++)
    mum512_prng_state.state[i] = _mc_const (0, 0);
}


static inline uint64_t
get_mum512_prn (void) {
  uint64_t res;
  
  if (mum512_prng_state.ind == 0) {
    mum512_prng_state.count = _mc_add (mum512_prng_state.count, _mc_const (0, 0));
    _mc_hash_aligned (mum512_prng_state.state, &mum512_prng_state.count, 16);
    _mc_mix (mum512_prng_state.state, mum512_prng_state.output);
  }
  res = mum512_prng_state.output[mum512_prng_state.ind];
  mum512_prng_state.ind = (mum512_prng_state.ind + 1) & 0x7;
  return res;
}

static inline void
finish_mum512_prng (void) {
}

#endif
