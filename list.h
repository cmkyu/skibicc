#ifndef SKIBICC_LIST_H
#define SKIBICC_LIST_H

#include <stddef.h>

struct list_node;
typedef struct list_node {
  void* data;
  struct list_node* next;
} list_node;

typedef struct list {
  list_node* head;
  list_node* tail;
  size_t size;
} list;

list* list_init(void);

list_node* list_push_front(list* lst, void* data);

list_node* list_push_back(list* lst, void* data);

list_node* list_insert(list* lst, list_node* node, void* data);

list_node* list_next(list_node* node);

void list_destroy(list* lst, void (*free_data)(void*));

#endif  // SKIBICC_LIST_H
