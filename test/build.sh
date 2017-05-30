#!/bin/bash
# Top level script to build everything
# binary is in src directory and is called fuse_kvfs
# Linux ip-172-31-18-200 3.13.0-48-generic #80-Ubuntu SMP Thu Mar 12 11:16:15 UTC 2015 x86_64 x86_64 x86_64 GNU/Linux
set -x
export TOP_DIR=$PWD
echo "TOPDIR="$TOP_DIR
export GOPATH=$TOP_DIR/thirdparty/cloudstoreadaptor/

MAKE="make BUILD_MODE=${BUILD_MODE-debug} ${EXTRA_MAKE_FLAGS}"

if [ -z "$NO_CLEAN"  -o "$NO_CLEAN" == "false" ]
then
    MAKE_CLEAN="$MAKE clean"
else
    MAKE_CLEAN="echo 'Skipping make clean'"
fi

# compile 3rd party tools

cd $TOP_DIR/thirdparty/cityhash
if [ ! -e /usr/local/lib/libcityhash.so ]; then
	#make clean
	./configure
	make
	sudo make install
	sudo /sbin/ldconfig
else
	echo "skipping building cityhash"
fi

if [ ! -e /usr/local/lib/librocskdb.so.1 ]; then
	cd $TOP_DIR/thirdparty/rocksdb
	#make clean
	make
	sudo cp librocksdb* /usr/local/lib
	sudo /sbin/ldconfig
else
	echo "skipping building rocksdb"
fi

if [ ! -e /usr/local/lib/libleveldb.so.1 ]; then
	cd $TOP_DIR/thirdparty/leveldb
	#make clean
	make
	sudo cp libleveldb* /usr/local/lib
	sudo /sbin/ldconfig
else
	echo "skipping building leveldb"
fi

if [ ! -e /usr/bin/rclone ]; then
        cd $TOP_DIR/thirdparty/cloudstoreadaptor/src/github.com/ncw/rclone
        go build
        sudo install ./rclone /usr/bin
fi

if [ ! -e $HOME/.rclone.conf ]; then
        cp $TOP_DIR/thirdparty/conf/cbfs.rclone.conf $HOME/.rclone.conf
fi

echo "install CBFS cli"
cd $TOP_DIR/src/cli
sudo pip install .

SRC_DIRS="\
    $TOP_DIR/test/qa/fsx\
    $TOP_DIR/src/config\
    $TOP_DIR/src/tiering\
    $TOP_DIR/src"

for DIR in $SRC_DIRS
do
cd $DIR
$MAKE_CLEAN || exit 1
done


cd $TOP_DIR/test/qa/fsx
$MAKE || exit 1

cd $TOP_DIR/src/config
$MAKE || exit 1

cd $TOP_DIR/src
# Hack! tiering depends on a proto here. fix dependendencies for real.
$MAKE sources

cd $TOP_DIR/src/tiering
$MAKE || exit 1

cd $TOP_DIR/src
$MAKE || exit 1

cat <<!
example invocation:
src/fuse_kvfs -s /tmp/mount

This will run the fuse-process in the foreground,
serving requests under /tmp/mount

(There will be logs and working data in /tmp)

Once some work has been done under /tmp/mount,
you can trigger a snapshot through a simple restful api:
wget http://localhost:8080/SNAP -O -
!
