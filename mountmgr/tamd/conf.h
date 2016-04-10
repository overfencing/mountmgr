
#ifndef __CONF_H__
#define __CONF_H__


#include <stddef.h>


#define GET_CONF(key) get_conf_##key()


#define DECLARE_CONF(key, type) type GET_CONF(key)

DECLARE_CONF(worker_n, size_t);
DECLARE_CONF(delay_unmount_msec, unsigned int);


void load_conf(const char *conf_file);


#endif /* __CONF_H__ */

