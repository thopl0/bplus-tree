CC=gcc
CFLAGS=-std=gnu11 -g

b_tree:
	$(CC) $(CFLAGS) src/b_tree.c -o build/ubuntu/b_tree 

b_tree_simple:
	$(CC) $(CFLAGS) src/b_tree_simple.c -o build/ubuntu/b_tree_simple

run: b_tree_simple
	./build/ubuntu/b_tree_simple
