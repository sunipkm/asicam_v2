CC = gcc
CXX = g++

EDCFLAGS = -O2 -Wall -I include/ $(CFLAGS)
EDCXXFLAGS = -O2 -Wall -I include/ $(CXXFLAGS)
EDLDFLAGS = -L lib/x64 -lm -lpthread -lASICamera2 $(LDFLAGS)

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

ALL_DEPS := $(CDEPS) $(CCDEPS) $(CXXDEPS)
ALL_OBJS := $(COBJS) $(CCOBJS) $(CXXOBJS)


all: $(LIBTARGET)

$(LIBTARGET): $(ALL_OBJS) $(ALL_DEPS)
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