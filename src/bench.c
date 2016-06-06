#if defined(Spooky)

#include "Spooky.h"
static void SpookyHash64_test(const void *key, int len, uint32_t seed, void *out) {
  *(uint64_t*)out = SpookyHash::Hash64(key, len, seed);
}

#define test SpookyHash64_test
#define test64 test

#elif defined(City)

#include "City.h"
static void CityHash64_test ( const void * key, int len, uint32_t seed, void *out) {
  *(uint64*)out = CityHash64WithSeed((const char *)key,len,seed);
}

#define test CityHash64_test
#define test64 test

#elif defined(SipHash)

#include <stdint.h>

extern int siphash(uint8_t *out, const uint8_t *in, uint64_t inlen, const uint8_t *k);

static void siphash_test (const void * key, int len, uint32_t seed, void * out) {
  uint64_t s[2];

  s[0] = seed; s[1] = 0;
  siphash(out, (const uint8_t *) key, len, (const uint8_t *) s);
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

#include "xxhash.h"
static XXH64_state_t* state = NULL;

static void xxHash64_test(const void *key, int len, uint32_t seed, void *out) {
  *(uint64_t*)out = XXH64 (key, len, seed);
}

#define test xxHash64_test
#define test64 test

#elif defined(METRO)

#include "metrohash64.h"
static void metro_test(const void *key, int len, uint32_t seed, void *out) {
  MetroHash64::Hash ((const uint8_t *)key, len, (uint8_t *) out, seed);
}

#define test metro_test
#define test64 test
#elif defined(MUM)

#include "mum.h"
static void mum_test(const void *key, int len, uint32_t seed, void *out) {
  *(uint64_t *)out = mum_hash (key, len, seed);
}

static void mum_test64(const void *key, int len, uint32_t seed, void *out) {
  *(uint64_t *)out = mum_hash64 (*(uint64_t *) key, seed);
}

#define test mum_test
#define test64 mum_test64

#else
#error "I don't know what to test"
#endif


#ifdef SPEED
#include <stdlib.h>
#include <stdio.h>
int main () {
  int i; uint32_t arr[256]; uint64_t out;
  
  for (i = 0; i < 256; i++) {
    arr[i] = rand ();
  }
  for (i = 0; i < 100000000; i++)
    test (arr, 256 * 4, 2, &out), arr[0] = out;
  printf ("%s:%llx\n", (size_t)arr & 0x7 ? "unaligned" : "aligned", out);
  return 0;
}
#endif

#ifdef SPEED2
/* We should use external to prevent optimizations for MUM after
   inlining.  Otherwise MUM results will be too good.  */
int len = 8;
#include <stdlib.h>
#include <stdio.h>
int main () {
  int i, j; uint64_t k = rand (); uint64_t out;
  
  for (j = 0; j < 128; j++)
    for (i = 0; i < 10000000; i++)
      test64 (&k, len, 2, &out), k = out;
  printf ("8-byte: %s:%llx\n", (size_t)&k & 0x7 ? "unaligned" : "aligned", k);
  return 0;
}
#endif

#ifdef SPEED3
int len = 16;
#include <stdlib.h>
#include <stdio.h>
int main () {
  int i, j; uint64_t k[2]; uint64_t out;
  
  k[0] = rand ();
  k[1] = rand ();
  for (j = 0; j < 128; j++)
    for (i = 0; i < 10000000; i++)
      test (k, len, 2, &out), k[0] = out;
  printf ("16-byte: %s:%llx\n", (size_t)k & 0x7 ? "unaligned" : "aligned", out);
  return 0;
}
#endif

#ifdef SPEED4
int len = 32;
#include <stdlib.h>
#include <stdio.h>
int main () {
  int i, j; uint64_t k[4]; uint64_t out;
  
  k[0] = rand (); k[1] = rand ();k[2] = rand (); k[3] = rand ();
  for (j = 0; j < 128; j++)
    for (i = 0; i < 10000000; i++)
      test (k, len, 2, &out), k[0] = out;
  printf ("32-byte: %s:%llx\n", (size_t)k & 0x7 ? "unaligned" : "aligned", out);
  return 0;
}
#endif

#ifdef SPEED5
int len = 64;
#include <stdlib.h>
#include <stdio.h>
int main () {
  int i, j; uint64_t k[8]; uint64_t out;
  
  k[0] = rand (); k[1] = rand ();k[2] = rand (); k[3] = rand ();
  k[4] = rand (); k[5] = rand ();k[6] = rand (); k[7] = rand ();
  for (j = 0; j < 128; j++)
    for (i = 0; i < 10000000; i++)
      test (k, len, 2, &out), k[0] = out;
  printf ("64-byte: %s:%llx\n", (size_t)k & 0x7 ? "unaligned" : "aligned", out);
  return 0;
}
#endif

#ifdef SPEED6
int len = 128;
#include <stdlib.h>
#include <stdio.h>
int main () {
  int i, j; uint64_t k[16]; uint64_t out;
  
  k[0] = rand (); k[1] = rand ();k[2] = rand (); k[3] = rand ();
  k[4] = rand (); k[5] = rand ();k[6] = rand (); k[7] = rand ();
  k[8] = rand (); k[9] = rand ();k[10] = rand (); k[11] = rand ();
  k[12] = rand (); k[13] = rand ();k[14] = rand (); k[15] = rand ();
  for (j = 0; j < 128; j++)
    for (i = 0; i < 10000000; i++)
      test (k, len, 2, &out), k[0] = out;
  printf ("128-byte: %s:%llx\n", (size_t)k & 0x7 ? "unaligned" : "aligned", out);
  return 0;
}

#endif

#ifdef SPEED1
/* We should use external to prevent optimizations for MUM after
   inlining.  Otherwise MUM results will be too good.  */
int len = 5;
#include <stdlib.h>
#include <stdio.h>
int main () {
  int i, j; uint64_t k = rand (); uint64_t out;
  
  for (j = 0; j < 128; j++)
    for (i = 0; i < 10000000; i++)
      test (&k, len, 2, &out), k = out;
  printf ("5-byte: %s:%llx\n", (size_t)&k & 0x7 ? "unaligned" : "aligned", k);
  return 0;
}
#endif
