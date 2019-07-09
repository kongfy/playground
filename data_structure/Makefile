CC              = gcc
LIB             = -lpthread -lstdc++
CFLAGS          = -Wall -O2 -g -std=c++11

%.o: %.cpp
	$(CC) $(CFLAGS) -o $@ -c $<

all: test_queue test_stack test_set test_hash test_skiplist

test_queue : test_queue.o hazard_pointer.o
	$(CC) test_queue.o hazard_pointer.o $(LIB) -o test_queue

test_stack : test_stack.o hazard_pointer.o
	$(CC) test_stack.o hazard_pointer.o $(LIB) -o test_stack

test_set : test_set.o
	$(CC) test_set.o $(LIB) -o test_set

test_hash : test_hash.o
	$(CC) test_hash.o $(LIB) -o test_hash

test_skiplist : test_skiplist.o
	$(CC) test_skiplist.o $(LIB) -o test_skiplist

clean :
	rm -f *.o test_queue test_stack test_set test_hash test_skiplist
