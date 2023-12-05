#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "tinyev.h"

#define BUF_SIZE 4096

struct event_data {
    int fd;
    void *tev;
};

static char input[BUF_SIZE] = {0};
int fd;

void ev_cb(void *udata)
{
    struct event_data *data = udata;
    int bytes;

    bytes = read(data->fd, input, BUF_SIZE);
    if (bytes < 0) {
        /* Handle error. */
        printf("Read error fd %d", data->fd);
        tinyev_remove_fd(data->fd, data->tev);
    } else if (bytes == 0) {
        /* Handle EOF. */
        printf("Close fd %d", data->fd);
        tinyev_remove_fd(data->fd, data->tev);
    } else {
        input[bytes] = '\0';
        if (data->fd == STDIN_FILENO) {
            bytes = write(fd, input, bytes);
            if (bytes < 0) {
                printf("Failed to write %s\n", input);
            }
        } else {
            printf("Received %s\n", input);
        }
    }
}

int main(int argc, char *argv[])
{
    struct event_data data_fd, data_stdio;
    struct sockaddr_in saddr;
    struct hostent *local_host;
    struct in6_addr ip;
    u_int16_t port;
    bool is4;
    int rc;

    is4 = strchr(argv[1], '.') ? true : false;

    if (inet_pton(is4 ? AF_INET : AF_INET6, argv[1], &ip) <= 0) {
        printf("inet_ntop\n");
        return -1;
    }

    port = atoi(argv[2]);

    fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd == -1) {
        perror("socket failed \n");
        exit(EXIT_FAILURE);
    }
    printf("Created a socket with fd: %d\n", fd);

    saddr.sin_family = AF_INET;         
    saddr.sin_port = htons(port);     
    local_host = gethostbyname(argv[1]);
    saddr.sin_addr = *((struct in_addr *)local_host->h_addr);

    rc = connect(fd, (struct sockaddr *)&saddr, sizeof(struct sockaddr_in));
    if (rc == -1) {
        perror("connect failed");
        close(fd);
        return -1;
    }
    printf("The Socket is now connected\n");

    rc = tinyev_init();
    if (rc) {
        printf("Failed to init tinyev\n");
        return -1;
    }

    data_fd.fd = fd;
    data_fd.tev = tinyev_add_fd(fd, &data_fd, ev_cb, &rc);
    if (!data_fd.tev) {
        printf("Failed to add fd, err %d\n", rc);
        return -1;
    }

    data_stdio.fd = STDIN_FILENO;
    data_stdio.tev = tinyev_add_fd(STDIN_FILENO, &data_stdio, ev_cb, &rc);
    if (!data_stdio.tev) {
        printf("Failed to add stdio fd, err %d\n", rc);
        return -1;
    }

    while (tinyev_waiting()) {
        tinyev_poll(200);   // wake up 5 times a second
    }

    tinyev_cleanup();
}