#ifndef _TYPES_H_
#define _TYPES_H_ 1

/* return code */
typedef enum
{
  SUCCESS = 0,
  FAIL    = -1
} retcode;

#ifndef __cpulsplus
typedef enum 
{
  false = 0, 
  true  = 1
} bool;
#endif

#define TRY( _cond ) if( _cond ) { goto _label_catch_end; }
#define TRY_GOTO( _cond, _label ) if( _cond ) { goto _label; }
#define CATCH( _label ) \
 goto _label_catch_end; \
 _label:
#define CATCH_END  _label_catch_end:

#endif /* _TYPES_H_ */
