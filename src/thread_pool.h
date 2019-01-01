#ifndef _THREAD_POOL_H_
#define _THREAD_POOL_H_ 1

#include "types.h"
#include "doubly_linked_list.h"

typedef void * (*thread_routine_t)( void * arg );
typedef struct thread_pool thp_pool_t;

retcode thp_pool_init( int           threads_count, 
                       thp_pool_t ** pthpool);

retcode thp_pool_add_work( thp_pool_t       * thpool, 
                           thread_routine_t   func,
                           void             * arg );

void thp_pool_execute( thp_pool_t * thpool );

retcode thp_pool_thread_join( thp_pool_t * thpool );

retcode thp_pool_destroy( thp_pool_t * thpool );

#endif /* _THREAD_POOL_H_ */
