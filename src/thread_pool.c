#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>

#if defined(__linux__)
#include <sys/prctl.h>
#endif /* __linux__ */

#include "types.h"
#include "atomic.h"
#include "doubly_linked_list.h"
#include "thread_pool.h"

#if 0 
#ifdef __darwin__
extern const int sys_nerr;
#define errno sys_nerr
#else
extern const int errno;
#endif
#endif

/* NOTICE:
 * Prefix "thp" is often used throughout the source code.
 * "thp" is acronym from "thread pool". */

typedef struct thread_job thp_job_t;
struct thread_job
{
  dlist_node_t      link;
  thread_routine_t  func;
  void            * func_arg;
};

/* Job queue */
typedef struct job_queue thp_jobq_t;
struct job_queue
{
  dlist_t          head;
  pthread_mutex_t  mutex;  /* used for queue r/w access */
};

#define JOB_ENQUE( head, job )    \
  (dlist_add_last( (head), (dlist_node_t *)job ))

#define JOB_DEQUE( head )   \
  ((thp_job_t *)dlist_delete_node( dlist_get_first_node( (head) ) ))

/* Thread */
typedef struct thp_thread thp_thread_t;
struct thp_thread
{
  int          id;      /* friendly id               */
  pthread_t    tid;     /* pointer to actual thread  */
  thp_pool_t * thpool;  /* access to thpool          */
};

/* Threadpool */
struct thread_pool
{
  thp_thread_t  ** threads;             /* pointer to threads        */
  int              threads_count;       /* pointer to threads        */
  volatile bool    execute_jobs;
  volatile bool    exit_all;
  volatile int     alive_thr_cnt;       /* threads currently alived  */
#ifdef LOCK_VERSION
  mutex_t          mutex_alive_thr_cnt;  /* used for thread count etc */
#endif /* LOCK */
  thp_jobq_t       jq;                  /* job queue                 */
};

static void thp_pool_exit_all( thp_pool_t * thpool );

static retcode thp_thread_init( thp_pool_t    * thpool, 
                                thp_thread_t ** _thr, 
                                int            id );

static void * thp_thread_do( void * arg );
static void thp_thread_destroy( thp_thread_t * thr );

static retcode thp_jobq_init( thp_jobq_t * jq );
static void thp_jobq_enque( thp_jobq_t * jq, thp_job_t * newjob );

static thp_job_t * thp_jobq_deque( thp_jobq_t * jq );
static void thp_jobq_destroy( thp_jobq_t * jq );

retcode thp_pool_init( int           threads_count, 
                   thp_pool_t ** athpool )
{
  int             i     = 0;
  int             state = 0;
  retcode         ret   = FAIL;
  thp_pool_t    * thpool = NULL;

  TRY( athpool == NULL );
  TRY( threads_count < 0 );

  /* Make new thread pool */
  thpool = (thp_pool_t *)malloc( sizeof(thp_pool_t) );
  TRY_GOTO( thpool == NULL, err_fail_alloc_thpool );
  state = 1;

  thpool->alive_thr_cnt = 0;

  /* Initialise the job queue */
  ret = thp_jobq_init( &(thpool->jq) );
  TRY( ret != SUCCESS );
  state = 2;

   /* Make threads in pool */
  thpool->threads = (thp_thread_t **)malloc( 
                    sizeof(thp_thread_t *) * threads_count );
  TRY_GOTO( thpool->threads == NULL, err_fail_alloc_threads );
  state = 3;

  thpool->threads_count = threads_count;

#ifdef LOCK_VERSION
  ret = pthread_mutex_init( &(thpool->lock_alive_thr_cnt),
                            NULL );
  TRY( ret != SUCCESS );
#endif

  /* we must set "running", before create threads */
  thpool->execute_jobs = false;
  thpool->exit_all = false;

  /* Thread init */
  for( i = 0; i < threads_count ; i++ )
    {
      ret = thp_thread_init( thpool,
                             &(thpool->threads[i]),
                             i /* id */ );
      TRY( ret != SUCCESS );
    }


  /* Wait for threads to initialize */
  while( thpool->alive_thr_cnt != threads_count )
    {
      NULL; /* do nothing */
    }

  *athpool = thpool;

  return SUCCESS;
  
  CATCH( err_fail_alloc_thpool )
    {
      fprintf( stderr,
              "%s(): could not allocate memory for job queue\n",
              __func__ );
    }
  CATCH( err_fail_alloc_threads )
   {
     fprintf( stderr,
             "%s(): Could not allocate memory for threads \n",
             __func__ );
   }
  CATCH_END;

  switch( state )
    {
    case 3:
      free( thpool->threads );
    case 2:
      thp_jobq_destroy( &(thpool->jq) );
    case 1:
      free( thpool );
      *athpool = NULL;
    default:
      break;
    }

  return FAIL;
}

