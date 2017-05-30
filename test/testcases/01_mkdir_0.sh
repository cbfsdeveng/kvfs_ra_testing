#! /bin/bash

((FAILED=0))
((SUCCEEDED=0))

if mkdir -p mount/dir/subdir
then
((++SUCCEEDED))
else
echo "$0: mkdir failed?"
((++FAILED))
fi

read -a OUT <<< "$(ls -ld mount/dir)"

echo "ls output: ${OUT[*]}"

if echo ${OUT[0]} | grep -q "^drwxr.xr.x"
then
((++SUCCEEDED))
else
echo "wrong mode or type: ${OUT[0]}"
((++FAILED))
fi

if [ ${OUT[1]} -ne 3 ]
then
echo "dir: bad link count (expected 3): ${OUT[1]}"
((++FAILED))
else
((++SUCCEEDED))
fi

if find mount -ctime +1 |grep -q mount/dir
then
echo "dir ctime too old? ${OUT[*]}"
((++FAILED))
fi

if rmdir mount/dir/subdir
then
((++SUCCEEDED))
else
echo "failed to remove subdir"
((++FAILED))
fi

if rmdir mount/dir
then
((++SUCCEEDED))
else
echo "failed to remove dir"
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
