#include <limits.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include "arena_alloc.h"
#include "tree.h"

void insert_item_into_array_index(int index, void *item, void **arr, int arr_len);
int find_index_and_insert_item_into_array(void *item, void **arr, int arr_len);
Node *find_insertion_node(void *key, Tree *tree);
void handle_node_key_overflow(Arena *arena, Node *node, Tree *tree);
void handle_node_key_underflow(Node *node, Tree *tree);
void handle_leaf_key_underflow(Arena *arena, Node *node, Tree *tree);


//  Array Utilities 

void insert_item_into_array_index(int index, void *item, void **arr, int arr_len) {
  void *tmp = NULL;
  if(arr[index] == NULL) {arr[index] = item;}
  else {
    for(int i=index; i<arr_len; i++) {
      tmp = arr[i];
      arr[i] = item;
      item = tmp;
    }
  }
}

int find_index_and_insert_item_into_array(void *item, void **arr, int arr_len) {
  int i = 0; 
  for(i=0; i<arr_len; i++) {
    if(arr[i] == NULL) break;
    else if(*(int*)(arr[i]) > *(int*)(item)) break;
  }
  insert_item_into_array_index(i, item, arr, arr_len);

  return i;
}


//  Node Creation & Display 

Node *create_node(Arena *arena, bool is_leaf, Node *parent, int order) {
  Node *node = arena_alloc(arena, sizeof(Node));
  void **keys = arena_alloc(arena, order * sizeof(void*));
  void **values = arena_alloc(arena, order * sizeof(void*));
  Node **children = arena_alloc(arena, (order + 1) * sizeof(Node*));

  node->is_leaf = is_leaf;
  node->parent = parent;
  node->keys = keys;
  node->values = values;
  node->children = children;
  node->children_count = 0;
  node->keys_count = 0;
  node->previous = NULL;
  node->next = NULL;

  return node;
}

void list_node(Node *node, int order, int level, int depth) {
    int indent = level * 8;

    // Top border
    printf("%*s+", indent, "");
    for (int slot = 0; slot < node->keys_count; slot++)
        printf(node->is_leaf ? "----------+" : "-----+");
    printf("\n");

    printf("%*s|", indent, "");
    for (int slot = 0; slot < node->keys_count; slot++) {
        if (node->is_leaf)
            printf(" %3d -> %3d |", *(int *)node->keys[slot], *(int *)node->values[slot]);
        else
            printf(" %3d |", *(int *)node->keys[slot]);
    }
    printf("\n");

    // Bottom border
    printf("%*s+", indent, "");
    for (int slot = 0; slot < node->keys_count; slot++)
        printf(node->is_leaf ? "----------+" : "-----+");
    printf("\n");

    // Node info
    printf("%*s%s  keys:%d  children:%d  address:%p\n",
        indent, "",
        node->is_leaf ? "leaf" : "node",
        node->keys_count,
        node->children_count,
        node
      );

    printf("\n");

    if (depth == 0) return;

    for (int slot = 0; slot < node->children_count; slot++)
        if (node->children[slot] != NULL)
            list_node(node->children[slot], order, level + 1, depth - 1);
}

void list_tree(Tree *tree) {
  // list_node(tree->root, tree->order, 0, INT_MAX);
}


//  Tree Traversal

Node *find_insertion_node(void *key, Tree *tree) {
  Node *node = tree->root;
  if(node->is_leaf) return node;
  else {
    int ikey = *(int*)key;
    while(!node->is_leaf) {
      if(node->keys[0] == NULL) break;
      if(ikey < *(int*)node->keys[0]) {
        node = node->children[0];
        continue;
      }
      else {
        for(int i=0; i<tree->order-1; i++) {
          if(node->keys[i+1] == NULL) {
            node = node->children[i+1];
            break;
          }
          else if (*(int*)node->keys[i] == ikey ) {
            node = node->children[i+1];
            break;
          }
          else if(*(int*)node->keys[i] < ikey && ikey < *(int*)node->keys[i+1]) {
            node = node->children[i+1];
            break;
          }
          
        }
      }
    }                                                        
  } 

  return node;
}


//  Node Key & Child Operations 

void insert_key_to_node(void *key, Node *node, Tree *tree) {
    find_index_and_insert_item_into_array(key, node->keys, tree->order);
    node->keys_count += 1;
    return;
}

