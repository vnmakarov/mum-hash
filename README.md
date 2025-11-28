# **Update (Nov. 28, 2025): Implemented collision attack prevention in VMUM and MUM-V3**
* The attack is described in Issue#18
* The code in question looks like
```
    state ^= _vmum (data[i] ^ _vmum_factors[i], data[i + 1] ^ _vmum_factors[i + 1]));
```
  It is easy to generate data which makes the 1st operand of `_vmum`
  to be zero.  In this case whatever the second operand is, the hash
  will be generated the same.  So an adversary can generate a lot of
  data with the same hash
* This is pretty common code mistake for a few fast hash-functions. At
  least I found the same vulnerability in [wyhash](https://github.com/wangyi-fudan/wyhash/blob/46cebe9dc4e51f94d0dca287733bc5a94f76a10d/wyhash.h#L130) and [rapidhash](https://github.com/Nicoshev/rapidhash/blob/d60698faa10916879f85b2799bfdc6996b94c2b7/rapidhash.h#L383)
* After the code change, the safe variants of VMUM and MUM hashes are
  switched on by default.  If you want previous variants, please use
  macros VMUM_V1 and MUM_V3 correspondinly.  I believe there are still
  cases when they can be used, e.g. for hash tables in compilers.
* The fix consist of checking _vmum operands on zero and use nonzero value instead
  * all checks are implemented to avoid branch instruction generations to keep hash calculation pipeline going
  * still the checks increase length of critical paths of calculation
  * in most cases, new versions of VMUM and MUM generates the same hashes as the previous versions
* The fix results in slowing down hash speeds by about **10%** according to my benchmarks
  * I updated all benchmark data related to the new versions of VMUM and MUM below
	
# MUM Hash
* MUM hash is a **fast non-cryptographic hash function**
  suitable for different hash table implementations
* MUM means **MU**ltiply and **M**ix
  * It is a name of the base transformation on which hashing is implemented
  * Modern processors have a fast logic to do long number multiplications
  * It is very attractive to use it for fast hashing
    * For example, 64x64-bit multiplication can do the same work as 32
      shifts and additions
  * I'd like to call it Multiply and Reduce.  Unfortunately, MUR
    (MUltiply and Rotate) is already taken for famous hashing
    technique designed by Austin Appleby
  * I've chosen the name also as the first release happened on Mother's day
* To use mum you just need one header file (mum.h)
* MUM hash passes **all** [SMHasher](https://github.com/aappleby/smhasher) tests
  * For comparison, only 4 out of 15 non-cryptographic hash functions
    in SMHasher passes the tests, e.g. well known FNV, Murmur2,
    Lookup, and Superfast hashes fail the tests
* MUM V3 hash does not pass the following tests of a more rigourous
  version of [SMHasher](https://github.com/rurban/smhasher):
  * It fails on Perlin noise and bad seeds tests.  It means it still
    qualitative enough for the most applications
  * To make MUM V3 to pass the Rurban SMHasher, macro `MUM_QUALITY` has been
    added.  Compilation with this defined macro makes MUM V3 to pass
    all tests of Rurban SMHasher.  The slowdown is about 5% in average
    or 10% at most on keys of length 8.  It also results in generating
    a target independent hash
* For historic reasons mum.h contains code for older version V1 and
  V2.  You can switch them on by defining macros **MUM_V1** and **MUM_V2**
* MUM algorithm is **simpler** than the VMUM one
* MUM is specifically **designed to be fast on 64-bit CPUs**
  * Still MUM will work for 32-bit CPUs and it will be sometimes
    faster than Spooky and City
* MUM has a **fast startup**.  It is particular good to hash small keys
  which are prevalent in hash table applications

# MUM implementation details

* Input 64-bit data is randomized by 64x64->128 bit multiplication and mixing
  high- and low-parts of the multiplication result by using addition.
  The result is mixed with the current internal state by using XOR
  * Instead of addition for mixing high- and low- parts, XOR could be
    used
    * Using addition instead of XOR improves performance by about
      10% on Haswell and Power7
* Factor numbers, randomly generated with an equal probability of their
  bit values, are used for the multiplication
* When all factors are used once, the internal state is randomized, and the same
  factors are used again for subsequent data randomization
* The main loop is formed to be **unrolled** by the compiler to benefit from the
  the compiler instruction scheduling optimization and OOO
  (out-of-order) instruction execution in modern CPUs
* MUM code does not contain assembly (asm) code anymore. This makes MUM less
  machine-dependent.  To have efficient mum implementation, the
  compiler should support 128-bit integer
  extension (true for GCC and Clang on many targets)

# VMUM Hash
* VMUM is a vector variant of mum hashing (see below)
  * It uses target SIMD instructions (insns)
  * In comparison with mum v3, vmum considerably (up to 3 times) improves the speed
    of hashing mid-range (32 to 256 bytes) to long-range (more 256 bytes) length keys
  * As with previous mum hashing, to use vmum you just need one header
    file (vmum.h)
  * vmum source code is considerably smaller than that of extremely
    fast xxHash3 and th1ha2 and competes with them on hashing speed
  * vmum passes a more rigorous version of
    [SMHasher](https://github.com/rurban/smhasher)
   
# VMUM implementation details
* For long keys vmum uses vector insns:
  * AVX2 256-bit vector insns on x86-64
  * Neon 128-bit vector insns on aarch64
  * Altivec 128-bit vector insns on ppc64
  * There is a scalar emulation of the vector insns, too, for other targets
	* This could be useful for understanding used the vector
      operations used
* You can add usage of vector insns for other targets.  For this you
    just need to add small functions `_vmum_update_block`,
    `_vmum_zero_block`, and `_vmum_fold_block`
  * For the beneficial usage of vector insns the target should have unsigned `32 x 32-bit ->
    64-bit` vector multiplication
* To run vector insns in parallel on OOO CPUs, two vmum code loops are formed
  to be **unrolled** by the compiler into one basic block
* I experimented a lot with other vector insns and found that the usage of
  carry-less (sometimes called polynomial) vector multiplication insns does not work
  well enough for hashing

# VMUM and MUM benchmarking vs other famous hash functions

* Here are the results of benchmarking VMUM and MUM with the fastest
  non-cryptographic hash functions I know:
  * Google City64 (sources are taken from SMHasher)
  * Bob Jenkins Spooky (sources are taken from SMHasher)
  * Yann Collet's xxHash3 (sources are taken from the
    [original repository](https://github.com/Cyan4973/xxHash))
* I also added J. Aumasson and D. Bernstein's
  [SipHash24](https://github.com/veorq/SipHash) for the comparison as it
  is a popular choice for hash table implementation these days
* A [metro hash](https://github.com/jandrewrogers/MetroHash)
  was added as people asked and as metro hash is
  claimed to be the fastest hash function
    * metro hash is not portable as others functions as it does not deal
      with the unaligned accesses problem on some targets
    * metro hash will produce different hash for LE/BE targets
* Measurements were done on 4 different architecture machines:
  * AMD Ryzen 9900X
  * Intel i5-1300K
  * IBM Power10
  * Apple M4 10 cores (mac mini)
* Hashing 10,000 of 16MB keys (bulk)
* Hashing 1,280M keys for all other length keys
* Each test was run 3 times and the minimal time was taken
  * GCC-14.2.1 was used on AMD and M4 machine, GCC-12.3.1 on Intel
    machine, GCC-11.5.0 was used on Power10
  * `-O3` was used for all compilations
  * The keys were generated by `rand` calls
  * The keys were aligned to see a hashing speed better and to permit runs for Metro
  * Some people complaint that my comparison is unfair as most hash functions are not inlined
    * I believe that the interface is the part of the implementation.  So when
      the interface does not provide an easy way for inlining, it is an
      implementation pitfall
    * Still to address the complaints I added `-flto` for benchmarking all hash
      functions excluding MUM and VMUM.  This option makes cross-file inlining
* Here are graphs summarizing the measurements:

![AMD](./benchmarks/amd.png)

![INTEL](./benchmarks/intel.png)

![M4](./benchmarks/m4.png)

![Power10](./benchmarks/power10.png)

* Exact numbers are given in the last section

# SMhasher Speed Measurements

* SMhasher also measures hash speeds.  It uses the CPU cycle counter (__rtdc)
  * __rtdc-based measurements might be inaccurate for a small number of
    executed insns as the process can migrate, not all insns can
    retire, and CPU freq can be different.  That is why I prefer long
    running benchmarks
* Here are the results on AMD Ryzen 9900X for the fastest quality hashes
  (chosen according to SMhasher bulk speed results from https://github.com/rurban/smhasher)
* More GB/sec is better.  Less cycles/hash is better
* Some hashes are based on the use of x86\_64 AES insns and are less portable.
  They are marked by "Yes" in the AES column 
* The SLOC column gives the source code lines to implement the hash
  
| Hash            | AES  | Bulk Speed (256KB): GB/s |Av. Speed on keys (1-32 bytes): cycles/hash| SLOC|
|:----------------|:----:|-------------------------:|------------------------------------------:|----:|
|VMUM-V2          |  -   |  103.7                   | 16.4                                      |459  |
|VMUM-V1          |  -   |  143.5                   | 16.8                                      |459  |
|MUM-V4           |  -   |   28.6                   | 15.8                                      |291  |
|MUM-V3           |  -   |   40.4                   | 16.3                                      |291  |
|xxh3             |  -   |   66.6                   | 17.6                                      |965  |
|umash64          |  -   |   63.1                   | 25.4                                      |1097 |
|FarmHash32       |  -   |   39.8                   | 32.6                                      |1423 |
|wyhash           |  -   |   39.3                   | 18.3                                      | 194 |
|clhash           |  -   |   38.4                   | 51.7                                      | 366 |
|t1ha2\_atonce    |  -   |   34.7                   | 25.5                                      |2262 |
|t1ha0\_aes\_avx2 | Yes  |  128.9                   | 25.0                                      |2262 |
|gxhash64         | Yes  |  197.1                   | 27.9                                      | 274 |
|aesni            | Yes  |   38.7                   | 28.5                                      | 132 |


# Using cryptographic vs. non-cryptographic hash function
  * People worrying about denial attacks based on generating hash
    collisions started to use cryptographic hash functions in hash tables
  * Cryptographic functions are very slow
    * *sha1* is about 20-30 times slower than MUM and City on the bulk speed tests
    * The new fastest cryptographic hash function *SipHash* is up to 10
      times slower
  * MUM and VMUM are also *resistant* to preimage attack (finding a
    key with a given hash) 
    * To make hard moving to previous state values we use mostly 1-to-1 one way
      function `lo(x*C) + hi(x*C)` where C is a constant.  Brute force
      solution of equation `f(x) = a` probably requires `2^63` tries.
      Another used function equation `x ^ y = a` has a `2^64`
      solutions.  It complicates finding the overal solution further
  * If somebody is not convinced, you can use **randomly chosen
    multiplication constants** (see functions `mum_hash_randomize` and
    `vmum_hash_randomize`).
    Finding a key with a given hash even if you know a key with such
    a hash probably will be close to finding two or more solutions of
    *Diophantine* equations
  * If somebody is still not convinced, you can implement hash tables
    to **recognize the attack and rebuild** the table using the MUM function
    with the new multiplication constants
  * Analogous approach can be used if you use weak hash function as
    MurMur or City.  Instead of using cryptographic hash functions
    **all the time**, hash tables can be implemented to recognize the
    attack and rebuild the table and start using a cryptographic hash
    function
  * This approach solves the speed problem and permits us to switch easily to a new
    cryptographic hash function if a flaw is found in the old one, e.g., switching from
    SipHash to SHA2
  
# How to use [V]MUM
* Please just include file `[v]mum.h` into your C/C++ program and use the following functions:
  * optional `[v]mum_hash_randomize` for choosing multiplication constants randomly
  * `[v]mum_hash_init`, `[v]mum_hash_step`, and `[v]mum_hash_finish` for hashing complex data structures
  * `[v]mum_hash64` for hashing a 64-bit data
  * `[v]mum_hash` for hashing any continuous block of data
  * Compile `vmum.h` with other code using options switching on vector
    insns if necessary (e.g. -mavx2 for x86\_64)
* To compare MUM and VMUM speed with other hash functions on your machine go to
  the directory `benchmarks` and run a script `./bench.sh`
* The script will compile source files and run the tests printing the
  results as a markdown table

# Crypto-hash function MUM512
  * [V]MUM is not designed to be a crypto-hash
    * The key (seed) and state are only 64-bit which are not crypto-level ones
    * The result can be different for different targets (BE/LE
      machines, 32- and 64-bit machines) as for other hash functions, e.g. City (hash can be
      different on SSE4.2 nad non SSE4.2 targets) or Spooky (BE/LE machines)
      * If you need the same MUM hash independent on the target, please
        define macro `[V]MUM_TARGET_INDEPENDENT_HASH`.  Defining the
        macro affects the performace only on big-endian targets or
        targets without int128 support
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
      * I might be do this in the future as I am interested in
        differential characteristics of the MUM512 base transformation
        step (128x128-bit multiplications with addition of high and
        low 128-bit parts)
      * I am also interested in the right choice of the multiplication constants
      * May be somebody will do the analysis.  I will be glad to hear anything.
        Who knows, may be it can be easily broken as Nimbus cipher.
    * The current code might be also vulnerable to timing attack on
      systems with varying multiplication instruction latency time.
      There is no code for now to prevent it
  * To compare the MUM512 speed with the speed of SHA-2 (SHA512) and
    SHA-3 (SHA3-512) go to the directory `benchmarks` and run a script `./bench-crypto.sh`
    * SHA-2 and SHA-3 code is taken from [RHash](https://github.com/rhash/RHash.git)
  * Blake2 crypto-hash from [github.com/BLAKE2/BLAKE2](https://github.com/BLAKE2/BLAKE2)
    was added for comparison.  I use sse version of 64-bit Blake2 (blake2b).
  * Here is the speed of the crypto hash functions on AMD 9900X:

|                        | MUM512 | SHA2  |  SHA3  | Blake2B|
:------------------------|-------:|------:|-------:|-------:|
10 bytes (20 M texts)    | 0.27s  | 0.27s |  0.44s |  0.81s |
100 bytes (20 M texts)   | 0.36s  | 0.25s |  0.84s |  0.84s |
1000 bytes (20 M texts)  | 1.21s  | 2.08s | 5.63s  |  3.70s |
10000 bytes (5 M texts)  | 5.60s  | 5.05s | 14.07s |  7.99s |

# Pseudo-random generators
  * Files `mum-prng.h` and `mum512-prng.h` provide pseudo-random
    functions based on MUM and MUM512 hash functions
  * All PRNGs passed *NIST Statistical Test Suite for Random and
    Pseudorandom Number Generators for Cryptographic Applications*
    (version 2.2.1) with 1000 bitstreams each containing 1M bits
    * Although MUM PRNG passed the test, it is not a cryptographically
      secure PRNG as is the hash function used for it
  * To compare the PRNG speeds go to
    the directory `benchmarks` and run a script `./bench-prng.sh`
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
    * I added AVX2 version functions to use the faster `MULX` instruction
    * The new version also passed NIST Statistical Test Suite.  It was
      tested even on bigger data (10K bitstreams each containing 10M
      bits).  The test took several days on i7-4790K
    * The new version is **almost 2 times** faster than the old one and MUM PRN
      speed became almost the same as xoroshiro/xoshiro ones
      * All xoroshiro/xoshiro and MUM PRNG functions are inlined in the benchmark program
      * Both code without inlining will be visibly slower and the speed
        difference will be negligible as one PRN calculation takes
        only about **3-4 machine cycle** for xoroshiro/xoshiro and MUM PRN.
  * **Update Nov.2 2019**: I found that MUM PRNG fails practrand on 512GB.  So I modified it.
    Instead of basically 16 independent PRNGs with 64-bit state, I made it one PRNG with 1024-bit state.
    I also managed to speed up MUM PRNG by 15%.
  * All PRNG were tested by [practrand](http://pracrand.sourceforge.net/) with
    4TB PRNG generated stream (it took a few days)
      * **GLIBC RAND, xoroshiro128+, xoshiro256+, and xoshiro512+ failed** on the first stages of practrand
      * The rest of the PRNGs passed
      * BBS PRNG was tested by only 64GB stream because it is too slow
  * Here is the speed of the PRNGs in millions generated PRNs
    per second:

|  M prns/sec  | AMD 9900X   |Intel i5-1360K| Apple M4    | Power10  |
:--------------|------------:|-------------:|------------:|---------:|
BBS            | 0.0886      | 0.0827       | 0.122       | 0.021    |
ChaCha         | 357.68      | 184.80       | 262.81      |  83.20   |
SipHash24      | 702.10      | 567.43       | 760.13      | 231.48   |
MUM512         |  91.54      | 179.62       | 268.04      |  44.28   |
MUM            |1947.27      |1620.65       |2263.68      | 694.42   |
XOSHIRO128**   |1797.02      |1386.87       |1095.37      | 477.67   |
XOSHIRO256**   |1866.35      |1364.85       |1466.15      | 607.65   |
XOSHIRO512**   |1663.86      |1235.15       |1423.90      | 631.90   |
GLIBC RAND     | 115.57      | 101.48       | 228.99      |  33.66   |
XOROSHIRO128+  |1786.62      |1299.59       |1296.48      | 549.85   |
XOSHIRO256+    |2321.99      |1720.67       |1690.96      | 711.41   |
XOSHIRO512+    |1808.81      |1525.18       |1659.76      | 717.12   |

# Table results for hash speed measurements
* Here are table variants of my measurements for people wanting the
  exact numbers.  The tables also contain time spent for hashing.
  
* AMD Ryzen 9900X:

| Length    |  VMUM-V2  |  VMUM-V1  |  MUM-V4   |  MUM-V3   |  Spooky   |   City    |  xxHash3  |   t1ha2   | SipHash24 |   Metro   |
|:----------|:---------:|:---------:|:---------:|:---------:|:---------:|:---------:|:---------:|:---------:|:---------:|:---------:|
|   3 bytes |1.00  4.57s|0.98  4.67s|0.98  4.64s|0.97  4.73s|0.76  6.01s|0.60  7.61s|0.94  4.84s|0.61  7.47s|0.61  7.51s|0.69  6.67s|
|   4 bytes |1.00  2.77s|1.00  2.78s|1.09  2.55s|1.09  2.55s|0.55  5.08s|0.39  7.15s|0.71  3.92s|0.69  4.03s|0.44  6.24s|0.75  3.69s|
|   5 bytes |1.00  4.63s|1.00  4.64s|1.00  4.62s|1.00  4.63s|0.80  5.78s|0.65  7.07s|0.88  5.28s|0.62  7.41s|0.61  7.59s|0.88  5.24s|
|   6 bytes |1.00  4.57s|1.00  4.56s|1.00  4.57s|1.00  4.56s|0.77  5.93s|0.65  7.06s|0.87  5.24s|0.62  7.38s|0.58  7.87s|0.87  5.24s|
|   7 bytes |1.00  4.84s|1.01  4.80s|1.01  4.80s|1.01  4.79s|0.79  6.15s|0.69  7.06s|0.92  5.24s|0.66  7.38s|0.60  8.10s|0.76  6.38s|
|   8 bytes |1.00  2.74s|1.00  2.74s|1.09  2.51s|1.09  2.51s|0.54  5.03s|0.39  7.06s|0.52  5.24s|0.69  3.97s|0.33  8.29s|0.75  3.67s|
|   9 bytes |1.00  3.01s|1.08  2.78s|1.07  2.82s|1.06  2.83s|0.59  5.06s|0.27 11.03s|0.45  6.66s|0.41  7.37s|0.36  8.29s|0.60  5.04s|
|  10 bytes |1.00  3.01s|1.08  2.78s|1.08  2.79s|1.04  2.89s|0.59  5.08s|0.27 11.02s|0.45  6.66s|0.41  7.36s|0.36  8.34s|0.60  5.05s|
|  11 bytes |1.00  3.01s|1.08  2.79s|1.08  2.79s|1.08  2.78s|0.59  5.08s|0.27 11.04s|0.45  6.67s|0.41  7.37s|0.36  8.32s|0.49  6.20s|
|  12 bytes |1.00  3.01s|1.09  2.77s|1.07  2.81s|1.07  2.82s|0.59  5.06s|0.27 11.03s|0.45  6.66s|0.41  7.35s|0.36  8.30s|0.60  5.02s|
|  13 bytes |1.00  2.98s|1.08  2.77s|1.05  2.83s|1.08  2.77s|0.59  5.02s|0.27 10.94s|0.45  6.61s|0.41  7.28s|0.36  8.21s|0.48  6.15s|
|  14 bytes |1.00  2.96s|1.08  2.74s|1.08  2.75s|1.08  2.74s|0.59  5.01s|0.27 10.95s|0.45  6.60s|0.41  7.29s|0.36  8.21s|0.48  6.16s|
|  15 bytes |1.00  2.98s|1.09  2.74s|1.08  2.77s|1.06  2.80s|0.59  5.01s|0.27 10.93s|0.45  6.61s|0.41  7.29s|0.36  8.21s|0.41  7.28s|
|  16 bytes |1.00  2.98s|1.09  2.74s|1.09  2.73s|1.09  2.73s|0.27 10.94s|0.41  7.28s|0.93  3.19s|0.59  5.08s|0.29 10.31s|0.62  4.78s|
|  32 bytes |1.00  3.28s|1.08  3.05s|1.00  3.27s|0.98  3.34s|0.30 10.95s|0.40  8.19s|1.03  3.19s|0.44  7.50s|0.23 14.39s|0.33  9.82s|
|  64 bytes |1.00  3.89s|1.09  3.58s|0.87  4.47s|0.85  4.58s|0.23 16.63s|0.47  8.36s|1.22  3.19s|0.68  5.69s|0.17 22.59s|0.37 10.49s|
|  96 bytes |1.00  4.55s|1.08  4.20s|0.79  5.79s|0.78  5.83s|0.20 22.31s|0.36 12.54s|1.43  3.19s|0.66  6.88s|0.15 31.11s|0.40 11.27s|
| 128 bytes |1.00  6.32s|1.40  4.52s|0.91  6.92s|0.90  7.06s|0.23 27.99s|0.50 12.54s|1.98  3.19s|0.83  7.57s|0.16 39.35s|0.53 11.85s|
| 192 bytes |1.00  8.55s|1.29  6.63s|0.90  9.46s|0.99  8.65s|0.36 23.81s|0.59 14.48s|1.25  6.83s|0.88  9.74s|0.15 55.96s|0.65 13.21s|
| 256 bytes |1.00 10.98s|1.38  7.95s|0.92 11.98s|1.06 10.32s|0.45 24.22s|0.66 16.71s|0.79 13.86s|0.92 11.89s|0.15 74.12s|0.75 14.57s|
| 512 bytes |1.00 14.42s|1.12 12.91s|0.65 22.33s|0.79 18.16s|0.41 35.30s|0.53 26.98s|0.91 15.92s|0.69 21.04s|0.10 140.39s|0.63 22.76s|
|1024 bytes |1.00 17.13s|1.09 15.75s|0.37 46.54s|0.50 34.26s|0.32 53.76s|0.38 45.34s|0.88 19.50s|0.44 38.78s|0.06 272.94s|0.44 39.07s|
| Bulk      |1.00  1.70s|1.36  1.25s|0.31  5.57s|0.44  3.85s|0.33  5.13s|0.34  4.94s|1.18  1.44s|0.37  4.62s|0.05 33.88s|0.40  4.26s|
| Average   |1.00       |1.11       |0.93       |0.96       |0.50       |0.43       |0.85       |0.58       |0.31       |0.59       |
| Geomean   |1.00       |1.10       |0.90       |0.93       |0.46       |0.41       |0.77       |0.55       |0.26       |0.56       |

* Intel i5-13600K:

| Length    |  VMUM-V2  |  VMUM-V1  |  MUM-V4   |  MUM-V3   |  Spooky   |   City    |  xxHash3  |   t1ha2   | SipHash24 |   Metro   |
|:----------|:---------:|:---------:|:---------:|:---------:|:---------:|:---------:|:---------:|:---------:|:---------:|:---------:|
|   3 bytes |1.00  4.67s|1.02  4.59s|1.03  4.53s|1.03  4.53s|0.67  6.94s|0.58  8.05s|0.93  5.03s|0.50  9.31s|0.47  9.93s|0.69  6.79s|
|   4 bytes |1.00  4.27s|1.00  4.27s|1.06  4.02s|1.06  4.02s|0.73  5.86s|0.51  8.37s|0.75  5.70s|0.77  5.58s|0.52  8.17s|0.81  5.28s|
|   5 bytes |1.00  4.66s|1.02  4.59s|1.03  4.53s|1.03  4.53s|0.73  6.37s|0.57  8.17s|0.82  5.70s|0.50  9.31s|0.44 10.49s|0.69  6.79s|
|   6 bytes |1.00  4.67s|1.02  4.56s|1.03  4.53s|1.03  4.53s|0.69  6.76s|0.57  8.17s|0.82  5.69s|0.50  9.31s|0.43 10.95s|0.69  6.79s|
|   7 bytes |1.00  4.87s|1.00  4.89s|1.01  4.83s|1.01  4.83s|0.69  7.02s|0.60  8.17s|0.85  5.70s|0.52  9.31s|0.45 10.89s|0.69  7.04s|
|   8 bytes |1.00  4.27s|1.00  4.28s|1.55  2.76s|1.55  2.76s|0.76  5.60s|0.53  8.08s|0.75  5.69s|0.99  4.30s|0.39 10.88s|1.06  4.02s|
|   9 bytes |1.00  4.53s|1.06  4.29s|1.50  3.02s|1.50  3.01s|0.81  5.60s|0.33 13.59s|0.53  8.58s|0.48  9.51s|0.42 10.88s|0.82  5.53s|
|  10 bytes |1.00  4.54s|1.06  4.29s|1.50  3.02s|1.51  3.01s|0.81  5.60s|0.33 13.59s|0.53  8.58s|0.48  9.51s|0.42 10.88s|0.82  5.53s|
|  11 bytes |1.00  4.52s|1.06  4.27s|1.50  3.02s|1.50  3.02s|0.81  5.60s|0.33 13.59s|0.53  8.58s|0.48  9.51s|0.42 10.88s|0.67  6.79s|
|  12 bytes |1.00  4.54s|1.06  4.29s|1.50  3.02s|1.51  3.01s|0.81  5.60s|0.33 13.59s|0.53  8.58s|0.48  9.51s|0.42 10.88s|0.82  5.53s|
|  13 bytes |1.00  4.52s|1.06  4.28s|1.49  3.03s|1.50  3.02s|0.81  5.60s|0.33 13.59s|0.53  8.59s|0.48  9.51s|0.42 10.88s|0.67  6.79s|
|  14 bytes |1.00  4.52s|1.06  4.27s|1.49  3.03s|1.50  3.02s|0.81  5.60s|0.33 13.59s|0.53  8.59s|0.48  9.51s|0.42 10.88s|0.67  6.79s|
|  15 bytes |1.00  4.53s|1.06  4.29s|1.50  3.02s|1.50  3.02s|0.81  5.60s|0.33 13.59s|0.53  8.58s|0.48  9.51s|0.42 10.88s|0.56  8.05s|
|  16 bytes |1.00  4.52s|1.06  4.28s|1.50  3.02s|1.50  3.01s|0.37 12.13s|0.56  8.05s|0.89  5.07s|0.85  5.29s|0.34 13.43s|0.83  5.46s|
|  32 bytes |1.00  4.79s|1.05  4.58s|1.30  3.69s|1.33  3.59s|0.39 12.38s|0.51  9.39s|0.94  5.10s|0.67  7.15s|0.25 18.92s|0.43 11.07s|
|  64 bytes |1.00  5.46s|1.06  5.15s|1.14  4.78s|1.11  4.91s|0.29 18.66s|0.58  9.36s|1.06  5.13s|0.88  6.22s|0.17 31.57s|0.46 11.83s|
|  96 bytes |1.00  6.43s|1.10  5.83s|0.84  7.68s|0.84  7.67s|0.26 25.17s|0.46 13.88s|1.23  5.23s|0.85  7.60s|0.15 42.71s|0.51 12.70s|
| 128 bytes |1.00  7.92s|1.25  6.36s|0.84  9.42s|0.86  9.19s|0.25 31.62s|0.57 13.87s|1.51  5.24s|0.91  8.67s|0.15 53.88s|0.59 13.51s|
| 192 bytes |1.00 11.52s|1.43  8.06s|1.05 11.02s|1.08 10.68s|0.39 29.49s|0.71 16.25s|1.23  9.34s|1.03 11.18s|0.15 76.23s|0.76 15.07s|
| 256 bytes |1.00 14.26s|1.60  8.89s|1.04 13.65s|1.18 12.11s|0.48 29.86s|0.75 19.06s|0.91 15.64s|1.03 13.82s|0.15 97.67s|0.85 16.68s|
| 512 bytes |1.00 15.93s|1.04 15.31s|0.62 25.67s|0.81 19.65s|0.35 45.39s|0.46 34.70s|0.96 16.68s|0.58 27.38s|0.09 186.04s|0.53 29.86s|
|1024 bytes |1.00 20.96s|1.08 19.32s|0.42 49.61s|0.56 37.74s|0.29 71.66s|0.34 61.75s|0.84 24.92s|0.42 49.86s|0.06 362.59s|0.36 58.67s|
| Bulk      |1.00  3.13s|1.09  2.86s|0.40  7.77s|0.61  5.17s|0.40  7.78s|0.42  7.37s|0.87  3.60s|0.49  6.38s|0.07 45.99s|0.45  6.88s|
| Average   |1.00       |1.10       |1.15       |1.18       |0.58       |0.48       |0.83       |0.65       |0.31       |0.67       |
| Geomean   |1.00       |1.09       |1.08       |1.13       |0.54       |0.46       |0.79       |0.62       |0.26       |0.65       |

* Apple M4:

| Length    |  VMUM-V2  |  VMUM-V1  |  MUM-V4   |  MUM-V3   |  Spooky   |   City    |  xxHash3  |   t1ha2   | SipHash24 |   Metro   |
|:----------|:---------:|:---------:|:---------:|:---------:|:---------:|:---------:|:---------:|:---------:|:---------:|:---------:|
|   3 bytes |1.00  5.15s|1.02  5.03s|1.03  5.02s|1.02  5.03s|0.69  7.50s|0.52  9.91s|0.87  5.92s|1.08  4.77s|0.51 10.01s|0.63  8.17s|
|   4 bytes |1.00  4.70s|1.01  4.66s|1.08  4.36s|1.08  4.36s|0.69  6.78s|0.49  9.64s|0.70  6.71s|0.99  4.77s|0.52  9.03s|0.73  6.41s|
|   5 bytes |1.00  5.05s|1.00  5.03s|1.01  5.01s|1.01  5.01s|0.71  7.08s|0.52  9.64s|0.74  6.78s|1.06  4.77s|0.49 10.38s|0.62  8.17s|
|   6 bytes |1.00  5.15s|1.02  5.03s|1.04  4.95s|1.04  4.95s|0.69  7.49s|0.53  9.64s|0.76  6.78s|1.08  4.76s|0.49 10.51s|0.63  8.17s|
|   7 bytes |1.00  5.52s|1.03  5.34s|1.04  5.32s|1.04  5.32s|0.71  7.82s|0.57  9.64s|0.81  6.79s|1.16  4.76s|0.51 10.77s|0.65  8.46s|
|   8 bytes |1.00  3.07s|1.02  3.02s|1.16  2.65s|1.16  2.65s|0.47  6.49s|0.32  9.64s|0.46  6.71s|0.68  4.53s|0.26 11.69s|0.66  4.66s|
|   9 bytes |1.00  3.26s|1.07  3.04s|1.03  3.17s|1.03  3.18s|0.50  6.48s|0.27 11.96s|0.56  5.85s|0.55  5.98s|0.28 11.69s|0.51  6.41s|
|  10 bytes |1.00  3.27s|1.08  3.03s|1.08  3.02s|1.08  3.02s|0.50  6.48s|0.27 11.96s|0.56  5.85s|0.55  5.98s|0.28 11.69s|0.51  6.41s|
|  11 bytes |1.00  3.38s|1.10  3.07s|1.11  3.05s|1.07  3.15s|0.52  6.48s|0.28 11.96s|0.58  5.85s|0.57  5.98s|0.29 11.69s|0.43  7.87s|
|  12 bytes |1.00  3.39s|1.08  3.13s|1.13  3.00s|1.13  3.00s|0.52  6.48s|0.28 11.96s|0.58  5.85s|0.57  5.98s|0.29 11.69s|0.53  6.41s|
|  13 bytes |1.00  3.33s|1.08  3.08s|1.10  3.04s|1.10  3.04s|0.51  6.48s|0.28 11.95s|0.57  5.85s|0.56  5.98s|0.28 11.69s|0.42  7.87s|
|  14 bytes |1.00  3.31s|1.09  3.05s|1.10  3.02s|1.09  3.03s|0.51  6.48s|0.28 11.96s|0.56  5.86s|0.55  5.98s|0.28 11.69s|0.42  7.87s|
|  15 bytes |1.00  3.32s|1.06  3.12s|1.08  3.07s|1.08  3.07s|0.51  6.48s|0.28 11.96s|0.57  5.85s|0.56  5.98s|0.28 11.69s|0.36  9.33s|
|  16 bytes |1.00  3.30s|1.10  3.00s|1.07  3.07s|1.07  3.08s|0.23 14.08s|0.35  9.33s|0.85  3.87s|0.55  5.98s|0.23 14.58s|0.54  6.12s|
|  32 bytes |1.00  3.66s|1.07  3.42s|1.01  3.64s|1.00  3.65s|0.26 14.07s|0.35 10.51s|0.96  3.80s|0.41  9.03s|0.18 20.66s|0.29 12.57s|
|  64 bytes |1.00  4.42s|1.09  4.07s|0.89  4.99s|0.89  4.99s|0.21 21.37s|0.41 10.84s|1.17  3.79s|0.62  7.11s|0.13 33.90s|0.33 13.44s|
|  96 bytes |1.00  5.16s|1.07  4.82s|0.82  6.27s|0.84  6.17s|0.18 28.70s|0.32 16.19s|1.36  3.80s|0.60  8.57s|0.11 45.54s|0.36 14.34s|
| 128 bytes |1.00  6.87s|1.13  6.08s|0.91  7.55s|0.93  7.40s|0.19 35.99s|0.42 16.19s|1.81  3.80s|0.72  9.53s|0.12 59.49s|0.45 15.22s|
| 192 bytes |1.00  8.65s|1.12  7.69s|0.82 10.60s|0.88  9.86s|0.27 31.55s|0.46 18.64s|0.88  9.86s|0.71 12.16s|0.10 84.13s|0.51 16.99s|
| 256 bytes |1.00  9.39s|1.30  7.20s|0.71 13.24s|0.76 12.29s|0.29 32.11s|0.44 21.28s|0.71 13.19s|0.63 14.87s|0.09 106.82s|0.50 18.77s|
| 512 bytes |1.00 14.79s|1.07 13.76s|0.69 21.33s|0.92 15.99s|0.29 50.41s|0.40 37.15s|0.91 16.28s|0.49 30.25s|0.07 204.80s|0.46 31.88s|
|1024 bytes |1.00 27.83s|1.56 17.79s|0.65 43.07s|1.04 26.70s|0.35 78.46s|0.44 62.77s|1.14 24.39s|0.51 54.28s|0.07 399.61s|0.55 50.90s|
| Bulk      |1.00  3.45s|1.39  2.49s|0.58  6.00s|1.13  3.06s|0.44  7.83s|0.51  6.70s|1.19  2.89s|0.54  6.36s|0.07 50.68s|0.67  5.13s|
| Average   |1.00       |1.11       |0.96       |1.02       |0.45       |0.39       |0.84       |0.68       |0.26       |0.51       |
| Geomean   |1.00       |1.10       |0.95       |1.01       |0.41       |0.38       |0.79       |0.66       |0.21       |0.50       |

* IBM Power10:

| Length    |  VMUM-V2  |  VMUM-V1  |  MUM-V4   |  MUM-V3   |  Spooky   |   City    |  xxHash3  |   t1ha2   | SipHash24 |   Metro   |
|:----------|:---------:|:---------:|:---------:|:---------:|:---------:|:---------:|:---------:|:---------:|:---------:|:---------:|
|   3 bytes |1.00 11.52s|1.00 11.53s|1.03 11.18s|1.03 11.21s|0.68 16.87s|0.61 18.95s|0.95 12.16s|1.09 10.57s|0.52 22.13s|0.66 17.35s|
|   4 bytes |1.00 10.86s|1.00 10.87s|1.07 10.19s|1.07 10.19s|0.72 15.18s|0.54 20.22s|0.89 12.27s|1.03 10.58s|0.51 21.13s|0.88 12.40s|
|   5 bytes |1.00 11.53s|0.98 11.73s|1.03 11.17s|1.03 11.17s|0.71 16.24s|0.55 20.91s|0.90 12.76s|1.09 10.58s|0.49 23.74s|0.66 17.35s|
|   6 bytes |1.00 11.52s|0.98 11.73s|1.03 11.17s|1.03 11.18s|0.68 16.87s|0.55 20.92s|0.90 12.76s|1.09 10.58s|0.48 23.91s|0.66 17.36s|
|   7 bytes |1.00 12.23s|1.02 11.96s|0.98 12.51s|1.03 11.84s|0.69 17.69s|0.58 20.92s|0.96 12.76s|1.16 10.56s|0.50 24.38s|0.56 22.01s|
|   8 bytes |1.00 10.85s|1.00 10.86s|1.06 10.19s|1.07 10.18s|0.76 14.27s|0.52 20.92s|0.85 12.75s|1.05 10.32s|0.37 29.14s|1.10  9.86s|
|   9 bytes |1.00 11.54s|1.06 10.87s|1.06 10.85s|1.06 10.85s|0.79 14.55s|0.40 28.92s|0.72 16.11s|0.84 13.73s|0.40 29.08s|0.84 13.80s|
|  10 bytes |1.00 11.53s|1.06 10.87s|1.06 10.85s|1.06 10.85s|0.79 14.54s|0.40 28.92s|0.72 16.10s|0.84 13.72s|0.40 29.17s|0.84 13.79s|
|  11 bytes |1.00 11.22s|1.03 10.87s|1.03 10.85s|1.03 10.85s|0.77 14.55s|0.39 28.90s|0.70 16.11s|0.82 13.72s|0.38 29.21s|0.66 17.08s|
|  12 bytes |1.00 11.53s|1.06 10.86s|1.06 10.86s|1.06 10.85s|0.79 14.54s|0.40 28.92s|0.72 16.11s|0.84 13.72s|0.40 29.13s|0.84 13.80s|
|  13 bytes |1.00 11.23s|1.03 10.87s|1.04 10.85s|1.04 10.85s|0.77 14.56s|0.39 28.91s|0.70 16.11s|0.82 13.72s|0.39 29.14s|0.66 17.09s|
|  14 bytes |1.00 11.23s|1.03 10.88s|1.03 10.87s|1.04 10.84s|0.77 14.55s|0.39 28.92s|0.70 16.11s|0.82 13.71s|0.38 29.20s|0.66 17.09s|
|  15 bytes |1.00 11.53s|1.06 10.88s|1.06 10.89s|1.06 10.89s|0.79 14.56s|0.40 28.91s|0.72 16.11s|0.84 13.72s|0.40 29.17s|0.57 20.38s|
|  16 bytes |1.00 12.20s|1.12 10.91s|1.12 10.85s|1.12 10.89s|0.44 27.70s|0.61 20.05s|1.36  8.96s|0.89 13.72s|0.31 39.95s|1.00 12.16s|
|  32 bytes |1.00 12.92s|1.11 11.59s|1.06 12.22s|1.06 12.22s|0.45 28.63s|0.58 22.34s|1.57  8.23s|0.64 20.32s|0.24 54.90s|0.52 24.97s|
|  64 bytes |1.00 14.54s|1.11 13.09s|0.97 15.02s|0.96 15.07s|0.35 41.29s|0.62 23.31s|1.70  8.54s|0.90 16.20s|0.16 88.17s|0.54 26.78s|
|  96 bytes |1.00 15.93s|1.13 14.07s|0.82 19.44s|0.96 16.64s|0.29 54.32s|0.45 35.33s|1.93  8.24s|0.81 19.57s|0.13 119.66s|0.55 28.81s|
| 128 bytes |1.00 16.71s|1.08 15.47s|0.75 22.21s|0.86 19.52s|0.24 68.72s|0.47 35.31s|2.03  8.22s|0.77 21.78s|0.11 156.55s|0.54 30.78s|
| 192 bytes |1.00 21.98s|1.16 18.99s|0.83 26.51s|0.83 26.60s|0.36 60.97s|0.53 41.32s|0.76 28.78s|0.76 29.01s|0.11 195.56s|0.62 35.17s|
| 256 bytes |1.00 22.31s|1.25 17.89s|0.73 30.47s|0.76 29.34s|0.36 62.61s|0.47 47.41s|0.67 33.50s|0.64 35.02s|0.09 252.22s|0.57 39.10s|
| 512 bytes |1.00 33.65s|1.19 28.16s|0.71 47.60s|0.74 45.60s|0.33 100.76s|0.46 73.34s|0.73 46.38s|0.56 60.20s|0.07 483.42s|0.60 55.93s|
|1024 bytes |1.00 56.70s|1.41 40.15s|0.69 81.61s|0.73 78.10s|0.34 167.11s|0.45 126.45s|0.80 70.61s|0.49 116.16s|0.06 944.76s|0.45 127.01s|
| Bulk      |1.00  6.75s|1.42  4.75s|0.70  9.62s|0.67 10.01s|0.38 17.80s|0.49 13.74s|0.93  7.27s|0.49 13.91s|0.06 118.69s|0.46 14.52s|
| Average   |1.00       |1.10       |0.95       |0.97       |0.58       |0.49       |1.00       |0.84       |0.30       |0.67       |
| Geomean   |1.00       |1.09       |0.94       |0.96       |0.54       |0.48       |0.93       |0.82       |0.24       |0.65       |

