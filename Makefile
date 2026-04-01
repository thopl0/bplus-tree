CC=gcc
CFLAGS=-std=gnu11 -g

b_tree:
	$(CC) $(CFLAGS) src/b_tree.c -o build/ubuntu/b_tree 

b_tree_simple:
	$(CC) $(CFLAGS) src/tree.c src/arena_alloc.c -o build/ubuntu/tree

run: b_tree_simple
	./build/ubuntu/tree
