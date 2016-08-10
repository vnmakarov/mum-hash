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

/* This file implements MUM512 (MUltiply and Mix) hash function.  It
   is a candidate for a crypto-level hash function and keyed hash
   function.

   The key size is 256-bit.  The size of the state, block, and output
   (digest) is 512-bit.

   The input data by 128x128-bit multiplication and mixing hi- and
   low-parts of the multiplication result by using an addition and
   then mix it into the current state.  We use prime numbers randomly
   generated with the equal probability of their bit values for the
   multiplication.  When all primes are used once, the state is
   randomized and the same prime numbers are used again for data
   randomization.

   The MUM512 hashing passes all SMHasher tests and NIST Statistical
    Test Suite for Random and Pseudorandom Number Generators for
    Cryptographic Applications (v. 2.2.1).  MUM512 is also faster SHA-2
    (512) and SHA-3 (512).  */

#ifndef __MC_HASH__
#define __MC_HASH__

#include <stddef.h>
#include <string.h>
#include <limits.h>

#if defined(_MSC_VER)
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;
#else
#include <stdint.h>
#endif

#if defined(_MSC_VER)
#define _mc_bswap_64(x) _byteswap_uint64_t (x)
#elif defined(__APPLE__)
#include <libkern/OSByteOrder.h>
#define _mc_bswap_64(x) OSSwapInt64 (x)
#elif defined(__GNUC__)
#define _mc_bswap64(x) __builtin_bswap64 (x)
#else
#include <byteswap.h>
#define _mc_bswap64(x) bswap64 (x)
#endif

#ifndef _MC_USE_INT128
/* In GCC if uint128_t is defined if HOST_BITS_PER_WIDE_INT >= 64.
   HOST_WIDE_INT is long if HOST_BITS_PER_LONG > HOST_BITS_PER_IN,
   otherwise int. */
#if defined(__GNUC__) && UINT_MAX != ULONG_MAX
#define _MC_USE_INT128 1
#else
#define _MC_USE_INT128 0
#endif
#endif

#ifdef __GNUC__
#define _MC_ATTRIBUTE_UNUSED  __attribute__((unused))
#define _MC_OPTIMIZE(opts) __attribute__((__optimize__ (opts)))
#define _MC_TARGET(opts) __attribute__((__target__ (opts)))
#else
#define _MC_ATTRIBUTE_UNUSED
#define _MC_OPTIMIZE(opts)
#define _MC_TARGET(opts)
#endif


#if defined(__GNUC__) && ((__GNUC__ == 4) &&  (__GNUC_MINOR__ >= 9) || (__GNUC__ > 4))
#define _MC_FRESH_GCC
#endif

#if _MC_USE_INT128
typedef __uint128_t _mc_ti;
#else
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
typedef struct {uint64_t lo, hi;} _mc_ti;
#else
typedef struct {uint64_t hi, lo;} _mc_ti;
#endif
#endif

#define inline

#if _MC_USE_INT128
static inline uint64_t _mc_lo64 (_mc_ti a) {return a;}
static inline uint64_t _mc_hi64 (_mc_ti a) {return a >> 64;}
static inline _mc_ti _mc_lo2hi (_mc_ti a) {return a << 64;}
static inline _mc_ti _mc_hi2lo (_mc_ti a) {return a >> 64;}
static inline _mc_ti _mc_add (_mc_ti a, _mc_ti b) {return a + b;}
static inline _mc_ti _mc_xor (_mc_ti a, _mc_ti b) {return a ^ b;}
static inline uint32_t _mc_lt (_mc_ti a, _mc_ti b) { return a < b;}
static inline _mc_ti _mc_const (uint64_t hi, uint64_t lo) {
  return (_mc_ti) hi << 64 | lo;
}
static inline _mc_ti _mc_rotr (_mc_ti a, int sh) {
  return (a >> sh) | (a << (128 - sh));
}

