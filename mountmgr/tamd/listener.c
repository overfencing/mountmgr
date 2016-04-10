
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/event.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>

#include <mountmgr/common.h>

#include "event.h"
#include "listener.h"
#include "log.h"
#include "worker.h"


static int g_kq;
static int g_ipc_sock;
static int g_devd_sock;
static FILE *g_devd_fp;


static int is_ipc_sock(int sock)
{
    return (sock == g_ipc_sock);
}

static void destroy_ipc_sock()
{
    unlink(SOCKFILE);
}

static int create_ipc_sock()
{
    int ipc_sock;
    struct sockaddr_un addr;
    size_t len;

    log_info(LOG_COLOR_CYAN, "Creating a socket for IPC");

    destroy_ipc_sock();

    ipc_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if(ipc_sock == -1)
        log_fatal_errno();

    if(fcntl(ipc_sock, F_SETFD, FD_CLOEXEC) == -1)
        log_fatal_errno();

    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SOCKFILE);
    len = sizeof(addr) - sizeof(addr.sun_path) + strlen(addr.sun_path) + 1;
    addr.sun_len = len;

    if(bind(ipc_sock, (struct sockaddr *)&addr, len) == -1)
        log_fatal_errno();

    if(chmod(SOCKFILE, SOCKMODE) == -1)
        log_fatal_errno();

    if(listen(ipc_sock, SOMAXCONN) == -1)
        log_fatal_errno();

    log_info(LOG_COLOR_GREEN, "Socket created on %s (mode=%04o, fd=%d)", SOCKFILE, SOCKMODE, ipc_sock);

    return ipc_sock;
}


int is_devd_sock(int sock)
{
    return (sock == g_devd_sock);
}

FILE *get_devd_fp()
{
    return g_devd_fp;
}

static int open_devd_sock()
{
    static const char devd_path[] = "/var/run/devd.pipe";

    int devd_sock;
    struct sockaddr_un addr;
    size_t len;

    log_info(LOG_COLOR_CYAN, "Opening %s", devd_path);

    devd_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if(devd_sock == -1)
        log_fatal_errno();

    if(fcntl(devd_sock, F_SETFD, FD_CLOEXEC) == -1)
        log_fatal_errno();

    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, devd_path);
    len = sizeof(addr) - sizeof(addr.sun_path) + strlen(addr.sun_path) + 1;
    addr.sun_len = len;

    if(connect(devd_sock, (struct sockaddr *)&addr, len) == -1)
        log_fatal_errno();

    log_info(LOG_COLOR_GREEN, "Connected to %s (fd=%d)", devd_path, devd_sock);

    return devd_sock;
}

static FILE *open_devd_fp(int devd_sock)
{
    FILE *fp;

    fp = fdopen(devd_sock, "r");
    if(fp == NULL)
        log_fatal_errno();

    return fp;
}


static void add_listening_fd(int fd, u_short flags)
{
    struct kevent ke;

    log_info(LOG_COLOR_NONE, "Adding the socket: %d", fd);

    EV_SET(&ke, fd, EVFILT_READ, EV_ADD | flags, 0, 0, NULL);
    if(kevent(g_kq, &ke, 1, NULL, 0, NULL) == -1)
        log_fatal_errno();
}

void enable_listening_fd(int fd)
{
    struct kevent ke;

    EV_SET(&ke, fd, EVFILT_READ, EV_ENABLE, 0, 0, NULL);
    if(kevent(g_kq, &ke, 1, NULL, 0, NULL) == -1)
        log_fatal_errno();
}


void add_timer(int ident, int msec, int first)
{
    struct kevent ke;

    if(first)
        log_info(LOG_COLOR_NONE, "Adding a timer (ident=%#x, msec=%d)", ident, msec);

    EV_SET(&ke, ident, EVFILT_TIMER, EV_ADD | EV_ONESHOT, 0, msec, NULL);
    if(kevent(g_kq, &ke, 1, NULL, 0, NULL) == -1)
        log_fatal_errno();
}

void init_listener()
{
    log_info(LOG_COLOR_CYAN, "Initializing listener");

    g_kq = kqueue();
    if(g_kq == -1)
        log_fatal_errno();

    g_ipc_sock = create_ipc_sock();
    add_listening_fd(g_ipc_sock, 0);

    g_devd_sock = open_devd_sock();
    g_devd_fp = open_devd_fp(g_devd_sock);
    add_listening_fd(g_devd_sock, EV_DISPATCH);
}


static void accept_new_connection()
{
    int sock;

    sock = accept(g_ipc_sock, NULL, NULL);
    if(sock == -1)
        log_fatal_errno();

    if(fcntl(sock, F_SETFD, FD_CLOEXEC) == -1)
        log_fatal_errno();

    log_info(LOG_COLOR_NONE, "Client connected: %d", sock);

    add_listening_fd(sock, EV_DISPATCH);
}

static void close_connection(int sock)
{
    log_info(LOG_COLOR_NONE, "Client disconnected: %d", sock);

    close(sock);
}

static void dispatch_event(const struct kevent *ke)
{
    struct event *event;
    struct worker *worker;

    if(ke->filter == EVFILT_READ) {
        int sock = ke->ident;

        if(is_ipc_sock(sock)) {
            accept_new_connection();
            return;
        }
        else if(ke->flags & EV_EOF) {
            close_connection(sock);
            return;
        }
    }

    event = create_event(ke);
    if(event == NULL)
        log_fatal_errno();

    worker = get_free_worker();

    push_event(worker, event);
}

void listener_main()
{
    log_info(LOG_COLOR_GREEN, "tamd has started successfully");

    for(;;) {
        struct kevent ke[64];
        int i, n;

        n = kevent(g_kq, NULL, 0, ke, sizeof(ke) / sizeof(*ke), NULL);
        if(n == -1) {
            if(errno == EINTR)
                continue;

            log_fatal_errno();
        }

        for(i = 0; i < n; ++i)
            dispatch_event(&ke[i]);
    }
}

