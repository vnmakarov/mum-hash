#if defined(Spooky)

#include "SpookyV2.h"
static void SpookyHash64_test (const void *key, int len, uint32_t seed, void *out) {
  *(uint64_t *) out = SpookyHash::Hash64 (key, len, seed);
}

#define test SpookyHash64_test
#define test64 test

#elif defined(City)

#include "City.h"
static void CityHash64_test (const void *key, int len, uint32_t seed, void *out) {
  *(uint64 *) out = CityHash64WithSeed ((const char *) key, len, seed);
}

#define test CityHash64_test
#define test64 test

#elif defined(SipHash)

#include <stdint.h>

extern int siphash (uint8_t *out, const uint8_t *in, uint64_t inlen, const uint8_t *k);

static void siphash_test (const void *key, int len, uint32_t seed, void *out) {
  uint64_t s[2];

  s[0] = seed;
  s[1] = 0;
  siphash (out, (const uint8_t *) key, len, (const uint8_t *) s);
}

#define test siphash_test
#define test64 test

#elif defined(xxHash)

#ifdef _MSC_VER
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;
#else
#include <stdint.h>
#endif

#include "xxhash.c"

static void xxHash64_test (const void *key, int len, uint32_t seed, void *out) {
  *(uint64_t *) out = XXH64 (key, len, seed);
}

#define test xxHash64_test
#define test64 test

#elif defined(xxh3)

#include "xxh3.h"

static void xxh3_test (const void *key, int len, uint32_t seed, void *out) {
  *(uint64_t *) out = XXH3_64bits (key, len);
}

#define test xxh3_test
#define test64 test

#elif defined(T1HA2)

#include "t1ha.h"
static void t1ha_test (const void *key, int len, uint32_t seed, void *out) {
  *(uint64_t *) out = t1ha2_atonce (key, len, seed);
}

#define test t1ha_test
#define test64 test

#elif defined(City)

#include "City.h"
static void CityHash64_test (const void *key, int len, uint32_t seed, void *out) {
  *(uint64 *) out = CityHash64WithSeed ((const char *) key, len, seed);
}

#define test CityHash64_test
#define test64 test

#elif defined(METRO)

#include "metrohash64.h"
static void metro_test (const void *key, int len, uint32_t seed, void *out) {
  MetroHash64::Hash ((const uint8_t *) key, len, (uint8_t *) out, seed);
}

#define test metro_test
#define test64 test

#elif defined(MeowHash)

#ifdef _MSC_VER
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;
#else
#include <stdint.h>
#endif

#include "meow_intrinsics.h"
#include "meow_hash.h"

static void meowhash_test (const void *key, int len, uint32_t seed, void *out) {
  *(uint64_t *) out = MeowU64From (MeowHash_Accelerated (seed, len, key), 0);
}

#define test meowhash_test
#define test64 test

#elif defined(MUM)

#include "mum.h"
static void mum_test (const void *key, int len, uint32_t seed, void *out) {
  *(uint64_t *) out = mum_hash (key, len, seed);
}

static void mum_test64 (const void *key, int len, uint32_t seed, void *out) {
  *(uint64_t *) out = mum_hash64 (*(uint64_t *) key, seed);
}

#define test mum_test
#define test64 mum_test64

#elif defined(VMUM)

#include "vmum.h"
static void mum_test (const void *key, int len, uint32_t seed, void *out) {
  *(uint64_t *) out = mum_hash (key, len, seed);
}

static void mum_test64 (const void *key, int len, uint32_t seed, void *out) {
  *(uint64_t *) out = mum_hash64 (*(uint64_t *) key, seed);
}

#define test mum_test
#define test64 mum_test64

#else
#error "I don't know what to test"
#endif

#if DATA_LEN == 0

#include <stdlib.h>
#include <stdio.h>
uint32_t arr[16 * 256 * 1024];
int main () {
  int i;
  uint64_t out;

  for (i = 0; i < 16 * 256 * 1024; i++) {
    arr[i] = rand ();
  }
  for (i = 0; i < 10000; i++) test (arr, 16 * 256 * 1024 * 4, 2, &out), arr[0] = out;
  printf ("%s:%llx\n", (size_t) arr & 0x7 ? "unaligned" : "aligned", out);
  return 0;
}

#else

int len = DATA_LEN;
uint64_t k[(DATA_LEN + 7) / 8];
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
/* We should use external to prevent optimizations for MUM after
   inlining.  Otherwise MUM results will be too good.  */
int main () {
  int i, j, n;
  uint64_t out;

  assert (len <= 1024);
  printf ("%d-byte: %s:\n", len, (size_t) k & 0x7 ? "unaligned" : "aligned");
  for (i = 0; i < sizeof (k) / sizeof (uint64_t); i++) k[i] = i;
  for (j = 0; j < 128; j++)
    for (n = i = 0; i < 10000000; i++) test (k, len, 2, &out), k[0] = out;
  printf ("%llx\n", out);
  return 0;
}

#endif
