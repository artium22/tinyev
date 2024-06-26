#ifndef __TINYEV_H__
#define __TINYEV_H__

#include <stdbool.h>
#include "error.h"

#define TINYEV_MAX_TNAME_LEN 16

/* List of events that tinyev supports.
    Note: setting up send event will cause the CPU 100% usage
    since it'll wake the process up ALL OF THE TIME! */
enum tinyev_events {
    TEV_RECV = 1 << 0,      // Ready to read
    TEV_SEND = 1 << 1,      // Ready to send
    TEV_CLOSE = 1 << 2,     // Close event
    TEV_ERROR = 1 << 3      // Something went bad
};

/**
 * @brief prototype for event loop user callback function,
 * receives the data to call this cb with - user data.
 */
typedef void (*event_cb)(void*);

enum tinyev_event {
    TINYEV_EVENT_TO = 0,
    TINYEV_EVENT_READ,
    TINYEV_EVENT_CLOSE
};

/**
 * @brief check if there're any file descriptors watched.
 * 
 * @return true fds are monitored.
 * @return false no fds polled.
 */
bool tinyev_waiting();

/**
 * @brief wait for events with timeout. Please be advised - the lower to
 * timeout (msec var) the more we run and use the CPU but also we have
 * low-latency and persision.
 * 
 * @param msec  maximum time to poll.
 * @return int  TINYEV_OK if all went well, any other error otherwise.
 */
int tinyev_poll(int msec);

/**
 * @brief add a one-time triggered timer after some time the user
 * wants to. The presision of this call is up to poll timout that
 * the user calls tinyev_poll() with.
 * 
 * @param sec       seconds timeout
 * @param msec      milli seconds timeout
 * @param data      user data to call the callback with
 * @param cb        user callback
 * @return void*    timer object 
 */
void* tinyev_add_timer(int sec, int msec, void* data, event_cb cb);

/**
 * @brief adds a periodic timer that is being called every secs + msecs
 * and call the callback. The presision of this call is up to poll timout that
 * the user calls tinyev_poll() with.
 * 
 * @param sec       seconds timeout
 * @param msec      milli seconds timeout
 * @param data      user data to call the callback with
 * @param cb        user callback
 * @return void*    timer object 
 */
void* tinyev_add_periodic(int sec, int msec, void* data, event_cb cb);

/**
 * @brief remove timer from timers list. Can either periodic or not.
 * 
 * @param tobj timer object pointer.
 */
void tinyev_del_timer(void* tobj);

/**
 * @brief adds file descriptor to poll on. All of the file descriptors
 * will be non blocking at the end of this function!
 * 
 * @param fd        the file descriptor to poll on
 * @param data      user data.
 * @param cb        user call back to upon any event, with the user data.
 * @param events    which events to track for this fd.
 * @param err       pointer to error as return code.
 * @return void*    returns pointer to new event object.
 */
void *tinyev_add_fd(int fd, void* data, event_cb cb, enum tinyev_events events, int *err);

/**
 * @brief remove fd and close it.
 * 
 * @param fd to remove.
 */
void tinyev_remove_fd(int fd, void *ev_data);

/**
 * @brief start things up.
 * 
 * @return int  TINYEV_OK if all went well, any other error otherwise.
 */
int tinyev_init();

/**
 * @brief bye bye Tinyev.
 * 
 */
void tinyev_cleanup();

#endif /* __TINYEV_H__ */