static inline _mc_ti _mc_mul64 (uint64_t a, uint64_t b) {
#if defined(__aarch64__)
  /* AARCH64 needs 2 insns to calculate 128-bit result of the
     multiplication.  If we use a generic code we actually call a
     function doing 128x128->128 bit multiplication.  The function is
     very slow.  */
  uint64_t lo = a * b, hi;

  asm ("umulh %0, %1, %2" : "=r" (hi) : "r" (a), "r" (b));
  return (_mc_ti) hi << 64 | lo;
#else
  return (_mc_ti) a * (_mc_ti) b;
#endif
}

static inline _mc_ti _mc_swap (_mc_ti v) {
  return _mc_const (_mc_bswap64 (_mc_lo64 (v)), _mc_bswap64 (_mc_hi64 (v)));
}

#else /* #if _MC_USE_INT128 */

static inline uint64_t _mc_lo64 (_mc_ti a) {return a.lo;}
static inline uint64_t _mc_hi64 (_mc_ti a) {return a.hi;}
static inline _mc_ti _mc_lo2hi (_mc_ti a) {a.hi = a.lo; a.lo = 0; return a;}
static inline _mc_ti _mc_hi2lo (_mc_ti a) {a.lo = a.hi; a.hi = 0; return a;}
static inline _mc_ti _mc_const (uint64_t hi, uint64_t lo) {
  _mc_ti r;
  r.hi = hi; r.lo = lo; return r;
}
static inline _mc_ti _mc_add (_mc_ti a, _mc_ti b) {
  a.lo += b.lo, a.hi += b.hi + (a.lo < b.lo); return a;
}
static inline uint32_t _mc_lt (_mc_ti a, _mc_ti b) {
  return a.hi < b.hi || (a.hi == b.hi && a.lo < b.lo);
}
static inline _mc_ti _mc_xor (_mc_ti a, _mc_ti b) {
  a.lo ^= b.lo; a.hi ^= b.hi; return a;
}
static inline _mc_ti _mc_rotr (_mc_ti a, int sh) {
  _mc_ti res;

  if (sh <= 64) {
    res.lo = a.lo >> sh | a.hi << (64 - sh);
    res.hi = a.hi >> sh | a.lo << (64 - sh);
  } else {
    sh -= 64;
    res.lo = a.hi >> sh | a.lo << (64 - sh);
    res.hi = a.lo >> sh | a.hi << (64 - sh);
  }
  return res;
}

static inline _mc_ti _mc_mul64 (uint64_t a, uint64_t b) {
  /* Implementation of 64x64->128-bit multiplication by four 32x32->64
     bit multiplication.  */
  uint64_t ha = a >> 32, hb = b >> 32;
  uint64_t la = (uint32_t) a, lb = (uint32_t) b;
  uint64_t rhi =  ha * hb;
  uint64_t rm_0 = ha * lb;
  uint64_t rm_1 = hb * la;
  uint64_t rlo =  la * lb;
  uint64_t lo, carry;
  _mc_ti res;
  
  lo = rlo + (rm_0 << 32);
  carry = lo < rlo;
  res.lo = lo + (rm_1 << 32);
  carry += res.lo < lo;
  res.hi = rhi + (rm_0 >> 32) + (rm_1 >> 32) + carry;
  return res;
}

static inline _mc_ti _mc_swap (_mc_ti v) {
  _mc_ti r;
  r.lo = _mc_bswap64 (v.hi); r.hi = _mc_bswap64 (v.lo); return r;
}

#endif /* #if _MC_USE_INT128 */

