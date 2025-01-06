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

/* Pseudo Random Number Generator (PRNG) based on ChaCha stream cipher
   designed by D.J. Bernstein.  The code is ChaCha reference code
   (please see https://cr.yp.to/chacha.html) adapted for our purposes.

   It is a crypto level PRNG as the stream cypher on which it is
   based.

   To use a generator call `init_chacha_prng` first, then call
   `get_chacha_prn` as much as you want to get a new PRN.  At the end
   of the PRNG use, call `finish_chacha_prng`.  You can change the
   default seed by calling `set_chacha_prng_seed`.

   The PRNG passes NIST Statistical Test Suite for Random and
   Pseudorandom Number Generators for Cryptographic Applications
   (version 2.2.1) with 1000 bitstreams each containing 1M bits.

   The generation of a new number takes about 40 CPU cycles on x86_64
   (Intel 4.2GHz i7-4790K), or speed of the generation is about 106M
   numbers per sec.  */

#ifndef __CHACHA_PRNG__
#define __CHACHA_PRNG__

#ifdef _MSC_VER
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;
#else
#include <stdint.h>
#endif

#include <stdlib.h>

static inline uint32_t
_chacha_prng_rotl (uint32_t v, int c) {
  return (v << c) | (v >> (32 - c));
}

/* ChaCha state transformation step.  */
static inline void
_chacha_prng_quarter_round (uint32_t *a, uint32_t *b, uint32_t *c, uint32_t *d) {
  *a += *b; *d = _chacha_prng_rotl (*d ^ *a, 16);
  *c += *d; *b = _chacha_prng_rotl (*b ^ *c, 12);
  *a += *b; *d = _chacha_prng_rotl (*d ^ *a, 8);
  *c += *d; *b = _chacha_prng_rotl (*b ^ *c, 7);
}

/* Major ChaCha state transformation.  */
static inline void
_chacha_prng_salsa20 (uint32_t output[16], const uint32_t input[16]) {
  int i;

  for (i = 0; i < 16; i++)
    output[i] = input[i];
  for (i = 8; i > 0; i -= 2) {
    _chacha_prng_quarter_round (&output[0], &output[4], &output[8],&output[12]);
    _chacha_prng_quarter_round (&output[1], &output[5], &output[9],&output[13]);
    _chacha_prng_quarter_round (&output[2], &output[6],&output[10],&output[14]);
    _chacha_prng_quarter_round (&output[3], &output[7],&output[11],&output[15]);
    _chacha_prng_quarter_round (&output[0], &output[5],&output[10],&output[15]);
    _chacha_prng_quarter_round (&output[1], &output[6],&output[11],&output[12]);
    _chacha_prng_quarter_round (&output[2], &output[7], &output[8],&output[13]);
    _chacha_prng_quarter_round (&output[3], &output[4], &output[9],&output[14]);
  }
  for (i = 0; i < 16; i++)
    output[i] += input[i];
}

/* Internal state of the PRNG.  */
static struct {
  int ind; /* position in the output */
  /* The current PRNG state and parts of the recently generated
     numbers.  */
  uint32_t input[16], output[16];
} _chacha_prng_state;

/* Some random prime numbers.  */
static const uint32_t chacha_prng_primes[4] = {0xfa835867, 0x2086ca69, 0x1467c0fb, 0x638e2b99};

/* Internal function to set ChaCha PRNG seed by K and IV.  */
static inline void
_set_chacha_prng_key_iv (uint32_t k[8], uint32_t iv[2]) {
  int i;
  
  for (i = 0; i < 8; i++)
    _chacha_prng_state.input[i + 4] = k[i];
  for (i = 0; i < 4; i++)
    _chacha_prng_state.input[i] = chacha_prng_primes[i];
  _chacha_prng_state.input[12] = 0;
  _chacha_prng_state.input[13] = 0;
  _chacha_prng_state.input[14] = iv[0];
  _chacha_prng_state.input[15] = iv[1];
}

/* Internal function to initiate ChaCha PRNG with seed given by K and
   IV.  */
static inline void
_init_chacha_prng_with_key_iv (uint32_t k[8], uint32_t iv[2]) {
  _set_chacha_prng_key_iv (k, iv);
  _chacha_prng_state.ind = 16;
}

/* Initiate the PRNG with some random seed.  */
static inline void
init_chacha_prng (void) {
  int i;
  uint32_t k[8], iv[2];

  for (i = 0; i < 8; i++)
    k[i] = rand ();
  iv[0] = rand ();
  iv[1] = rand ();
  _init_chacha_prng_with_key_iv (k, iv);
}

/* Set ChaCha PRNG SEED.  */
static inline void
set_chacha_prng_seed (uint32_t seed) {
  static uint32_t k[8] = {0, 0, 0, 0};
  static uint32_t iv[2] = {0, 0};
  
  k[0] = seed;
  _set_chacha_prng_key_iv (k, iv);
}

/* Return the next pseudo-random number.  */
static inline uint64_t
get_chacha_prn (void)
{
  uint64_t res;

  for (;;) {
    if (_chacha_prng_state.ind < 16) {
      res = ((uint64_t) _chacha_prng_state.output[_chacha_prng_state.ind] << 32
	     | _chacha_prng_state.output[_chacha_prng_state.ind + 1]);
      _chacha_prng_state.ind += 2;
      return res;
    }
    _chacha_prng_state.ind = 0;
    _chacha_prng_salsa20 (_chacha_prng_state.output, _chacha_prng_state.input);
    _chacha_prng_state.input[12]++;
    if (! _chacha_prng_state.input[12])
      /* If it is becomming zero we produced too many numbers by
	 current PRNG.  */
      _chacha_prng_state.input[13]++;
  }
}

/* Empty function for our PRNGs interface.  */
static inline void
finish_chacha_prng (void) {
}

#endif
