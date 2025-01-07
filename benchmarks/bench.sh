#!/bin/bash

# Benchmarking different hash functions.

temp=__hash-temp.out
temp2=__hash-temp2.out
temp3=__hash-temp3.out

COPTFLAGS=${COPTFLAGS:--O3}
if test `uname -m` == x86_64; then
    COPTFLAGS=`echo $COPTFLAGS -march=native`
elif test `uname -m` == ppc64; then
    COPTFLAGS=`echo $COPTFLAGS -mcpu=native`
fi
LTO=${LTO:--flto}
CC=${CC:-cc}
CXX=${CXX:-c++}

if test x${MUM_ONLY} == x; then
    echo compiling Spooky
    ${CXX} ${COPTFLAGS} ${LTO} -w -c Spooky.cpp || exit 1
    echo compiling City
    ${CXX} ${COPTFLAGS} ${LTO} -w -c City.cpp || exit 1
    echo compiling t1ha
    ${CC} ${COPTFLAGS} ${LTO} -w -c t1ha/src/t1ha*.c || exit 1
    echo compiling metrohash64
    ${CXX} ${COPTFLAGS} ${LTO} -w -c metrohash64.cpp || exit 1
    echo compiling SipHash24
    ${CC} ${COPTFLAGS} ${LTO} -w -c siphash24.c || exit 1
fi

rm -f $temp3

percent () {
    val=`awk "BEGIN {if ($2==0) print \"Inf\"; else printf \"%.2f\n\", $1/$2;}"`
    echo "$val"
    echo "$3:$val" >>$temp3
}

skip () {
    l=$1
    n=$2
    while test $l -le $n; do echo -n " "; l=`expr $l + 1`; done
}

print_time() {
    title="$1"
    secs=$2
    printf '%8.2f |' `percent $base_time $secs "$title"`
}

TASKSET=""
if type taskset >/dev/null 2>&1;then TASKSET="taskset -c 0";fi
echo $TASKSET

run () {
  title=$1
  program=$2
  flag=$3
  ok=
  if (time -p $TASKSET $program) >$temp 2>$temp2; then
      ok=y
      (time -p $TASKSET $program) >$temp 2>>$temp2
      (time -p $TASKSET $program) >$temp 2>>$temp2
  fi
  if test x$ok = x;then echo $program: FAILED; return 1; fi
  secs=`grep -E 'user[ 	]*[0-9]' $temp2 | sed s/.*user// | sed s/\\t// | sort -n | head -1`
  if test x$flag != x;then base_time=$secs;fi
  print_time "$title" $secs
}

echo -n '| Length  | MUM-V3  | MUM-V1  | MUM-V2  | Spooky  |  City   | xxHash  | xxHash3 |  t1ha2  |SipHash24|  Metro  |'
if test `uname -m` == x86_64; then echo 'MeowHash |'; else echo; fi
echo -n ':---------|--------:|--------:|--------:|--------:|--------:|--------:|--------:|--------:|--------:|--------:|'
if test `uname -m` == x86_64; then echo '--------:|'; else echo; fi

for i in 3 4;do # 5 6 7 8 9 10 11 12 13 14 15 16 32 64 128 256 0;do
    if test $i == 0; then echo -n '| Bulk    |'; else printf '|%3d bytes|' $i;fi
    ${CXX} -DDATA_LEN=$i ${COPTFLAGS} -w -fpermissive -DMUM -I../ bench.c && run "00MUM-V3" "./a.out" first
    ${CXX} -DDATA_LEN=$i ${COPTFLAGS} -w -fpermissive -DMUM -DMUM_V1 -I../ bench.c && run "01MUM-V1" "./a.out"
    ${CXX} -DDATA_LEN=$i ${COPTFLAGS} -w -fpermissive -DMUM -DMUM_V2 -I../ bench.c && run "02MUM-V2" "./a.out"
    if test x${MUM_ONLY} == x; then
	${CXX} -DDATA_LEN=$i ${COPTFLAGS} ${LTO} -w -fpermissive -DSpooky Spooky.o bench.c && run "03Spooky" "./a.out"
	${CXX} -DDATA_LEN=$i ${COPTFLAGS} ${LTO} -w -fpermissive -DCity City.o bench.c && run "04City" "./a.out"
	${CXX} -DDATA_LEN=$i ${COPTFLAGS} ${LTO} -w -fpermissive -DxxHash bench.c && run "05xxHash" "./a.out"
	${CXX} -DDATA_LEN=$i ${COPTFLAGS} ${LTO} -w -fpermissive -Dxxh3 bench.c && run "06xxh3" "./a.out"
	${CC} -DDATA_LEN=$i ${COPTFLAGS} ${LTO} -w -fpermissive -It1ha -DT1HA2 t1ha*.o bench.c && run "07t1ha2" "./a.out"
	${CC} -DDATA_LEN=$i ${COPTFLAGS} ${LTO} -w -fpermissive -DSipHash siphash24.o bench.c && run "08Siphash24" "./a.out"
	${CXX} -DDATA_LEN=$i ${COPTFLAGS} ${LTO} -w -fpermissive -DMETRO metrohash64.o -I../ bench.c && run "09Metro" "./a.out"
	if test `uname -m` == x86_64; then
	    # Don't do LTO for MUM or MeowHash.  Otherwise it will be optimized too much and results will be too good on some targets
	    ${CXX} -DDATA_LEN=$i ${COPTFLAGS} -w -fpermissive -mavx2 -maes -DUseMeowHash -I../ bench.c && run "10Meowhash" "./a.out"
	fi
    fi
    echo
done

echo -n '| Average |'
for i in `awk -F: '{print $1}' $temp3|sort|uniq`; do
    printf '%8.2f |' `awk -F: -v name="$i" 'name==$1 {f = f + $2; n++} END {printf "%0.2f\n", f / n}' $temp3`
done
echo

echo -n '| Geomean |'
for i in `awk -F: '{print $1}' $temp3|sort|uniq`; do
    printf '%8.2f |' `awk -F: -v name="$i" 'BEGIN{f=1.0} name==$1 {f = f * $2; n++} END {printf "%0.2f\n", exp (log(f)/n)}' $temp3`
done
echo

rm -rf ./a.out $temp $temp2 $temp3 Spooky.o City.o siphash24.o t1ha*.o