static inline _mc_ti _mc_mum (_mc_ti a, _mc_ti b) {
  /* Implementation of 128x128->256-bit multiplication by four
     64x64->128 bit multiplication.  */
  uint64_t ha = _mc_hi64 (a), hb = _mc_hi64 (b);
  uint64_t la = _mc_lo64 (a), lb = _mc_lo64 (b);
  _mc_ti rhi =  _mc_mul64 (ha, hb);
  _mc_ti rm_0 = _mc_mul64 (ha, lb);
  _mc_ti rm_1 = _mc_mul64 (hb, la);
  _mc_ti rlo =  _mc_mul64 (la, lb);
  _mc_ti hi, lo, t;
  uint32_t carry;
  
  t = _mc_add (_mc_lo2hi (rm_0), rlo); carry = _mc_lt (t, rlo);
  lo = _mc_add (_mc_lo2hi (rm_1), t); carry += _mc_lt (lo, t);
  hi = _mc_add (_mc_add (_mc_add (rhi, _mc_hi2lo (rm_0)), _mc_hi2lo (rm_1)),
		_mc_const (0, carry));
  return _mc_add (hi, lo);
}

static inline _mc_ti _mc_2le (_mc_ti v) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  return v;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
  return _mc_swap (v);
#else
#error "Unknown endianess"
#endif
}

static inline _mc_ti _mc_get (const uint64_t a[2]) {return _mc_const (a[0], a[1]);}

/* Here are different primes randomly generated with the equal
   probability of their bit values.  They are used to randomize input
   values.  */
static const uint64_t _mc_block_start_primes[4][2] = {
  {0Xcdc037073edf8fd4ULL, 0X9e2a1f46e21930f3ULL},
  {0X128281cfcc981531ULL, 0Xae5a66b7a27ba90fULL},
  {0X676f0ed409f0019aULL, 0X4bd8035682ec0095ULL},
  {0X6fae98c0d49ed3ffULL, 0X5268b2b43337690bULL},
};
static const uint64_t _mc_unroll_primes[4][2] = {
  {0X5cc2a100bf7b7c05ULL, 0Xd5ce2499d1844187ULL},
  {0X2fdc8fe047488edaULL, 0Xd4c8739663e296e9ULL},
  {0Xd36d0a4431717b61ULL, 0X10cb847ca52a3e25ULL},
  {0Xf011560af9264d79ULL, 0Xc9d48bb40afed5d3ULL},
};
static const uint64_t _mc_tail_primes[4][2] = {
  {0X270957969935d145ULL, 0Xcb75144659dbd40dULL},
  {0X12db17d4be72d52cULL, 0X3b0cbf3954138dfdULL},
  {0X5022d90d6ccc2372ULL, 0X889add17a2618f99ULL},
  {0Xe282c1104925025bULL, 0Xfdf6cd06e47a8c85ULL},
};
static const uint64_t _mc_primes [16][2] = {
  {0Xfc35b01709510063ULL, 0Xec2fdbeaade88605ULL},
  {0Xd2b356ae579f47a6ULL, 0X65862b62bdf5ef4dULL},
  {0X288eea216831e6a7ULL, 0Xa79128dc46b0173fULL},
  {0Xb737b72062479c04ULL, 0X5f6c924506e59f99ULL},
  {0X92a4d7c15a574735ULL, 0X5d1d4782c102eadfULL},
  {0Xf9c1acb5076cf7c7ULL, 0Xb9ecfe43117ae249ULL},
  {0X10ee204d93f26f50ULL, 0Xb70b8f530f15aa6bULL},
  {0X181da242743a22dcULL, 0Xff14e8e58f6e1721ULL},
  {0X3e431aaaac0f869eULL, 0X9feebb1f9c386fe3ULL},
  {0X817292a324fa17b0ULL, 0X2c37dcc844bc3d37ULL},
  {0Xcdf95a3e7c6fcb0dULL, 0Xcd2aa0073fbeb7b9ULL},
  {0Xfc67b9a71847832aULL, 0X640c8f5ed3777417ULL},
  {0X6cb4dcd475f71ae5ULL, 0X51f0328a65fc65bbULL},
  {0Xdcab58b43c79dee4ULL, 0X745bd64098a26981ULL},
  {0X1a8f800285c50091ULL, 0X2afffc9213aa8a5fULL},
    {0X532e0082221804dbULL, 0X20c941f9b512df11ULL},
};