void remove_key_from_node(void *key, Node *node, Tree *tree) {
  for(int i=0; i<node->keys_count; i++) {
    if(*(int*)node->keys[i] == *(int*)key) {
      node->keys[i] = NULL;
      node->keys_count -= 1;
      break;
    }
  }
  if(node->keys_count > 0) {
    for(int j=0; j<tree->order; j++) {
      if(node->keys[j] != NULL) {
        void *tmp_key = node->keys[j];

        node->keys[j] = NULL;

        find_index_and_insert_item_into_array(tmp_key, node->keys, tree->order);
      }
    }
  }
  
  if(node->keys_count < (tree->order / 2) - 1) {
    if(tree->root == node) {
      tree->root = node->children[0];
    }
    else {
      handle_node_key_underflow(node, tree);
    }
  }
  return;
}

void insert_child_to_node(Node *child, Node *parent, Tree *tree) {  
  for(int i=0; i<parent->keys_count; i++) {
    if(*(int*)parent->keys[i] > *(int*)child->keys[0]) {
      insert_item_into_array_index(i, child, (void**)parent->children, tree->order+1);
      parent->children_count += 1;
      return;
    }
  }
  insert_item_into_array_index(parent->keys_count, child, (void**)parent->children, tree->order+1);
  parent->children_count += 1;
  return;
}

void remove_child_from_node(Node *child, Node *node, Tree *tree) {
  for(int i=0; i<node->children_count; i++) {
    if(node->children[i] == child) {
      if (i != tree->order+1) {
        for(int j=i; j<tree->order; j++) {
          node->children[j] = node->children[j+1];
        }
      } 
      else {
        node->children[i] = NULL;
      }
      node->children_count -= 1;
      return;
    }
  }
}

void split_children(Node *original_parent, Node *first_parent, Node *second_parent, Tree *tree){
  Node **children = original_parent->children;
  int children_count = original_parent->children_count;
  int mid = children_count / 2;

  for(int i=0; i<original_parent->keys_count; i++) {
    if(i<mid) {
      insert_key_to_node(original_parent->keys[i], first_parent, tree); 
    }
    else if (i>mid){
      insert_key_to_node(original_parent->keys[i], second_parent, tree); 
    }
  }

  for(int i=0; i<children_count; i++) {
    if(i<=mid) {
      insert_child_to_node(children[i], first_parent, tree);
      children[i]->parent = first_parent;
    }
    else {
      insert_child_to_node(children[i], second_parent, tree);
      children[i]->parent = second_parent;
    }
  }
}


//  Insertion

void handle_node_key_overflow(Arena *arena, Node *node, Tree *tree) {
  Node *parent;
  if(node->parent == NULL) {
    parent = create_node(arena, false, NULL, tree->order);
    tree->root = parent;
  }
  else parent = node->parent;

  int node_mid = node->keys_count / 2;

  insert_key_to_node(node->keys[node_mid], parent, tree);

  Node *left_split_node = create_node(arena, false, parent, tree->order);
  Node *right_split_node = create_node(arena, false, parent, tree->order);

  split_children(node, left_split_node, right_split_node, tree);

  remove_child_from_node(node, parent, tree);

  insert_child_to_node(left_split_node, parent, tree);
  insert_child_to_node(right_split_node, parent, tree);

  if(parent->keys_count > tree->order - 1) {
    handle_node_key_overflow(arena, parent, tree);
  }

  return;
}

void insert_record_to_leaf(Arena *arena, void *key, void *value, Node *node, Tree *tree) {
  
  if(node->keys_count == tree->order-1) {
    int key_index = find_index_and_insert_item_into_array(key, node->keys, tree->order);
    insert_item_into_array_index(key_index, value, node->values, tree->order);
    node->keys_count += 1;

    int mid = (node->keys_count) / 2;
    void *mid_key = node->keys[mid];

    Node *parent;
    if(node->parent == NULL) {
      parent = create_node(arena, false, NULL, tree->order); 
      tree->root = parent; 
    }
    else parent = node->parent;

    Node *left_split = create_node(arena, true, parent, tree->order);
    Node *right_split = create_node(arena, true, parent, tree->order);

    left_split->previous = node->previous;
    right_split->next = node->next;

    left_split->next = right_split;
    right_split->previous = left_split;
    
    if(node->previous) node->previous->next = left_split;
    if(node->next) node->next->previous = right_split;

    for(int i=0; i<node->keys_count; i++) {
      if(i<mid) {
        insert_record_to_leaf(arena, node->keys[i], node->values[i], left_split, tree); 
      }
      else {
        insert_record_to_leaf(arena, node->keys[i], node->values[i], right_split, tree); 
      }
    }

    insert_key_to_node(node->keys[mid], parent, tree);

    remove_child_from_node(node, parent, tree);

    insert_child_to_node(left_split, parent, tree);
    insert_child_to_node(right_split, parent, tree);

    if(parent->keys_count > tree->order - 1) {
      handle_node_key_overflow(arena, parent, tree);
    }

    return;
  } 
  else {
    int key_index = find_index_and_insert_item_into_array(key, node->keys, tree->order);
    if(node->is_leaf) insert_item_into_array_index(key_index, value, node->values, tree->order);
    node->keys_count += 1;
  }
}

