#! /bin/bash

((FAILED=0))
((SUCCEEDED=0))

set -x
if :> mount/chmod_file
then
((++SUCCEEDED))
else
echo "$0: create failed?"
((++FAILED))
fi

read -a OUT <<< "$(ls -ld mount/chmod_file)"
echo "ls output: ${OUT[*]}"
if echo ${OUT[0]} | grep -q '^-rw.r..r..'
then
((++SUCCEEDED))
else
echo "wrong mode or type: ${OUT[0]}"
((++FAILED))
fi

if chmod 600 mount/chmod_file
then
((++SUCCEEDED))
else
echo "chmod error"
((++FAILED))
fi

read -a OUT <<< "$(ls -ld mount/chmod_file)"
echo "ls output: ${OUT[*]}"
if echo ${OUT[0]} | grep -q '^-rw-------'
then
((++SUCCEEDED))
else
echo "after 'chmod 600', wrong mode or type: ${OUT[0]}"
((++FAILED))
fi

if rm mount/chmod_file
then
((++SUCCEEDED))
else
echo "failed to remove file."
((++FAILED))
fi

if ls mount/chmod_file 2>&1 | grep "No such file or directory"
then
((++SUCCEEDED))
else
echo "mount/chmod_file still exists."
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
