#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include "../src/arena_alloc.h"
#include "../src/tree.h"

static int pass = 0;
static int fail = 0;
static const char *current_test = "";

#define RUN(fn) do { \
    current_test = #fn; \
    printf("\n>> %s\n", current_test); \
    fn(); \
} while(0)

#define CHECK(cond, name) do { \
    if (cond) { printf("   [PASS] %s\n", name); pass++; } \
    else { printf("   [FAIL] %s  (line %d)\n", name, __LINE__); fail++; } \
} while(0)

// helpers to allocate int keys/values into arena for tree tests
static int *make_int(Arena *a, int v) {
    int *p = arena_alloc(a, sizeof(int));
    *p = v;
    return p;
}


// ---- arena tests ----

void test_arena_init() {
    Arena *a = arena_init(256);
    CHECK(a != NULL, "arena_init returns non-null");
    CHECK(a->head != NULL, "arena head chunk exists");
    CHECK(a->current == a->head, "current == head on init");
    CHECK(a->default_chunk_memory_size == 256, "chunk size stored correctly");
    CHECK(a->head->used == 0, "initial used is 0");
    CHECK(a->head->size == 256, "chunk size matches");
    CHECK(a->head->next == NULL, "head->next is null on init");
    arena_destroy(a);
}

void test_arena_alloc_basic() {
    Arena *a = arena_init(256);
    void *p = arena_alloc(a, 16);
    CHECK(p != NULL, "alloc returns non-null");
    CHECK(a->head->used == 16, "used bumped correctly after alloc");

    void *q = arena_alloc(a, 32);
    CHECK(q != NULL, "second alloc returns non-null");
    CHECK(a->head->used == 48, "used accumulates across allocs");
    CHECK(p != q, "two allocs return different pointers");
    arena_destroy(a);
}

void test_arena_alloc_fills_exactly() {
    Arena *a = arena_init(64);
    // alloc exactly the chunk size minus 1 byte to stay within
    void *p = arena_alloc(a, 63);
    CHECK(a->current == a->head, "still on first chunk");
    CHECK(a->head->used == 63, "used is 63");
    // one more byte: crosses boundary (used+1 >= size triggers new chunk)
    void *q = arena_alloc(a, 1);
    CHECK(a->current != a->head, "overflowed to new chunk");
    CHECK(a->current->used == 1, "new chunk used is 1");
    CHECK(q != NULL, "overflow alloc non-null");
    arena_destroy(a);
}

void test_arena_alloc_triggers_new_chunk() {
    Arena *a = arena_init(64);
    arena_alloc(a, 64);  // fills chunk (used+64 >= 64 triggers new chunk)
    CHECK(a->head->next != NULL, "new chunk linked after overflow");
    CHECK(a->current != a->head, "current advanced to new chunk");
    CHECK(a->current->size == 64, "new chunk has same default size");
    arena_destroy(a);
}

void test_arena_multiple_chunks() {
    Arena *a = arena_init(32);
    for (int i = 0; i < 10; i++) arena_alloc(a, 32);
    int chunk_count = 0;
    ArenaChunk *c = a->head;
    while (c) { chunk_count++; c = c->next; }
    CHECK(chunk_count > 1, "multiple chunks created");
    arena_destroy(a);
}

void test_arena_alloc_writable() {
    Arena *a = arena_init(256);
    int *p = arena_alloc(a, sizeof(int));
    *p = 42;
    CHECK(*p == 42, "allocated memory is writable and readable");
    char *s = arena_alloc(a, 8);
    memcpy(s, "hello\0", 6);
    CHECK(strcmp(s, "hello") == 0, "string write to arena memory works");
    arena_destroy(a);
}

void test_arena_small_chunk_size() {
    // chunk size of 1 byte means every alloc spills to a new chunk
    Arena *a = arena_init(1);
    void *p = arena_alloc(a, 1);
    void *q = arena_alloc(a, 1);
    CHECK(p != NULL && q != NULL, "allocs succeed with 1-byte chunks");
    CHECK(a->head->next != NULL, "multiple chunks with 1-byte chunk size");
    arena_destroy(a);
}

void test_arena_destroy_no_crash() {
    // just make sure destroy doesnt crash / leak visibly
    Arena *a = arena_init(128);
    arena_alloc(a, 64);
    arena_alloc(a, 64); // triggers second chunk
    arena_alloc(a, 64); // triggers third chunk
    arena_destroy(a);
    CHECK(true, "arena_destroy on multi-chunk arena doesn't crash");
}


// ---- tree helpers ----

static Tree *make_tree(Arena *a, int order) {
    Tree *t = arena_alloc(a, sizeof(Tree));
    t->order = order;
    t->root = create_node(a, true, NULL, order);
    return t;
}

static void bulk_insert(Arena *a, Tree *t, int *vals, int n) {
    for (int i = 0; i < n; i++){
      insert_record_to_tree(a, make_int(a, vals[i]), make_int(a, vals[i] * 10), t);
    }
}


// ---- tree tests ----

