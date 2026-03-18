#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

typedef struct Record Record;
typedef struct Node Node;
typedef struct Tree Tree;

struct Record {
  int key;
  int value;
};

struct Node{
  bool is_leaf;
  int records_count;
  int children_count;
  Node *parent;
  union {
    Node **children;
    Record **records;
  };
  int **keys;
};

struct Tree{
  int degree;
  int order;
  Node *root;
};

Node *create_node(bool is_leaf, Tree *tree, Node *parent, Node **children) {
  Node *node = malloc(sizeof(Node));
  Record **records = malloc(sizeof(Record*)*(tree->order + 1));
  
  node->is_leaf = is_leaf;
  node->records = records;
  node->parent = parent;
  node->children = children;
  node->children_count = 0;
  node->records_count = 0;

  return node;
}


int get_record_index(int key, Record **arr, int arr_len) {
  if(arr_len == 1) return 0;
  else if(arr_len == 2) return arr[0]->key == key ? 0 : 1;
  else {
    int mid = arr_len / 2;
    if(arr[mid]->key == key) return mid;
    else if(arr[mid]->key > key) return get_record_index(key, arr, arr_len - mid - 1);
    else return get_record_index(key, arr+mid+1, arr_len);
  }
}

Node *search_node_for_insertion(int key, Tree *tree) {
  if(tree->root->children == NULL) return tree->root;
  else {
    int i=0;
    Node *cur_node = tree->root;

    while(i < cur_node->children_count) {
      if (key < cur_node->records[0]->key) {
        cur_node = cur_node->children[0];
        if (cur_node->children_count > 0) {
          i = 0;
          continue;
        }
        else break;
      }    
      else if (key > cur_node->records[cur_node->records_count - 1]->key) {
        cur_node = cur_node->children[cur_node->children_count - 1];
        if (cur_node->children_count > 0) {
          i = 0;
          continue;
        }
        else break;
      }    
      else {
        for(int i=0; i<cur_node->children_count; i++) {
          if(cur_node->records[i]->key < key && key < cur_node->records[i+1]->key){
            cur_node = cur_node->children[i+1];
            if (cur_node->children_count > 0) {
              i = 0;
              continue;
            }
            else break;
          }
        }
      }
      i++;
    }
    return cur_node;
  }
}


int find_potential_record_index(int key, Record **records, int records_count) {
  int i = 0;
  for(i=0; i<records_count; i++) {
    if(records[i] == NULL) {
      return i;
    }
    if(records[i]->key > key) return i; 
  }
  return i;
}

int find_potential_key_index(int key, int keys[], int keys_len) {
  int i = 0;
  for (i=0; i<keys_len; i++) {
    if(keys[i] > key) return i;
  }
  return i;
}


void insert_record_to_array(Record *record, Record **records, int records_len) {
  int index = find_potential_record_index(record->key, records, records_len);
  Record *tmp = NULL;
  if(records[index] == NULL) records[index] = record;
  for(int i=index; i<records_len; i++) {
    tmp = records[i];
    records[i] = record;
    record = tmp;
  }
}

void insert_key_to_array(int key, int keys[], int keys_len) {
  int index = find_potential_key_index(key, keys, keys_len);
  int tmp = 0;
  for(int i=index; i<keys_len; i++) {
    tmp = keys[i];
    keys[i] = key;
    key = tmp;
  }
}

void split_records(Record **records, int records_len, Record **left_split, Record **right_split, Record *split_record) {
  int mid = records_len / 2;

  left_split = calloc(records_len, sizeof(Record*) * records_len);
  right_split = calloc(records_len, sizeof(Record*) * records_len);

  for(int i=0; i<records_len; i++) {
    if(i <= mid) left_split[i] = records[i];
    else if(i > mid) right_split[i] = records[i];
    else split_record = records[mid];
  }
}

void split_children(Node *original, Node *left_split, Node *right_split) {
  int mid = *(original->keys[(original->children_count - 1) / 2]);

  for(int i=0; i<original->children_count; i++) {
    if(i>mid) left_split->children[i] = NULL;
    else right_split->children[i] = NULL;
  }
}

void copy_node(Node *dest, Node *src) {
  dest->keys = src->keys;
  dest->parent = src->parent;

  if(!src->is_leaf) memcpy(dest->children, src->children, src->children_count * sizeof(Node*));
  else memcpy(dest->records, src->records, src->records_count * sizeof(Record*));

  dest->children_count = src->children_count;
  dest->records_count = src->records_count;
  dest->is_leaf = src->is_leaf;
}

void insert_record_to_node(Record *record, Node *node, Tree *tree) {
  int order = tree->order;
  if(node->records_count == order - 1) {
   
    Node *left_node = malloc(sizeof(Node));
    Node *right_node = malloc(sizeof(Node));

    copy_node(left_node, node);
    copy_node(right_node, node);

   
    if(node->is_leaf) {
      Record **tmp_rec_arr = malloc(sizeof(Record*) * order);
      memcpy(tmp_rec_arr, node->records, sizeof(Record*)*(order - 1));

      insert_record_to_array(record, tmp_rec_arr, order);

      Record **left_split;
      Record **right_split;
      Record *split_record;

      left_node->records = left_split;
      right_node->records = right_split;

      split_records(tmp_rec_arr, order, left_split, right_split, split_record);
     
      if(!node->parent) {
        Node *parent = create_node(false, tree, NULL, NULL);
        Node **children = malloc(sizeof(Node*) * order);

        parent->children = children;

        children[0] = left_node;
        children[1] = right_node;

        left_node->parent = parent;
        right_node->parent = parent;
        
        tree->root = parent;
        insert_record_to_node(split_record, parent, tree);
      }
      else {
         insert_record_to_node(split_record, node->parent, tree);
      }
    } else {
      insert_key_to_array(record->key, *(node->keys), order - 1);
      split_children(node, left_node, right_node);
    }
  }
}


Record *tree_insert(Tree *tree, int key, int value) {
  Record *record = malloc(sizeof(Record));
  record->key = key;
  record->value = value;

  Node *insertion_node = search_node_for_insertion(key, tree);

  insert_record_to_node(record, insertion_node, tree);
    
  return record;
}

void list_tree(Tree *tree) {
  // printf("Listing...%d\n", tree->root->count);

  // for(int i=0; i<tree->root->count; i++) {
  //   if(tree->root->records[i] == NULL) continue;
  //   printf("[ROOT]____Key: %d\tValue:%d\n", tree->root->records[i]->key, tree->root->records[i]->value);
  // }

  // for(int i=0; i<tree->order; i++) {
  //   if(tree->root->children[i]) {
  //     for(int j=0; j<tree->root->children[i]->count; j++) {
  //       printf("%*d[LEVEL 1]____Key: %d\tValue:%d\n", (level+1)*4, tree->root->children[i]->records[j]->key, tree->root->children[i]->records[j]->value); 
  //     }
  //   }
  // }
}

int main(int argc, char *argv[])
{
  Tree *tree = malloc(sizeof(Tree));
  Node *node = create_node(false, tree, NULL, NULL);

  tree->root = node;
  tree->degree = 2;
  tree->order = 4;
 
  node->records = malloc(sizeof(Record*)*(tree->order-1));

  tree_insert(tree, 12, 24);
  tree_insert(tree, 6, 36);
  tree_insert(tree, 3, 240);
  tree_insert(tree, -1, 361);
  tree_insert(tree, 1, 214);
  

  printf("Created///\n");

  // list_tree(tree);

  // free(node->records);
  // // free(record);
  // free(node);
  // free(tree);
  //
  return EXIT_SUCCESS;
}
