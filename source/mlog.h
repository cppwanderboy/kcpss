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

#ifndef KCPSS_MLOG_H
#define KCPSS_MLOG_H

#include <iostream>

namespace mlog {
enum class LogLevel : uint8_t { DBUG = 0, INFO, WARN, CRIT };

struct mlog {
  mlog(std::ostream &os, LogLevel level) : os(os), level_(level) {}
  struct A {
    A() : os(std::cout), live(false) {}
    A(std::ostream &r) : os(r), live(true) {}
    A(A &r) : os(r.os), live(true) {
      if (r.live)
        r.live = false;
      else
        live = false;
    }
    A(A &&r) noexcept : os(r.os), live(true) {
      if (r.live)
        r.live = false;
      else
        live = false;
    }
    ~A() {
      if (live) {
        os << std::endl;
      }
    }
    std::ostream &os;
    bool          live;
  };
  std::ostream &os;
  LogLevel      level_;
  static mlog & debug() {
    static mlog *out = new mlog(std::clog, LogLevel::DBUG);
    return *out;
  }
  static mlog &info() {
    static mlog *out = new mlog(std::clog, LogLevel::INFO);
    return *out;
  }
  static mlog &warn() {
    static mlog *out = new mlog(std::cerr, LogLevel::WARN);
    return *out;
  }
  static mlog &crit() {
    static mlog *out = new mlog(std::cerr, LogLevel::CRIT);
    return *out;
  }
  static LogLevel &level() {
    static LogLevel outlevel_{LogLevel::INFO};
    return outlevel_;
  }
  static const char *level_str(LogLevel level) {
    static const char levelStr[4][8] = {"[DBUG] ", "[INFO] ", "[WARN] ", "[CRIT] "};
    return levelStr[(int)level];
  }
};

template<class T>
mlog::A operator<<(mlog::A &&a, const T &t) {
  if (a.live) {
    a.os << t;
  }
  return a;
}

template<class T>
mlog::A operator<<(mlog &m, const T &t) {
  if (m.level_ >= mlog::level()) {
    return mlog::A(m.os) << mlog::level_str(m.level_) << t;
  } else {
    return mlog::A();
  }
}

inline void set_level(const LogLevel &level) {
  mlog::level() = level;
}
}  // namespace mlog

#define LOG_DBUG mlog::mlog::debug() << ""
#define LOG_INFO mlog::mlog::info()
#define LOG_WARN mlog::mlog::warn()
#define LOG_CRIT mlog::mlog::crit()

#endif  // KCPSS_MLOG_H
