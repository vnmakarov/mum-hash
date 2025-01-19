/* Copyright (c) 2025
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

/* This file implements MUM (MUltiply and Mix) hashing. We randomize input data by 64x64-bit
   multiplication and mixing hi- and low-parts of the multiplication result by using an addition and
   then mix it into the current state. We use prime numbers randomly generated with the equal
   probability of their bit values for the multiplication. When all primes are used once, the state
   is randomized and the same prime numbers are used again for data randomization.

   The MUM hashing passes all SMHasher tests. Pseudo Random Number Generator based on MUM also
   passes NIST Statistical Test Suite for Random and Pseudorandom Number Generators for
   Cryptographic Applications (version 2.2.1) with 1000 bitstreams each containing 1M bits. MUM
   hashing is also faster Spooky64 and City64 on small strings (at least upto 512-bit) on Haswell
   and Power7. The MUM bulk speed (speed on very long data) is bigger than Spooky and City on
   Power7. On Haswell the bulk speed is bigger than Spooky one and close to City speed. */

#ifndef __MUM_HASH__
#define __MUM_HASH__

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#ifdef _MSC_VER
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;
#else
#include <stdint.h>
#endif

#ifdef __GNUC__
#define _MUM_ATTRIBUTE_UNUSED __attribute__ ((unused))
#define _MUM_OPTIMIZE(opts) __attribute__ ((__optimize__ (opts)))
#define _MUM_TARGET(opts) __attribute__ ((__target__ (opts)))
#define _MUM_INLINE inline __attribute__ ((always_inline))
#else
#define _MUM_ATTRIBUTE_UNUSED
#define _MUM_OPTIMIZE(opts)
#define _MUM_TARGET(opts)
#define _MUM_INLINE inline
#endif

/* Macro saying to use 128-bit integers implemented by GCC for some targets. */
#ifndef _MUM_USE_INT128
/* In GCC uint128_t is defined if HOST_BITS_PER_WIDE_INT >= 64. HOST_WIDE_INT is long if
   HOST_BITS_PER_LONG > HOST_BITS_PER_INT, otherwise int. */
#if defined(__GNUC__) && UINT_MAX != ULONG_MAX
#define _MUM_USE_INT128 1
#else
#define _MUM_USE_INT128 0
#endif
#endif

/* Here are different primes randomly generated with the equal probability of their bit values. They
   are used to randomize input values. */
static uint64_t _mum_hash_step_prime = 0x2e0bb864e9ea7df5ULL;
static uint64_t _mum_key_step_prime = 0xcdb32970830fcaa1ULL;
static uint64_t _mum_block_start_prime = 0xc42b5e2e6480b23bULL;
static uint64_t _mum_tail_prime = 0xaf47d47c99b1461bULL;
static uint64_t _mum_finish_prime1 = 0xa9a7ae7ceff79f3fULL;
static uint64_t _mum_finish_prime2 = 0xaf47d47c99b1461bULL;

static uint64_t _mum_unroll_primes[]
  = {0x14b09aa58cb268d5, 0xf34d372c6f503cc9, 0x6289be577a4ae0a6, 0x3ba5692b1eafeb15};

static uint64_t _mum_primes[] = {
  0X6413c7b09fc3dcc8, 0X55fc3da745579871, 0X160111a644959396, 0Xd12e059acb048b01,
  0Xc66e719ac9a96714, 0X832bb4f760cb0c0d, 0X92b777a80cfd2a59, 0X883dc6537f8d2a6b,
  0Xf40a3463bd3220d7, 0Xadd624b1439d6507, 0X2d11c5c5e6c3bf75, 0Xfc027148e7ed66c9,
  0Xe9437c1b3494cb46, 0Xacd35405f3d2f4b7, 0X838f551f688bd046, 0X3e87440f1a4642e9,
  0Xce20a8f09e985b88, 0Xa98931ac95adde85, 0Xe3580702cf5a1c80, 0X8e76e05a566733cd,
  0X15ff369908c7d67d, 0X11bc44ed911d2aa7, 0Xab2694c16fe59e02, 0Xc1f5ba02d82d5e85,
  0X7c59a06636a7eddd, 0X684643ed7e49b619, 0X40b6270f7f9fa251, 0X9d8b29eef5b17a8d,
  0X4044ad0f26d7e774, 0Xf3d1fd1ab3ca5037, 0X5ec3d199e0063437, 0X12fb529ef8cc3a73,
};

