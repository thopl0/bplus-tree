# bplus-tree

A B+ tree implementation in C with a custom arena allocator. Configurable order, doubly-linked leaf nodes, and support for insertion, deletion, point search, and range queries. Built as a foundation for a disk-persistent, WAL-backed storage engine.

---

## Structure

```
src/
  tree.c          # B+ tree — insertion, deletion, search, range query
  tree.h
  arena_alloc.c   # Bump allocator with chunk-based growth
  arena_alloc.h
tests/
  test.c          # In progress
```

---

## Design

### Tree
- Configurable order at initialization
- Internal nodes store keys only; values are stored exclusively in leaf nodes
- Leaf nodes are doubly linked for efficient range traversal
- Splits propagate upward on overflow; merges and borrows handle underflow on deletion

### Memory
- All allocations go through a bump allocator backed by a linked list of fixed-size chunks
- No per-node `free` — the arena is destroyed as a whole
- Individual deallocation is intentionally omitted: handling fragmentation from fine-grained frees adds complexity that isn't warranted for an in-memory tree. The tradeoff is accepted.

---

## Build

```bash
make tree
```

Requires GCC. Uses `-std=gnu11`.

---

## Status

- ✓ Insertion with overflow handling and splits
- ✓ Deletion with underflow handling, borrow, and merge
- ✓ Point search
- ✓ Range query via leaf chain traversal
- ✗ Tests — in progress
- ✗ Disk persistence — planned
- ✗ WAL — planned

---

## Roadmap

The end goal is a disk-backed key-value store. Planned work in order:

1. Test suite covering correctness of structural operations and edge cases
2. Fixed-size page layout and serialization for disk persistence
3. Write-ahead log for crash recovery
