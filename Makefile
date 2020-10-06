CC=g++
CXXFLAGS=-O3 -std=c++11 -Wall -pedantic -D_GNU_SOURCE -Wno-format -Wno-long-long -I.

LDFLAGS=-lpthread

MAKEDEPEND=${CC} -MM
PROGRAM=tcpforwarder

OBJS = ${PROGRAM}.o \
			 net/tcp/forwarder.o net/tcp/worker.o net/tcp/listeners.o \
			 net/tcp/connections.o net/tcp/connection.o net/socket/addresses.o \
			 net/socket/address.o string/buffer.o

DEPS:= ${OBJS:%.o=%.d}

all: $(PROGRAM)

${PROGRAM}: ${OBJS}
	${CC} ${OBJS} ${LIBS} -o $@ ${LDFLAGS}

clean:
	rm -f ${PROGRAM} ${OBJS} ${DEPS}

${OBJS} ${DEPS} ${PROGRAM} : Makefile

.PHONY : all clean

%.d : %.cpp
	${MAKEDEPEND} ${CXXFLAGS} $< -MT ${@:%.d=%.o} > $@

%.o : %.cpp
	${CC} ${CXXFLAGS} -c -o $@ $<

-include ${DEPS}
