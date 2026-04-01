CC=gcc
CFLAGS=-std=gnu11 -g

tree:
	$(CC) $(CFLAGS) src/tree.c src/arena_alloc.c -o build/tree

test:
	${CC} ${CFLAGS} tests/test.c src/tree.c src/arena_alloc.c -o build/test

run: tree
	./build/tree
