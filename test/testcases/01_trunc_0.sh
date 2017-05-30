#! /bin/bash

set -x
((FAILED=0))
((SUCCEEDED=0))

size() {
    stat --format="%s" $1
}

F=mount/trunc_file
R=./trunc_file
# Make a reference file in (non-kvfs-mount!) current dir.
# We'll make parallel changes to this, and verify against our mount.
echo hello > $R
if cp $R $F
then
((++SUCCEEDED))
else
echo "$0: initial write of $F failed"
((++FAILED))
fi

if [ "$( size $F )"  -eq $( size $R ) ]
then
((++SUCCEEDED))
else
echo "initial size $( size $F ) != $( size $R )"
((++FAILED))
fi

for SIZE in 100 1000 6 3
do
truncate -s $SIZE $R
truncate -s $SIZE $F
if [ "$( size $F )" -eq $SIZE ]
then
((++SUCCEEDED))
else
echo "wrong size after trunc $SIZE: $( size $F )"
((++FAILED))
fi

if cmp -b $R $F
then
((++SUCCEEDED))
else
echo "cmp $R $F failed"
((++FAILED))
fi

#TODO:  mtime / ctime!

done

rm $R
if rm $F
then
((++SUCCEEDED))
else
echo "failed to remove $F"
((++FAILED))
fi

OUT=$(ls mount 2>&1)
if [ -z "$OUT" ]
then
((++SUCCEEDED))
else
echo "mount not empty: $OUT"
((++FAILED))
fi

#####
echo "Done with $0: $SUCCEEDED succeeded, $FAILED failed"
echo 
if ((FAILED > 0))
then
    exit 1
fi
exit 0