/* Add work to the thread pool */
retcode thp_pool_add_work( thp_pool_t       * thpool, 
                           thread_routine_t   func,
                           void             * func_arg )
{
  thp_job_t * newjob = NULL;

  newjob = (thp_job_t *)malloc( sizeof(thp_job_t) );
  TRY_GOTO( newjob == NULL, err_fail_alloc_job );

  /* add function and argument */
  newjob->func = func;
  newjob->func_arg = func_arg;

  /* add job to queue */
  thp_jobq_enque( &(thpool->jq), newjob );

  return SUCCESS;

  CATCH( err_fail_alloc_job )
    {
      fprintf( stderr,
               "%s(): Could not allocate memory for new job\n",
               __func__ );
    }
  CATCH_END;

  return FAIL;
}

void thp_pool_execute( thp_pool_t * thpool )
{
  assert( thpool != NULL );
  thpool->execute_jobs = true;
}

static void thp_pool_exit_all( thp_pool_t * thpool )
{
  assert( thpool != NULL );
  thpool->exit_all = true;
}

retcode thp_pool_thread_join( thp_pool_t * thpool )
{
  if( thpool != NULL )
    {
      /* End each thread 's infinite loop */
      thp_pool_exit_all( thpool );

      while( thpool->alive_thr_cnt > 0 )
        {
          usleep( 10 );
        }
    }

  return SUCCESS;
}

/* Destroy the threadpool */
retcode thp_pool_destroy( thp_pool_t * thpool )
{
  int i;

  if( thpool != NULL )
    {
      TRY( thpool->alive_thr_cnt > 0 );

      /* Job queue cleanup */
      thp_jobq_destroy( &(thpool->jq) );

      /* Deallocs */
      for( i = 0; i < thpool->threads_count ; i++ )
        {
          thp_thread_destroy( thpool->threads[i] );
        }

      free( thpool->threads );

      free( thpool );
    }

  return SUCCESS;

  CATCH_END;

  return FAIL;
}

static retcode thp_thread_init( thp_pool_t    * thpool, 
                                thp_thread_t ** _thr, 
                                int             id )
{
  int            ret = 0;
  int            state = 0;
  thp_thread_t * thr = NULL;

  assert( _thr != NULL );
  assert( thpool != NULL );

  thr = (thp_thread_t *)malloc( sizeof(thp_thread_t) );
  TRY_GOTO( thr == NULL, err_fail_alloc_thread );
  state = 1;

  thr->thpool = thpool;
  thr->id = id;

  ret = pthread_create( &(thr->tid),
                        NULL,
                        thp_thread_do,
                        (void *)thr );
  TRY_GOTO( ret != 0, err_fail_create_thread );

  ret = pthread_detach( thr->tid );
  TRY_GOTO( ret != 0, err_fail_detach );

  *_thr = thr;

  return SUCCESS;

  CATCH( err_fail_alloc_thread )
    {
      fprintf( stderr,
               "%s(): Could not allocate memory for thread\n",
               __func__ );
      return -1;
    }
  CATCH( err_fail_create_thread )
    {
      fprintf( stderr,
               "%s(): fail to create thread [%s] \n",
               __func__,
               strerror(errno) );
      return -1;
    }
  CATCH( err_fail_detach )
    {
      fprintf( stderr,
               "%s(): fail to detach thread [%s]\n",
               __func__,
               strerror(errno) );
      return -1;
    }
  CATCH_END;

  if( state == 1 )
    {
      free( thr );
    }

  return FAIL;
}

