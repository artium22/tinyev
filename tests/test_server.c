#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>

#include "tinyev.h"

#define MIN_PORT 1024
#define MAX_PORT 65535
#define BUF_SIZE 1024
#define DEFAULT_PORT 1234

#define CMD "cmd: "

#define IP_ADDR_FMT_STRING      \
    "%d.%d.%d.%d"
#define IP_ADDR_FMT_PARAMS(x)   \
    ((ntohl(x) >> 24) & 0xff),     \
    ((ntohl(x) >> 16) & 0xff),     \
    ((ntohl(x) >> 8) & 0xff),      \
    (ntohl(x) & 0xff)


struct timer_data {
    char *msg;
};

struct event_data {
    int fd;
    void *tev;
};

void timer_cb(void *udata)
{
    printf("Timer cb called!\n");
}

void ev_cb(void *udata)
{
    struct event_data *data = udata;
    struct timer_data *tdata;
    char buf[BUF_SIZE] = {0};
    char *p;
    int bytes, delay;

    bytes = read(data->fd, buf, BUF_SIZE);
    if (bytes == -1) {
        printf("read failed with errno %d", errno);
        goto err;
    }

    if (bytes == 0) {
        // EOF
        printf("Connection closed\n");
        goto err;
    }

    printf("Received data %.*s, %d bytes\n", bytes - 1, buf, bytes);

    if (strcmp(buf, CMD) == 0) {
        tdata = calloc(1, sizeof(*tdata));
        if (strcmp(buf + sizeof(CMD), "delay: ") == 0) {
            delay = (int)strtol(buf + sizeof(CMD) + sizeof("delay: "), &p, 10);
            if (errno != 0) {
                printf("Error receiving delay message\n");
                return;
            }
            printf("Message \"%s\" size is %ld\n", p, buf + bytes - p);
            tdata->msg = malloc(buf + bytes - p);
            strlcpy(tdata->msg, p, BUF_SIZE);
            tinyev_add_timer(delay, 0, &tdata, timer_cb);
        } else if (strcmp(buf + sizeof(CMD), "periodic: ") == 0) {
            delay = (int)strtol(buf + sizeof(CMD) + sizeof("periodic: "), &p, 10);
            if (errno != 0) {
                printf("Error receiving periodic message\n");
                return;
            }
            printf("Message \"%s\" size is %ld\n", p, buf + bytes - p);
            tdata->msg = malloc(buf + bytes - p);
            strlcpy(tdata->msg, p, BUF_SIZE);
            tinyev_add_periodic(delay, 0, &tdata, timer_cb);
        } else if (strcmp(buf + sizeof(CMD), "stop") == 0) {
            goto err;
        }
    } else {
        bytes = write(data->fd, buf, bytes);
        if (bytes <= 0) {
            printf("Failed to send\n");
        }
    }

    return;
err:

    tinyev_remove_fd(data->fd, data->tev);
}

void listen_cb(void *udata)
{
    struct event_data *data = udata;
    struct event_data *new;
    int client_sock, client_size;
    struct sockaddr_in client_addr;
    int err;

    printf("Listen event, fd %d\n", data->fd);

    // Accept an incoming connection:
    client_size = sizeof(client_addr);
    client_sock = accept(data->fd, (struct sockaddr*)&client_addr,  (socklen_t*)&client_size);
    if (client_sock < 0){
        printf("Can't accept\n");
        return;
    }
    printf("Client connected at IP: "IP_ADDR_FMT_STRING" and port: %i, socket %d\n", 
            IP_ADDR_FMT_PARAMS(client_addr.sin_addr.s_addr), ntohs(client_addr.sin_port), client_sock);

    new = malloc(sizeof(struct event_data));
    new->fd = client_sock;
    new->tev = tinyev_add_fd(client_sock, new, ev_cb, &err);
    if (!new->tev) {
        printf("Failed to add fd, err %d", err);
        return;
    }
}

int main(int argc, char *argv[])
{
    struct event_data data;
    struct sockaddr_in addr;
    int fd, err;
    uint16_t port = 0;

    err = tinyev_init();
    if (err) {
        printf("Failed to init tinyev\n");
        return -1;
    }

    if (argc > 1) {
        port = atoi(argv[1]);
        if (errno != 0 || port > MAX_PORT || port < MIN_PORT) {
            printf("Illigel port, use default %d\n", DEFAULT_PORT);
            port = 0;
        } else {
            // No error
            printf("Try port %d\n", port);
        }
    }

    if (!port) port = DEFAULT_PORT;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd == -1) {
        printf("Error opening socket\n");
        return -1;
    }

    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_family = AF_INET;

    if (bind(fd, (struct sockaddr *)&addr,sizeof(struct sockaddr_in) ) == -1) {
        printf("Error binding socket\n");
        return -1;
    }

    printf("Successfully bound to port %u (fd is %d)\n", port, fd);

    // Listen for clients:
    if (listen(fd, 1) < 0) {
        printf("Error while listening\n");
        return -1;
    }
    printf("Listening for incoming connections.....\n");

    data.fd = fd;

    data.tev = tinyev_add_fd(fd, &data, listen_cb, &err);
    if (!data.tev) {
        printf("Failed to add fd, err %d", err);
        return -1;
    }

    while (tinyev_waiting()) {
        tinyev_poll(200);   // wake up 5 times a second
    }

    tinyev_cleanup();

    printf("Finished work, bye bye.\n");
}