static const uint64_t _mc_finish_primes [64][2] = {
  {0X1e0055fb724474e8ULL, 0Xff29de49c378e459ULL},
  {0X83e8be07b5531ec3ULL, 0X4266dedbf60f5523ULL},
  {0X25d03dacd0a5c8e5ULL, 0Xdbbcac5c7cda883fULL},
  {0X9984f048ee48645eULL, 0X12c928afdf625a47ULL},
  {0X8100dab7b00755d3ULL, 0X12a2f064b6def9fULL},
  {0X2a4936c9ef78297cULL, 0Xa9c902ba35037359ULL},
  {0X833ac9a368619006ULL, 0X16fc020d9f40323ULL},
  {0X56f880bf9ae924acULL, 0X87262cbecf623dc5ULL},
  {0Xf92080d63124cc29ULL, 0X94da893165805cfbULL},
  {0X852b1ea7d206adfaULL, 0Xded5f478baf4f65bULL},
  {0X6c090f99eb7da834ULL, 0X8619a705f355b9d1ULL},
  {0Xc24556cc93aff883ULL, 0X5df87f6b5076a2cfULL},
  {0X24fa93e0575eb1f6ULL, 0X3d7063f15281ff03ULL},
  {0X47ee8f94f08cb813ULL, 0X8128602ec25d6a19ULL},
  {0X3625bd5ebdd10f45ULL, 0Xe43054d209066c7bULL},
  {0X120db6d8b3382a5ULL, 0X314fb5d22fced3f9ULL},
  {0X78276dc95fb85e7dULL, 0X57bfc5460f873c9dULL},
  {0X96d099814184b5b0ULL, 0Xd89f836c35b9b0c9ULL},
  {0X6d79572ccf590f33ULL, 0Xab8030fdeffd48f5ULL},
  {0X19c5d2c51e451eddULL, 0Xc675c5e74f1c2c45ULL},
  {0X63b09e2290211695ULL, 0X494b134c5d324f3ULL},
  {0Xed2cda11f978bb7dULL, 0X827e3d4edf1475e1ULL},
  {0X9a5e00a94de13a14ULL, 0X55667b0f406987abULL},
  {0Xcbcabf81b1338e26ULL, 0X2dd53df2ae66d191ULL},
  {0X13c880d6b45a6c9cULL, 0Xe0a28ed2c8049f5fULL},
  {0X55279312bf2ed6feULL, 0Xc6c19c752e4bcb2dULL},
  {0Xf138de98d83585caULL, 0Xc90ad41770782dd1ULL},
  {0X81f28fded1813f57ULL, 0X94900f7877c93a37ULL},
  {0X473457080f9406d2ULL, 0Xc5989b97426594afULL},
  {0Xe5799a1363835a22ULL, 0Xce4ca5f532d3980bULL},
  {0Xfd7b3c358f596498ULL, 0X70fd7270db8949b5ULL},
  {0Xfd6a0c5c43a6c653ULL, 0Xeb3aa98686ffa645ULL},
  {0X3df53625c3afc40aULL, 0Xdf3bffec06a23541ULL},
  {0Xef491e13aa526595ULL, 0Xf6688ada28d7e81fULL},
  {0X8419c0511b7e4242ULL, 0X7ba8ddb66b20a5d7ULL},
  {0X11b3da4f4f90fb72ULL, 0X71de3984d3c23847ULL},
  {0X6616fb20c0231403ULL, 0Xfa06db687a1bc2ffULL},
  {0X6e661bbd3bd6e950ULL, 0X6e45cbd289a77c23ULL},
  {0X599d1220a03da949ULL, 0Xff568091f9c59349ULL},
  {0X4464d21d20da203bULL, 0X352006ad6fb6de5ULL},
  {0X6413c7b09fc3dcc8ULL, 0X55fc3da745579871ULL},
  {0X160111a644959396ULL, 0Xd12e059acb048b01ULL},
  {0Xc66e719ac9a96714ULL, 0X832bb4f760cb0c0dULL},
  {0X92b777a80cfd2a59ULL, 0X883dc6537f8d2a6bULL},
  {0Xf40a3463bd3220d7ULL, 0Xadd624b1439d6507ULL},
  {0X2d11c5c5e6c3bf75ULL, 0Xfc027148e7ed66c9ULL},
  {0Xe9437c1b3494cb46ULL, 0Xacd35405f3d2f4b7ULL},
  {0X838f551f688bd046ULL, 0X3e87440f1a4642e9ULL},
  {0Xce20a8f09e985b88ULL, 0Xa98931ac95adde85ULL},
  {0Xe3580702cf5a1c80ULL, 0X8e76e05a566733cdULL},
  {0X15ff369908c7d67dULL, 0X11bc44ed911d2aa7ULL},
  {0Xab2694c16fe59e02ULL, 0Xc1f5ba02d82d5e85ULL},
  {0X7c59a06636a7edddULL, 0X684643ed7e49b619ULL},
  {0X40b6270f7f9fa251ULL, 0X9d8b29eef5b17a8dULL},
  {0X4044ad0f26d7e774ULL, 0Xf3d1fd1ab3ca5037ULL},
  {0X5ec3d199e0063437ULL, 0X12fb529ef8cc3a73ULL},
  {0X6b5f89cb3017e3e2ULL, 0Xfdb9b50566a2626dULL},
  {0X76ad12644718202cULL, 0X967fbb370d71862fULL},
  {0Xb3b7cb5a21f3eaeaULL, 0X18f9fe203ad955ffULL},
  {0Xfd28dc38cbec1a6cULL, 0X8e1920c5a0132445ULL},
  {0X23f9cf9b2158b106ULL, 0Xd9a70361d96b31f9ULL},
  {0X224fa274d6ea69f7ULL, 0X6316130d5d925dc3ULL},
  {0X14b09aa58cb268d5ULL, 0Xf34d372c6f503cc9ULL},
  {0X6289be577a4ae0a6ULL, 0X3ba5692b1eafeb15ULL},
};

