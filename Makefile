CC=gcc
CFLAGS=-std=gnu11 -g

tree:
	$(CC) $(CFLAGS) src/tree.c src/arena_alloc.c -o build/tree

run: tree
	./build/tree
