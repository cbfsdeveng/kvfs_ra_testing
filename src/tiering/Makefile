include ../common.mk

CPPFLAGS+= -I .
CPPFLAGS+= -I ..

SOURCES=\
        cloud_proxy.cc \
        eviction_agent.cc \
        file_test_util.cc \
        fs_change_log.cc \
        index_manager.cc \
        restful_web_server.cc \
        segment.cc \
	snapshot_persister.cc \
        tiered_in_file.cc \
        tiering_in_agent.cc \
        tiering_out_agent.cc \
	web_controller.cc \

#
WEB_SERVER_OBJECTS=mongoose.o

LIBS=libtiering.a

# (note: need to do a build in .. first).
OBJECTS=$(SOURCES:.cc=.o) change_log.pb.o index.pb.o $(WEB_SERVER_OBJECTS)
EXECUTABLE=tiering_out_test
GEN_FILES=change_log.pb.h change_log.pb.cc index.pb.h index.pb.cc

all: $(LIBS) $(GEN_FILES) $(SOURCES)

# Overkill but safe.
$(SOURCES): $(GEN_FILES)

# for debugging only.
$(EXECUTABLE): $(OBJECTS) test_main.o
	rm -f ../KVFS.o ../fuse_kvfs_main.o  # hack
	$(CC) $(LINKER_OPTS) $(OBJECTS) ../*.o $(LDFLAGS) -o $@

libtiering.a: $(OBJECTS)
	rm -f libtiering.a
	ar -cvq  libtiering.a $(OBJECTS)

change_log.pb.o: change_log.pb.cc

# TODO: Must be a better way than explicit CXX.
$(WEB_SERVER_OBJECTS): ../../thirdparty/mongoose/mongoose.c
	$(CXX) -c $(CXXFLAGS) $(CPPFLAGS) $< -o $@

# To install the proto stuff on ubuntu:
# apt-get install libprotobuf-dev
# apt-get install protobuf-compiler

%.pb.h %.pb.cc: %.proto
	protoc $< --cpp_out=.

clean:
	rm -rf $(EXECUTABLE) *.o *.db  2> /dev/null
	rm -f $(GEN_FILES)
	rm -f libtiering.a
