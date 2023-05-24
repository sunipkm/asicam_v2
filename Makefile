CC = gcc
CXX = g++

EDCFLAGS = -O2 -Wall -I include/ $(CFLAGS)
EDCXXFLAGS = -O2 -Wall -I include/ -std=c++11 -fPIC $(CXXFLAGS)
EDLDFLAGS = -L lib/x64 -lASICamera2 -lm -lpthread -lcfitsio $(LDFLAGS)

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

testprog: $(CXXEXEOBJS) $(LIBTARGET)
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
	rm -f $(ALL_OBJS) $(ALL_DEPS) $(LIBTARGET)