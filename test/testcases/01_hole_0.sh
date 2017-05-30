#! /bin/bash
seek_write()
{
if dd oflag=seek_bytes conv=notrunc seek=$1 of=$2
then
    ((++SUCCEEDED))
else
    echo "$0: create or write of mount/hole failed"
    ((++FAILED))
fi
}

# check_size file size error_str
check_size()
{
read -a OUT <<< "$(ls -ld $1)"
#-rwxrwxr-x 1 ubuntu ubuntu 1134 Apr 29 04:12 build.sh
if [ ${OUT[4]} -eq $2 ]
then
    ((++SUCCEEDED))
else
    echo "ls output: ${OUT[*]}"
    echo "check_size: $1 expect $2 got ${OUT[4]} $3"
    ((++FAILED))
fi
}

check_cmp() # file1 file2 err
if cmp $1 $2
then
((++SUCCEEDED))
else
echo "File content mismatch $3"
((++FAILED))
fi

set -x
((FAILED=0))
((SUCCEEDED=0))

truncate --size=1000000 mount/hole
truncate --size=1000000 hole
check_size mount/hole 1000000 "after trunc 1M"

echo hole | seek_write 200 mount/hole
echo hole | seek_write 200 hole
check_size mount/hole 1000000

check_cmp hole mount/hole

truncate --size=202 mount/hole
truncate --size=202 hole
check_size mount/hole 202
check_cmp hole mount/hole

rm mount/hole hole

#####
echo "Done with $0: $SUCCEEDED succeeded, $FAILED failed"
echo 
if ((FAILED > 0))
then
    exit 1
fi
exit 0
