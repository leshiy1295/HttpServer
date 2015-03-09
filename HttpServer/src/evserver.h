//
//  evserver.h
//  HttpServer
//
//  Created by Alexey Halaidzhy on 09.03.15.
//  Copyright (c) 2015 Alexey Halaidzhy. All rights reserved.
//
//  In that module there are callbacks for liveb watchers
//
#ifndef HttpServer_evserver_h
#define HttpServer_evserver_h
#include "ev.h"

void accept_cb(struct ev_loop *loop, struct ev_io *watcher, int revents);
void read_cb(struct ev_loop *loop, struct ev_io *watcher, int revents);
void start_server();
#endif
