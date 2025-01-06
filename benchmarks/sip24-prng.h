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

/* Pseudo Random Number Generator (PRNG) based on SipHash24 designed
   by J.P. Aumasson and D.J. Bernstein.  The SipHash24 reference code
   (please see https://github.com/veorq/SipHash) adapted for our
   purposes.

   To use a generator call `init_sip24_prng` or
   `init_sip24_prng_with_seed` first, then call `get_sip24_prn` as much
   as you want to get a new PRN.  At the end of the PRNG use, call
   `finish_sip24_prng`.  You can change the default seed by calling
   `set_sip24_prng_seed`.

   The PRNG passes NIST Statistical Test Suite for Random and
   Pseudorandom Number Generators for Cryptographic Applications
   (version 2.2.1) with 1000 bitstreams each containing 1M bits.

   The generation of a new number takes about 11 CPU cycles on x86_64
   (Intel 4.2GHz i7-4790K), or speed of the generation is about 380M
   numbers per sec.  So it is pretty fast.  */

#ifndef __SIP24_PRNG__
#define __SIP24_PRNG__

#ifdef _MSC_VER
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;
#else
#include <stdint.h>
#endif

#include <stdlib.h>

static inline uint64_t
_sip24_prng_rotl (uint64_t v, int c) {
  return (v << c) | (v >> (64 - c));
}

/* A Sip round */
static inline void
_sip24_prng_round (uint64_t *v0, uint64_t *v1, uint64_t *v2, uint64_t *v3) {
    *v0 += *v1; *v1 = _sip24_prng_rotl (*v1, 13);
    *v1 ^= *v0; *v0 = _sip24_prng_rotl (*v0, 32);
    *v2 += *v3; *v3 = _sip24_prng_rotl (*v3, 16);
    *v3 ^= *v2;
    *v0 += *v3; *v3 = _sip24_prng_rotl (*v3, 21);
    *v3 ^= *v0;
    *v2 += *v1; *v1 = _sip24_prng_rotl (*v1, 17);
    *v1 ^= *v2; *v2 = _sip24_prng_rotl (*v2, 32);
}

/* The number of intermediate and final Sip rounds.  */
#define cROUNDS 2
#define dROUNDS 4

/* Internal state of the PRNG.  */
static struct {
  int ind; /* position in the output */
  uint64_t v; /* current message from which PRNs were generated */
  /* The current PRNG state and parts of the recenty generated
     numbers.  */
  uint64_t init_state[4], state[4];
} _sip24_prng_state;

static inline void
_sip24_prng_gen (void)
{
  int i;

  for (i = 0; i < 4; i++)
    _sip24_prng_state.state[i] = _sip24_prng_state.init_state[i];
  _sip24_prng_state.state[3] ^= _sip24_prng_state.v;
  for (i = 0; i < cROUNDS; ++i)
    _sip24_prng_round (&_sip24_prng_state.state[0], &_sip24_prng_state.state[1],
		       &_sip24_prng_state.state[2], &_sip24_prng_state.state[3]);
  _sip24_prng_state.state[0] ^= _sip24_prng_state.v;
  for (i = 0; i < cROUNDS; ++i)
    _sip24_prng_round (&_sip24_prng_state.state[0], &_sip24_prng_state.state[1],
		       &_sip24_prng_state.state[2], &_sip24_prng_state.state[3]);
  _sip24_prng_state.state[2] ^= 0xff;
  for (i = 0; i < dROUNDS; ++i)
    _sip24_prng_round (&_sip24_prng_state.state[0], &_sip24_prng_state.state[1],
		       &_sip24_prng_state.state[2], &_sip24_prng_state.state[3]);
}

/* Some random numbers from Siphash24.  */
static const uint64_t _sip24_prng_nums[4] = {
  0x736f6d6570736575ULL, 0x646f72616e646f6dULL,
  0x6c7967656e657261ULL, 0x7465646279746573ULL,
};

/* Set the PRNG seed by K.  */
static inline void
set_sip24_prng_seed (uint64_t k[4]) {
  int i;
  
  for (i = 0; i < 4; i++)
    _sip24_prng_state.init_state[i] = k[i] ^ _sip24_prng_nums[i];
}

/* Initiate the PRNG with seed given by K.  */
static inline void
init_sip24_prng_with_seed (uint64_t k[4]) {
  set_sip24_prng_seed (k);
  _sip24_prng_state.v = 0;
  _sip24_prng_state.ind = 4;
}

/* Initiate the PRNG with some random seed.  */
static inline void
init_sip24_prng (void) {
  int i;
  uint64_t k[4];

  for (i = 0; i < 4; i++)
    k[i] = rand ();
  init_sip24_prng_with_seed (k);
}

/* Return the next pseudo-random number.  */
static inline uint64_t
get_sip24_prn (void)
{
  uint64_t res;
  
  for (;;) {
    if (_sip24_prng_state.ind < 4) {
      res = _sip24_prng_state.state[_sip24_prng_state.ind];
      _sip24_prng_state.ind++;
      return res;
    }
    _sip24_prng_state.ind = 0;
    _sip24_prng_gen ();
    _sip24_prng_state.v++; 
  }
}

/* Empty function for our PRNGs interface.  */
static inline void
finish_sip24_prng (void) {
}

#endif
