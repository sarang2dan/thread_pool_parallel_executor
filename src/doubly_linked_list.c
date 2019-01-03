#include "doubly_linked_list.h"
#ifndef NULL
#define NULL  ((void *)0)
#endif

void dlist_init( dlist_t * head )
{
  head->prev = head;
  head->next = head;
  head->obj = NULL;
}

#define dlist_node_init( node )   dlist_init( (dlist_t *)(node) )

void dlist_init_obj( dlist_node_t * node, void * obj )
{
  node->prev = node;
  node->next = node;
  node->obj = obj;
}

dlist_node_t * dlist_get_first_node( dlist_t * head )
{
  return head->next;
}

dlist_node_t * dlist_get_last_node( dlist_t * head )
{
  return head->prev;
}

dlist_node_t * dlist_get_next_node( dlist_node_t * node )
{
  return node->next;
}

dlist_node_t * dlist_get_prev_node( dlist_node_t * node )
{
  return node->prev;
}

bool dlist_is_empty( dlist_t * head )
{
  return (((head->prev == head) && (head->next == head)) ? true : false);
}

void dlist_insert_after( dlist_node_t * pivot_node,
                         dlist_node_t * add_node )
{
  pivot_node->next->prev = add_node;
  add_node->next         = pivot_node->next;
  add_node->prev         = pivot_node;
  pivot_node->next       = add_node;
}

void dlist_insert_before( dlist_node_t * pivot_node,
                          dlist_node_t * add_node )
{
  pivot_node->prev->next = add_node;
  add_node->prev         = pivot_node->prev;
  pivot_node->prev       = add_node;
  add_node->next         = pivot_node;
}

void dlist_add_first( dlist_t * head, 
                      dlist_node_t * add_node )
{
  dlist_insert_before( head->next, add_node );
}

void dlist_add_last( dlist_t * head,
                     dlist_node_t * add_node )
{
  dlist_insert_after( head->prev, add_node );
}

dlist_node_t * dlist_delete_node( dlist_node_t * node )
{
  node->next->prev = node->prev;
  node->prev->next = node->next;
  node->prev = node;
  node->next = node;
  return node;
}
