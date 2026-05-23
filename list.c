#include "list.h"

#include <stdlib.h>

#include "errors.h"

list* list_init(void) {
  list* lst = malloc_safe(sizeof(list));
  lst->size = 0;
  lst->head = NULL;
  lst->tail = NULL;
  return lst;
}

list_node* list_push_front(list* lst, void* data) {
  list_node* new_node = malloc_safe(sizeof(list_node));
  new_node->data = data;
  new_node->next = lst->head;
  lst->head = new_node;
  if (lst->size == 0) {
    lst->tail = new_node;
  }
  ++lst->size;
  return new_node;
}

list_node* list_push_back(list* lst, void* data) {
  list_node* new_node = malloc_safe(sizeof(list_node));
  new_node->data = data;
  new_node->next = NULL;
  if (lst->size == 0) {
    lst->head = new_node;
  } else {
    lst->tail->next = new_node;
  }
  lst->tail = new_node;
  ++lst->size;
  return new_node;
}

list_node* list_insert(list* lst, list_node* node, void* data) {
  list_node* new_node = malloc_safe(sizeof(list_node));
  new_node->data = data;
  new_node->next = node->next;
  node->next = new_node;
  if (node == lst->tail) {
    lst->tail = new_node;
  }
  ++lst->size;
  return new_node;
}

list_node* list_next(list_node* node) { return node->next; }

void list_destroy(list* lst, void (*free_data)(void*)) {
  list_node* head = lst->head;
  while (head) {
    list_node* cur = head;
    head = head->next;
    free_data(cur->data);
    free(cur);
  }
  free(lst);
}
