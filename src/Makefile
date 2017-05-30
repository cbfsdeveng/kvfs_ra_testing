include common.mk

SOURCES=\
	Cycles.cc \
        dir_entry_adder.cc \
	FileSystemManager.cc \
	fuse_err_ops.cc \
	fuse_kvfs_ops.cc \
	hexdump.cc \
	KVFSDirectoryPartition.cc \
	KVFSFile.cc \
	kvfs_mount.cc \
	KVFSObject.cc \
	LevelDBFactory.cc \
	LevelDBStore.cc \
	RocksDBFactory.cc \
	RocksDBStore.cc \
	Status.cc \
	WallTime.cc \

#       KVFSDirectory.cc \


OBJECTS=$(SOURCES:.cc=.o) super.pb.o tiering/libtiering.a config/libconfig.a

EXECUTABLES=fuse_kvfs

all: $(SOURCES) $(EXECUTABLES)

#$(SOURCES): super.pb.h

fuse_kvfs: fuse_kvfs_main.o $(OBJECTS) 
	$(CXX) $(CXXFLAGS) fuse_kvfs_main.o $(OBJECTS)  $(LDFLAGS) -lsnappy -o $@

# Early testing code - for history.
#kvfs: KVFS.o $(OBJECTS)
#	$(CC) KVFS.o $(OBJECTS)  $(LDFLAGS) -lfuse -o $@

super.pb.o: super.pb.cc
super.pb.h super.pb.cc: super.proto  # why necessary?

%.pb.h %.pb.cc: %.proto
	protoc $< --cpp_out=.

clean:
	rm -rf $(EXECUTABLES) *.o 2> /dev/null
#	rm -rf $(EXECUTABLES) *.o super.pb.* 2> /dev/null
