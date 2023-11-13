CC = gcc
OPT=-O0
DEPFLAGS=-MP -MD
CFLAGS=-lpthread -Wall -Wextra -g $(OPT) $(DEPFLAGS)CFILES = %.c
OBJECTS=$(patsubst %.c,%.o,$(CFILES))
DEPFILES=$(patsubst %.c,%.d,$(CFILES))

all: collection 

#dynamicList: dynamicList.c dynamicList.h
#	$(CC) $(ARGS) -c dynamicList.c -o dynamicList.o

collection: $(OBJECTS)
	$(CC) -o $@ $^

%.o : %.c
	$(CC) $(CFLAGS) -c -o $@ $<

check-mem:
	valgrind --leak-check=full --track-origins=yes -s ./collection.run

-include $(DEPFILES)