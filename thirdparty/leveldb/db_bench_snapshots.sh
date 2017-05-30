#! /bin/bash
set -x
# The default set of benchmarks from db_bench.cc, with snapshots added.

BENCHMARKS=\
fillseq,\
snapshot,\
fillsync,\
snapshot,\
fillrandom,\
snapshot,\
overwrite,\
snapshot,\
readrandom,\
readrandom,\
readseq,\
readreverse,\
snapshot,\
compact,\
readrandom,\
unsnapshot,\
unsnapshot,\
readseq,\
unsnapshot,\
readreverse,\
fill100K,\
unsnapshot,\
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

