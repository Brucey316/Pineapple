BINARY=collection.run
CC = gcc
OPT=-O0
DEPFLAGS=-MP -MD
LIBRARIES=-lncurses
CFLAGS=-Wall -g $(OPT) $(DEPFLAGS) 
CFILES = $(wildcard *.c)
OBJECTS=$(patsubst %.c,%.o,$(CFILES))
DEPFILES=$(patsubst %.c,%.d,$(CFILES))

all: $(BINARY) 

$(BINARY): $(OBJECTS)
	$(CC) -o $@ $^ $(LIBRARIES)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $< $(LIBRARIES)
package:
	@tar -czvf collection.tar.gz *.[ch] .git .gitignore Makefile README.md
kill:
	@killall airodump-ng
clean:
	rm -f scanning*.cap scanning*.csv
clean-prog:
	rm -f $(BINARY) $(OBJECTS) $(DEPFILES)
clean-hash:
	rm -f results.pcap results.hc22000
clean-all:
	@make clean
	@make clean-prog
	@make clean-hash
	rm -rf *.tar.gz 
check-mem:
	valgrind --leak-check=full --track-origins=yes --show-leak-kinds=all -s ./collection.run

-include $(DEPFILES)