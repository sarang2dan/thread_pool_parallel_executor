#ifndef _ATOMIC_H_
#define _ATOMIC_H_ 1
#define GCC
#if defined(GCC) || defined (llvm)
// 32-bit
#define atomic_inc( _ptr ) __sync_fetch_and_add( _ptr, 1 )
#define atomic_dec( _ptr ) __sync_fetch_and_sub( _ptr, 1 )

#define atomic_inc_32( _ptr ) __sync_fetch_and_add( _ptr, 1 )
#define atomic_dec_32( _ptr ) __sync_fetch_and_sub( _ptr, 1 )

#define atomic_inc_64( _ptr ) __sync_fetch_and_add( _ptr, 1 )
#define atomic_dec_64( _ptr ) __sync_fetch_and_sub( _ptr, 1 )

#else
// defines for another compiler
#endif /* compiler */
#endif /* _ATOMIC_H_ */
