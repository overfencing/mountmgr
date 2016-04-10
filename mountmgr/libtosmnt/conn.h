
#ifndef __CONN_H__
#define __CONN_H__


#include <jansson.h>
#include <mountmgr/req_id.h>


json_t *send_request(enum req_id req_id, const json_t *arg, int *err_code);


#endif /* __CONN_H__ */