/* Macro defining how many times the most nested loop in
   _mc_hash_aligned will be unrolled by the compiler (although it can
   make an own decision:).  Use only a constant here to help a
   compiler to unroll a major loop.  Don't change the value.  Changing
   it changes the hash.  If you increase it you need more numberss in
   _mc_primes.  */
#define _MC_UNROLL_FACTOR 4

static inline void _MC_OPTIMIZE ("unroll-all-loops")
_mc_hash_aligned (_mc_ti state[4], const void *data, size_t len) {
  _mc_ti result[4];
  const unsigned char *str = (const unsigned char *) data;
  union {_mc_ti i; unsigned char s[sizeof (_mc_ti)];} p;
  int j;
  size_t i, n;
  
  for (i = 0; i < 4; i++)
    result[i] = _mc_mum (state[i], _mc_get (_mc_block_start_primes[i]));
  while  (len > _MC_UNROLL_FACTOR * sizeof (_mc_ti)) {
    for (i = 0; i < _MC_UNROLL_FACTOR; i++)
      for (j = 0; j < 4; j++)
	result[j] = _mc_xor (result[j], _mc_mum (_mc_2le (((_mc_ti *) str)[i]),
						 _mc_get (_mc_primes[4 * i + j])));
    /* We might use the same prime numbers on the next iterations --
       randomize the state.  */
    for (i = 0; i < 4; i++)
      result[i] = _mc_mum (result[i], _mc_get (_mc_unroll_primes[i]));
    len -= _MC_UNROLL_FACTOR * sizeof (_mc_ti);
    str += _MC_UNROLL_FACTOR * sizeof (_mc_ti);
  }
  n = len / sizeof (_mc_ti);
  for (i = 0; i < n; i++)
    for (j = 0; j < 4; j++)
      result[j] = _mc_xor (result[j], _mc_mum (_mc_2le (((_mc_ti *) str)[i]),
					       _mc_get (_mc_primes[4 * i + j])));
  len -= n * sizeof (_mc_ti); str += n * sizeof (_mc_ti);
  if (len) {
    p.i = _mc_const (0, 0); memcpy (p.s, str, len);
    for (i = 0; i < 4; i++)
      result[i] = _mc_xor (result[i],
			   _mc_mum (_mc_2le (((_mc_ti *) p.s)[0]),
				    _mc_get (_mc_tail_primes[i])));
  }
  for (i = 0; i < 4; i++)
    state[i] = _mc_xor (state[i], result[i]);
}

