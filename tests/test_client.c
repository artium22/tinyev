#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "tests.h"
#include "tinyev.h"

#define BUF_SIZE 2048
#define LOOPBACK_ADDR "127.0.0.1"

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
    struct hostent *host = NULL;
    struct in6_addr ip;
    u_int16_t port = 0;
    int inet = AF_INET;
    int rc;

    if (argc < 2) {
        port = DEFAULT_PORT;
        host = gethostbyname(LOOPBACK_ADDR);
        printf("Using default port %d and address %s\n", port, LOOPBACK_ADDR);
        goto conn;
    }

    if (strchr(argv[1], '.') || strchr(argv[1], ':')) {
        /* IP address was provided. */
        host = gethostbyname(argv[1]);
        if (!host) {
            printf("Illegal IP address, abort\n");
            exit(EXIT_FAILURE);
        }

        inet = strchr(argv[1], '.') ? AF_INET : AF_INET6;
        if (inet_pton(inet, argv[1], &ip) <= 0) {
            printf("inet_pton failed\n");
            return -1;
        }
        port = DEFAULT_PORT;
    } else {
        /* Port was provided. */
        port = atoi(argv[1]);
        /* Use default address. */
        host = gethostbyname(LOOPBACK_ADDR);
    }

    if (argc == 3) {
        /* IP address and port were provided. */
        host = gethostbyname(argv[1]);
        if (!host) {
            printf("Illegal IP address, abort\n");
            exit(EXIT_FAILURE);
        }
        port = atoi(argv[2]);
    } else if (argc > 3) {
        printf("Wrong usage! Try 'test_client 192.168.1.123 5432'\n");
        exit(EXIT_FAILURE);
    }

conn:
    fd = socket(inet, SOCK_STREAM, IPPROTO_TCP);
    if (fd == -1) {
        perror("socket failed \n");
        exit(EXIT_FAILURE);
    }
    printf("Created a socket with fd: %d\n", fd);

    saddr.sin_family = inet;
    saddr.sin_port = htons(port);     
    saddr.sin_addr = *((struct in_addr *)host->h_addr);

    rc = connect(fd, (struct sockaddr *)&saddr, sizeof(struct sockaddr_in));
    if (rc == -1) {
        perror("connect failed");
        close(fd);
        exit(EXIT_FAILURE);
    }
    printf("The Socket is now connected\n");

    rc = tinyev_init();
    if (rc) {
        printf("Failed to init tinyev\n");
        exit(EXIT_FAILURE);
    }

    data_fd.fd = fd;
    data_fd.tev = tinyev_add_fd(fd, &data_fd, ev_cb, &rc);
    if (!data_fd.tev) {
        printf("Failed to add fd, err %d\n", rc);
        exit(EXIT_FAILURE);
    }

    data_stdio.fd = STDIN_FILENO;
    data_stdio.tev = tinyev_add_fd(STDIN_FILENO, &data_stdio, ev_cb, &rc);
    if (!data_stdio.tev) {
        printf("Failed to add stdio fd, err %d\n", rc);
        exit(EXIT_FAILURE);
    }

    while (tinyev_waiting()) {
        tinyev_poll(200);   // wake up 5 times a second
    }

    tinyev_cleanup();
}