#include <stdio.h>
#include <stdlib.h>
#include "arena_alloc.h"

ArenaChunk *arena_init_chunk(Arena *arena);

Arena *arena_init(size_t chunk_size) {
  Arena *arena = malloc(sizeof(Arena));
  arena->default_chunk_memory_size = chunk_size;

  ArenaChunk *chunk = arena_init_chunk(arena);
  
  arena->head = chunk;
  arena->current = chunk;

  return arena;
}

ArenaChunk *arena_init_chunk(Arena *arena) {
  ArenaChunk *chunk = malloc(sizeof(ArenaChunk));
  void *memory = malloc(arena->default_chunk_memory_size);

  chunk->memory = memory;
  chunk->size = arena->default_chunk_memory_size;
  chunk->used = 0;
  chunk->next = NULL;

  return chunk;
}

void *arena_alloc(Arena *arena, size_t bytes) {
  if(arena->current->used + bytes >= arena->current->size) {
    ArenaChunk *new_chunk = arena_init_chunk(arena);
    arena->current->next = new_chunk;
    arena->current = new_chunk;
  }

  void *ptr = arena->current->used + arena->current->memory;

  arena->current->used += bytes;

  return ptr;

}

void arena_destroy(Arena *arena) {
  ArenaChunk *chunk = arena->head;

  while(chunk != NULL) {
    ArenaChunk *next_chunk = chunk->next;
    free(chunk->memory);
    free(chunk);
    chunk = next_chunk;
  }
  free(arena);
}

void display_arena_status(Arena *arena) {
  int total_chunks=0, total_size=0, total_usage=0;
  ArenaChunk *chunk = arena->head;
  while(chunk != NULL) {
    total_chunks += 1;
    total_size += chunk->size;
    total_usage += chunk->used;
    chunk = chunk->next;
  }
  printf("\n--------------------------------\n");
  printf("-----------ARENA STATUS---------\n");
  printf("NUMBER OF CHUNKS: %d\n", total_chunks);
  printf("TOTAL SIZE: %d\n", total_size);
  printf("TOTAL USED: %d\n", total_usage);
  printf("--------------------------------\n");
}
