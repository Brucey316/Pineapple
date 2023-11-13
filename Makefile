BINARY=collection.run
CC = gcc
OPT=-O0
DEPFLAGS=-MP -MD
CFLAGS=-Wall -g $(OPT) $(DEPFLAGS) 
CFILES = $(wildcard *.c)
OBJECTS=$(patsubst %.c,%.o,$(CFILES))
DEPFILES=$(patsubst %.c,%.d,$(CFILES))

all: $(BINARY) 

$(BINARY): $(OBJECTS)
	$(CC) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf $(OBJECTS) $(DEPFILES)
clean-all:
	rm -rf $(BINARY) $(OBJECTS) $(DEPFILES)
check-mem:
	valgrind --leak-check=full --track-origins=yes -s ./collection.run

-include $(DEPFILES)