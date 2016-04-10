
#include <unistd.h>
#include <sys/event.h>

#include "conf.h"
#include "drive.h"
#include "event.h"
#include "event_unmount.h"
#include "listener.h"
#include "log.h"


struct event_unmount
{
    struct event event;

    int ident;
};


static int event_unmount_create(struct event *event, const struct kevent *ke)
{
    struct event_unmount *this = (struct event_unmount *)event;

    this->ident = ke->ident;

    return 0;
}

static void event_unmount_handle(struct event *event)
{
    struct event_unmount *this = (struct event_unmount *)event;

    run_unmount_manager();

    add_timer(this->ident, GET_CONF(delay_unmount_msec), 0);
}


const struct event_ops event_unmount_ops = {
    .size = sizeof(struct event_unmount),
    .create = event_unmount_create,
    .destroy = destroy_event,
    .handle = event_unmount_handle,
};