#ifndef MUM512_ROUNDS
#define MUM512_ROUNDS 8
#endif

#if MUM512_ROUNDS < 2
#error "too few final rounds"
#elif MUM512_ROUNDS > 16
#error "too many final rounds"
#endif

static inline void
_mc_permute (_mc_ti t[4]) {
  /* Row */
  t[0] = _mc_rotr (_mc_xor (t[0], t[1]), 17);
  t[1] = _mc_rotr (_mc_xor (t[1], t[0]), 33);
  t[3] = _mc_rotr (_mc_xor (t[3], t[2]), 49);
  t[2] = _mc_rotr (_mc_xor (t[2], t[3]), 65);
  /* Column */
  t[0] = _mc_rotr (_mc_xor (t[0], t[2]), 9);
  t[2] = _mc_rotr (_mc_xor (t[2], t[0]), 25);
  t[3] = _mc_rotr (_mc_xor (t[3], t[1]), 41);
  t[1] = _mc_rotr (_mc_xor (t[1], t[3]), 57);
  /* Diagonal */
  t[0] = _mc_rotr (_mc_xor (t[0], t[3]), 13);
  t[3] = _mc_rotr (_mc_xor (t[3], t[0]), 29);
  t[2] = _mc_rotr (_mc_xor (t[2], t[1]), 45);
  t[1] = _mc_rotr (_mc_xor (t[1], t[2]), 61);
}

static inline void
_mc_mix (_mc_ti state[4], uint64_t out[8]) {
  int i, j;
  _mc_ti t[4];
  
  for (i = 0; i < 4; i++)
    t[i] = state[i];
  for (i = 0; i < MUM512_ROUNDS; i++) {
    for (j = 0; j < 4; j++)
      t[j] = _mc_xor (t[j], _mc_mum (t[j], _mc_get (_mc_finish_primes[i * 4 + j])));
    _mc_permute (t);
  }
  for (i = 0; i < 4; i++) {
    t[i] = _mc_2le (t[i]);
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    out[2 * i] = _mc_lo64 (t[i]);
    out[2 * i + 1] = _mc_hi64 (t[i]);
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    out[2 * i] = _mc_hi64 (t[i]);
    out[2 * i + 1] = _mc_lo64 (t[i]);
#endif
  }
}

/* Random prime numbers for the initialization.  */
#define SEED_PRIME0 0x2e0bb864e9ea7df5ULL
#define SEED_PRIME1 0xcdb32970830fcaa1ULL
#define SEED_PRIME2 0xc42b5e2e6480b23bULL
#define SEED_PRIME3 0x746addee2093e7e1ULL
#define SEED_PRIME4 0xa9a7ae7ceff79f3fULL
#define SEED_PRIME5 0xaf47d47c99b1461bULL
#define SEED_PRIME6 0x7b51ec3d22f7096fULL

static inline void
_mc_init_state (_mc_ti state[4], const uint64_t seed[4], size_t len) {
  state[0] = _mc_const (seed[0] ^ SEED_PRIME0, seed[1] ^ SEED_PRIME1);
  state[1] = _mc_const (seed[2] ^ SEED_PRIME2, seed[3] ^ SEED_PRIME3);
  state[2] = _mc_const (len, SEED_PRIME4);
  state[3] = _mc_const (SEED_PRIME5, SEED_PRIME6);
}