/* Multiply 64-bit V and P and return sum of high and low parts of the result. */
static _MUM_INLINE uint64_t _mum (uint64_t v, uint64_t p) {
  uint64_t hi, lo;
#if _MUM_USE_INT128
  __uint128_t r = (__uint128_t) v * (__uint128_t) p;
  hi = (uint64_t) (r >> 64);
  lo = (uint64_t) r;
#else
  /* Implementation of 64x64->128-bit multiplication by four 32x32->64 bit multiplication. */
  uint64_t hv = v >> 32, hp = p >> 32;
  uint64_t lv = (uint32_t) v, lp = (uint32_t) p;
  uint64_t rh = hv * hp;
  uint64_t rm_0 = hv * lp;
  uint64_t rm_1 = hp * lv;
  uint64_t rl = lv * lp;
  return rh + rl + rm_0 + rm_1;
  uint64_t t, carry = 0;

  /* We could ignore a carry bit here if we did not care about the same hash for 32-bit and 64-bit
     targets. */
  t = rl + (rm_0 << 32);
#ifdef MUM_TARGET_INDEPENDENT_HASH
  carry = t < rl;
#endif
  lo = t + (rm_1 << 32);
#ifdef MUM_TARGET_INDEPENDENT_HASH
  carry += lo < t;
#endif
  hi = rh + (rm_0 >> 32) + (rm_1 >> 32) + carry;
#endif
  /* We could use XOR here too but, for some reasons, on Haswell and Power7 using an addition
     improves hashing performance by 10% for small strings. */
  return hi + lo;
}

#if defined(_MSC_VER)
#define _mum_bswap_32(x) _byteswap_uint32_t (x)
#define _mum_bswap_64(x) _byteswap_uint64_t (x)
#elif defined(__APPLE__)
#include <libkern/OSByteOrder.h>
#define _mum_bswap_32(x) OSSwapInt32 (x)
#define _mum_bswap_64(x) OSSwapInt64 (x)
#elif defined(__GNUC__)
#define _mum_bswap32(x) __builtin_bswap32 (x)
#define _mum_bswap64(x) __builtin_bswap64 (x)
#else
#include <byteswap.h>
#define _mum_bswap32(x) bswap32 (x)
#define _mum_bswap64(x) bswap64 (x)
#endif

static _MUM_INLINE uint64_t _mum_le (uint64_t v) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__ || !defined(MUM_TARGET_INDEPENDENT_HASH)
  return v;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
  return _mum_bswap64 (v);
#else
#error "Unknown endianess"
#endif
}

static _MUM_INLINE uint32_t _mum_le32 (uint32_t v) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__ || !defined(MUM_TARGET_INDEPENDENT_HASH)
  return v;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
  return _mum_bswap32 (v);
#else
#error "Unknown endianess"
#endif
}

static _MUM_INLINE uint64_t _mum_le16 (uint16_t v) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__ || !defined(MUM_TARGET_INDEPENDENT_HASH)
  return v;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
  return (v >> 8) | ((v & 0xff) << 8);
#else
#error "Unknown endianess"
#endif
}

/* Macro defining how many times the most nested loop in _mum_hash_aligned will be unrolled by the
   compiler (although it can make an own decision:). Use only a constant here to help a compiler to
   unroll a major loop.

   The macro value affects the result hash for strings > 128 bit. The unroll factor greatly affects
   the hashing speed. We prefer the speed. */

