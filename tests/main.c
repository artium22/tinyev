#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "tinyev.h"

struct event_data {
    int fd;
    void *tev;
};

char* msg1 = "hello, world #1";
int fds[2];

static void timer_cb(void *user_data)
{
    printf("Timeout called! data is %d\n", *(int*)user_data);
    // tinyev_add_timer(1, 500, NULL, timer_cb);
    // tinyev_add_timer(0, 750, NULL, timer_cb);
    write(fds[1], msg1, 16);
}

static void ev_cb(void *user_data)
{
    char buf[16] = {0};
    struct event_data *data = (struct event_data *)user_data;
    int err;

    err = read(data->fd, buf, 15);
    if (err == -1) {
        printf("read failed with errno %d", errno);
        goto fin;
    }

    printf("Received data %s, %d bytes\n", buf, err);
    tinyev_add_timer(1, 500, &fds[0], timer_cb);

    // write(fds[1], msg1, 16);

fin:
    // tinyev_remove_fd(data->fd, data->tev);
    return;
}

int main(int argc, char *argv[])
{
    void *tev;
    struct event_data data;
    int err;

    err = tinyev_init();
    if (err) {
        printf("Failed to init tinyev\n");
        return -1;
    }

    pipe(fds);

    data.fd = fds[0];

    tev = tinyev_add_fd(fds[0], &data, ev_cb, TEV_RECV | TEV_CLOSE | TEV_ERROR, &err);
    if (!tev) {
        printf("Failed to add fd, err %d", err);
        return -1;
    }

    data.tev = tev;

    write(fds[1], msg1, 16);

    tinyev_add_timer(1, 500, &data, timer_cb);

    while (tinyev_waiting()) {
        tinyev_poll(200);   // wake up 5 times a second
        usleep(50000);
    }

    tinyev_cleanup();
}
