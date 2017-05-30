#! /bin/bash
# + build from scratch
# + run with heap_check=draconian.
# + caching disabled (is this ok for all correctness tests?).
# + isolate: run in its own empty dir to start
# + create some files, verify contents,
# + take snapshot
# + more changes
# + more snapshots
# + extract snapshots into empty dir,
# + verify contents.
set -x
export ORIG_DIR=$(pwd)
# Make sure cores can be created.
ulimit -c unlimited

cd ..
pwd
# TODO: build Options.
# NO_CLEAN non-empty makes build faster (doesn't do clean first).
# "NO_CLEAN=false sh this-script.sh" (sorry) to force clean.
export NO_CLEAN=${NO_CLEAN:-true}
if  [ ! -z "$RUN_FAST" ] ; then
echo RUN_FAST
    export BUILD_MODE=release
else
echo NOT RUN_FAST
fi
if  [ ! -z "$RUN_FAST" ] ; then
echo RUN_FAST
    export BUILD_MODE=release
else
echo NOT RUN_FAST
fi
if ! $ORIG_DIR/build.sh 
then
echo "ERROR: build failed."
exit 1
fi

export SRC_DIR=$(pwd)/src
export FUSE_KVFS=$(pwd)/src/fuse_kvfs
export TEST_DIR=$( mktemp -d '/tmp/kvfs_work.XXXXXXXX' )
export TEST_FS_BASE=$(echo $TEST_DIR | sed 's|.*/||' )

export PQFS_CONFIG_FILE=${PQFS_CONFIG_FILE-test/test_config.proto.txt}
export CONFIG_FILE_LOCAL_PATH=$( echo ${PQFS_CONFIG_FILE}  | sed 's,.*/,,' )

cd $TEST_DIR
export GLOG_log_dir=$TEST_DIR
export GLOG_use_rocksdb=1   # 0=>LEVELDB, 1=>ROCKSDB
export GLOG_v=${GLOG_v-3}   # Verbose logging.
export GLOG_minloglevel=${GLOG_minloglevel-2}   # 0=>INFO, 1=>WARN, 2=>ERROR
#export GLOG_logtostderr=0
#export GLOG_stderrthreshold=2

export WEB_SERVER_PORT=${WEB_SERVER_PORT-8081}

is_mounted() {
# grep exits w true when it matches (when mount point is fuse_kvfs).
# grep -q...
    df $1 | grep fuse_kvfs
}

M=$TEST_DIR/mount
mkdir $M

# We call the single threaded event loop anyway, so this is ignored.
# SINGLE_THREADED=-s

export CPUPROFILE=${CPUPROFILE-/tmp/fuse_kvfs.prof}

# Set RUN_FAST environment variable to disable these (slow) debugging options.
if  [ -z "$RUN_FAST" ] ; then
# Ideally we'd test with & without, but evict test requires no cache to
# verify cloud interaction.
export FUSE_CACHE_ENABLE=false

# setting HEAPCHECK to {normal,draconian} here checks the whole program, but I
# don't care about some of the junk that thirdparty routines leave around (for
# now). Default here will just check the code bracketed by checker in main().
export HEAPCHECK=${HEAPCHECK-normal}
export HEAP_CHECK_MAX_LEAKS=100
export PERFTOOLS_VERBOSE=2
export GLOG_minloglevel=0
export GLOG_verbose=3
fi

# Create our config file from a template by substituting the above
# environment variables.
# envsubst < ${PQFS_CONFIG_FILE} > $TEST_DIR/${CONFIG_FILE_LOCAL_PATH}
# Ideally we'd test with & without, but evict test requires no cache to
# verify cloud interaction.
export FUSE_CACHE_ENABLE=false

$ORIG_DIR/gen_config.sh $TEST_DIR/${CONFIG_FILE_LOCAL_PATH}

TIME_CMD=${TIME_CMD-/usr/bin/time}
echo "Starting server in ${PWD}"
#        --cloud_proxy_dir="s3:CBFS/${TEST_FS_BASE}/proxy"
$TIME_CMD $FUSE_KVFS \
        $FUSE_OPTS \
export GLOG_verbose=3
FUSE_DEBUG=${FUSE_DEBUG-"-d"}
        $SINGLE_THREADED \
        $FUSE_DEBUG \
# Create our config file from a template by substituting the above
# environment variables.
# envsubst < ${PQFS_CONFIG_FILE} > $TEST_DIR/${CONFIG_FILE_LOCAL_PATH}
$ORIG_DIR/gen_config.sh $TEST_DIR/${CONFIG_FILE_LOCAL_PATH}

TIME_CMD=${TIME_CMD-/usr/bin/time}
echo "Starting server in ${PWD}"
        --minloglevel=${GLOG_minloglevel} \
        --use_rocksdb=${GLOG_use_rocksdb} \
        --config_file=${CONFIG_FILE_LOCAL_PATH} \
        $M > fuse_kvfs.out 2>&1 &

for (( t = 0; t < 4; t++)) 
do
    if is_mounted $M
    then
    echo "$M Mounted"
    break
    fi
    sleep $t
done

if ! is_mounted $M
then
    cat fuse_kvfs.out
    echo "$M/tmp never appeared. fuse_kvfs not running?"
    echo "Exiting without running sub-shell."
    exit 1
fi

SHELL_ARGS="$*"
if [ -z "$SHELL_ARGS" ]
then
    cat <<!
Running $SHELL in a testing dir $PWD.
kvfs mount point in subdir "mount".
web api available on http://localhost:$WEB_SERVER_PORT
Exit shell when done (will unmount).
!
    $SHELL
    SHELL_STATUS=$?
else
    $SHELL -x -c $SHELL_ARGS
    SHELL_STATUS=$?
fi

if [ -z "$LEAVE_MOUNTED" ]
then
    #sudo umount $M
    fusermount -u $M
    wait -n
    STATUS="$?"
    if [ $STATUS != 0 ]
    then
        echo "ERROR: $FUSE_KVFS exited with status $STATUS"
        exit $STATUS
    fi
else
    echo "Leaving $M mounted."
fi
exit $SHELL_STATUS
