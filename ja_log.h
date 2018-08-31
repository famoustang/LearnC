

#ifndef __JA_LOG_H__
#define __JA_LOG_H__
	
#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include "ja_type.h"
#include "slog.h"

#define JLOG		SLOG
#define JA_TRACE	SLOG_TRACE
#define JA_ASSERT	SLOG_ASSERT

#ifdef WIN32
#define JA_LOG_ERRNO 0
#else
#define JA_LOG_ERRNO errno
#endif

#define ja_return_if_fail(expr)		JA_STMT_START{			\
     if (expr) { } else       					\
       {								\
	 		JA_TRACE (#expr);				\
	 		return;							\
       };	\
	 }JA_STMT_END

#define ja_goto_if_fail(expr, where)		JA_STMT_START{			\
		  if (expr) { } else 						 \
			{								 \
				 JA_TRACE (#expr); 			 \
				 goto where;						 \
			};	 \
		  }JA_STMT_END
	 

#define ja_return_val_if_fail(expr,val)	JA_STMT_START{			\
		if (expr) { } else						\
		{								\
			JA_TRACE (#expr);				\
			return (val);							\
		};		\
	}JA_STMT_END

/* int type as return val, true for 0, fail for non-zero	*/

#define ja_z_return_if_fail(expr)		JA_STMT_START{			\
	 if (0 == (expr)) { } else 						\
	   {								\
			JA_TRACE (#expr"; errno:%d", JA_LOG_ERRNO);				\
			return; 						\
	   };	\
	 }JA_STMT_END

#define ja_z_goto_if_fail(expr, where)		JA_STMT_START{			\
	  if (0 == (expr)) { } else						 \
		{								 \
		  JA_TRACE (#expr"; errno:%d", JA_LOG_ERRNO);			  \
			 goto where;						 \
		};	 \
	  }JA_STMT_END
	 

#define ja_z_return_val_if_fail(expr,val)	JA_STMT_START{			\
		if (0 == (expr)) { } else						\
		{								\
			JA_TRACE (#expr"; errno:%d", JA_LOG_ERRNO);				\
			return (val);							\
		};		\
	}JA_STMT_END	

/* bool type as return val  */
#define ja_b_return_if_fail(expr)		JA_STMT_START{			\
		 if (JA_TRUE == (expr)) { } else						\
		   {								\
				JA_TRACE (#expr"; errno:%d", JA_LOG_ERRNO);				\
				return; 						\
		   };	\
		 }JA_STMT_END
	
#define ja_b_goto_if_fail(expr, where)		JA_STMT_START{			\
		  if (JA_TRUE == (expr)) { } else 					 \
			{								 \
			  JA_TRACE (#expr"; errno:%d", JA_LOG_ERRNO);			  \
				 goto where;						 \
			};	 \
		  }JA_STMT_END
		 
	
#define ja_b_return_val_if_fail(expr,val)	JA_STMT_START{			\
			if (JA_TRUE == (expr)) { } else						\
			{								\
				JA_TRACE (#expr"; errno:%d", JA_LOG_ERRNO);				\
				return (val);							\
			};		\
		}JA_STMT_END	

/**/
#define ja_z_return_if_fail2(expr, r)		JA_STMT_START{			\
				 if (0 == ((r) = (expr))) { } else						\
				   {								\
						JA_TRACE (#expr"; errcode: 0x%x, errno:%d", r, JA_LOG_ERRNO);				\
						return; 						\
				   };	\
				 }JA_STMT_END
			
#define ja_z_goto_if_fail2(expr, r, where)		JA_STMT_START{			\
				  if (0 == ((r) = (expr))) { } else 					 \
					{								 \
					  JA_TRACE (#expr"; errcode: 0x%x, errno:%d", r, JA_LOG_ERRNO);			  \
						 goto where;						 \
					};	 \
				  }JA_STMT_END
				 
			
#define ja_z_return_val_if_fail2(expr, r, val)	JA_STMT_START{			\
					if (0 == ((r) = (expr))) { } else						\
					{								\
						JA_TRACE (#expr"; errcode: 0x%x, errno:%d", r, JA_LOG_ERRNO);				\
						return (val);							\
					};		\
				}JA_STMT_END	
			
			/* bool type as return val	*/
#define ja_b_return_if_fail2(expr, r)		JA_STMT_START{			\
					 if (JA_TRUE == ((r) = (expr))) { } else						\
					   {								\
							JA_TRACE (#expr"; errcode: 0x%x, errno:%d", r, JA_LOG_ERRNO);				\
							return; 						\
					   };	\
					 }JA_STMT_END
				
#define ja_b_goto_if_fail2(expr, r, where)		JA_STMT_START{			\
					  if (JA_TRUE == ((r) = (expr))) { } else					 \
						{								 \
						  JA_TRACE (#expr"; errcode: 0x%x, errno:%d", r, JA_LOG_ERRNO);			  \
							 goto where;						 \
						};	 \
					  }JA_STMT_END
					 
				
#define ja_b_return_val_if_fail2(expr, r,val)	JA_STMT_START{			\
						if (JA_TRUE == ((r) = (expr))) { } else 					\
						{								\
							JA_TRACE (#expr"; errcode: 0x%x, errno:%d", r, JA_LOG_ERRNO);				\
							return (val);							\
						};		\
					}JA_STMT_END	


#ifdef __cplusplus
}
#endif
#endif /* __JA_LOG_H__ */


