CC = gcc
CXX = g++

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
		ifneq ($(findstring armv6,$(UNAME_P)),)
			LIBASIDIR = lib/armv6/
		endif
		ifneq ($(findstring armv7,$(UNAME_P)),)
			LIBASIDIR = lib/armv7/
		endif
		ifneq ($(findstring aarch64,$(UNAME_P)),)
			LIBASIDIR = lib/armv8/
		endif
	endif
	ifeq ($(UNAME_S), Darwin)
		LIBASIDIR = lib/macos/
	endif
endif

LIBASISTATIC = $(LIBASIDIR)/libASICamera2.a

EDCFLAGS = -O2 -Wall -I include/ $(CFLAGS)
EDCXXFLAGS = -O2 -Wall -I include/ -std=c++11 -fPIC $(CXXFLAGS)
EDLDFLAGS = -lm -lpthread -lcfitsio -lusb-1.0 $(LDFLAGS)

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


all: $(LIBTARGET) testprog

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
	rm -f $(ALL_OBJS) $(ALL_DEPS) $(CXXEXEDEPS) $(CXXEXEOBJS) $(LIBTARGET)