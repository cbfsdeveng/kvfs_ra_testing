#! /bin/bash

((FAILED=0))
((SUCCEEDED=0))

set -x
if echo hello > mount/hello
then
((++SUCCEEDED))
else
echo "$0: create or write of mount/hello failed"
((++FAILED))
fi

read -a OUT <<< "$(ls -ld mount/hello)"

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
echo "hello: bad link count : ${OUT[1]}"
((++FAILED))
fi

if find mount -ctime +1 |grep -q mount/hello
then
echo "hello ctime too old? ${OUT[*]}"
((++FAILED))
fi

#-rwxrwxr-x 1 ubuntu ubuntu 1134 Apr 29 04:12 build.sh
# Check size.
if [ ${OUT[4]} -eq 6 ]
then
((++SUCCEEDED))
else
echo "hello: bad size: ${OUT[4]}"
((++FAILED))
fi

if echo "hello" | diff mount/hello -
then
((++SUCCEEDED))
else
echo "File content mismatch."
((++FAILED))
fi

if rm mount/hello
then
((++SUCCEEDED))
else
echo "failed to remove hello"
((++FAILED))
fi

if ls mount/hello 2>&1 | grep "No such file or directory"
then
((++SUCCEEDED))
else
echo "mount/hello still exists."
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
