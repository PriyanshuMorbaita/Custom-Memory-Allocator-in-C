flags=-O2 -Wall -std=c23
ldflags=

.PHONY: clean

all: clean alloc

alloc: alloc.o heap.o
	cc $(flags) -no-pie $^ -o $@ $(ldflags)

alloc.o: alloc.c alloc.h
	cc $(flags) -c $<

heap.o: heap.asm
	nasm -f elf64 $^

clean:
	rm -f *.o alloc
