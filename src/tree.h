#ifndef TREE
#define TREE

typedef struct Node Node;
typedef struct Tree Tree;

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

Node *create_node(Arena *arena, bool is_leaf, Node *parent, int order);
void list_node(Node *node, int order, int level, int depth);
void remove_child_from_node(Node *child, Node *node, Tree *tree);
void insert_child_to_node(Node *child, Node *parent, Tree *tree);
void split_children(Node *original_parent, Node *first_parent, Node *second_parent, Tree *tree);
void insert_key_to_node(void *key, Node *node, Tree *tree);
void insert_record_to_leaf(Arena *arena, void *key, void *value, Node *node, Tree *tree);
void insert_record_to_tree(Arena *arena, void *key, void *value, Tree *tree);
void *search(void *key, Tree *tree);
void range_query(Tree *tree, void *start_key, void *end_key);
void merge_node_with_sibling(Node *node, Node *sibling, Tree *tree);
void remove_key_from_node(void *key, Node *node, Tree *tree);
void merge_leaf_with_sibling(Arena *arena, Node *leaf, Node *sibling, int sibling_dir, Tree *tree);
void remove_record(Arena *arena, void *key, Tree *tree);
void list_tree(Tree *tree);

#endif
