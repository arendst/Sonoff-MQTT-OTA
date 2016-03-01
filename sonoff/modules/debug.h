#ifndef USER_DEBUG_H_
#define USER_DEBUG_H_

#if defined(GLOBAL_DEBUG_ON)
#define INFO( format, ... ) os_printf( format, ## __VA_ARGS__ )
#else
#define INFO( format, ... )
#endif

#endif /* USER_DEBUG_H_ */
