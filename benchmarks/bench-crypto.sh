#!/bin/bash

# Benchmarking different crypto hash functions.

IFS='%'
temp=__temp


print() {
    s=`grep -E 'user[ 	]*[0-9]' $2 | sed s/.*user// | sed s/\\t//`
    echo $1 "$s"s
}

echo compiling sha512
gcc -O3 -w -c sha512.c byte_order.c || exit 1
echo compiling sha3
gcc -O3 -w -c sha3.c || exit 1

str86_64="`uname -a|grep x86_64`"
if test -n str86_64; then
    echo compiling blake2b
    gcc -O3 -w -c -I. -std=gnu99 blake2b.c || exit 1
fi

echo +++10-byte speed '(20M texts)':
gcc -DSPEED1 -O3 -w -DSHA2 sha512.o byte_order.o bench-crypto.c && (time -p ./a.out) >$temp 2>&1 && print "sha512:" $temp
gcc -DSPEED1 -O3 -w -DSHA3 sha3.o byte_order.o bench-crypto.c && (time -p ./a.out) >$temp 2>&1 && print "sha3  :" $temp
if test -n str86_64; then
    gcc -DSPEED1 -O3 -w -DBLAKE2B blake2b.o bench-crypto.c && (time -p ./a.out) >$temp 2>&1 && print "blake2:" $temp
fi
gcc -DSPEED1 -O3 -w -DMUM512 -I../ bench-crypto.c && (time -p ./a.out) >$temp 2>&1 && print "mum512:" $temp

echo +++100-byte speed '(20M texts)':
gcc -DSPEED2 -O3 -w -DSHA2 sha512.o byte_order.o bench-crypto.c && (time -p ./a.out) >$temp 2>&1 && print "sha512:" $temp
gcc -DSPEED2 -O3 -w -DSHA3 sha3.o byte_order.o bench-crypto.c && (time -p ./a.out) >$temp 2>&1 && print "sha3  :" $temp
if test -n str86_64; then
    gcc -DSPEED2 -O3 -w -DBLAKE2B blake2b.o bench-crypto.c && (time -p ./a.out) >$temp 2>&1 && print "blake2:" $temp
fi
gcc -DSPEED2 -O3 -w -DMUM512 -I../ bench-crypto.c && (time -p ./a.out) >$temp 2>&1 && print "mum512:" $temp

echo +++1000-byte speed '(20M texts)':
gcc -DSPEED3 -O3 -w -DSHA2 sha512.o byte_order.o bench-crypto.c && (time -p ./a.out) >$temp 2>&1 && print "sha512:" $temp
gcc -DSPEED3 -O3 -w -DSHA3 sha3.o byte_order.o bench-crypto.c && (time -p ./a.out) >$temp 2>&1 && print "sha3  :" $temp
if test -n str86_64; then
    gcc -DSPEED3 -O3 -w -DBLAKE2B blake2b.o bench-crypto.c && (time -p ./a.out) >$temp 2>&1 && print "blake2:" $temp
fi
gcc -DSPEED3 -O3 -w -DMUM512 -I../ bench-crypto.c && (time -p ./a.out) >$temp 2>&1 && print "mum512:" $temp

echo +++10000-byte speed '(5M texts)':
gcc -DSPEED4 -O3 -w -DSHA2 sha512.o byte_order.o bench-crypto.c && (time -p ./a.out) >$temp 2>&1 && print "sha512:" $temp
gcc -DSPEED4 -O3 -w -DSHA3 sha3.o byte_order.o bench-crypto.c && (time -p ./a.out) >$temp 2>&1 && print "sha3  :" $temp
if test -n str86_64; then
    gcc -DSPEED4 -O3 -w -DBLAKE2B blake2b.o bench-crypto.c && (time -p ./a.out) >$temp 2>&1 && print "blake2:" $temp
fi
gcc -DSPEED4 -O3 -w -DMUM512 -I../ bench-crypto.c && (time -p ./a.out) >$temp 2>&1 && print "mum512:" $temp

rm -rf ./a.out $temp sha512.o sha3.o byte_order.o blake2b.o
