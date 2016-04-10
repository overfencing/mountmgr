
#ifndef __MOUNTMGR_COMMON_H__
#define __MOUNTMGR_COMMON_H__


#include <stddef.h>


#define SOCKFILE "/var/run/tamd.socket"
#define SOCKMODE 0666


#ifdef __cplusplus
extern "C" {
#endif

ssize_t recv_full(int fd, void *buf, size_t size);
ssize_t send_full(int fd, const void *buf, size_t size);

#ifdef __cplusplus
}
#endif


#endif /* __MOUNTMGR_COMMON_H__ */

