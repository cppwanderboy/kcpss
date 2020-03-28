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

#ifndef KCPSS_PUBLIC_H
#define KCPSS_PUBLIC_H

#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <sys/types.h>

#include <set>
#include <vector>
#include <functional>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <cerrno>
#include <unordered_set>

#include <ev.h>
#include "ikcp.h"

#include "mlog.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;
using std::placeholders::_4;
using std::placeholders::_5;

const static size_t SIZE_1M  = 1024 * 1024;
const static size_t SIZE_4M  = SIZE_1M * 4;
const static size_t SIZE_16M = SIZE_1M * 16;
const static size_t SIZE_64M = SIZE_1M * 64;

inline void dump(unsigned char *buf, int len, const char *tag) {
  printf("\n%s Start [%d] ===============\n", tag, len);
  int i = 0;
  for (; i + 8 <= len; i += 8) {
    printf(
      "%02x %02x %02x %02x %02x %02x %02x %02x |"
      "%c%c%c%c%c%c%c%c "
      "\n",
      buf[i],
      buf[i + 1],
      buf[i + 2],
      buf[i + 3],
      buf[i + 4],
      buf[i + 5],
      buf[i + 6],
      buf[i + 7],
      isprint(buf[i]) ? buf[i] : '.',
      isprint(buf[i + 1]) ? buf[i + 1] : '.',
      isprint(buf[i + 2]) ? buf[i + 2] : '.',
      isprint(buf[i + 3]) ? buf[i + 3] : '.',
      isprint(buf[i + 4]) ? buf[i + 4] : '.',
      isprint(buf[i + 5]) ? buf[i + 5] : '.',
      isprint(buf[i + 6]) ? buf[i + 6] : '.',
      isprint(buf[i + 7]) ? buf[i + 7] : '.');
  }
  while (i < len) {
    printf("%02x ", buf[i++]);
  }
  printf("\n%s End   [%d] ===============\n", tag, len);
}

#endif  // KCPSS_PUBLIC_H