#define _MUM_UNROLL_BLOCK_FACTOR 8
#if defined(__AVX2__)
typedef long long __attribute__ ((vector_size (32), aligned (1))) _mum_block_t;
typedef int __attribute__ ((vector_size (32), aligned (1))) _mum_v8si;
static _MUM_INLINE _mum_block_t _mum_block (_mum_block_t v, _mum_block_t p) {
  _mum_block_t hv = v >> 32, hp = p >> 32;
  _mum_block_t r = (_mum_block_t) (__builtin_ia32_pmuludq256 ((_mum_v8si) v, (_mum_v8si) p)
                                   + __builtin_ia32_pmuludq256 ((_mum_v8si) hv, (_mum_v8si) hp));
  return r;
}
static _MUM_INLINE void _mum_update_block (_mum_block_t *s, const _mum_block_t *v,
                                           const _mum_block_t *p) {
  *s ^= _mum_block (v[0] ^ p[0], v[1] ^ p[1]);
}
static _MUM_INLINE void _mum_factor_block (_mum_block_t *s, const _mum_block_t *p) {
  *s = _mum_block (*s, *p);
}
static _MUM_INLINE void _mum_zero_block (_mum_block_t *b) { *b = (_mum_block_t) {0, 0, 0, 0}; }
static _MUM_INLINE uint64_t _mum_fold_block (const _mum_block_t *b) {
  return (*b)[0] ^ (*b)[1] ^ (*b)[2] ^ (*b)[3];
}
#elif defined(__ARM_NEON)
#include <arm_neon.h>
typedef struct {
  uint64x2_t v[2];
} _mum_block_t;
static _MUM_INLINE uint64x2_t _mum_val (uint64x2_t v, uint64x2_t p) {
  uint32x4_t v32 = (uint32x4_t) v, p32 = (uint32x4_t) p;
  uint64x2_t r = vmull_u32 (vget_low_u32 (v32), vget_low_u32 (p32)), r2 = vmull_high_u32 (v32, p32);
  return vpaddq_u64 (r, r2);
}
static _MUM_INLINE void _mum_update_block (_mum_block_t *s, const _mum_block_t *v,
                                           const _mum_block_t *p) {
  const _mum_block_t *v2 = &v[1], *p2 = &p[1];
  s->v[0] ^= _mum_val (v->v[0] ^ p->v[0], v2->v[0] ^ p2->v[0]);
  s->v[1] ^= _mum_val (v->v[1] ^ p->v[1], v2->v[1] ^ p2->v[1]);
}
static _MUM_INLINE void _mum_factor_block (_mum_block_t *s, const _mum_block_t *p) {
  s->v[0] = _mum_val (s->v[0], p->v[0]);
  s->v[1] = _mum_val (s->v[1], p->v[1]);
}
static _MUM_INLINE void _mum_zero_block (_mum_block_t *b) { *b = (_mum_block_t) {0, 0, 0, 0}; }
static _MUM_INLINE uint64_t _mum_fold_block (_mum_block_t *b) {
  return b->v[0][0] ^ b->v[0][1] ^ b->v[1][0] ^ b->v[1][1];
}
#elif defined(__ALTIVEC__)
#include <altivec.h>
typedef unsigned long long __attribute__ ((vector_size (16))) _mum_v2di;
typedef struct {
  _mum_v2di v[2];
} _mum_block_t;
static _MUM_INLINE _mum_v2di _mum_val (_mum_v2di v, _mum_v2di p) {
  typedef unsigned int __attribute__ ((vector_size (16))) v4si;
  return (_mum_v2di) (vec_mule ((v4si) v, (v4si) p) + vec_mulo ((v4si) v, (v4si) p));
}
static _MUM_INLINE void _mum_update_block (_mum_block_t *s, const _mum_block_t *v,
                                           const _mum_block_t *p) {
  const _mum_block_t *v2 = &v[1], *p2 = &p[1];
  s->v[0] ^= _mum_val (v->v[0] ^ p->v[0], v2->v[0] ^ p2->v[0]);
  s->v[1] ^= _mum_val (v->v[1] ^ p->v[1], v2->v[1] ^ p2->v[1]);
}
static _MUM_INLINE void _mum_factor_block (_mum_block_t *s, const _mum_block_t *p) {
  s->v[0] = _mum_val (s->v[0], p->v[0]);
  s->v[1] = _mum_val (s->v[1], p->v[1]);
}
static _MUM_INLINE void _mum_zero_block (_mum_block_t *b) { *b = (_mum_block_t) {0, 0, 0, 0}; }
static _MUM_INLINE uint64_t _mum_fold_block (_mum_block_t *b) {
  return b->v[0][0] ^ b->v[0][1] ^ b->v[1][0] ^ b->v[1][1];
}
#else
typedef struct {
  uint64_t v[4];
} _mum_block_t;
static _MUM_INLINE uint64_t _mum_val (uint64_t v, uint64_t p) {
  uint64_t lv = (uint32_t) v, lp = (uint32_t) p, hv = v >> 32, hp = p >> 32;
  return lv * lp + hv * hp;
}
static _MUM_INLINE void _mum_update_block (_mum_block_t *s, const _mum_block_t *v,
                                           const _mum_block_t *p) {
  const _mum_block_t *v2 = &v[1], *p2 = &p[1];
  s->v[0] ^= _mum_val (_mum_le (v->v[0]) ^ p->v[0], _mum_le (v2->v[0]) ^ p2->v[0]);
  s->v[1] ^= _mum_val (_mum_le (v->v[1]) ^ p->v[1], _mum_le (v2->v[1]) ^ p2->v[1]);
  s->v[2] ^= _mum_val (_mum_le (v->v[2]) ^ p->v[2], _mum_le (v2->v[2]) ^ p2->v[2]);
  s->v[3] ^= _mum_val (_mum_le (v->v[3]) ^ p->v[3], _mum_le (v2->v[3]) ^ p2->v[3]);
}
static _MUM_INLINE void _mum_factor_block (_mum_block_t *s, const _mum_block_t *p) {
  s->v[0] = _mum_val (s->v[0], p->v[0]);
  s->v[1] = _mum_val (s->v[1], p->v[1]);
  s->v[2] = _mum_val (s->v[2], p->v[2]);
  s->v[3] = _mum_val (s->v[3], p->v[3]);
}
static _MUM_INLINE void _mum_zero_block (_mum_block_t *b) { *b = (_mum_block_t) {0, 0, 0, 0}; }
static _MUM_INLINE uint64_t _mum_fold_block (_mum_block_t *b) {
  return b->v[0] ^ b->v[1] ^ b->v[2] ^ b->v[3];
}
#endif

