include ../common.mk

#CPPFLAGS+= -I ..

SOURCES=\
config_manager.cc

LIBS=libconfig.a


# (note: need to do a build in .. first).
OBJECTS=$(SOURCES:.cc=.o) config.pb.o
GEN_FILES=config.pb.h config.pb.cc

all: $(LIBS) $(GEN_FILES) $(SOURCES)

# Overkill but safe.
$(SOURCES): $(GEN_FILES)

EXECUTABLE=config_test
# for debugging only.
$(EXECUTABLE): $(OBJECTS) test_main.o
	$(CXX) $(CXXFLAGS) $(LINKER_OPTS) $(OBJECTS) test_main.o $(LDFLAGS) -o $@

libconfig.a: $(OBJECTS)
	rm -f libconfig.a
	ar -cvq  libconfig.a $(OBJECTS)

config.pb.o: config.pb.cc

# To install the proto stuff on ubuntu:
# apt-get install libprotobuf-dev
# apt-get install protobuf-compiler

%.pb.h %.pb.cc: %.proto
	protoc $< --cpp_out=.
	protoc $< --python_out=../cli/

clean:
	rm -rf $(EXECUTABLE) *.o *.db  2> /dev/null
	rm -f $(GEN_FILES)
	rm -f $(LIBS)
