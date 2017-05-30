#! /bin/bash
set -x
# TODO: incomplete test - just sends evict request to local server,
# doesn't check results. At best would detect crash or corruption.

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
    sleep 2
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
    echo "FAILURE: SNAPSHOT DID NOT FINISH"
    ((++FAILED))
    else
    ((++SUCCEEDED))
    fi
    ls -lrt proxy_dir/0/*.tgz
}

Evict() {
    WEB_SERVER_PORT="${WEB_SERVER_PORT-8080}"
    wget "http://localhost:${WEB_SERVER_PORT}/EVICT" -O evict.out
    cat evict.out
}

WaitEvict() {
    EVICT_FINISHED="false"
    for (( i = 0 ; i < 10 ; i++))
    do
        sleep 1
        # When eviction is complete, the stat range start and end should match the
        # eviction range end.
        # { "Result": "/evict tiering_next_xid=1234 log_next_xid=2264 eviction_next_xid=0" }
        # { "Result": "/STAT tiering_next_xid=2264 log_next_xid=2264 eviction_next_xid=0" }
        wget "http://localhost:${WEB_SERVER_PORT}/STAT" -O stat.out
        cat stat.out
        TIERED_TO=$(sed 's/.* tiered_out_xid=\([0-9]*\).*/\1/' stat.out)
        EVICTED_TO=$(sed 's/.* eviction_next_xid=\([0-9]*\).*/\1/' stat.out)
        if (( EVICTED_TO >= TIERED_TO ))
        then
            EVICT_FINISHED="true"
            break;
        fi
    done
    if [ "$EVICT_FINISHED" = "false" ]
    then
    echo "FAILURE: EVICTION DID'T FINISH"
    ((++FAILED))
    else
    ((++SUCCEEDED))
    fi
}

# TODO(joe): move this and other generally useful funcs into library.
DbStats() {
    WEB_SERVER_PORT="${WEB_SERVER_PORT-8080}"
    # cbfs dbstats > dbstats.out
    wget "http://localhost:${WEB_SERVER_PORT}/DBSTATS" -O dbstats.out
    cat dbstats.out
}

if mkdir mount/evict_dir
then
((++SUCCEEDED))
else
echo "$0: mkdir failed?"
((++FAILED))
fi

# Make a few random files (first in tmp to compare later).
echo "Before creating evict_files, db:" ; DbStats
mkdir -p tmp/

for F in evict_file.{1,2,3}
do
    if dd if=/dev/urandom of=tmp/$F bs=8k count=128
    then
    ((++SUCCEEDED))
    else
    echo "$0: create or write of tmp/$F failed"
    ((++FAILED))
    fi
done

for F in tmp/evict_file.*
do
    if cp -p $F mount/evict_dir
    then
    ((++SUCCEEDED))
    else
    echo "$0: copy of" $F "failed"
    ((++FAILED))
    fi

    if cmp -s tmp/$F mount/evict_dir/$F
    then
    ((++SUCCEEDED))
    else
    echo "$0: compare of" mount/evict_dir/$F "failed"
    ((++FAILED))
    fi

    Snapshot; WaitSnapshot
    Evict; WaitEvict;
done


# Check that there are snapshots in our proxy-cache, then remove them.

ls -lrt proxy_dir/0/
if ls proxy_dir/0/ | grep -s 'tb_.*\.tgz'
then
((++SUCCEEDED))
else
echo "$0: tarballs missing from proxy_dir/0"
ls -lr
((++FAILED))
fi

# Remove tarballs and their extracted forms.
rm -v proxy_dir/0/tb_*-*.tgz
rm -rv seg_[0-9]*-[0-9]*

echo "After evict ls:" 
ls -li mount/evict_dir/evict_file.*
###

for F in evict_file.{1,2,3}
    do
    if cmp -s tmp/$F mount/evict_dir/$F
    then
    ((++SUCCEEDED))
    else
    echo "$0: compare of" mount/evict_dir/$F "failed"
    ((++FAILED))
    fi
done

if rm mount/evict_dir/evict_file.* && rmdir mount/evict_dir
then
((++SUCCEEDED))
else
echo "Cleanup failed."
((++FAILED))
fi

rm tmp/evict_file.*

ls -lrt proxy_dir/0/

if ls proxy_dir/0/ | grep -s 'tb_.*\.tgz'
then
# Successfully copied back from cloud during our compares.
# (also indicates that a fault-in happened).
((++SUCCEEDED))
else
echo "$0: after tiering-in, tarballs missing from proxy_dir/0"
((++FAILED))
fi

echo "Done with $0: $SUCCEEDED succeeded, $FAILED failed"
echo 
if ((FAILED > 0))
then
    exit 1
fi
exit 0
