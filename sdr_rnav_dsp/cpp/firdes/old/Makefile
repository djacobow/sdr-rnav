CC=g++
CFLAGS=-g -c -Wall -O0 
LDFLAGS=-lm

# for profiling
#CFLAGS+=-pg
#LDFLAGS+=-pg

SOURCES=oct_remez.cc f_helpers.cpp 

# which main do we want to build
# SOURCES+=main.cpp
SOURCES+=t_main.cpp

OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=t.exe

all: $(SOURCES) $(EXECUTABLE) depend

depend:	.depend

.depend: $(SOURCES)
	rm -f ./.depend
	$(CC) $(CFLAGS) -MM $^ > ./.depend

include .depend

$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm $(OBJECTS) $(EXECUTABLE) .depend 