void test_tree_insert_and_search_single() {
    Arena *a = arena_init(4096);
    Tree *t = make_tree(a, 4);
    insert_record_to_tree(a, make_int(a, 5), make_int(a, 50), t);
    void *v = search(make_int(a, 5), t);
    CHECK(v != NULL, "search finds single inserted key");
    CHECK(*(int*)v == 50, "search returns correct value");
    arena_destroy(a);
}

void test_tree_search_missing_key() {
    Arena *a = arena_init(4096);
    Tree *t = make_tree(a, 4);
    insert_record_to_tree(a, make_int(a, 1), make_int(a, 10), t);
    void *v = search(make_int(a, 99), t);
    CHECK(v == NULL, "search returns NULL for missing key");
    arena_destroy(a);
}

void test_tree_insert_sequential_ascending() {
    Arena *a = arena_init(8192);
    Tree *t = make_tree(a, 4);
    int vals[] = {1,2,3,4,5,6,7,8,9,10};
    bulk_insert(a, t, vals, 10);
    bool all_found = true;
    for (int i = 0; i < 10; i++) {
        void *v = search(make_int(a, vals[i]), t);
        if (!v || *(int*)v != vals[i] * 10) { all_found = false; break; }
    }
    CHECK(all_found, "all sequentially inserted keys found");
    arena_destroy(a);
}

void test_tree_insert_sequential_descending() {
    Arena *a = arena_init(8192);
    Tree *t = make_tree(a, 4);
    int vals[] = {10,9,8,7,6,5,4,3,2,1};
    bulk_insert(a, t, vals, 10);
    bool all_found = true;
    for (int i = 1; i <= 10; i++) {
        void *v = search(make_int(a, i), t);
        if (!v || *(int*)v != i * 10) { all_found = false; break; }
    }
    CHECK(all_found, "all descending-inserted keys found");
    arena_destroy(a);
}

void test_tree_insert_causes_split() {
    // order 3 means max 2 keys per leaf before split
    Arena *a = arena_init(8192);
    Tree *t = make_tree(a, 3);
    int vals[] = {1,2,3,4,5};
    bulk_insert(a, t, vals, 5);
    bool all_found = true;
    for (int i = 0; i < 5; i++) {
        void *v = search(make_int(a, vals[i]), t);
        if (!v || *(int*)v != vals[i] * 10) { all_found = false; break; }
    }
    CHECK(all_found, "keys all found after multiple splits");
    CHECK(!t->root->is_leaf, "root is no longer a leaf after splits");
    arena_destroy(a);
}

void test_tree_root_splits_correctly() {
    Arena *a = arena_init(8192);
    Tree *t = make_tree(a, 4);
    int vals[] = {10,20,30,40,50};
    bulk_insert(a, t, vals, 5);
    CHECK(!t->root->is_leaf, "root promoted to internal node after split");
    CHECK(t->root->keys_count >= 1, "root has at least one key after split");
    arena_destroy(a);
}

void test_tree_large_insert() {
    Arena *a = arena_init(65536);
    Tree *t = make_tree(a, 5);
    int n = 100;
    for (int i = 1; i <= n; i++)
        insert_record_to_tree(a, make_int(a, i), make_int(a, i * 100), t);
    bool all_found = true;
    for (int i = 1; i <= n; i++) {
        void *v = search(make_int(a, i), t);
        if (!v || *(int*)v != i * 100) { all_found = false; break; }
    }
    CHECK(all_found, "all 100 keys found after large insert");
    arena_destroy(a);
}

void test_tree_delete_single() {
    Arena *a = arena_init(4096);
    Tree *t = make_tree(a, 4);
    insert_record_to_tree(a, make_int(a, 7), make_int(a, 70), t);
    remove_record(a, make_int(a, 7), t);
    void *v = search(make_int(a, 7), t);
    CHECK(v == NULL, "deleted key no longer found");
    arena_destroy(a);
}

void test_tree_delete_nonexistent() {
    // removing a key not in the tree shouldn't crash
    Arena *a = arena_init(4096);
    Tree *t = make_tree(a, 4);
    int vals[] = {1,2,3};
    bulk_insert(a, t, vals, 3);
    remove_record(a, make_int(a, 99), t);
    CHECK(true, "remove of nonexistent key doesn't crash");
    void *v = search(make_int(a, 2), t);
    CHECK(v != NULL && *(int*)v == 20, "other keys intact after bad delete");
    arena_destroy(a);
}

void test_tree_delete_and_search_remaining() {
    Arena *a = arena_init(8192);
    Tree *t = make_tree(a, 4);
    int vals[] = {1,2,3,4,5,6,7,8};
    bulk_insert(a, t, vals, 8);
    remove_record(a, make_int(a, 4), t);
    remove_record(a, make_int(a, 7), t);
    CHECK(search(make_int(a, 4), t) == NULL, "deleted key 4 gone");
    CHECK(search(make_int(a, 7), t) == NULL, "deleted key 7 gone");
    CHECK(search(make_int(a, 1), t) != NULL, "key 1 still present");
    CHECK(search(make_int(a, 8), t) != NULL, "key 8 still present");
    arena_destroy(a);
}

