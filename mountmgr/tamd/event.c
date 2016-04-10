
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/event.h>

#include "event.h"
#include "event_ipc.h"
#include "event_mount.h"
#include "event_unmount.h"
#include "listener.h"
#include "log.h"


struct event *create_event(const struct kevent *ke)
{
    struct event *event = NULL;
    const struct event_ops *ops;

    switch(ke->filter) {
    case EVFILT_READ:
        if(is_devd_sock(ke->ident))
            ops = &event_mount_ops;
        else
            ops = &event_ipc_ops;
        break;
    case EVFILT_TIMER:
        if(ke->ident == 0xc0ffee) {
            ops = &event_unmount_ops;
            break;
        }
    default:
        log_fatal(LOG_COLOR_FATAL, "Unknown event type (ident=%#x filter=%hd)", ke->ident, ke->filter);
    }

    event = (struct event *)malloc(ops->size);
    if(event == NULL)
        goto error;

    memset(event, 0, ops->size);

    event->ops = ops;

    if(event->ops->create != NULL)
        if(event->ops->create(event, ke) == -1)
            goto error;

    return event;

error:
    if(event)
        destroy_event(event);

    return NULL;
}

void destroy_event(struct event *event)
{
    free(event);
}

