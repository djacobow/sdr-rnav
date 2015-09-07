CC=C:\MinGW\bin\g++
CFLAGS=-g -c -Wall -O0 -march=native
LDFLAGS=-lm

# for profiling
# CFLAGS+=-pg
# LDFLAGS+=-pg

SOURCES=pm_remez.cpp f_helpers.cpp

# which main do we want to build
SOURCES+=main.cpp 
# SOURCES+=foo.cpp

OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=firdes.exe

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

clean:
	rm $(OBJECTS) $(EXECUTABLE)


