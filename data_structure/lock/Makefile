CC              = gcc
LIB             = -lpthread -lstdc++
CFLAGS          = -Wall -O2 -g -std=c++11

%.o: %.cpp
	$(CC) $(CFLAGS) -o $@ -c $<

all: test_lock

test_lock : main.o
	$(CC) main.o $(LIB) -o test_lock

clean :
	rm -f *.o test_lock
