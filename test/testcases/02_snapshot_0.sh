#! /bin/bash
set -x

((FAILED=0))
((SUCCEEDED=0))

Snapshot() {
    WEB_SERVER_PORT="${WEB_SERVER_PORT-8080}"
    # cbfs snapshot create blah > snap.out
    wget "http://localhost:${WEB_SERVER_PORT}/SNAP" -O snap.out
    cat snap.out
}

WaitSnapshot() {
# TODO: Wait for proxydir to be done somehow? wget .../STAT until # start==end
# in the snap.out?
    SNAP_FINISHED="false"
    for (( i = 0 ; i < 10 ; i++))
    do
    sleep 1
# When snap is complete, the stat range start and end should match the snap range end.
# (the range end may have advanced).
# { "Result": "/SNAP tiering_next_xid=1234 log_next_xid=2264 eviction_next_xid=0" }
# { "Result": "/STAT tiering_next_xid=2264 log_next_xid=2264 eviction_next_xid=0" }
    # cbfs snapshot status blah > stat.out
    wget "http://localhost:${WEB_SERVER_PORT}/STAT" -O stat.out
    cat stat.out
    SNAP_END=$(sed 's/.* log_last_xid=\([0-9]*\).*/\1/' snap.out)
# TODO: shouldn't be necessary. Bug in when stat changes?
#sleep 1
    TIERED_TO=$(sed 's/.* tiered_out_xid=\([0-9]*\).*/\1/' stat.out)
    if (( TIERED_TO >= SNAP_END ))
    then
        SNAP_FINISHED="true"
        break
    fi
    done
    if [ "$SNAP_FINISHED" = "false" ]
    then
    ((++FAILED))
    else
    ((++SUCCEEDED))
    fi
    ls -lrt proxy_dir/0/*.tgz
}

Extract() {
# Only most recent tarball / snapshot.
    rm -rf extract
    mkdir extract
    ls -lrt proxy_dir/0/*.tgz
    TARBALL=$(ls -rt proxy_dir/0 | grep '\.tgz' | tail -1)
    for T in proxy_dir/0/*.tgz
    do
        (  cd extract ; tar xvf ../$T )
    done
    ls -l extract
}

Compare() {
    if diff -r mount extract |grep '^Only in mount'
    then
    echo "mismatch on extracted snapshot."
    ((++FAILED))
    else
    ((++SUCCEEDED))
    fi
}

Snapshot
WaitSnapshot
Extract
Compare

if mkdir mount/snap_dir
then
((++SUCCEEDED))
else
echo "$0: mkdir failed?"
((++FAILED))
fi

if echo hello > mount/snap_dir/snap_file
then
((++SUCCEEDED))
else
echo "$0: create or write of mount/snap_file failed"
((++FAILED))
fi

Snapshot
WaitSnapshot
Extract
Compare

if rm mount/snap_dir/snap_file && rmdir mount/snap_dir
then
((++SUCCEEDED))
else
echo "Cleanup failed."
((++FAILED))
fi

Snapshot
WaitSnapshot
Extract
Compare

#####
echo "Done with $0: $SUCCEEDED succeeded, $FAILED failed"
echo 
if ((FAILED > 0))
then
    exit 1
fi
exit 0