#if defined(__x86_64__) && defined(_MC_FRESH_GCC)

/* We want to use AVX2 insn MULX instead of generic x86-64 MULQ where
   it is possible.  Although on modern Intel processors MULQ takes
   3-cycles vs. 4 for MULX, MULX permits more freedom in insn
   scheduling as it uses less fixed registers.  */
static inline void _MC_TARGET("arch=haswell")
_mc_hash_avx2 (const void * data, size_t len, const uint64_t seed[4], unsigned char *out) {
  _mc_ti state[4];
  
  _mc_init_state (state, seed, len);
  _mc_hash_aligned (state, data, len);
  _mc_mix (state, (uint64_t *) out);
}
#endif

#ifndef _MC_UNALIGNED_ACCESS
#if defined(__x86_64__) || defined(__i386__) || defined(__PPC64__) \
    || defined(__s390__) || defined(__m32c__) || defined(cris)     \
    || defined(__CR16__) || defined(__vax__) || defined(__m68k__)  \
    || defined(__aarch64__) || defined(_M_AMD64) || defined(_M_IX86)
#define _MC_UNALIGNED_ACCESS 1
#else
#define _MC_UNALIGNED_ACCESS 0
#endif
#endif

/* When we need an aligned access to data being hashed we move part of
   the unaligned data to an aligned block of given size and then
   process it, repeating processing the data by the block.  */
#ifndef _MC_BLOCK_LEN_POW2
#define _MC_BLOCK_LEN_POW2 10
#endif

#if _MC_BLOCK_LEN_POW2 < 4
/* We should process by full 128-bit unless it is a tail.  */
#error "too small block length"
#endif

static inline void
#if defined(__x86_64__)
_MC_TARGET("inline-all-stringops")
#endif
_mc_hash_default (const void *data, size_t len, const uint64_t seed[4], unsigned char *out) {
  _mc_ti state[4];
  const unsigned char *str = (const unsigned char *) data;
  size_t block_len;
  _mc_ti buf[_MC_BLOCK_LEN_POW2 / sizeof (_mc_ti)];
  
  _mc_init_state (state, seed, len);
  if (_MC_UNALIGNED_ACCESS || ((size_t) str & 0x7) == 0)
    _mc_hash_aligned (state, data, len);
  else {
    while (len != 0) {
      block_len
	= len < (1 << _MC_BLOCK_LEN_POW2) ? len : (1 << _MC_BLOCK_LEN_POW2);
      memcpy (buf, str, block_len);
      _mc_hash_aligned (state, buf, block_len);
      len -= block_len;
      str += block_len;
    }
  }
  if (_MC_UNALIGNED_ACCESS || ((size_t) out & 0x7) == 0)
    _mc_mix (state, (uint64_t *) out);
  else {
    uint64_t ui64[8];
    
    _mc_mix (state, ui64);
    memmove (out, ui64, 64);
  }
}

/* ++++++++++++++++++++++++++ Interface functions: +++++++++++++++++++  */

/* Hash DATA of length LEN and SEED.  */
static inline void
mum512_keyed_hash (const void *data, size_t len, const uint64_t key[4], unsigned char *out) {
#if defined(__x86_64__) && defined(_MC_FRESH_GCC)
  static int avx2_support = 0;
  
  if (avx2_support > 0) {
    _mc_hash_avx2 (data, len, key, out);
    return;
  }
  else if (! avx2_support) {
    __builtin_cpu_init ();
    avx2_support =  __builtin_cpu_supports ("avx2") ? 1 : -1;
    if (avx2_support > 0) {
      _mc_hash_avx2 (data, len, key, out);
      return;
    }
  }
#endif
  _mc_hash_default (data, len, key, out);
}

/* Hash DATA of length LEN.  */
static inline void
mum512_hash (const void *data, size_t len, unsigned char *out) {
  static const uint64_t key[4] = {0, 0, 0, 0};

  mum512_keyed_hash (data, len, key, out);
}

#endif