static void * thp_thread_do( void * arg )
{
  char          thread_name[128] = { 0, }; /* Set thread name for profiling and debuging */
  thp_job_t    * job    = NULL;
  thp_thread_t * thr   = (thp_thread_t *)arg;
  thp_pool_t   * thpool = thr->thpool; /* Assure all threads have been created before starting serving */
  thp_jobq_t   * jq    = &(thpool->jq);

  thread_routine_t func;

  sprintf( thread_name, "seltree-thr-parallel-sort-%d", thr->id );
#if defined(__linux__)
  /* Use prctl instead to prevent using _GNU_SOURCE flag and implicit declaration */
  prctl( PR_SET_NAME, thread_name );
#elif defined(__APPLE__) && defined(__MACH__)
  pthread_setname_np( thread_name );
#else
  // thp_thread_do(): pthread_setname_np is not supported on this system 
#endif

#ifdef LOCK_VERSION
  pthread_mutex_lock( &thpool->lock_alive_thr_cnt );
  (thpool->alive_thr_cnt)++;
  pthread_mutex_unlock( &thpool->lock_alive_thr_cnt );
#else
  atomic_inc( &(thpool->alive_thr_cnt) );
#endif

  while( true )
    {
      if( thpool->execute_jobs == true )
        {
          job = thp_jobq_deque( jq );
          if( job != NULL )
            {
              func = job->func;
              func( job->func_arg );
              free( job );
            }
          else
            {
              usleep( 10 );
            }
        }
      else
        {
          usleep( 10 );
        }

      if( (thpool->exit_all == true) &&
          (dlist_is_empty( &(jq->head) ) == true) )
        {
          break;
        }
    }

#ifdef LOCK_VERSION
  pthread_mutex_lock( &thpool->lock_alive_thr_cnt );
  (thpool->alive_thr_cnt)--;
#ifdef THP_DEBUG
  fprintf( stderr, 
           "[tid:%d] (alive_thr_cnt:%d\n",
           thr->id,
           thpool->alive_thr_cnt );
  fflush( stderr );
#endif /* THP_DEBUG */
  pthread_mutex_unlock( &thpool->lock_alive_thr_cnt );
#else
  atomic_dec( &(thpool->alive_thr_cnt) );
#endif 

  return NULL;
}

/* Frees a thread  */
static void thp_thread_destroy( thp_thread_t * thr )
{
  free( thr );
}

/* initialize queue */
static retcode thp_jobq_init( thp_jobq_t * jq )
{
  int ret = 0;

  dlist_init( &(jq->head) );

  ret = pthread_mutex_init( &(jq->mutex), 0 /* shared_p */ );
  TRY( ret != 0 );

  return SUCCESS;

  CATCH_END;

  return FAIL;
}

/* Add (allocated) job to queue
*/
static void thp_jobq_enque( thp_jobq_t * jq, thp_job_t * newjob )
{
  pthread_mutex_lock( &jq->mutex );

  JOB_ENQUE( &(jq->head), newjob );

  pthread_mutex_unlock( &jq->mutex );
}

static thp_job_t * thp_jobq_deque( thp_jobq_t * jq )
{
  thp_job_t * job = NULL;

  pthread_mutex_lock( &jq->mutex );

  if( dlist_is_empty( &(jq->head) ) == false )
    {
      // dequeue
      job = JOB_DEQUE( &(jq->head) );
    }

  pthread_mutex_unlock( &jq->mutex );

  return job;
}

/* Free all queue resources back to the system */
static void thp_jobq_destroy( thp_jobq_t * jq )
{
  thp_job_t * job = NULL;

  while( dlist_is_empty( &(jq->head) ) == false )
    {
      job = JOB_DEQUE( &(jq->head) );
      if( job != NULL )
        {
          free( job );
        }
    }

  (void)thp_jobq_init( jq );
}
