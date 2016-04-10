
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include "conf.h"
#include "drivemgr.h"
#include "listener.h"
#include "log.h"
#include "worker.h"


#define DEFAULT_CONF_FILE CFG_RELEASE_DIR "/etc/tamd.conf"


#define LOCKFILE "/var/run/tamd.pid"
#define LOCKMODE 0644


static pid_t is_already_running()
{
    int fd;
    struct flock fl;
    char buf[0x10];
    ssize_t n, len;

    fd = open(LOCKFILE, O_CREAT | O_RDWR, LOCKMODE);
    if(fd == -1)
        log_fatal_errno();

    if(fcntl(fd, F_SETFD, FD_CLOEXEC) == -1)
        log_fatal_errno();

    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;
    if(fcntl(fd, F_SETLK, &fl) == -1) {
        if(errno == EAGAIN || errno == EACCES) {
            pid_t pid;

            do n = read(fd, buf, sizeof(buf) - 1);
            while(n == -1 && errno == EINTR);
            if(n == -1)
                log_fatal_errno();
            buf[n] = '\0';

            pid = (pid_t)atoi(buf);
            if(!pid) {
                errno = EINVAL;
                log_fatal_errno();
            }

            close(fd);

            return pid;
        }
        else
            log_fatal_errno();
    }

    if(ftruncate(fd, 0) == -1)
        log_fatal_errno();

    len = snprintf(buf, sizeof(buf), "%u", (unsigned int)getpid());
    do n = write(fd, buf, len);
    while(n == -1 && errno == EINTR);
    if(n != len)
        log_fatal_errno();

    return (pid_t)(0);
}


int main(int argc, char *argv[])
{
    int opt;
    const char *conf_file = NULL;
    pid_t pid;

    opterr = 0;
    while((opt = getopt(argc, argv, "c:h")) != -1)
        switch(opt) {
        case 'c':
            conf_file = optarg;
            break;
        case 'h':
        default:
            fprintf(stderr, "Usage: %s [-c configuration_file] [-h]\n", argv[0]);
            exit(EXIT_FAILURE);
        }

    init_log();

    pid = is_already_running();
    if(pid)
        log_fatal(LOG_COLOR_FATAL, "Another instance of tamd is already running: %u", (unsigned int)pid);

    if(conf_file == NULL)
        conf_file = DEFAULT_CONF_FILE;
    load_conf(conf_file);

    init_drive_manager();

    spawn_workers(GET_CONF(worker_n));
    init_listener();

    add_timer(0xc0ffee, GET_CONF(delay_unmount_msec), 1);

    listener_main();
}

