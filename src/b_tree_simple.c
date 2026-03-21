#include <limits.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

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
void range_query(Tree *tree, void *start_key, void *end_key);

void handle_leaf_key_underflow(Node *node, Tree *tree);
void remove_record(void *key, Tree *tree);

void remove_key_from_node(void *key, Node *node, Tree *tree);

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
  printf("inserting child\n");
  list_node(child, tree->order, 0, false);
  for(int i=0; i<parent->keys_count; i++) {
    if(*(int*)parent->keys[i] > *(int*)child->keys[0]) {
      insert_item_into_array_index(i, child, (void**)parent->children, tree->order+1);
      printf("insert child %d\n", i);
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

  // list_node(node, tree->order, 0, 1);

  for(int i=0; i<node->keys_count; i++) {
    if(*(int*)node->keys[i] == *(int*)key) return node->values[i]; 
  }
  return NULL;
}

void range_query(Tree *tree, void *start_key, void *end_key) {
  printf("Getting Values from %d to %d\n", *(int*)start_key, *(int*)end_key);

  Node *node = find_insertion_node(start_key, tree);

  list_node(node, tree->order, 0, 1);
  
  while(node != NULL) {
    for(int i=0; i<node->keys_count; i++) {
      if(*(int*)node->keys[i] >= *(int*)start_key && *(int*)node->keys[i] <= *(int*)end_key) {
        printf("Key: %d, Value: %d\n", *(int*)node->keys[i], *(int*)node->values[i]);
      }
    }
    node = node->next;
  }
}

void merge_node_with_sibling(Node *node, Node *sibling, Tree *tree) {
  list_node(node, tree->order, 0, 1);
  printf("key_count: %d\n", node->keys_count);
  for(int i=0; i<sibling->keys_count; i++) {
      insert_key_to_node(sibling->keys[i], node, tree); 
  }
  printf("key_count: %d\n", node->keys_count);
  printf("children_count: %d\n", node->children_count);
  for(int i=0; i<sibling->children_count; i++) {
    // printf("inserting child: \n");
    // list_node(sibling->children[i], tree->order, 0, 1);
    insert_child_to_node(sibling->children[i], node, tree);
    sibling->children[i]->parent = node;
  }
  printf("children_count: %d\n", node->children_count);
}


void handle_node_key_underflow(Node *node, Tree *tree) {
  Node *left_sibling = NULL, *right_sibling = NULL;
  int i;

  for(i=0; i<node->parent->children_count; i++) {
    if(node->parent->children[i] == node) break;
  }

  if(i < (node->parent->children_count - 1)) {
    right_sibling = node->parent->children[i+1];
    printf("%p\n", right_sibling);
  }

  if(i > 0) {
    left_sibling = node->parent->children[i-1];
    printf("%p\n", left_sibling);
  }


  printf("Siblings: %p, %p\n", left_sibling, right_sibling);

  
  if(left_sibling != NULL && left_sibling->keys_count > (tree->order / 2) - 1) {
    printf("borrowing key from left node\n");

    // insert_key_to_node(left_sibling->keys[left_sibling->keys_count - 1], node, tree);
    insert_key_to_node(node->parent->keys[i-1], node, tree);
    node->parent->keys[i-1] = left_sibling->keys[left_sibling->keys_count - 1];

    node->parent->keys[i-1] = left_sibling->keys[left_sibling->keys_count - 1];

    remove_key_from_node(left_sibling->keys[left_sibling->keys_count - 1], left_sibling, tree);
    insert_child_to_node(left_sibling->children[left_sibling->children_count - 1], node, tree);
    remove_child_from_node(left_sibling->children[left_sibling->children_count - 1], left_sibling, tree);
    
    

    list_node(node, tree->order, 0, 1);

    list_node(node->parent, tree->order, 0, 2);
  }
  else if(right_sibling != NULL && right_sibling->keys_count > (tree->order / 2) - 1) {
    printf("borrowing key from right node\n");
    // insert_key_to_node(right_sibling->keys[0], node, tree);
    insert_key_to_node(node->parent->keys[i], node, tree);
    node->parent->keys[i] = right_sibling->keys[0];
    
    node->parent->keys[i] = right_sibling->keys[0];

    remove_key_from_node(right_sibling->keys[0], right_sibling, tree);
    insert_child_to_node(right_sibling->children[0], node, tree);
    remove_child_from_node(right_sibling->children[0], right_sibling, tree);

      }
  else {
    printf("combining\n");
    if(left_sibling != NULL) {
      list_node(node, tree->order, 0, 1);
      find_index_and_insert_item_into_array(node->parent->keys[i-1], node->keys, tree->order);
      node->keys_count += 1;
      merge_node_with_sibling(node, left_sibling, tree);
      list_node(node, tree->order, 0, 2);
      // list_node(node, tree->order, 0, 1);
      if(node->parent == tree->root && node->parent->keys_count <= (tree->order / 2) - 1) {
        tree->root = node;
      }
      else {
        remove_key_from_node(node->parent->keys[i-1], node->parent, tree);
        remove_child_from_node(left_sibling, node->parent, tree);
      }     
      list_node(node, tree->order, 0, 1);
    }
    else if (right_sibling != NULL) {
      find_index_and_insert_item_into_array(node->parent->keys[i], node->keys, tree->order);
      node->keys_count += 1;
      merge_node_with_sibling(node, right_sibling, tree);
      remove_child_from_node(right_sibling, node->parent, tree);
      if(node->parent == tree->root && node->parent->keys_count <= (tree->order / 2) - 1) {
        printf("root decrease\n");
        tree->root = node;
      }
      else {
        remove_key_from_node(node->parent->keys[i], node->parent, tree);
        
      }
    }
    
  }
 
}

void remove_key_from_node(void *key, Node *node, Tree *tree) {
  printf("removing key %d from node...\n", *(int*)key);
  for(int i=0; i<node->keys_count; i++) {
    if(*(int*)node->keys[i] == *(int*)key) {
      node->keys[i] = NULL;
      node->keys_count -= 1;
      break;
    }
  }
  printf("removed key\n");
  if(node->keys_count > 0) {
    for(int j=0; j<tree->order; j++) {
      if(node->keys[j] != NULL) {
        void *tmp_key = node->keys[j];

        node->keys[j] = NULL;

        printf("reordering %d\n", *(int*) tmp_key);

        find_index_and_insert_item_into_array(tmp_key, node->keys, tree->order);
      }
    }
  }

  printf("found remobed and reordered key\n");
  
  if(node->keys_count < (tree->order / 2) - 1) {
    printf("underflow\n");
    if(tree->root == node) {
      tree->root = node->children[0];
    }
    else {
      handle_node_key_underflow(node, tree);
    }
  }
  return;

}

void merge_leaf_with_sibling(Node *leaf, Node *sibling, int sibling_dir, Tree *tree) {
  printf("add: %p sibling_count %d\n", sibling, sibling->keys_count);
  for(int i=0; i<sibling->keys_count; i++) {
      insert_record_to_leaf(sibling->keys[i], sibling->values[i], leaf, tree);
  }
  printf("keys_count %d\n", leaf->keys_count);
  if(sibling_dir < 0) leaf->previous = sibling->previous;
  else leaf->next = sibling->next;
  list_node(leaf, tree->order, 0, 1);
}

void handle_leaf_key_underflow(Node *node, Tree *tree) {
  Node *left_sibling = NULL, *right_sibling = NULL;
  int i;

  for(i=0; i<node->parent->children_count; i++) {
    if(node->parent->children[i] == node) break;
  }

  printf("node index %d\n", i);

  if(i < (node->parent->children_count - 1)) {
    right_sibling = node->parent->children[i+1];
    printf("%p\n", right_sibling);
  }

  if(i > 0) {
    left_sibling = node->parent->children[i-1];
    printf("%p\n", node->previous);
  }


  printf("Siblings: %p, %p\n", left_sibling, right_sibling);

  if(left_sibling != NULL && left_sibling->keys_count > (tree->order / 2) - 1) {
    int key_index = find_index_and_insert_item_into_array(left_sibling->keys[left_sibling->keys_count - 1], node->keys, tree->order);
    printf("Borrowed key from left and inserted at index %d\n", key_index);
    insert_item_into_array_index(key_index, left_sibling->values[left_sibling->keys_count - 1], node->values, tree->order);

    node->keys_count += 1;

    remove_record(left_sibling->keys[left_sibling->keys_count - 1], tree);

    node->parent->keys[i-1] = node->keys[0];

    list_node(node->parent, tree->order, 0, 1);
  }
  else if(right_sibling != NULL && right_sibling->keys_count > (tree->order / 2) - 1) {
    int key_index = find_index_and_insert_item_into_array(right_sibling->keys[right_sibling->keys_count - 1], node->keys, tree->order);
    printf("Borrowed key from right and inserted at index %d\n", key_index);

    insert_item_into_array_index(key_index, right_sibling->values[right_sibling->keys_count - 1], node->values, tree->order);

    node->keys_count += 1;

    remove_record(right_sibling->keys[right_sibling->keys_count - 1], tree);

    node->parent->keys[i] = right_sibling->keys[0];
  }
  else {
    printf("combining\n");
    if(left_sibling != NULL) {
      printf("keys_count %d\n", node->keys_count);
      merge_leaf_with_sibling(node, left_sibling, -1, tree);
      printf("merged\n");
      printf("keys_count %d\n", node->keys_count);

      printf("index %d\n", i);
      
            remove_child_from_node(left_sibling, node->parent, tree);
      printf("removed siblimg\n");

      list_node(node->parent, tree->order, 0, 1);
      remove_key_from_node(node->parent->keys[i-1], node->parent, tree);
      printf("removed parent separator\n");

      


      list_node(node, tree->order, 0, 1);
      printf("keys_count %d\n", node->keys_count);
    }
    else if (right_sibling != NULL) {
      merge_leaf_with_sibling(node, right_sibling, 1, tree);
      remove_child_from_node(right_sibling, node->parent, tree);

      remove_key_from_node(node->parent->keys[i], node->parent, tree);
    }
    else {
    }
  }
}

void remove_record(void *key, Tree *tree) {
  Node *node = find_insertion_node(key, tree);
  list_node(node, tree->order, 0, 1);
  list_node(node->parent, tree->order, 0, 1);
  printf("node to be removed\n");
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
    printf("underflow\n");
    list_node(node->parent, tree->order, 0, 1);
    handle_leaf_key_underflow(node, tree);
  }
  return;
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

  list_tree(tree);

  for(int i=0; i<1000; i++) {
    int *key_p = malloc(sizeof(int));
    int *value_p = malloc(sizeof(int));
    *key_p = i + 1;
    *value_p = i;

    printf("Inserting %d...\n", i+1);

    insert_record_to_tree(key_p, value_p, tree);
 
    // printf("\n\n-------------------\n");
    // printf("ROOT KEY: %d, CHILDREN: %d\n", tree->root->keys_count, tree->root->children_count);
    // list_tree(tree);
    // printf("\n-------------------\n\n");
    // printf("\n\n-------------------\n");
    // printf("ROOT KEY: %d, CHILDREN: %d\n", tree->root->keys_count, tree->root->children_count);
    // list_node(tree->root, tree->order, 0, 1);
    // printf("\n-------------------\n\n");
  }
  
  list_tree(tree);

  for(int i=999; i>=0; i--) {
    int *key_p = malloc(sizeof(int));
    int *value_p = malloc(sizeof(int));
    *key_p = i + 1;
    *value_p = i;

    printf("Removing %d...\n", i+1);

    remove_record(key_p, tree);
 
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
  *test_key2 = 5;

  // Node *test = find_insertion_node(test_key, tree);
  // list_node(test, tree->order, 0, 1);
  // Node *next_node = test->next;
  // while (next_node != NULL) {
  //   list_node(next_node, tree->order, 0, 1);
  //   next_node = next_node->next;
  // }

  // printf("Searched Key: %d, found value: %d\n", *(int*)test_key, *(int*)search(test_key, tree));

  // remove_record(test_key, tree);
  // remove_record(test_key2, tree);



  // range_query(tree, test_key, test_key2);
  


  

  
 

  // list_node(root, tree->order);
  //
  // list_tree(tree);
  //
  // list_node(tree->root, tree->order, 0, false);
  return EXIT_SUCCESS;
}
