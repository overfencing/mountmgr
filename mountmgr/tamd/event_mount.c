
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/event.h>

#include "drive.h"
#include "event.h"
#include "event_mount.h"
#include "listener.h"
#include "log.h"


static char *find_key(const char *key, const char *msg)
{
    char *val;

    val = strstr(msg, key);
    if(val == NULL)
        return NULL;

    val += strlen(key);

    return val;
}

static void make_token(char *val)
{
    while(*val != ' ' && *val != '\n' && *val != '\0')
        val++;

    *val = '\0';
}

static int is_usb(const char *dev_name)
{
    return (
        dev_name[0] == 'd' &&
        dev_name[1] == 'a' &&
        isdigit(dev_name[2]) &&
        dev_name[3] == '\0'
    );
}

static int is_cdrom(const char *dev_name)
{
    return (
        dev_name[0] == 'c' &&
        dev_name[1] == 'd' &&
        isdigit(dev_name[2]) &&
        dev_name[3] == '\0'
    );
}

static int is_repeated(const char *dev_name)
{
    static int repeated[10] = {0};

    int *p = &repeated[dev_name[2] - '0'];
    int flag = *p;

    *p = !flag;
    return flag;
}

static void run_mount_manager_lazily(const char *dev_name)
{
    log_info(LOG_COLOR_GREEN, "Attached: %s", dev_name);

    sleep(2);

    run_mount_manager(dev_name);
}

static void handle_devd_msg(char *msg)
{
    char *msg_system;
    char *msg_subsystem;
    char *msg_type;
    char *msg_cdev;

    msg_system = find_key("system=", msg);
    msg_subsystem = find_key("subsystem=", msg);
    msg_type = find_key("type=", msg);
    msg_cdev = find_key("cdev=", msg);

    if(!msg_system || !msg_subsystem || !msg_type || !msg_cdev)
        return;

    make_token(msg_system);
    make_token(msg_subsystem);
    make_token(msg_type);
    make_token(msg_cdev);

    if(strcmp(msg_system, "DEVFS") || strcmp(msg_subsystem, "CDEV"))
        return;

    if(!strcmp(msg_type, "CREATE") && is_usb(msg_cdev))
        run_mount_manager_lazily(msg_cdev);
    else if(!strcmp(msg_type, "MEDIACHANGE") && is_cdrom(msg_cdev)) {
        if(!is_repeated(msg_cdev))
            run_mount_manager_lazily(msg_cdev);
    }
}


struct event_mount
{
    struct event event;

    int devd_sock;
    FILE *devd_fp;
};


static int event_mount_create(struct event *event, const struct kevent *ke)
{
    struct event_mount *this = (struct event_mount *)event;

    this->devd_sock = ke->ident;
    this->devd_fp = get_devd_fp();

    return 0;
}

static void event_mount_handle(struct event *event)
{
    struct event_mount *this = (struct event_mount *)event;

    char msg[0x1000];

    if(fgets(msg, sizeof(msg), this->devd_fp) == NULL)
        log_fatal_errno();

    enable_listening_fd(this->devd_sock);

    handle_devd_msg(msg);
}


const struct event_ops event_mount_ops = {
    .size = sizeof(struct event_mount),
    .create = event_mount_create,
    .destroy = destroy_event,
    .handle = event_mount_handle,
};

