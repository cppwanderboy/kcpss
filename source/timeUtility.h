// MIT License
//
// Copyright (c) 2020 Gui Yang
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

#ifndef KCPSS_TIMEUTILITY_H
#define KCPSS_TIMEUTILITY_H
#include <dlfcn.h>
#include <ctime>
#include "mlog.h"

typedef long (*clock_func_type)(clockid_t which_clock, struct timespec *tp);
inline long system_clock_gettime(clockid_t which_clock, struct timespec *tp) {
  return (long)clock_gettime(which_clock, tp);
}

inline clock_func_type get_clock_func() {
  void *          vdso;
  clock_func_type ret;

  vdso = dlopen("linux-vdso.so.1", RTLD_NOW | RTLD_NOLOAD);
  if (!vdso) {
    LOG_CRIT << "get_clock_func() cannot open linux-vdso.so.1!";
    return system_clock_gettime;
  }
  ret = (clock_func_type)dlsym(vdso, "__vdso_clock_gettime");
  if (!ret) {
    LOG_CRIT << "get_clock_func() dlsym failed!";
    return system_clock_gettime;
  }
  return ret;
}

inline uint32_t now_ms() {
  static clock_func_type clock_func = get_clock_func();
  struct timespec        ts {};
  clock_func(CLOCK_REALTIME, &ts);
  auto now_ms = (uint32_t)ts.tv_sec * 1000 + (uint32_t)ts.tv_nsec / 1000000;
  return now_ms;
}

#endif  // KCPSS_TIMEUTILITY_H
