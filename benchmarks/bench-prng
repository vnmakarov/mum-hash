#!/bin/bash

# Benchmarking different Pseudo Random Generators

echo +++pseudo random number generation speed '(PRNs/sec)':
if test x${MUM_ONLY} == x; then
    gcc -DBBS -O3 -w bench-prng.c -lgmp && echo -n 'BBS           : ' && ./a.out 2>&1
    gcc -DCHACHA -O3 -w bench-prng.c && echo -n 'ChaCha        : ' && ./a.out 2>&1
    gcc -DSIP24 -O3 -w bench-prng.c && echo -n 'Sip24         : ' && ./a.out 2>&1
fi
gcc -DMUM512 -DMUM512_ROUNDS=2 -I../ -O3 -w bench-prng.c && echo -n 'MUM512        : ' && ./a.out 2>&1
gcc -DMUM -I../ -O3 -w bench-prng.c && echo -n 'MUM           : ' && ./a.out 2>&1
if test x${MUM_ONLY} == x; then
    gcc -DXOROSHIRO128STARSTAR -I../ -std=c99 -O3 -w bench-prng.c && echo -n 'XOROSHIRO128**: ' && ./a.out 2>&1
    gcc -DXOSHIRO256STARSTAR -I../ -std=c99 -O3 -w bench-prng.c && echo -n 'XOSHIRO256**  : ' && ./a.out 2>&1
    gcc -DXOSHIRO512STARSTAR -I../ -std=c99 -O3 -w bench-prng.c && echo -n 'XOSHIRO512**  : ' && ./a.out 2>&1
    gcc -DRAND -I../ -O3 -w bench-prng.c && echo -n 'RAND          : ' && ./a.out 2>&1
    gcc -DXOROSHIRO128P -I../ -std=c99 -O3 -w bench-prng.c && echo -n 'XOROSHIRO128+ : ' && ./a.out 2>&1
    gcc -DXOSHIRO256P -I../ -std=c99 -O3 -w bench-prng.c && echo -n 'XOSHIRO256+   : ' && ./a.out 2>&1
    gcc -DXOSHIRO512P -I../ -std=c99 -O3 -w bench-prng.c && echo -n 'XOSHIRO512+   : ' && ./a.out 2>&1
fi

rm -rf ./a.out
