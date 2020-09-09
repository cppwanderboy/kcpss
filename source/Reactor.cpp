// MIT License
//
// Copyright (c) 2019 Gui Yang
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "Reactor.h"

struct TimerInfo {
  Reactor::Callback *handler;
  int                evId;
};

Reactor::Reactor() {
  loop_ = ev_default_loop(0);
}

static void timer_callback(EV_P_ ev_timer *w, int revents) {
  auto *info = (TimerInfo *)w->data;
  if (info && info->handler) {
    Reactor::Callback &callback = *info->handler;
    callback(info->evId);
  }
}

static void io_callback(EV_P_ ev_io *w, int revents) {
  auto *handler = (Reactor::Callback *)w->data;
  if (handler) {
    (*handler)(w->fd);
  }
}

void Reactor::Run() {
  ev_run(loop_, 0);
}

void Reactor::Stop(int nStopCode) {
  ev_break(loop_, nStopCode);
}

void Reactor::RegisterTimer(Callback &handler, int evId, double elapse, double after) {
  auto *timer = new ev_timer;
  timer->data = new TimerInfo{new Callback(handler), evId};
  ev_timer_init(timer, timer_callback, after, 0.001 * elapse);
  ev_timer_start(loop_, timer);
  timers_[evId] = timer;
}

void Reactor::RemoveTimer(int evId) {
  auto *timer = timers_[evId];
  if (timer) {
    ev_timer_stop(loop_, timer);
    delete (((TimerInfo *)timer->data)->handler);
    delete ((TimerInfo *)timer->data);
    delete (timer);
  }
}

void Reactor::RegisterIO(Callback &handler, int fd) {
  auto *io = new ev_io;
  io->data = new Callback(handler);
  ev_io_init(io, io_callback, fd, EV_READ);
  ev_io_start(loop_, io);
  ios_[fd] = io;
}

void Reactor::RemoveIO(int fd) {
  auto *io = ios_[fd];
  if (io) {
    ev_io_stop(loop_, io);
    delete ((Callback *)io->data);
    delete (io);
  }
}
