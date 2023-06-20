CC = gcc
ARGS = -lpthread -Wall

all: collection

collection: collection.c collection.h wifi.h
	$(CC) collection.c $(ARGS) -o collection.rn

check-mem:
	valgrind --leak-check=full --track-origins=yes -s ./collection
