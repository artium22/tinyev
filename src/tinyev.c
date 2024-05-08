/**
 * @file tinyev.c
 * @author Artium Acheldaev
 * @brief this module is a basic and very slim event loop that manages
 * sockets and timers for the user. All the rest of the work (networking,
 * for example) is in the hands of the user.
 * @version 0.0.1
 * @date 2023-11-17
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/epoll.h>
#include <sys/fcntl.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <errno.h>

#include "log.h"
#include "tinyev.h"

/* Defines. */
#define MAX_EVENTS 512              // Maximum amount of events to handle each time
#define MAX_TIMERS 0x1 << 16 - 1    // Maximum timers to set at the same time

#ifdef DEBUG
#   define DEFAULT_LOG "tinyev.log"
#endif

/* Structures. */
/* Struct associated to fd. */
struct event_data {
    event_cb cb;
    void *data;
};

struct timer_obj {
    struct timer_obj *next;
    // struct timer_obj *prev;
    struct event_data to_user_data;
    uint64_t to_msec;                   // Global time in millisecs
    int sec;                            // Requested time, seconds
    int msec;                           // Requested time, millisecs
};

/* ==*== GLOBAL VARIABLES ==*== */

/* Hold the events that occured. */
static struct epoll_event events[MAX_EVENTS];
/* Triggered timers list. */
struct timer_obj *timers = NULL;
/* Global fds. */
static int epoll_fd = -1;
/* fds and timers on. */
static int16_t watched_fds = 0;
static uint16_t watched_timers = 0;

static uint64_t time_in_millisecs(void)
{
    struct timeval tv;

    gettimeofday(&tv,NULL);
    return (((long long)tv.tv_sec) * 1000) + (tv.tv_usec / 1000);
}

bool tinyev_waiting()
{
    return (watched_fds != 0 || watched_timers != 0);
}

#ifdef DEBUG
static void print_list()
{
    struct timer_obj *tmp_t = timers;

    while (tmp_t) {
        SLOG("p %p, to %lu, next %p ->", tmp_t, tmp_t->to_msec, tmp_t->next);
        tmp_t = tmp_t->next;
    }

    SLOG("NULL\n# of timers %d\n", watched_timers);
}
#endif

static void insert_timer(struct timer_obj *td)
{
    struct timer_obj *tmp_t, *tmp_p = NULL;

    tmp_t = timers;
    while (tmp_t && (tmp_t->to_msec < td->to_msec)) {
        tmp_p = tmp_t;
        tmp_t = tmp_t->next;
    }

    if (tmp_p)
        tmp_p->next = td;   // Insert link
    else
        timers = td;    // New first
    td->next = tmp_t;

#ifdef DEBUG
    print_list();
#endif
}

static void* do_add_timer(int sec, int msec, void* data, bool per, event_cb cb)
{
    struct timer_obj *to = calloc(1, sizeof(struct timer_obj));
    uint64_t now = time_in_millisecs();

    if (!to) {
        SLOG("Failed allocating new timer");
        return NULL;
    }

    to->to_msec = now + msec + sec*1000;    // Total time in msecs
    to->to_user_data.cb = cb;
    to->to_user_data.data = data;

    SLOG("Adding timer %p, timeout in %lu milli\n", to, to->to_msec);

    if (per) {
        /* Periodic, save the times. */
        to->msec = msec;
        to->sec = sec;
    }

    insert_timer(to);
    watched_timers++;

    return to;
}

static void do_del_timer(void *timer)
{
    struct timer_obj *tmp_t, *tmp_p = NULL;
    struct timer_obj *td = timer;

    if (!td) {
        return;
    }

    tmp_t = timers;
    while (tmp_t && td != tmp_t) {
        tmp_p = tmp_t;
        tmp_t = tmp_t->next;
    }

    if (!tmp_t) {
        /* Was not found. */
    } else {
        tmp_p = tmp_t->next;
        free(tmp_t);
    }
}

