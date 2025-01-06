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

/* Blum-Blum-Shub Pseudo Random Number Generator (PRNG).  It is a
   crypto level PRNG: asymptotically the prediction of the next
   generated number is NP-complete task as finding the solution is
   equivalent to solving quadratic residue modulo N problem.

   The PRNG equation is simple x[n+1] = (x[n] * x[n]) mod N, where N
   is a product of two large prime numbers.

   The PRNG finds the two prime numbers during the PRNG
   initialization.  The prime number search is a probabilistic one.
   The numbers might be composite but the probability of this is very
   small 4^(-100).

   Working with big numbers are implemented by GMP.  So you need link
   a GMP Library (-lgmp) if you are going to use the PRNG.

   To use a generator call `init_bbs_prng` first, then call
   `get_bbs_prn` as much as you want to get a new PRN.  At the end of
   the PRNG use, call `finish_bbs_prng`.  You can change the default
   seed by calling set_bbs_seed.

   The PRNG passes NIST Statistical Test Suite for Random and
   Pseudorandom Number Generators for Cryptographic Applications
   (version 2.2.1) with 1000 bitstreams each containing 1M bits.

   The generation of a new number takes about 73K CPU cycles on x86_64
   (Intel 4.2GHz i7-4790K), or speed of the generation is about 58K
   numbers per sec. */

#ifndef __BBS_PRNG__
#define __BBS_PRNG__

#ifdef _MSC_VER
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;
#else
#include <stdint.h>
#endif

#include <stdlib.h>
#include <gmp.h>

/* What size numbers should we use to make the next number prediction
   hard and how hard the prediction can be for a given number is a
   tricky question.  Please, read the Blum Blum Shub article and
   numerous discussion on the Internet.  I believe the default value
   is good for my purposes.  */
#ifndef BBS_PRIME_BITS
#define BBS_PRIME_BITS 512
#endif

/* BBS N which is a factor of two primes and the last generated pseudo
   random number which also can be considered as the current state of
   the PRNG.  */
static MP_INT _BBS_N, _BBS_xn;

/* Set up _BBS_N and _BBS_xn. */
static inline void
init_bbs_prng (void) {
  int i, n;
  MP_INT start;
  
  mpz_init (&start);
  mpz_init (&_BBS_N);
  mpz_init (&_BBS_xn);
  for (n = 0; n != 3;) {
    mpz_set_ui (&start, 0);
    for (i = 0; i < BBS_PRIME_BITS; i++) {
      mpz_mul_ui (&start, &start, 2);
      mpz_add_ui (&start, &start, rand () % 2);
    }
    if (mpz_probab_prime_p (&start, 100) == 0
	/* The following is BBS requirement to have only one solution
	   for the quadratic residue equation.  */
	|| mpz_tdiv_ui (&start, 4) != 3)
      continue;
    if (n == 0) mpz_set (&_BBS_N, &start);
    else if (n == 1) mpz_mul (&_BBS_N, &_BBS_N, &start);
    else mpz_set (&_BBS_xn, &start);
    n++;
  }
  mpz_clear (&start);
}

/* Make _BBS_xn equal to SEED.  */
static inline void
set_bbs_seed (uint32_t seed) {
  mpz_set_ui (&_BBS_xn, seed);
}


/* Update _BBS_xn.  */
static inline void
_update_bbs_prng (void) {
  mpz_mul (&_BBS_xn, &_BBS_xn, &_BBS_xn);
  mpz_tdiv_r (&_BBS_xn, &_BBS_xn, &_BBS_N);
}

/* Return the next pseudo random number.  */
static inline uint64_t
get_bbs_prn (void) {
  int i;
  uint64_t res = 0;
  
  for (i = 0; i < 32; i++) {
    _update_bbs_prng ();
    /* We conservatively use only 2 LS bits.  Depending of
       BBS_PRIME_BITS value it could be more.  */
    res = res << 2 | mpz_tdiv_ui (&_BBS_xn, 4);
  }
  return res;
}

/* Finish work with the PRNG.  */
static inline void
finish_bbs_prng (void) {
  mpz_clear (&_BBS_N);
  mpz_clear (&_BBS_xn);
}

#endif