void test_tree_delete_all_keys() {
    Arena *a = arena_init(8192);
    Tree *t = make_tree(a, 4);
    int vals[] = {1,2,3,4,5};
    bulk_insert(a, t, vals, 5);
    for (int i = 0; i < 5; i++)
        remove_record(a, make_int(a, vals[i]), t);
    bool none_found = true;
    for (int i = 0; i < 5; i++)
        if (search(make_int(a, vals[i]), t) != NULL) { none_found = false; break; }
    CHECK(none_found, "all keys gone after deleting all");
    arena_destroy(a);
}

void test_tree_range_query_no_crash() {
    Arena *a = arena_init(8192);
    Tree *t = make_tree(a, 4);
    int vals[] = {1,2,3,4,5,6,7,8,9,10};
    bulk_insert(a, t, vals, 10);
    // range_query doesn't return a value but should not crash
    range_query(t, make_int(a, 3), make_int(a, 7));
    CHECK(true, "range_query on valid range doesn't crash");
    range_query(t, make_int(a, 1), make_int(a, 10));
    CHECK(true, "range_query full range doesn't crash");
    range_query(t, make_int(a, 5), make_int(a, 5));
    CHECK(true, "range_query single key range doesn't crash");
    arena_destroy(a);
}

void test_tree_insert_duplicate_key() {
    // inserting the same key twice -- tree doesn't guard against it,
    // but shouldn't crash and search should still return a value
    Arena *a = arena_init(8192);
    Tree *t = make_tree(a, 4);
    insert_record_to_tree(a, make_int(a, 5), make_int(a, 50), t);
    insert_record_to_tree(a, make_int(a, 5), make_int(a, 99), t);
    void *v = search(make_int(a, 5), t);
    CHECK(v != NULL, "search still returns a value after duplicate insert");
    arena_destroy(a);
}

void test_tree_order_5() {
    Arena *a = arena_init(32768);
    Tree *t = make_tree(a, 5);
    for (int i = 1; i <= 50; i++)
        insert_record_to_tree(a, make_int(a, i), make_int(a, i), t);
    bool ok = true;
    for (int i = 1; i <= 50; i++) {
        void *v = search(make_int(a, i), t);
        if (!v || *(int*)v != i) { ok = false; break; }
    }
    CHECK(ok, "order-5 tree: all 50 keys found correctly");
    arena_destroy(a);
}

void test_tree_leaf_linked_list_order() {
    // after inserts, leftmost leaf's next pointers should traverse in key order
    Arena *a = arena_init(8192);
    Tree *t = make_tree(a, 4);
    int vals[] = {5,3,8,1,7,2,6,4};
    bulk_insert(a, t, vals, 8);

    // walk down to leftmost leaf
    Node *leaf = t->root;
    while (!leaf->is_leaf) leaf = leaf->children[0];

    bool ordered = true;
    int prev = -1;
    while (leaf != NULL) {
        for (int i = 0; i < leaf->keys_count; i++) {
            int k = *(int*)leaf->keys[i];
            if (k <= prev) { ordered = false; break; }
            prev = k;
        }
        leaf = leaf->next;
    }
    CHECK(ordered, "leaf linked list traverses keys in ascending order");
    arena_destroy(a);
}

void test_tree_empty_then_insert() {
    Arena *a = arena_init(4096);
    Tree *t = make_tree(a, 4);
    CHECK(t->root->keys_count == 0, "fresh tree root has no keys");
    insert_record_to_tree(a, make_int(a, 42), make_int(a, 420), t);
    void *v = search(make_int(a, 42), t);
    CHECK(v != NULL && *(int*)v == 420, "first insert into empty tree works");
    arena_destroy(a);
}


// ---- runner ----

int main(void) {
    printf("=== arena tests ===");
    RUN(test_arena_init);
    RUN(test_arena_alloc_basic);
    RUN(test_arena_alloc_fills_exactly);
    RUN(test_arena_alloc_triggers_new_chunk);
    RUN(test_arena_multiple_chunks);
    RUN(test_arena_alloc_writable);
    RUN(test_arena_small_chunk_size);
    RUN(test_arena_destroy_no_crash);

    printf("\n=== tree tests ===");
    RUN(test_tree_insert_and_search_single);
    RUN(test_tree_search_missing_key);
    RUN(test_tree_insert_sequential_ascending);
    RUN(test_tree_insert_sequential_descending);
    RUN(test_tree_insert_causes_split);
    RUN(test_tree_root_splits_correctly);
    RUN(test_tree_large_insert);
    RUN(test_tree_delete_single);
    RUN(test_tree_delete_nonexistent);
    RUN(test_tree_delete_and_search_remaining);
    RUN(test_tree_delete_all_keys);
    RUN(test_tree_range_query_no_crash);
    RUN(test_tree_insert_duplicate_key);
    RUN(test_tree_order_5);
    RUN(test_tree_leaf_linked_list_order);
    RUN(test_tree_empty_then_insert);

    printf("\n%d passed, %d failed\n", pass, fail);
    return fail > 0 ? 1 : 0;
}
