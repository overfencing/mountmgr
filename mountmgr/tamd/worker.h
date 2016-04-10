
#ifndef __WORKER_H__
#define __WORKER_H__


#include <stddef.h>

#include "event.h"


struct worker;


struct worker *get_free_worker();
void push_event(struct worker *worker, struct event *event);

void spawn_workers(size_t worker_n);


#endif /* __WORKER_H__ */

