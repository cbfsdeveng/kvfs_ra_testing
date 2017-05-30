#! /bin/bash
set -x

# makefiles aren't very well integrated.
make clean ; make
( cd .. ; make clean ; make )
make

while :
do
    rm -rf /tmp/proxy_dir/ /tmp/seg_* mykvfs.db/
    s3cmd del s3://pqfs-test-0/proxy_dir/0/*
    if ! ./tiering_out_test > /tmp/tot.log 2>&1
    then
        break
    fi
    mv /tmp/tot.log /tmp/tot.$((LOG_NUM++)).log
done