#define _MUM_MIN_BLOCK_ROUNDS 2
#define _MUM_UNROLL_FACTOR 8

static _MUM_INLINE uint64_t _MUM_OPTIMIZE ("unroll-loops")
  _mum_hash_aligned (uint64_t start, const void *key, size_t len) {
  uint64_t result = start;
  const unsigned char *str = (const unsigned char *) key;
  uint64_t u64;
  size_t i;
  size_t n;

  result = _mum (result, _mum_block_start_prime);
  if (len >= _MUM_MIN_BLOCK_ROUNDS * _MUM_UNROLL_BLOCK_FACTOR * sizeof (_mum_block_t)) {
    _mum_block_t state;
    _mum_zero_block (&state);
    do {
      for (i = 0; i < _MUM_UNROLL_BLOCK_FACTOR; i += 2)
        _mum_update_block (&state, &((_mum_block_t *) str)[i], &((_mum_block_t *) _mum_primes)[i]);
      len -= _MUM_UNROLL_BLOCK_FACTOR * sizeof (_mum_block_t);
      str += _MUM_UNROLL_BLOCK_FACTOR * sizeof (_mum_block_t);
      /* We will use the same prime numbers on the next iterations -- randomize the state. */
      _mum_factor_block (&state, (_mum_block_t *) _mum_unroll_primes);
    } while (len >= _MUM_UNROLL_BLOCK_FACTOR * sizeof (_mum_block_t));
    result ^= _mum_fold_block (&state);
  }
  while (len >= _MUM_UNROLL_FACTOR * sizeof (uint64_t)) {
    for (i = 0; i < _MUM_UNROLL_FACTOR; i += 2)
      result ^= _mum (_mum_le (((uint64_t *) str)[i]) ^ _mum_primes[i],
                      _mum_le (((uint64_t *) str)[i + 1]) ^ _mum_primes[i + 1]);
    len -= _MUM_UNROLL_FACTOR * sizeof (uint64_t);
    str += _MUM_UNROLL_FACTOR * sizeof (uint64_t);
    /* We will use the same prime numbers on the next iterations -- randomize the state. */
    result = _mum (result, _mum_unroll_primes[0]);
  }
  n = len / sizeof (uint64_t);
  for (i = 0; i < n; i++)
    result ^= _mum (_mum_le (((uint64_t *) str)[i]) + _mum_primes[i], _mum_primes[i]);
  len -= n * sizeof (uint64_t);
  str += n * sizeof (uint64_t);
  switch (len) {
  case 7:
    u64 = _mum_primes[0] + _mum_le32 (*(uint32_t *) str);
    u64 += (_mum_le16 (*(uint16_t *) (str + 4)) << 32) + ((uint64_t) str[6] << 48);
    return result ^ _mum (u64, _mum_tail_prime);
  case 6:
    u64 = _mum_primes[1] + _mum_le32 (*(uint32_t *) str);
    u64 += _mum_le16 (*(uint16_t *) (str + 4)) << 32;
    return result ^ _mum (u64, _mum_tail_prime);
  case 5:
    u64 = _mum_primes[2] + _mum_le32 (*(uint32_t *) str);
    u64 += (uint64_t) str[4] << 32;
    return result ^ _mum (u64, _mum_tail_prime);
  case 4:
    u64 = _mum_primes[3] + _mum_le32 (*(uint32_t *) str);
    return result ^ _mum (u64, _mum_tail_prime);
  case 3:
    u64 = _mum_primes[4] + _mum_le16 (*(uint16_t *) str);
    u64 += (uint64_t) str[2] << 16;
    return result ^ _mum (u64, _mum_tail_prime);
  case 2:
    u64 = _mum_primes[5] + _mum_le16 (*(uint16_t *) str);
    return result ^ _mum (u64, _mum_tail_prime);
  case 1: u64 = _mum_primes[6] + str[0]; return result ^ _mum (u64, _mum_tail_prime);
  }
  return result;
}

