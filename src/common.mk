# common stuff for kvfs related makefiles.

TOP_DIR=$(shell pwd | sed 's,\(.*/kvfs/\).*,\1,' )
# WEXTRA=-Wextra
#CXXFLAGS=-Werror -Wall $(WEXTRA) -g -std=c++11
CXXFLAGS=-Wall $(WEXTRA) -g -std=c++11
ifeq ($(BUILD_MODE),release)
    #optimize release builds.
    CXXFLAGS=-Wall $(WEXTRA) -O3 -std=c++11
endif

# libtcmalloc is installed:
# $ sudo apt-get install libgoogle-perftools-dev google-perftools
# but may need to manually make symlink in /usr/lib (check first)
# $ sudo ln -s libtcmalloc_minimal.so.4.1.2 libtcmalloc.so
# $ HEAPCHECK=normal PPROF_PATH=/usr/bin/google-pprof
# (will run the binary with heap checking turned on to normal level).
# See https://github.com/gperftools/gperftools
LIBTCMALLOC=-ltcmalloc

# Comes with tcmalloc. Use env var CPUPROFILE=/tmp/prof.out to enable.
LIBPROFILER=-lprofiler

CPPFLAGS=-I $(TOP_DIR)/src \
	-I $(TOP_DIR)/src/tiering \
	-I $(TOP_DIR)/thirdparty/rocksdb/include \
	-I $(TOP_DIR)/thirdparty/leveldb/include \
	-I $(TOP_DIR)/thirdparty/mongoose \
	-I /usr/include/fuse \
	-DFUSE_USE_VERSION=26 \
	-DLOCAL_HD=1 -D_FILE_OFFSET_BITS=64

LDFLAGS= \
	-lboost_serialization \
	-lcityhash \
	-lglog \
	-lrocksdb \
	-lleveldb \
	-lprotobuf \
	-lpthread \
        -lfuse  \
        -lz \
         $(LIBPROFILER) \
         $(LIBTCMALLOC) \

