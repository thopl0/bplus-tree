#include <limits.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

// Type definitions
//
typedef struct Node Node;
typedef struct Tree Tree;

// Structs
//
struct Node {
  bool is_leaf;
  int keys_count;
  int children_count;
  Node *parent;
  Node *next;
  Node *previous;
  void **keys;
  void **values;
  Node **children;
};

struct Tree {
  int order;
  Node *root;
};

// declarations
//
Node *create_node(bool is_leaf, Node *parent, int order);
Node *find_insertion_node(void *key, Tree *tree);

void list_node(Node *node, int order, int level, int depth);
void list_tree(Tree *tree);

void insert_item_into_array_index(int index, void *item, void **arr, int arr_len);
int find_index_and_insert_item_into_array(void *item, void **arr, int arr_len);

void insert_child_to_node(Node *child, Node *parent, Tree *tree);
void remove_child_from_node(Node *child, Node *node, Tree *tree);
void split_children(Node *original_parent, Node *first_parent, Node *second_parent, Tree *tree);
void insert_key_to_node(void *key, Node *node, Tree *tree);

void insert_record_to_leaf(void *key, void *value, Node *node, Tree *tree);
void insert_record_to_tree(void *key, void *value, Tree *tree);

void *search(void *key, Tree *tree);


Node *create_node(bool is_leaf, Node *parent, int order) {
  Node *node = malloc(sizeof(Node));
  void **keys = calloc(order, sizeof(void*));
  void **values = calloc(order, sizeof(void*));
  Node **children = calloc(order+1, sizeof(Node*));

  node->is_leaf = is_leaf;
  node->parent = parent;
  node->keys = keys;
  node->values = values;
  node->children = children;
  node->children_count = 0;
  node->keys_count = 0;

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
    printf("%*s%s  keys:%d  children:%d\n",
        indent, "",
        node->is_leaf ? "leaf" : "node",
        node->keys_count,
        node->children_count);

    printf("\n");

    if (depth == 0) return;

    for (int slot = 0; slot < node->children_count; slot++)
        if (node->children[slot] != NULL)
            list_node(node->children[slot], order, level + 1, depth - 1);
}


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
          else if(*(int*)node->keys[i] < ikey && ikey < *(int*)node->keys[i+1]) {
            node = node->children[i+1];
            break;
          }
          
        }
      }
    }                                                        
  } 

  // printf("found insertion node: \n");
  // list_node(node, tree->order, 0, 1);
  //
  //   if(node->parent) list_node(node->parent, tree->order, 0, 1);
  //   list_tree(tree);
  return node;
}

// node operations 
//

