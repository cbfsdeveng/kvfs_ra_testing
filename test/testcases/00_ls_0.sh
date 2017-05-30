#! /bin/bash

((FAILED=0))
((SUCCEEDED=0))
#drwxrwxr-x 2 ubuntu ubuntu 4096 Apr 21 02:54 /tmp/mount

read -a OUT <<< "$(ls -ld mount)"

echo "ls output: ${OUT[*]}"

if echo ${OUT[0]} | grep -q "^drwxr.xr.x"
then
((++SUCCEEDED))
else
echo "root dir: wrong mode or type: ${OUT[0]}"
((++FAILED))
fi

if [ ${OUT[1]} -lt 1 -o ${OUT[1]} -gt 2 ]
then
echo "root dir: bad link count : ${OUT[1]}"
((++FAILED))
else
((++SUCCEEDED))
fi

if [ -z "$( find mount -ctime -1 )" ]
then
echo "root dir: mount point ctime too old? ${OUT[*]}"
((++FAILED))
else
((++SUCCEEDED))
fi

#####
echo "Done with $0: $SUCCEEDED succeeded, $FAILED failed"
if ((FAILED > 0))
then
    exit 1
fi
exit 0