/* Final randomization of H. */
static _MUM_INLINE uint64_t _mum_final (uint64_t h) {
  h = _mum (h, h);
  return h;
}

#ifndef _MUM_UNALIGNED_ACCESS
#if defined(__x86_64__) || defined(__i386__) || defined(__PPC64__) || defined(__s390__) \
  || defined(__m32c__) || defined(cris) || defined(__CR16__) || defined(__vax__)        \
  || defined(__m68k__) || defined(__aarch64__) || defined(_M_AMD64) || defined(_M_IX86)
#define _MUM_UNALIGNED_ACCESS 1
#else
#define _MUM_UNALIGNED_ACCESS 0
#endif
#endif

/* When we need an aligned access to data being hashed we move part of the unaligned data to an
   aligned block of given size and then process it, repeating processing the data by the block. */
#ifndef _MUM_BLOCK_LEN
#define _MUM_BLOCK_LEN 1024
#endif

#if _MUM_BLOCK_LEN < 8
#error "too small block length"
#endif

static _MUM_INLINE uint64_t
#if defined(__x86_64__)
_MUM_TARGET ("inline-all-stringops")
#endif
  _mum_hash_default (const void *key, size_t len, uint64_t seed) {
  uint64_t result;
  const unsigned char *str = (const unsigned char *) key;
  size_t block_len;
  uint64_t buf[_MUM_BLOCK_LEN / sizeof (uint64_t)];

  result = seed + len;
  if (((size_t) str & 0x7) == 0)
    result = _mum_hash_aligned (result, key, len);
  else {
    while (len != 0) {
      block_len = len < _MUM_BLOCK_LEN ? len : _MUM_BLOCK_LEN;
      memmove (buf, str, block_len);
      result = _mum_hash_aligned (result, buf, block_len);
      len -= block_len;
      str += block_len;
    }
  }
  return _mum_final (result);
}

static _MUM_INLINE uint64_t _mum_next_factor (void) {
  uint64_t start = 0;
  int i;

  for (i = 0; i < 8; i++) start = (start << 8) | rand () % 256;
  return start;
}

/* ++++++++++++++++++++++++++ Interface functions: +++++++++++++++++++  */

/* Set random multiplicators depending on SEED. */
static _MUM_INLINE void mum_hash_randomize (uint64_t seed) {
  size_t i;

  srand (seed);
  _mum_hash_step_prime = _mum_next_factor ();
  _mum_key_step_prime = _mum_next_factor ();
  _mum_finish_prime1 = _mum_next_factor ();
  _mum_finish_prime2 = _mum_next_factor ();
  _mum_block_start_prime = _mum_next_factor ();
  _mum_tail_prime = _mum_next_factor ();
  for (i = 0; i < sizeof (_mum_unroll_primes) / sizeof (uint64_t); i++)
    _mum_unroll_primes[i] = _mum_next_factor ();
  for (i = 0; i < sizeof (_mum_primes) / sizeof (uint64_t); i++)
    _mum_primes[i] = _mum_next_factor ();
}

/* Start hashing data with SEED. Return the state. */
static _MUM_INLINE uint64_t mum_hash_init (uint64_t seed) { return seed; }

/* Process data KEY with the state H and return the updated state. */
static _MUM_INLINE uint64_t mum_hash_step (uint64_t h, uint64_t key) {
  return _mum (h, _mum_hash_step_prime) ^ _mum (key, _mum_key_step_prime);
}

/* Return the result of hashing using the current state H. */
static _MUM_INLINE uint64_t mum_hash_finish (uint64_t h) { return _mum_final (h); }

/* Fast hashing of KEY with SEED. The hash is always the same for the same key on any target. */
static _MUM_INLINE size_t mum_hash64 (uint64_t key, uint64_t seed) {
  return mum_hash_finish (mum_hash_step (mum_hash_init (seed), key));
}

/* Hash data KEY of length LEN and SEED. The hash depends on the target endianess and the unroll
   factor. */
static _MUM_INLINE uint64_t mum_hash (const void *key, size_t len, uint64_t seed) {
#if _MUM_UNALIGNED_ACCESS
  return _mum_final (_mum_hash_aligned (seed + len, key, len));
#else
  return _mum_hash_default (key, len, seed);
#endif
}

#endif
