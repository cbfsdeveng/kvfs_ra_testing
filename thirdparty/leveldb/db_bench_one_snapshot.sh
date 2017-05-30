#! /bin/bash
set -x
# The default set of benchmarks from db_bench.cc, with snapshots added.

BENCHMARKS=\
fillseq,\
fillsync,\
fillrandom,\
snapshot,\
overwrite,\
readrandom,\
readrandom,\
readseq,\
readreverse,\
unsnapshot,\
compact,\
readrandom,\
readseq,\
readreverse,\
fill100K,\
crc32c,\
snappycomp,\
snappyuncomp,\
acquireload,


for RUN in  1 2 3
do
    # First without snapshots
    ./db_bench  --benchmarks="$(echo $BENCHMARKS |sed 's/u*n*snapshot,//g')"

    # Now, with snapshots.
    ./db_bench  --benchmarks="$BENCHMARKS"
done

