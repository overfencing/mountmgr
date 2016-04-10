
#ifndef __EVENT_H__
#define __EVENT_H__


#include <stddef.h>
#include <unistd.h>
#include <sys/queue.h>
#include <sys/event.h>


struct event;

struct event_ops
{
    size_t size;

    int (*create)(struct event *, const struct kevent *);
    void (*destroy)(struct event *);
    void (*handle)(struct event *);
};

struct event
{
    const struct event_ops *ops;

    STAILQ_ENTRY(event) entry;
};


struct event *create_event(const struct kevent *ke);
void destroy_event(struct event *event);


#endif /* __EVENT_H__ */