static void check_timers()
{
    struct timer_obj *prev;
    uint64_t now;

    if (!watched_timers) return;

    now = time_in_millisecs();

    SLOG("Checking timers, # timers %d, first to at %lu, now is %lu\n", watched_timers, timers->to_msec, now);

    while (timers && timers->to_msec <= now) {
        /* Timeout occured. */
        timers->to_user_data.cb(timers->to_user_data.data);
        prev = timers;
        timers = timers->next;

        if (prev->sec || prev->msec) {
            /* Periodic timer. */
            prev->to_msec = now + prev->msec + prev->sec*1000;  // Update timeout
            prev->next = NULL;
            insert_timer(prev);
        } else {
            SLOG("Released timer %p\n", prev);
            free(prev);
            watched_timers--;
#ifdef DEBUG
            print_list();
#endif
        }
    }
}

static int tev_to_events(int events)
{
    int res = EPOLLPRI;

    if (events & TEV_RECV) res |= EPOLLIN;
    if (events & TEV_SEND) res |= EPOLLOUT;
    if (events & TEV_ERROR) res |= EPOLLERR;

    return res;
}

int tinyev_poll(int msec)
{
    struct event_data *fd_d;
    int nfds, i;

    nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, msec);
    if (nfds == -1) {
        SLOG("epoll_wait\n");
        return TINYEV_ERR_POLL;
    }

    /* First check messages/traffic. */
    for (i = 0; i < nfds; i++) {
        fd_d = (struct event_data *)events[i].data.ptr;
        /* Call the user. */
        fd_d->cb(fd_d->data);
    }

    /* Check timers. */
    check_timers();

    return TINYEV_ERR_OK;
}

void* tinyev_add_timer(int sec, int msec, void* data, event_cb cb)
{
    return do_add_timer(sec, msec, data, false, cb);
}

void* tinyev_add_periodic(int sec, int msec, void* data, event_cb cb)
{
    return do_add_timer(sec, msec, data, true, cb);
}

void tinyev_del_timer(void* tobj)
{
    do_del_timer(tobj);
}

void *tinyev_add_fd(int fd, void* data, event_cb cb, enum tinyev_events events, int *err)
{
    struct epoll_event ev;
    struct event_data *fd_d;

    fd_d = malloc(sizeof(struct event_data));
    if (!fd_d) {
        *err = TINYEV_ERR_MEM;
        return NULL;
    }

    fd_d->cb = cb;
    fd_d->data = data;

    /* Set the fd to be non-blocking. */
    if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0) {
        SLOG("ERROR setting up non-blocking socket");
        *err = TINYEV_ERR_ADD;
        return NULL;
    }

    ev.events = tev_to_events(events);
    ev.data.fd = fd;
    ev.data.ptr = fd_d;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1) {
        SLOG("epoll_ctl error add");
        *err = TINYEV_ERR_ADD;
        return NULL;
    }

    watched_fds++;
    *err = TINYEV_ERR_OK;
    
    return fd_d;
}

void tinyev_remove_fd(int fd, void *evobj)
{
    struct event_data *fd_d = evobj;

    if (!fd_d) return;

    close(fd); // Will remove fd from epoll watch list
    free(fd_d);
    if (watched_fds)
        watched_fds--;
}

int tinyev_init()
{
#ifdef DEBUG
    log_init(DEFAULT_LOG);
#endif

    if (epoll_fd != -1) {
        SLOG("Already initialized");
        return TINYEV_ERR_OK;
    }

    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        SLOG("Failed epoll_create1");
        return TINYEV_ERR_INIT;
    }

    SLOG("Tinyev is ready");

    return TINYEV_ERR_OK;
}

void tinyev_cleanup()
{
    SLOG("Cleaning all, timers %d\n", watched_timers);
    if (epoll_fd == -1) {
        SLOG("Already finalized epoll, or never initialized");
        return;
    } else {
        close(epoll_fd);
        epoll_fd = -1;
    }
#ifdef DEBUG
    log_cleanup();
#endif
}
