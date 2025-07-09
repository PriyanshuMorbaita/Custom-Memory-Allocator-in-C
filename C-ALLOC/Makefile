/* PRIYANSHU MORBAITA */

CC = gcc
CFLAGS = -std=c11 -pthread -O2
OBJS = alloc.o test.o
TARGET = test

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

alloc.o: alloc.c alloc.h
	$(CC) $(CFLAGS) -c alloc.c

test.o: test.c alloc.h
	$(CC) $(CFLAGS) -c test.c

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
