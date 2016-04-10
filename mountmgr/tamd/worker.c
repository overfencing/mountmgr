
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <pthread.h>
#include <sys/queue.h>

#include "event.h"
#include "log.h"
#include "worker.h"


struct worker
{
    pthread_t tid;

    STAILQ_HEAD(event_list, event) event_list;
    volatile size_t event_n;
    pthread_mutex_t event_lock;
    pthread_cond_t event_pushed;

    STAILQ_ENTRY(worker) entry;
};

static STAILQ_HEAD(worker_list, worker) g_worker_list;
static size_t g_worker_n;

static volatile size_t g_spawning;


static struct worker *create_worker()
{
    struct worker *worker;

    worker = (struct worker *)malloc(sizeof(*worker));
    if(worker == NULL)
        log_fatal_errno();

    STAILQ_INIT(&worker->event_list);

    worker->event_n = 0;

    errno = pthread_mutex_init(&worker->event_lock, NULL);
    if(errno)
        log_fatal_errno();

    errno = pthread_cond_init(&worker->event_pushed, NULL);
    if(errno)
        log_fatal_errno();

    return worker;
}


struct worker *get_free_worker()
{
    struct worker *free_worker = NULL;
    size_t min_event_n = SIZE_MAX;
    struct worker *worker;

    STAILQ_FOREACH(worker, &g_worker_list, entry) {
        size_t event_n = worker->event_n;

        if(event_n < min_event_n) {
            free_worker = worker;
            min_event_n = event_n;
        }
    }

    return free_worker;
}

void push_event(struct worker *worker, struct event *event)
{
    pthread_mutex_lock(&worker->event_lock);

    STAILQ_INSERT_TAIL(&worker->event_list, event, entry);
    (void)__sync_add_and_fetch(&worker->event_n, 1);

    pthread_cond_signal(&worker->event_pushed);

    pthread_mutex_unlock(&worker->event_lock);
}

static struct event *pop_event(struct worker *worker)
{
    struct event *event;

    pthread_mutex_lock(&worker->event_lock);

    for(;;) {
        event = STAILQ_FIRST(&worker->event_list);
        if(event != NULL)
            break;

        pthread_cond_wait(&worker->event_pushed, &worker->event_lock);
    }

    STAILQ_REMOVE_HEAD(&worker->event_list, entry);

    pthread_mutex_unlock(&worker->event_lock);

    return event;
}


static void *worker_main(void *arg)
{
    struct worker *worker = (struct worker *)arg;

    log_info(LOG_COLOR_NONE, "Worker %#lx is ready", (unsigned long)worker->tid);

    (void)__sync_sub_and_fetch(&g_spawning, 1);

    for(;;) {
        struct event *event;

        event = pop_event(worker);

        event->ops->handle(event);
        event->ops->destroy(event);

        (void)__sync_sub_and_fetch(&worker->event_n, 1);
    }
}


void spawn_workers(size_t worker_n)
{
    pthread_attr_t attr;
    int i;

    log_info(LOG_COLOR_CYAN, "Spawning %zu worker(s)", worker_n);

    errno = pthread_attr_init(&attr);
    if(errno)
        log_fatal_errno();

    errno = pthread_attr_setstacksize(&attr, 0x400000);
    if(errno)
        log_fatal_errno();

    errno = pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
    if(errno)
        log_fatal_errno();

    errno = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if(errno)
        log_fatal_errno();

    STAILQ_INIT(&g_worker_list);
    g_spawning = worker_n;

    for(i = 0; i < worker_n; ++i) {
        struct worker *worker = create_worker();

        errno = pthread_create(&worker->tid, &attr, worker_main, worker);
        if(errno)
            log_fatal_errno();

        STAILQ_INSERT_TAIL(&g_worker_list, worker, entry);
    }

    log_info(LOG_COLOR_WHITE, "Waiting until all workers are ready");

    do usleep(100000);
    while(g_spawning > 0);

    g_worker_n = worker_n;
}

