//
//  evserver.c
//  HttpServer
//
//  Created by Alexey Halaidzhy on 09.03.15.
//  Copyright (c) 2015 Alexey Halaidzhy. All rights reserved.
//
#include "common.h"
#include "evserver.h"
#include <netinet/in.h>

void accept_cb(struct ev_loop *loop, struct ev_io *watcher, int revents) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_sd;
    struct ev_io *w_client = (struct ev_io *)malloc(sizeof(struct ev_io));
    if (EV_ERROR & revents) {
        if (DEBUG_MODE) {
            perror("Got invalid event\n");
        }
        return;
    }
    client_sd = accept(watcher->fd, (struct sockaddr *)&client_addr, &client_len);
    if (client_sd == -1) {
        if (DEBUG_MODE) {
            perror("Accept error\n");
        }
        return;
    }
    if (DEBUG_MODE) {
        printf("Successfully connected with client with fd = %d on worker %d\n", client_sd, getpid());
    }

    ev_io_init(w_client, read_cb, client_sd, EV_READ);
    ev_io_start(loop, w_client);
}

void read_cb(struct ev_loop *loop, struct ev_io *watcher, int revents) {
    char buffer[BUFFER_SIZE];
    bzero(buffer, BUFFER_SIZE);
    ssize_t read;
    if (EV_ERROR & revents) {
        if (DEBUG_MODE) {
            perror("Got invalid event\n");
        }
        return;
    }
    read = recv(watcher->fd, buffer, BUFFER_SIZE, 0);

    if (DEBUG_MODE) {
        printf("read = %zd\n", read);
    }
    if (read == -1) {
        if (DEBUG_MODE) {
            perror("Read error\n");
        }
        return;
    }
    if (read == 0) {
        //  Stop and free watcher of client socket is closing
        ev_io_stop(loop, watcher);
        close(watcher->fd);
        free(watcher);
        if (DEBUG_MODE) {
            perror("Peer might closing\n");
        }
        return;
    }
    else {
        printf("message: %s\n", buffer);
    }
    send(watcher->fd, "HTTP/1.1 200 OK\nContent-Length:3\n\n123", 100, 0);
}

int set_nonblock(int sock, bool flag) {
    int fl = fcntl(sock, F_GETFL);
    if (fl == -1) {
        return fl;
    }
    if (flag) {
        fl |= O_NONBLOCK;
    }
    else {
        fl &= ~O_NONBLOCK;
    }
    return fcntl(sock, F_SETFL, fl);
}

void create_workers() {
    int pid = 1;
    for (int i = 0; i < CORES - 1; ++i) {
        if (pid > 0) {
            pid = fork();
        }
    }
}

void start_server() {
    struct sockaddr_in sa;
    sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    sa.sin_family = AF_INET;
    sa.sin_port = htons(SERVER_PORT);
    
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        if (DEBUG_MODE) {
            perror("Socket creation error\n");
        }
        exit(1);
    }
    int flag = true;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
    long fl = fcntl(sock, F_GETFL);
    if (fl == -1) {
        if (DEBUG_MODE) {
            perror("Can't get fcntl sock option\n");
        }
        exit(1);
    }
    
    set_nonblock(sock, true);
    
    if (bind(sock, (struct sockaddr *)&sa, sizeof(sa)) == -1) {
        if (DEBUG_MODE) {
            perror("Bind error\n");
        }
        exit(1);
    }
    
    if (listen(sock, CONNECTION_QUEUE_SIZE) == -1) {
        if (DEBUG_MODE) {
            perror("Listen error\n");
        }
        exit(1);
    }
    
    if (DEBUG_MODE) {
        printf("Server is listening with fd = %d on port %d\n", sock, SERVER_PORT);
    }
    
    create_workers();
    if (DEBUG_MODE) {
        printf("Hi from worker with pid %d and ppid %d\n", getpid(), getppid());
    }
    
    struct ev_loop *loop = ev_default_loop(0);
    
    struct ev_io w_accept;
    ev_io_init(&w_accept, accept_cb, sock, EV_READ);
    ev_io_start(loop, &w_accept);
    
    ev_loop(loop, 0);
}