void insert_record_to_tree(Arena *arena, void *key, void *value, Tree *tree) {
  Node *insertion_node = find_insertion_node(key, tree);
  insert_record_to_leaf(arena, key, value, insertion_node, tree);
}


//  Deletion

void merge_node_with_sibling(Node *node, Node *sibling, Tree *tree) {
  for(int i=0; i<sibling->keys_count; i++) {
      insert_key_to_node(sibling->keys[i], node, tree); 
  }
  for(int i=0; i<sibling->children_count; i++) {
    insert_child_to_node(sibling->children[i], node, tree);
    sibling->children[i]->parent = node;
  }
}

void handle_node_key_underflow(Node *node, Tree *tree) {
  Node *left_sibling = NULL, *right_sibling = NULL;
  int i;

  for(i=0; i<node->parent->children_count; i++) {
    if(node->parent->children[i] == node) break;
  }

  if(i < (node->parent->children_count - 1)) {
    right_sibling = node->parent->children[i+1];
  }

  if(i > 0) {
    left_sibling = node->parent->children[i-1];
  }

  if(left_sibling != NULL && left_sibling->keys_count > (tree->order / 2) - 1) {
    insert_key_to_node(node->parent->keys[i-1], node, tree);
    node->parent->keys[i-1] = left_sibling->keys[left_sibling->keys_count - 1];

    node->parent->keys[i-1] = left_sibling->keys[left_sibling->keys_count - 1];

    remove_key_from_node(left_sibling->keys[left_sibling->keys_count - 1], left_sibling, tree);
    insert_child_to_node(left_sibling->children[left_sibling->children_count - 1], node, tree);
    remove_child_from_node(left_sibling->children[left_sibling->children_count - 1], left_sibling, tree);
  }
  else if(right_sibling != NULL && right_sibling->keys_count > (tree->order / 2) - 1) {
    insert_key_to_node(node->parent->keys[i], node, tree);
    node->parent->keys[i] = right_sibling->keys[0];
    
    node->parent->keys[i] = right_sibling->keys[0];

    remove_key_from_node(right_sibling->keys[0], right_sibling, tree);
    insert_child_to_node(right_sibling->children[0], node, tree);
    remove_child_from_node(right_sibling->children[0], right_sibling, tree);
  }
  else {
    if(left_sibling != NULL) {
      find_index_and_insert_item_into_array(node->parent->keys[i-1], node->keys, tree->order);
      node->keys_count += 1;
      merge_node_with_sibling(node, left_sibling, tree);
      if(node->parent == tree->root && node->parent->keys_count <= (tree->order / 2) - 1) {
        tree->root = node;
      }
      else {
        remove_key_from_node(node->parent->keys[i-1], node->parent, tree);
        remove_child_from_node(left_sibling, node->parent, tree);
      }     
    }
    else if (right_sibling != NULL) {
      find_index_and_insert_item_into_array(node->parent->keys[i], node->keys, tree->order);
      node->keys_count += 1;
      merge_node_with_sibling(node, right_sibling, tree);
      remove_child_from_node(right_sibling, node->parent, tree);
      if(node->parent == tree->root && node->parent->keys_count <= (tree->order / 2) - 1) {
        tree->root = node;
      }
      else {
        remove_key_from_node(node->parent->keys[i], node->parent, tree);
      }
    }
  }
}

void merge_leaf_with_sibling(Arena *arena, Node *leaf, Node *sibling, int sibling_dir, Tree *tree) {
  for(int i=0; i<sibling->keys_count; i++) {
      insert_record_to_leaf(arena, sibling->keys[i], sibling->values[i], leaf, tree);
  }
  if(sibling_dir < 0) leaf->previous = sibling->previous;
  else leaf->next = sibling->next;
}

