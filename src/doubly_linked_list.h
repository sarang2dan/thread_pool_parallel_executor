#ifndef _DOUBLY_LINKED_LIST_H_
#define _DOUBLY_LINKED_LIST_H_ 1

#include "types.h"

typedef struct _list_node dlist_node_t;
typedef struct _list_node dlist_t;

struct _list_node
{
  dlist_node_t * prev;
  dlist_node_t * next;
  void         * obj;
};

#define DLIST_ITERATE(head, iter)                       \
  for ((iter) = (head)->next;                           \
       (iter) != (head);                                \
       (iter) = (iter)->next)

#define DLIST_ITERATE_FROM(head, node, iter)            \
  for ((iter) = node;                                   \
       (iter) != (head);                                \
       (iter) = (iter)->next)

#define DLIST_ITERATE_BACK(head, iter)                  \
  for ((iter) = (head)->prev;                           \
       (iter) != (head);                                \
       (iter) = (iter)->prev)

#define DLIST_ITERATE_SAFE(head, iter, iter_next)           \
  for ((iter) = (head)->next, (iter_next) = (iter)->next;   \
       (iter) != (head);                                    \
       (iter) = (iter_next), (iter_next) = (iter)->next)

#define DLIST_ITERATE_BACK_SAFE(head, iter, iter_next)     \
  for ( (iter) = (head)->prev, (iter_next) = (iter)->prev; \
        (iter) != (head);                                  \
        (iter) = (iter_next), (iter_next) = (iter)->prev)

#define DLIST_ITERATE_SAFE_FROM(head, node, iter, iter_next)     \
  for ((iter) = node, (iter_next) = (iter)->next;                \
       (iter) != (head);                                         \
       (iter) = (iter_next), (iter_next) = (iter)->next)

void dlist_init( dlist_t * head );

#define dlist_node_init( node )   dlist_init( (dlist_t *)(node) )

void dlist_init_obj( dlist_node_t * node, void * obj );

dlist_node_t * dlist_get_first_node( dlist_t * head );

dlist_node_t * dlist_get_last_node( dlist_t * head );

dlist_node_t * dlist_get_next_node( dlist_node_t * node );

dlist_node_t * dlist_get_prev_node( dlist_node_t * node );

bool dlist_is_empty( dlist_t * head );

void dlist_insert_after( dlist_node_t * pivot_node,
                         dlist_node_t * add_node );

void dlist_insert_before( dlist_node_t * pivot_node,
                          dlist_node_t * add_node );

void dlist_add_first( dlist_t      * head, 
                      dlist_node_t * add_node );

void dlist_add_last( dlist_t      * head,
                     dlist_node_t * add_node );

dlist_node_t * dlist_delete_node( dlist_node_t * node );

#endif /* _DOUBLY_LINKED_LIST_H_ */
