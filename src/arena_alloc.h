#ifndef ARENA_H
#define ARENA_H

typedef struct ArenaChunk ArenaChunk;
typedef struct Arena Arena;

struct Arena {
  ArenaChunk *head;
  ArenaChunk *current;
  size_t default_chunk_memory_size;
};

struct ArenaChunk {
  void *memory;
  size_t size;
  size_t used;
  ArenaChunk *next;
};

Arena *arena_init(size_t size);
void *arena_alloc(Arena *arena, size_t bytes);

void arena_destroy(Arena *arena);
void display_arena_status(Arena *arena);

#endif // !ARENA_H