void handle_leaf_key_underflow(Arena *arena, Node *node, Tree *tree) {
  Node *left_sibling = NULL, *right_sibling = NULL;
  int i;

  for(i=0; i<node->parent->children_count; i++) {
    if(node->parent->children[i] == node) break;
  }

  if(i < (node->parent->children_count - 1)) {
    right_sibling = node->parent->children[i+1];
  }

  if(i > 0) {
    left_sibling = node->parent->children[i-1];
  }

  if(left_sibling != NULL && left_sibling->keys_count > (tree->order / 2) - 1) {
    int key_index = find_index_and_insert_item_into_array(left_sibling->keys[left_sibling->keys_count - 1], node->keys, tree->order);
    insert_item_into_array_index(key_index, left_sibling->values[left_sibling->keys_count - 1], node->values, tree->order);

    node->keys_count += 1;

    remove_record(arena, left_sibling->keys[left_sibling->keys_count - 1], tree);

    node->parent->keys[i-1] = node->keys[0];
  }
  else if(right_sibling != NULL && right_sibling->keys_count > (tree->order / 2) - 1) {
    int key_index = find_index_and_insert_item_into_array(right_sibling->keys[right_sibling->keys_count - 1], node->keys, tree->order);
    insert_item_into_array_index(key_index, right_sibling->values[right_sibling->keys_count - 1], node->values, tree->order);

    node->keys_count += 1;

    remove_record(arena, right_sibling->keys[right_sibling->keys_count - 1], tree);

    node->parent->keys[i] = right_sibling->keys[0];
  }
  else {
    if(left_sibling != NULL) {
      merge_leaf_with_sibling(arena, node, left_sibling, -1, tree);

      remove_child_from_node(left_sibling, node->parent, tree);
      remove_key_from_node(node->parent->keys[i-1], node->parent, tree);
    }
    else if (right_sibling != NULL) {
      merge_leaf_with_sibling(arena, node, right_sibling, 1, tree);
      remove_child_from_node(right_sibling, node->parent, tree);

      remove_key_from_node(node->parent->keys[i], node->parent, tree);
    }
    else {
    }
  }
}

void remove_record(Arena *arena, void *key, Tree *tree) {
  Node *node = find_insertion_node(key, tree);
  for(int i=0; i<node->keys_count; i++) {
    if(*(int*)node->keys[i] == *(int*)key) {
      node->keys[i] = NULL;
      node->values[i] = NULL;
      node->keys_count -= 1;
      break;
    }
  }
  if(node->keys_count > 0) {
    for(int j=0; j<tree->order; j++) {
      if(node->keys[j] != NULL) {
        void *tmp_key = node->keys[j];
        void *tmp_value = node->values[j];

        node->keys[j] = NULL;
        node->values[j] = NULL;

        int key_index = find_index_and_insert_item_into_array(tmp_key, node->keys, tree->order);
        insert_item_into_array_index(key_index, tmp_value, node->values, tree->order);
      }
    }
  }
  
  if(tree->root != node && node->keys_count < (tree->order / 2) - 1) {
    handle_leaf_key_underflow(arena, node, tree);
  }
  return;
}


//  Search & Query

void *search(void *key, Tree *tree) {
  Node *node = find_insertion_node(key, tree);

  for(int i=0; i<node->keys_count; i++) {
    if(*(int*)node->keys[i] == *(int*)key) return node->values[i]; 
  }
  return NULL;
}

void range_query(Tree *tree, void *start_key, void *end_key) {
  Node *node = find_insertion_node(start_key, tree);

  while(node != NULL) {
    for(int i=0; i<node->keys_count; i++) {
      if(*(int*)node->keys[i] >= *(int*)start_key && *(int*)node->keys[i] <= *(int*)end_key) {
        // printf("Key: %d, Value: %d\n", *(int*)node->keys[i], *(int*)node->values[i]);
      }
    }
    node = node->next;
  }
}




int main(int argc, char *argv[])
{
  Arena *arena = arena_init(1024 * 1024);
  Tree *tree = arena_alloc(arena, sizeof(Tree));
  tree->order = 4;

  Node *root = create_node(arena, true, NULL, tree->order);
  tree->root = root;

  int keys[22] = {10, 20, 15, 21, 5, 33, 23, 40, 100, 30, 35, 2, 11, 12, 13, 14, 16, 17, 6, 7, 8, 9};
  int values[22] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22};

  for(int i=0; i<1000000; i++) {
    int *key_p = arena_alloc(arena, sizeof(int));
    int *value_p = arena_alloc(arena, sizeof(int));
    *key_p = i + 1;
    *value_p = i;

    printf("Inserting %d...\n", i+1);


    insert_record_to_tree(arena, key_p, value_p, tree);

    // display_arena_status(arena);
  }

  for(int i=9999999; i>=0; i--) {
    int *key_p = arena_alloc(arena, sizeof(int));
    int *value_p = arena_alloc(arena, sizeof(int));
    *key_p = i + 1;
    *value_p = i;

    printf("Removing %d...\n", i+1);

    remove_record(arena, key_p, tree);

    display_arena_status(arena);
  }

  int *test_key = arena_alloc(arena, sizeof(int));
  *test_key = 2;
  int *test_key2 = arena_alloc(arena, sizeof(int));
  *test_key2 = 5;

  arena_destroy(arena);
  return EXIT_SUCCESS;
}
