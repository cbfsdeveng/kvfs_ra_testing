#! /bin/bash
# Create and remove files of the same name (specific for a readdir bug).

((FAILED=0))
((SUCCEEDED=0))

:> mount/file1
rm mount/file1
:> mount/file1
NUM_FILE1=$(ls -l mount | grep -c file1)
if [ $NUM_FILE1 -eq 1 ]
then
((++SUCCEEDED))
else
echo "Wrong number of copies of file1 in mount: ${NUM_FILE1}"
ls -l mount
((++FAILED))
fi

rm mount/file1

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
