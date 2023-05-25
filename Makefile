CC = gcc
CXX = g++

EDCFLAGS := -O2 -Wall -I include/ $(shell pkg-config --cflags cfitsio) $(CFLAGS)
EDCXXFLAGS := -O2 -Wall -I include/ -std=c++11 -fPIC $(shell pkg-config --cflags cfitsio) $(CXXFLAGS)
EDLDFLAGS := -lm -lpthread -lcfitsio -lusb-1.0 -latomic $(shell pkg-config --libs cfitsio) $(LDFLAGS)

LIBASIDIR = lib/armv7/
ifeq ($(OS), Windows_NT)
	$(error Windows is not supported.)
else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S), Linux)
		UNAME_P := $(shell uname -m)
		ifeq ($(UNAME_P), x86_64)
			LIBASIDIR = lib/x64/
		endif
		ifeq ($(UNAME_P), x86)
			LIBASIDIR = lib/x86/
		endif
	endif
	ifeq ($(UNAME_S), Darwin)
		LIBASIDIR = lib/mac/
	endif
endif

LIBASISTATIC = $(LIBASIDIR)/libASICamera2.a

LIBTARGET = lib/libCameraUnit_ASI.a

CSRCS := $(wildcard src/*.c)
COBJS := $(patsubst %.c,%.o,$(CSRCS))
CDEPS := $(patsubst %.c,%.d,$(CSRCS))

CCSRCS := $(wildcard src/*.cc)
CCOBJS := $(patsubst %.cc,%.o,$(CCSRCS))
CCDEPS := $(patsubst %.cc,%.d,$(CCSRCS))

CXXSRCS := $(wildcard src/*.cpp)
CXXOBJS := $(patsubst %.cpp,%.o,$(CXXSRCS))
CXXDEPS := $(patsubst %.cpp,%.d,$(CXXSRCS))

CXXEXESRCS := $(wildcard example/*.cpp)
CXXEXEOBJS := $(patsubst %.cpp,%.o,$(CXXEXESRCS))
CXXEXEDEPS := $(patsubst %.cpp,%.d,$(CXXEXESRCS))

ALL_DEPS := $(CDEPS) $(CCDEPS) $(CXXDEPS)
ALL_OBJS := $(COBJS) $(CCOBJS) $(CXXOBJS)


all: $(LIBTARGET) $(CXXEXEOBJS) testprog

testprog: $(CXXEXEOBJS) $(LIBTARGET) $(LIBASISTATIC)
	$(CXX) -o $@ $^ $(LIBTARGET) $(EDLDFLAGS)

$(LIBTARGET): $(ALL_OBJS)
	ar -crs $@ $(ALL_OBJS)

-include $(CDEPS)

%.o: %.c Makefile
	$(CC) $(EDCFLAGS) -MMD -MP -c $< -o $@

-include $(CCDEPS)

%.o: %.cc Makefile
	$(CC) $(EDCFLAGS) -MMD -MP -c $< -o $@

-include $(CXXDEPS)

%.o: %.cpp Makefile
	$(CXX) $(EDCXXFLAGS) -MMD -MP -c $< -o $@

clean:
	rm -f $(ALL_OBJS) $(ALL_DEPS) $(CXXEXEDEPS) $(CXXEXEOBJS) $(LIBTARGET) testprog

spotless: clean
	rm -f bootcount*
	rm -rf data