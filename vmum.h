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

#if defined(__GNUC__)
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
static uint64_t _mum_hash_step_prime = 0x7fc65e8a22c2f74bull;
static uint64_t _mum_key_step_prime = 0x9b307d68270e94e5ull;
static uint64_t _mum_block_start_prime = 0x6608b54dafbc797cull;
static uint64_t _mum_tail_prime = 0xc6f58747253b0e84ull;
static uint64_t _mum_finish_prime1 = 0xbc4bb29ce739b5d9ull;
static uint64_t _mum_finish_prime2 = 0x7007946aa4fdb987ull;

static uint64_t _mum_unroll_primes[] = {
  0x191fb5fc4a9bf2deull,
  0xd9a09a0a2c4eb3ebull,
  0x90f15ee96deb1eecull,
  0x1a970df0a79d09baull,
};

static uint64_t _mum_primes[] = {
  0x0c3ec9639d3a203full, 0x898368b5fb060422ull, 0xfe3c08767c1e5068ull, 0x4535ac3cb182d3caull,
  0xba6ba8dcc8a2d978ull, 0x9f1221df37b27ca1ull, 0x57b1b40817cde05eull, 0xadb5c6075e5dd1c3ull,
  0x0f91abf611686bc3ull, 0x72fc850fbe9023f4ull, 0x4922ec730400d7e1ull, 0x7452d927d9970eb2ull,
  0x607ad8c0dbe89ff9ull, 0x463bf8a99a222448ull, 0x6a0c8e5c2171d499ull, 0x88a9af1a70175e0full,

  0x50c07b9351011316ull, 0xce26a1f4e66e3d1full, 0x396d79c90c86b1edull, 0x3739b72e11fc4684ull,
  0x7fe4adc8400af08eull, 0xfa2e47475b11a3ceull, 0xd519635f1d7b9242ull, 0x866d04bd866130fcull,
  0x6132e913fd0ae2c9ull, 0xaeacc8d99a02880dull, 0xf196fbab2ef62dbbull, 0x62a6a4ae6d3f5fddull,
  0x3147dbedb6618cf4ull, 0x18dc2a4e6e46b7f5ull, 0x98ce62d3e346f804ull, 0x3e507b6626b9c9c6ull,

  0x08d2d6aba49b53a7ull, 0x64f897cf54cc0984ull, 0xe6568e3081d4dc31ull, 0x936eff54d90e79bdull,
  0x45ecee4cb55e5529ull, 0xe7729fe4b83bd1f0ull, 0x8ced03855b3a62aeull, 0xc29ee17b601d5ea6ull,
  0xfadd87cd819b8b23ull, 0x5ff6b6787a02da34ull, 0xcc0f997728759ba9ull, 0x06a51956916c45ffull,
  0xd31923282290a41full, 0x9e2fcda93feb36aaull, 0xfdc11827f9bb4b63ull, 0xe11b2c099cf17325ull,

  0xd4efd1919316bb4eull, 0x7749918c7756d897ull, 0x32eff42dc1c4f41eull, 0x4171f5e422bdf4faull,
  0xdd5c7f50a7977e65ull, 0xc52dc937487cc197ull, 0xbff445a1c9b19184ull, 0xb2fd8206a0325cb9ull,
  0xbf32bb70e354a83cull, 0x587313d37681ffc2ull, 0x687c0b98ef7ad731ull, 0x2c09a37608248f89ull,
  0x20c8d9b1b6e28faaull, 0x747008703cbc9483ull, 0x0bdcacce877cb231ull, 0xf740c0ac1dc79a0cull,

  0x8e843baef228089dull, 0xc379d4c3b6e28c1bull, 0xb5d44eee257f1206ull, 0xb5dfee44ef6b05adull,
  0xaab0edc0f566f359ull, 0x21883ede514d45ddull, 0xcdaeea0482c73a46ull, 0xb3751631e5645c4full,
  0xd22ac81254aad1acull, 0x59c0f8f5c93121dfull, 0x3b343a848e3097c3ull, 0x87c65a3a866ed9edull,
  0xf112c880c63ddafdull, 0xff98b9575bfce057ull, 0xd6e8f6ad6f4da8aeull, 0xba0379ba20645a51ull,
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

#define _MUM_UNROLL_BLOCK_FACTOR 16
#define _MUM_UNROLL_FACTOR 16

static _MUM_INLINE uint64_t _MUM_OPTIMIZE ("unroll-loops")
  _mum_hash_aligned (uint64_t start, const void *key, size_t len) {
  uint64_t result = start;
  const unsigned char *str = (const unsigned char *) key;
  uint64_t u64, w;
  size_t i, j;
  size_t n;

  result = _mum (result, _mum_block_start_prime);
  i = 0;
  if (len >= _MUM_UNROLL_BLOCK_FACTOR * sizeof (_mum_block_t)) {
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
    result += _mum_fold_block (&state);
  }
  static_assert (_MUM_UNROLL_BLOCK_FACTOR <= sizeof (_mum_primes) / sizeof (uint64_t));
  static_assert (_MUM_UNROLL_BLOCK_FACTOR * sizeof (_mum_block_t) / sizeof (uint64_t) + 7
                 <= sizeof (_mum_primes) / sizeof (uint64_t));
  i = 0;
  while (len >= _MUM_UNROLL_FACTOR * sizeof (uint64_t)) {
    for (j = 0; j < _MUM_UNROLL_FACTOR; j += 2, i += 2)
      result ^= _mum (_mum_le (((uint64_t *) str)[i]) ^ _mum_primes[i],
                      _mum_le (((uint64_t *) str)[i + 1]) ^ _mum_primes[i + 1]);
    len -= _MUM_UNROLL_FACTOR * sizeof (uint64_t);
  }
  n = len / sizeof (uint64_t) & ~(size_t) 1;
  for (j = 0; j < n; j += 2, i += 2)
    result ^= _mum (_mum_le (((uint64_t *) str)[i]) + _mum_primes[i],
                    _mum_le (((uint64_t *) str)[i + 1]) + _mum_primes[i + 1]);
  len -= n * sizeof (uint64_t);
  str += i * sizeof (uint64_t);
  switch (len) {
  case 15:
    w = _mum_le (*(uint64_t *) str) + _mum_primes[i];
    u64 = _mum_primes[i + 1] + _mum_le32 (*(uint32_t *) (str + 8));
    u64 += (_mum_le16 (*(uint16_t *) (str + 12)) << 32) + ((uint64_t) str[14] << 48);
    return result ^ _mum (u64 ^ _mum_tail_prime, w);
  case 14:
    w = _mum_le (*(uint64_t *) str) + _mum_primes[i];
    u64 = _mum_primes[i + 2] + _mum_le32 (*(uint32_t *) (str + 8));
    u64 += _mum_le16 (*(uint16_t *) (str + 12)) << 32;
    return result ^ _mum (u64 ^ _mum_tail_prime, w);
  case 13:
    w = _mum_le (*(uint64_t *) str) + _mum_primes[i];
    u64 = _mum_primes[i + 3] + _mum_le32 (*(uint32_t *) (str + 8));
    u64 += (uint64_t) str[12] << 32;
    return result ^ _mum (u64 ^ _mum_tail_prime, w);
  case 12:
    w = _mum_le (*(uint64_t *) str) + _mum_primes[i];
    u64 = _mum_primes[i + 4] + _mum_le32 (*(uint32_t *) (str + 8));
    return result ^ _mum (u64 ^ _mum_tail_prime, w);
  case 11:
    w = _mum_le (*(uint64_t *) str) + _mum_primes[i];
    u64 = _mum_primes[i + 5] + _mum_le16 (*(uint16_t *) (str + 8));
    u64 += (uint64_t) str[10] << 16;
    return result ^ _mum (u64 ^ _mum_tail_prime, w);
  case 10:
    w = _mum_le (*(uint64_t *) str) + _mum_primes[i];
    u64 = _mum_primes[i + 6] + _mum_le16 (*(uint16_t *) (str + 8));
    return result ^ _mum (u64 ^ _mum_tail_prime, w);
  case 9:
    w = _mum_le (*(uint64_t *) str) + _mum_primes[i];
    u64 = _mum_primes[i + 7] + str[8];
    return result ^ _mum (u64 ^ _mum_tail_prime, w);
  case 8: return result ^ _mum (_mum_le (*(uint64_t *) str) + _mum_primes[i], _mum_primes[i]);
  case 7:
    u64 = _mum_primes[i + 1] + _mum_le32 (*(uint32_t *) str);
    u64 += (_mum_le16 (*(uint16_t *) (str + 4)) << 32) + ((uint64_t) str[6] << 48);
    return result ^ _mum (u64, _mum_tail_prime);
  case 6:
    u64 = _mum_primes[i + 2] + _mum_le32 (*(uint32_t *) str);
    u64 += _mum_le16 (*(uint16_t *) (str + 4)) << 32;
    return result ^ _mum (u64, _mum_tail_prime);
  case 5:
    u64 = _mum_primes[i + 3] + _mum_le32 (*(uint32_t *) str);
    u64 += (uint64_t) str[4] << 32;
    return result ^ _mum (u64, _mum_tail_prime);
  case 4:
    u64 = _mum_primes[i + 4] + _mum_le32 (*(uint32_t *) str);
    return result ^ _mum (u64, _mum_tail_prime);
  case 3:
    u64 = _mum_primes[i + 5] + _mum_le16 (*(uint16_t *) str);
    u64 += (uint64_t) str[2] << 16;
    return result ^ _mum (u64, _mum_tail_prime);
  case 2:
    u64 = _mum_primes[i + 6] + _mum_le16 (*(uint16_t *) str);
    return result ^ _mum (u64, _mum_tail_prime);
  case 1: u64 = _mum_primes[i + 7] + str[0]; return result ^ _mum (u64, _mum_tail_prime);
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
  return mum_hash_finish (mum_hash_step (mum_hash_init (seed + 8), key));
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
