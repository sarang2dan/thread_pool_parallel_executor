#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>

#include "thread_pool.h"
#include "types.h"
#include "atomic.h"

#define MAX_JOBS_CNT 100
#define MAX_THREAD_CNT 10

typedef struct _test_job_arg test_job_arg_t;
struct _test_job_arg
{
  volatile int * count;
  pthread_mutex_t * lock;
};

void * test_routine( void * arg )
{
  test_job_arg_t * jobarg = (test_job_arg_t *)arg;

  TRY_GOTO( pthread_mutex_lock( jobarg->lock ) != 0,
            err_fail_lock );

  atomic_inc_32(jobarg->count);
  printf("count:%06d\n", *(jobarg->count) );

  pthread_mutex_unlock( jobarg->lock );

  return NULL;

  CATCH( err_fail_lock )
    {
      fprintf(stderr, "lock error!:%s\n", strerror(errno) );
      abort();
    }
  CATCH_END;
}

int main( void )
{
  long     i      = 0;
  retcode  ret    = FAIL;
  int      state = 0;
  int      count = 0;
  pthread_mutex_t lock;

  thp_pool_t     * thpool = NULL;
  test_job_arg_t * jobargs = NULL;

  TRY_GOTO( pthread_mutex_init( &lock, NULL ) != 0, err_fail_init_mutex );

  jobargs = (test_job_arg_t *)malloc( sizeof(test_job_arg_t) * MAX_JOBS_CNT );
  TRY_GOTO( jobargs == NULL, err_fail_alloc_jobargs );
  state = 1;

  ret = thp_pool_init( MAX_THREAD_CNT, &thpool );
  TRY( ret != SUCCESS );
  state = 2;

  for( i = 0 ; i < MAX_JOBS_CNT ; i++ )
    {
      jobargs[i].count  = &count;
      jobargs[i].lock   = &lock;
      TRY_GOTO( thp_pool_add_work( thpool,
                                   test_routine,
                                   (void *)&(jobargs[i]) )
                != SUCCESS, err_fail_add_work );
    }

  thp_pool_execute( thpool ); 

  ret = thp_pool_thread_join( thpool );
  TRY( ret != SUCCESS );

  state = 1;
  ret = thp_pool_destroy( thpool );
  TRY( ret != SUCCESS );

  state = 0;
  free( jobargs );

  return SUCCESS;

  CATCH( err_fail_init_mutex )
    {
      fprintf( stderr, 
               "%s(): fail to init mutex\n",
               __func__ );
    }
  CATCH( err_fail_alloc_jobargs )
    {
      fprintf( stderr, 
               "%s(): fail to alloc jobargs\n",
               __func__ );
    }
  CATCH( err_fail_add_work )
    {
      fprintf( stderr, 
               "%s(): fail to add jobs\n",
               __func__ );
    }
  CATCH_END;

  switch( state )
    {
    case 2:
      (void)thp_pool_destroy( thpool );
    case 1:
      free( jobargs );
    default:
      break;
    }

  return FAIL;
}
