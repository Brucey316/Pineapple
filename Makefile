CC = gcc
ARGS = -lpthread -Wall

all: collection 

#dynamicList: dynamicList.c dynamicList.h
#	$(CC) $(ARGS) -c dynamicList.c -o dynamicList.o

collection: collection.c collection.h wifi.h
	$(CC) $(ARGS) collection.c  -o collection.run

#link: dynamicList collection
#	$(CC) $(ARGS) collection.o dynamicList.o collection.run

check-mem:
	valgrind --leak-check=full --track-origins=yes -s ./collection.run