void handle_node_key_overflow(Node *node, Tree *tree) {
  // printf("Key overflow\n");
  // list_node(node, tree->order, 0, 1); 
  Node *parent;
  if(node->parent == NULL) {
    parent = create_node(false, NULL, tree->order);
    tree->root = parent;
  }
  else parent = node->parent;

  int node_mid = node->keys_count / 2;

  insert_key_to_node(node->keys[node_mid], parent, tree);

  Node *left_split_node = create_node(false, parent, tree->order);
  Node *right_split_node = create_node(false, parent, tree->order);

  split_children(node, left_split_node, right_split_node, tree);

  remove_child_from_node(node, parent, tree);


  insert_child_to_node(left_split_node, parent, tree);
  insert_child_to_node(right_split_node, parent, tree);


  
  if(parent->keys_count > tree->order - 1) {
    handle_node_key_overflow(parent, tree);
  }

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

void insert_child_to_node(Node *child, Node *parent, Tree *tree) {  
  // printf("inserting child\n");
  // list_node(child, tree->order, 0, false);
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

void insert_key_to_node(void *key, Node *node, Tree *tree) {
    find_index_and_insert_item_into_array(key, node->keys, tree->order);
    node->keys_count += 1;
    return;
  
}

// record insert
//
void insert_record_to_leaf(void *key, void *value, Node *node, Tree *tree) {
  
  if(node->keys_count == tree->order-1) {
    // if(node->parent) list_node(node->parent, tree->order, 0, 1);
    int key_index = find_index_and_insert_item_into_array(key, node->keys, tree->order);
    insert_item_into_array_index(key_index, value, node->values, tree->order);
    node->keys_count += 1;


    int mid = (node->keys_count) / 2;
    void *mid_key = node->keys[mid];

    Node *parent;
    if(node->parent == NULL) {
      parent = create_node(false, NULL, tree->order); 
      tree->root = parent; 
    }
    else parent = node->parent;


    Node *left_split = create_node(true, parent, tree->order);
    Node *right_split = create_node(true, parent, tree->order);

    left_split->previous = node->previous;
    right_split->next = node->next;

    left_split->next = right_split;
    right_split->previous = left_split;
    
    if(node->previous) node->previous->next = left_split;
    if(node->next) node->next->previous = right_split;


    for(int i=0; i<node->keys_count; i++) {
      if(i<mid) {
        insert_record_to_leaf(node->keys[i], node->values[i], left_split, tree); 
      }
      else {
        insert_record_to_leaf(node->keys[i], node->values[i], right_split, tree); 
      }
    }

    
  
    // list_node(parent, tree->order, 0, 1); 
    insert_key_to_node(node->keys[mid], parent, tree);

    remove_child_from_node(node, parent, tree);

    // list_node(parent, tree->order, 0, 1); 
    insert_child_to_node(left_split, parent, tree);
    insert_child_to_node(right_split, parent, tree);

    if(parent->keys_count > tree->order - 1) {
      handle_node_key_overflow(parent, tree);
    }

    return;
  } 
  else {
    // printf("inserting...\n");
    int key_index = find_index_and_insert_item_into_array(key, node->keys, tree->order);
    if(node->is_leaf) insert_item_into_array_index(key_index, value, node->values, tree->order);
    node->keys_count += 1;
  }
}

void insert_record_to_tree(void *key, void *value, Tree *tree) {
  Node *insertion_node = find_insertion_node(key, tree);

  // printf("found insertion node\n");

  insert_record_to_leaf(key, value, insertion_node, tree);
}


void *search(void *key, Tree *tree) {
  Node *node = find_insertion_node(key, tree);

  list_node(node, tree->order, 0, 1);

  for(int i=0; i<node->keys_count; i++) {
    if(*(int*)node->keys[i] == *(int*)key) return node->values[i]; 
  }
  return NULL;
}

void range_query(Tree *tree, void *start_key, void *end_key) {
  printf("Getting Values from %d to %d", *(int*)start_key, *(int*)end_key);

  Node *node = find_insertion_node(start_key, tree);
  
  while(node->next) {
    for(int i=0; i<node->keys_count; i++) {
      if(*(int*)node->keys[i] >= *(int*)start_key && *(int*)node->keys[i] <= *(int*)end_key) {
        printf("Key: %d, Value: %d\n", *(int*)node->keys[i], *(int*)node->values[i]);
      }
    }
    node = node->next;
  }
}

void list_tree(Tree *tree) {
  list_node(tree->root, tree->order, 0, INT_MAX);
}


int main(int argc, char *argv[])
{
  Tree *tree = malloc(sizeof(Tree));
  tree->order = 4;

  Node *root = create_node(true, NULL, tree->order);
  tree->root = root;

  int keys[22] = {10, 20, 15, 21, 5, 33, 23, 40, 100, 30, 35, 2, 11, 12, 13, 14, 16, 17, 6, 7, 8, 9};
  int values[22] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22};

  

  for(int i=0; i<22; i++) {
    int *key_p = malloc(sizeof(int));
    int *value_p = malloc(sizeof(int));
    *key_p = keys[i];
    *value_p = values[i];

    printf("Inserting %d...\n", i+1);

    insert_record_to_tree(key_p, value_p, tree);
 
    printf("\n\n-------------------\n");
    printf("ROOT KEY: %d, CHILDREN: %d\n", tree->root->keys_count, tree->root->children_count);
    list_tree(tree);
    printf("\n-------------------\n\n");
    // printf("\n\n-------------------\n");
    // printf("ROOT KEY: %d, CHILDREN: %d\n", tree->root->keys_count, tree->root->children_count);
    // list_node(tree->root, tree->order, 0, 1);
    // printf("\n-------------------\n\n");
  }

  int *test_key = malloc(sizeof(int));
  *test_key = 2;
  int *test_key2 = malloc(sizeof(int));
  *test_key2 = 20;

  // Node *test = find_insertion_node(test_key, tree);
  // list_node(test, tree->order, 0, 1);
  // Node *next_node = test->next;
  // while (next_node != NULL) {
  //   list_node(next_node, tree->order, 0, 1);
  //   next_node = next_node->next;
  // }

  printf("Searched Key: %d, found value: %d\n", *(int*)test_key, *(int*)search(test_key, tree));

  range_query(tree, test_key, test_key2);
  


  

  
 

  // list_node(root, tree->order);
  //
  // list_tree(tree);
  //
  // list_node(tree->root, tree->order, 0, false);
  return EXIT_SUCCESS;
}
