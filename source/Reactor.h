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

#ifndef KCPSS_REACTOR_H
#define KCPSS_REACTOR_H

#include "public.h"

class Reactor {
public:
  using Callback = std::function<int(int)>;

public:
  Reactor();
  virtual void Run();
  void         Stop(int nStopCode = 0);

  // IO
  virtual void RegisterIO(Callback &handler, int fd);
  virtual void RemoveIO(int fd);

  // Timers
  void RegisterTimer(Callback &handler, int evId, double elapse, double after = 0);
  void RemoveTimer(int evId);

protected:
  struct ev_loop *                    loop_;
  std::unordered_map<int, ev_timer *> timers_;
  std::unordered_map<int, ev_io *>    ios_;
};

#endif  // KCPSS_REACTOR_H
