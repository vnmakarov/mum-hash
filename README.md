# Update (Oct 31, 2018)
* A **new version of mum hash** was created (version 2 or Halloween version)
* The new version works **faster for short keys** which are a majority of hash table cases usages
* The new version also passes all tests of SMHasher
* The old version still can be used by definining macro `MUM_V1` before compiling `mum.h`
  * When `MUM_V1` is defined, you will get the same hashes as previously
* The new version was also **simplified** by removing specialized code using features of x86-64 CPU with BMI2 flag
  * This has a tiny impact on mum hash performance
* I posted performance results for more fresh CPUs (i7-8700K, Power9, and APM X-Gene CPU Potenza A3)
* I also added performance results for a new hash, [**Meow hash**](https://github.com/cmuratori/meow_hash)
  * Meow hash is based on usage of x86-64 AES insns
  * Meow hash is the fastest hash for very long keys but it is not suitable for hash tables
    * Meow is too slow for most hash table cases
    * Meow can be used **only for x86-64**
    * Meow hash requires **aligned data** because AES insns needs aligned data
* **MUM PRNG performance was improved**
  * A performance bug (preventing inlining of code specialized for different architectures) was fixed 
* **MUM-512 performance was improved**
  * The same performance bug was fixed but in a different way

# MUM Hash
* MUM hash is a **fast non-cryptographic hash function**
  suitable for different hash table implementations
* MUM means **MU**ltiply and **M**ix
  * It is a name of the base transformation on which hashing is implemented
  * Modern processors have a fast logic to do long number multiplications
  * It is very attractable to use it for fast hashing
    * For example, 64x64-bit multiplication can do the same work as 32
      shifts and additions
  * I'd like to call it Multiply and Reduce.  Unfortunately, MUR
    (MUltiply and Rotate) is already taken for famous hashing
    technique designed by Austin Appleby
  * I've chosen the name also as I am releasing it on Mother's day
* MUM hash passes **all** [SMHasher](https://github.com/aappleby/smhasher) tests
  * For comparison, only 4 out of 15 non-cryptographic hash functions
    in SMHasher passes the tests, e.g. well known FNV, Murmur2,
    Lookup, and Superfast hashes fail the tests
* MUM algorithm is **simpler** than City64 and Spooky ones
* MUM is specifically **designed to be fast for 64-bit CPUs** (Sorry, I did not want to
  spend my time on dying architectures)
  * Still MUM will work for 32-bit CPUs and it will be sometimes
    faster Spooky and City
* On x86-64 MUM hash is **faster** than City64 and Spooky on all tests except for one
  test for the bulky speed
  * Starting with 240-byte strings, City uses Intel SSE4.2 crc32 instruction
  * I could use the same instruction but I don't want to complicate the algorithm
  * In typical scenario, such long strings are rare.  Usually another
    interface (see `mum_hash_step`) is used for hashing big data structures
* MUM has a **fast startup**.  It is particular good to hash small keys
  which are a majority of hash table applications

# MUM implementation details

* Input 64-bit data are randomized by 64x64->128 bit multiplication and mixing
  high- and low-parts of the multiplication result by using an addition.
  The result is mixed with the current state by using XOR
  * Instead of the addition for mixing high- and low- parts, XOR could be
    used
    * Using the addition instead of XOR improves performance by about
      10% on Haswell and Power7
* Prime numbers randomly generated with the equal probability of their
  bit values are used for the multiplication
* When all primes are used once, the state is randomized and the same
  prime numbers are used again for subsequent data randomization
* Major loop is transformed to be **unrolled** by compiler to benefit from the
  compiler instruction scheduling optimization and OOO instruction
  execution in modern CPUs
* AARCH64 128-bit result multiplication is very slow as it is
  implemented by a GCC library function
  * To use only 2 insns for such multiplication one GCC **asm extension**
    was added
    
   
# MUM benchmarking vs Spooky, City64, xxHash64, MetroHash64, MeowHash, and SipHash24

* Here are the results of benchmarking MUM and the fastest
  non-cryptographic hash functions I know:
  * Google City64 (sources are taken from SMHasher)
  * Bob Jenkins Spooky (sources are taken from SMHasher)
  * Yann Collet's xxHash64 (sources are taken from the
    [original repository](https://github.com/Cyan4973/xxHash))
* Murmur hash functions are slower so I don't compare it here
* I also added J. Aumasson and D. Bernstein's
  [SipHash24](https://github.com/veorq/SipHash) for the comparison as it
  is a popular choice for hash table implementation these days
* A [metro hash](https://github.com/jandrewrogers/MetroHash)
  was added as people asked and as metro hash is
  claimed to be the fastest hash function
    * metro hash is not portable as others functions as it does not deal
      with unaligned accesses problem on some targets
    * metro hash will produce different hash for LE/BE targets
    * some people on hackernews pointed out that the algorithm is very
      close to xxHash one but still it is much faster xxHash
* Measurements were done on 3 different architecture machines:
  * 4.7 GHz Intel i7-8700K
  * 3.8 GHz Power9
  * 2.4 GHz APM X-Gene CPU Potenza A3
* Each test was run 3 times and the minimal time was taken
  * GCC-7.3.1 was used for Intel machine, GCC-8.2.1 was used AARCH64, and GCC-4.9 was used for Power9
  * `-O3` was used for all compilations
  * The strings were generated by `rand` calls
  * The strings were aligned to see a hashing speed better and to permit runs for MeowHash and Metro
  * No constant propagation for string length is forced.  Otherwise,
    the results for MUM hash would be even better
  * The best results in the table below are highlighted.
  * Some people complaint that my comparison is unfair as most hash functions are not inlined
    * I believe that the interface is the part of the implementation.  So when
      the interface does not provide an easy way for inlining, it is an
      implementation pitfall
    * Still to address the complaints I added `-flto` for benchmarking all hash
      functions excluding MUM.  This option makes cross-file inlining
    * xxHash64 results became worse for small strings and better for the bulk speed test
    * All results for other functions improved, sometimes quite a lot
  
# Intel i7-8700K (4.7GHz)

|                           |MUM-V1|  MUM-V2  | City64|  Spooky| xxHash64| Metro64 |SipHash24|MeowHash | 
:---------------------------|-----:|---------:|------:|-------:|--------:|--------:|--------:|--------:|
5 bytes  (1,280M strings)   | 6.85s| **5.76s**|  9.19s|   6.93s|  7.08s  |  5.99s  |   10.72s| 50.06s  |
8 bytes  (1,280M strings)   | 4.93s| **3.83s**|  9.17s|   6.55s|  6.81s  |  4.36s  |   14.64s| 51.21s  |
16 bytes (1,280M strings)   | 6.44s| **5.05s**|  8.53s|  13.85s|  8.45s  |  5.95s  |   18.00s| 41.68s  |
32 bytes (1,280M strings)   | 7.27s| **5.73s**|  9.99s|  13.94s| 12.09s  | 12.18s  |   23.82s| 41.68s  |
64 bytes (1,280M strings)   | 9.63s| **9.36s**| 10.99s|  20.85s| 13.47s  | 13.03s  |   38.83s| 41.40s  |
128 bytes (1,280M strings)  |13.65s|**13.62s**| 15.00s|  35.30s| 16.13s  | 15.25s  |   65.56s| 53.12s  |
16MB (10K strings)          |11.72s| 11.70s   | 10.66s|  11.07s| 10.98s  | 10.28s  |   56.95s|**5.93s**|

# Power9 (3.8GHz)

|                           |  MUM-V1  |  MUM-V2  | City64|  Spooky| xxHash64| Metro64  |SipHash24| 
:---------------------------|---------:|---------:|------:|-------:|--------:|---------:|--------:|
5 bytes  (1,280M strings)   |  17.50s  |**17.22s**| 23.69s|  22.07s| 20.48s  |  18.02s  |   45.08s|
8 bytes  (1,280M strings)   |   9.14s  | **7.50s**| 23.72s|  17.59s| 19.83s  |   9.85s  |   54.54s|
16 bytes (1,280M strings)   |  17.40s  |  17.05s  | 18.42s|  33.97s| 23.68s  |**15.71s**|   60.43s|
32 bytes (1,280M strings)   |  18.39s  |**18.30s**| 21.59s|  32.19s| 35.51s  |  27.92s  |   76.31s|
64 bytes (1,280M strings)   |**20.51s**|  20.62s  | 23.54s|  53.51s| 38.07s  |  29.85s  |  105.25s|
128 bytes (1,280M strings)  |  29.81s  |**28.74s**| 33.15s|  87.36s| 45.11s  |  34.02s  |  176.88s|
16MB (10K strings)          |  11.67s  |**11.58s**| 13.38s|  17.91s| 14.55s  |  12.22s  |  116.90s|

# AARCH64 (APM X-Gene)

|                           |  MUM-V1  |  MUM-V2  | City64   |  Spooky  | xxHash64| Metro64  |SipHash24| 
:---------------------------|---------:|---------:|---------:|---------:|--------:|---------:|--------:|
5 bytes  (1,280M strings)   |  20.80s  |**16.00s**|  25.60s  |  18.13s  | 22.40s  |  18.67s  |   27.72s|
8 bytes  (1,280M strings)   |  14.39s  |**11.19s**|  25.60s  |  17.60s  | 21.34s  |  13.33s  |   35.74s|
16 bytes (1,280M strings)   |  21.34s  |**16.01s**|  25.06s  |  30.94s  | 26.14s  |  17.06s  |   45.34s|
32 bytes (1,280M strings)   |  28.27s  |**24.00s**|  29.34s  |  30.94s  | 36.27s  |  36.27s  |   62.94s|
64 bytes (1,280M strings)   |  41.61s  |  37.34s  |**30.40s**|  44.81s  | 40.54s  |  38.41s  |  101.87s|
128 bytes (1,280M strings)  |  69.34s  |  64.55s  |  45.34s  |  73.07s  | 49.07s  |**43.74s**|  195.76s|
16MB (10K strings)          |  48.48s  |  48.47s  |  45.80s  |**39.93s**| 53.26s  |  53.25s  |  188.45s|

# Vectorization
* A major loop in function `_mum_hash_aligned` could be vectorized
  using vector multiplication, addition, and shuffle instructions
* Unfortunately, x86-64 CPUs currently does not have vector
  multiplication `64 x 64-bit -> 128-bit` (`pclmulqdq` only 1 `64x64->128-bit` multiplication)
* AVX2 CPUs only have vector multiplication `32 x 32-bit -> 64-bit`
  * One such vector instruction makes 4 multiplication which is
    roughly equivalent what one `MULQ/MULX` insn does but it has bigger
    latency time than `MULQ/MULX`
  * Therefore all my vectorized code I tried was slower
* If Intel introduces a new vector insn for `64 x 64-bit -> 128-bit`
  multiplication, potentially it could increase MUM speed up to 2
  times (may be less as it requires to provide aligned data)

# Using cryptographic vs. non-cryptographic hash function
  * People worrying about denial attacks based on generating hash
    collisions started to use cryptographic hash functions in hash tables
  * Cryptographic functions are very slow
    * *sha1* is about 20-30 slower than MUM and City on the bulk speed tests
    * The new fastest cryptographic hash function *SipHash* is up to 10
      times slower
  * MUM is also *resistant* to preimage attack (finding a string with given hash)
    * To make hard moving to previous state values we use mostly 1-to-1 one way
      function `lo(x*C) + hi(x*C)` where C is a constant.  Brute force
      solution of equation `f(x) = a` probably requires `2^63` tries.
      Another used function equation `x ^ y = a` has a `2^64`
      solutions.  It complicates finding the overal solution further
  * If somebody is not convinced, you can use **randomly chosen
    multiplication constants** (see function `mum_hash_randomize`).
    Finding a string with a given hash even if you know a string with such
    hash probably will be close to finding two or more solutions of
    *Diophantine* equations
  * If somebody is still not convinced, you can implement hash tables
    to **recognize the attack and rebuild** the table using MUM function
    with the new multiplication constants
  * Analogous approach can be used if you use weak hash function as
    MurMur or City.  Instead of using cryptographic hash functions
    **all the time**, hash tables can be implemented to recognize the
    attack and rebuild the table and start using a cryptographic hash
    function
  * This approach solves the speed problem and permits to switch easily to a new
    cryptographic hash function if a flaw is found in the old one, e.g. switching from
    SipHash to SHA2
  
# How to use MUM
* Please just include file `mum.h` into your C/C++ program and use the following functions:
  * optional `mum_hash_randomize` for choosing multiplication constants randomly
  * `mum_hash_init`, `mum_hash_step`, and `mum_hash_finish` for hashing complex data structures
  * `mum_hash64` for hashing a 64-bit data
  * `mum_hash` for hashing any continuous block of data
* To compare MUM speed with Spooky, City64, and SipHash24 on your machine go to
  the directory `src` and run a script

```
sh bench
```

* The script will compile source files and run the tests printing the
  results

# Crypto-hash function MUM512
  * MUM is not designed to be a crypto-hash
    * The key (seed) and state are only 64-bit which are not crypto-level ones
    * The result can be different for different targets (BE/LE
      machines, 32- and 64-bit machines) as for other hash functions, e.g. City (hash can be
      different on SSE4.2 nad non SSE4.2 targets) or Spooky (BE/LE machines)
      * If you need the same MUM hash independent on the target, please
        define macro `MUM_TARGET_INDEPENDENT_HASH`
  * There is a variant of MUM called MUM512 which can be a **candidate**
    for a crypto-hash function and keyed crypto-hash function and
    might be interesting for researchers
    * The **key** is **256**-bit
    * The **state** and the **output** are **512**-bit
    * The **block** size is **512**-bit
    * It uses 128x128->256-bit multiplication which is analogous to about
      64 shifts and additions for 128-bit block word instead of 80
      rounds of shifts, additions, logical operations for 512-bit block
      in sha2-512.
  * It is **only a candidate** for a crypto hash function
    * I did not make any differential crypto-analysis or investigated
      probabilities of different attacks on the hash function (sorry, it
      is too big job)
      * I might be do this in the future as I am interesting in
        differential characteristics of the MUM512 base transformation
        step (128x128-bit multiplications with addition of high and
        low 128-bit parts)
      * I am interesting also in the right choice of the multiplication constants
      * May be somebody will do the analysis.  I will be glad to hear anything.
        Who knows, may be it can be easily broken as Nimbus cipher.
    * The current code might be also vulnerable to timing attack on
      systems with varying multiplication instruction latency time.
      There is no code for now to prevent it
  * To compare the MUM512 speed with the speed of SHA-2 (SHA512) and
    SHA-3 (SHA3-512) go to the directory `src` and run a script `sh bench-crypto`
    * SHA-2 and SHA-3 code is taken from [RHash](https://github.com/rhash/RHash.git)
  * Blake2 crypto-hash from [github.com/BLAKE2/BLAKE2](https://github.com/BLAKE2/BLAKE2)
    was added for comparison.  I use sse version of 64-bit Blake2 (blake2b).
  * Here is the speed of the crypto hash functions on 4.7 GHz Intel i7-8700K:

|                        | MUM512 | SHA2  |  SHA3  | Blake2B|
:------------------------|-------:|------:|-------:|-------:|
10 bytes (20 M texts)    | 0.57s  | 0.53s |  0.87s |  0.68s |
100 bytes (20 M texts)   | 0.77s  | 0.51s |  1.68s |  0.68s |
1000 bytes (20 M texts)  | 2.75s  | 3.79s | 11.58s |  2.85s |
10000 bytes (5 M texts)  | 5.60s  | 9.21s | 28.37s |  6.23s |

# Pseudo-random generators
  * Files `mum-prng.h` and `mum512-prng.h` provides pseudo-random
    functions based on MUM and MUM512 hash functions
  * All PRNGs passed *NIST Statistical Test Suite for Random and
    Pseudorandom Number Generators for Cryptographic Applications*
    (version 2.2.1) with 1000 bitstreams each containing 1M bits
    * Although MUM PRNG pass the test, it is not a cryptographically
      secure PRNG as the hash function used for it
  * To compare the PRNG speeds go to
    the directory `src` and run a script `sh bench-prng`
  * For the comparison I wrote crypto-secured Blum Blum Shub PRNG
    (file `bbs-prng.h`) and PRNGs based on fast cryto-level hash
    functions in ChaCha stream cipher (file `chacha-prng.h`) and
    SipHash24 (file `sip24-prng.h`).
    * The additional PRNGs also pass the Statistical Test Suite
  * For the comparison I also added the fastest PRNGs
    * [xoroshiro128+](http://xoroshiro.di.unimi.it/xoroshiro128plus.c)
    * [xoroshiro128**](http://xoroshiro.di.unimi.it/xoroshiro128starstar.c)
    * [xoshiro256+](http://xoroshiro.di.unimi.it/xoshiro256plus.c)
    * [xoshiro256**](http://xoroshiro.di.unimi.it/xoshiro256starstar.c)
    * [xoshiro512**](http://xoroshiro.di.unimi.it/xoshiro512starstar.c)
    * As recommended the first numbers generated by splitmix64 were used as a seed
  * I had no intention to tune MUM based PRNG first but
    after adding xoroshiro128+ and finding how fast it is, I've decided
    to speedup MUM PRNG
    * I added code to calculate a few PRNs at once to calculate them in parallel
    * I added AVX2 version functions to use faster `MULX` instruction
    * The new version also passed NIST Statistical Test Suite.  It was
      tested even on bigger data (10K bitstreams each containing 10M
      bits).  The test took several days on i7-4790K
    * The new version is **almost 2 times** faster the old one and MUM PRN
      speed became almost the same as xoroshiro/xoshiro ones
      * All xoroshiro/xoshiro and MUM PRNG functions are inlined in the benchmark program
      * both code without inlining will be visibly slower and the speed
        difference will be negligible as one PRN calculation takes
        only about **3-4 machine cycle** for xoroshiro/xoshiro and MUM PRN.
  * All PRNG were tested by [practrand](http://pracrand.sourceforge.net/) with
    256GB PRNG generated stream (it took a lot of hours)
      * **xoroshiro128+, xoshiro256+, and xoshiro512+ failed** on the first stages of practrand
      * the rest PRNGs passed
      * BBS PRNG was tested by only 16GB stream because it is too slow
  * Here is the speed of the PRNGs in millions generated PRNs
    per second on 4.7 GHz Intel i7-8700K:

|                        | M prns/sec  |
:------------------------|------------:|
BBS                      | 0.078       |
ChaCha                   | 199         |
SipHash24                | 413         |
MUM512                   |  83         |
MUM                      |1156         |
XOSHIRO128**             |1130         |
XOSHIRO256**             |1337         |
XOSHIRO512**             |1944         |
GLIBC RAND               | 193         |
XOROSHIRO128+            |1342         |
XOSHIRO256+              |1339         |
XOSHIRO512+              |1253         |
