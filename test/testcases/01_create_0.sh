#! /bin/bash

((FAILED=0))
((SUCCEEDED=0))

set -x
if :> mount/file
then
((++SUCCEEDED))
else
echo "$0: create failed?"
((++FAILED))
fi

read -a OUT <<< "$(ls -ld mount/file)"

echo "ls output: ${OUT[*]}"

if echo ${OUT[0]} | grep -q "^-rw.r..r.."
then
((++SUCCEEDED))
else
echo "wrong mode or type: ${OUT[0]}"
((++FAILED))
fi

if [ ${OUT[1]} -eq 1 ]
then
((++SUCCEEDED))
else
echo "file: bad link count : ${OUT[1]}"
((++FAILED))
fi

if find mount -ctime +1 |grep -q mount/file
then
echo "file ctime too old? ${OUT[*]}"
((++FAILED))
fi

if rm mount/file
then
((++SUCCEEDED))
else
echo "failed to remove file."
((++FAILED))
fi

if ls mount/file 2>&1 | grep "No such file or directory"
then
((++SUCCEEDED))
else
echo "mount/file still exists."
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
