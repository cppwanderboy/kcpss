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

#ifndef KCPSS_SOCKET_H
#define KCPSS_SOCKET_H

#include "public.h"

class endpoint {
public:
  endpoint() : path_(""), port_(0), host_("") { memset(&sockaddr_, 0, sizeof(struct sockaddr_in)); }
  endpoint(const char *path) : path_(path) {
    char         type[32]  = {0};
    char         host[256] = {0};
    unsigned int port      = 0;

    /** addr is protocol://ip:port */
    sscanf(path, "%31[^:]://%[^:]:%u", type, host, &port);
    port_ = (int)port;
    host_ = host;
    memset(&sockaddr_, 0, sizeof(struct sockaddr_in));
    sockaddr_.sin_family      = AF_INET;
    sockaddr_.sin_port        = htons(port);
    sockaddr_.sin_addr.s_addr = inet_addr(host);
  }

  endpoint(struct sockaddr_in addr) : port_(addr.sin_port), host_(inet_ntoa(addr.sin_addr)) {
    memcpy(&sockaddr_, &addr, sizeof(sockaddr_in));
  }

  endpoint(const char *prorocol, const char *ip, int port) : port_(port), host_(ip) {
    memset(&sockaddr_, 0, sizeof(struct sockaddr_in));
    sockaddr_.sin_family      = AF_INET;
    sockaddr_.sin_port        = htons(port);
    sockaddr_.sin_addr.s_addr = inet_addr(ip);
  }

  endpoint(const endpoint &other) : port_(other.port_), path_(other.path_), host_(other.host_) {
    memcpy(&sockaddr_, &other.sockaddr_, sizeof(sockaddr_));
  }

  endpoint &operator=(const endpoint &other) {
    port_ = other.port_;
    path_ = other.path_;
    host_ = other.host_;
    memcpy(&sockaddr_, &other.sockaddr_, sizeof(sockaddr_));

    return *this;
  }

  bool operator==(const endpoint &other) const {
    return memcmp(&sockaddr_, &other.sockaddr_, sizeof(sockaddr_)) == 0;
  }

  int port() const { return port_; }

  int port(int port) {
    sockaddr_.sin_port = htons(port);
    return port_       = port;
  }

  const char *host() const { return host_.c_str(); }

  const sockaddr_in *sockaddr() const { return &sockaddr_; }

  static endpoint null() { return endpoint("", "", -1); }
  bool            is_null() { return port_ == -1 && host_.empty(); }

private:
  int                port_;
  std::string        host_;
  std::string        path_;
  struct sockaddr_in sockaddr_ {};
};

class socket {
public:
  static int create_udp(const endpoint &ep) {
    int fd = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
      LOG_CRIT << "socket creation failed, please check maximum fd size";
      exit(0);
    }

    LOG_INFO << "socket successfully created, fd = " << fd;
    int flag    = 1;
    int bufsize = SIZE_16M;
    setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize));
    setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize));
    int iptos = (48 << 2) & 0xFF;
    setsockopt(fd, IPPROTO_IP, IP_TOS, &iptos, sizeof(iptos));

    if (ep.port() != 0) {
      struct sockaddr_in srv_addr {};
      memset(&srv_addr, 0, sizeof(struct sockaddr_in));
      srv_addr.sin_family      = AF_INET;
      srv_addr.sin_port        = htons(ep.port());
      srv_addr.sin_addr.s_addr = inet_addr(ep.host());
      if (::bind(fd, (struct sockaddr *)&srv_addr, sizeof(srv_addr)) < 0) {
        LOG_CRIT << "udp can not bind to port " << ep.port();
        exit(1);
      }
    }
    return fd;
  }

  static int create_tcp(const endpoint &ep) {
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
      LOG_CRIT << "socket creation failed";
      exit(0);
    } else {
      LOG_INFO << "socket successfully created, fd=" << fd << ", connect to " << ep.host() << ":"
               << ep.port();
      int flag    = 1;
      int bufsize = SIZE_16M;
      setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize));
      setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize));
      setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
      setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &flag, sizeof(flag));
      int flags = fcntl(fd, F_GETFL, 0);
      fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }

    int res = ::connect(fd, reinterpret_cast<const sockaddr *>(ep.sockaddr()), sizeof(sockaddr_in));
    if (res < 0 && errno != EINPROGRESS) {
      LOG_WARN << "connection with the remote server failed: " << ep.host() << ":" << ep.port();
      return fd;
    }

    if (res == 0) {
      LOG_INFO << "connected to the server, fd[" << fd << "]" << ep.host() << ":" << ep.port();
    } else {  // connection attempt is in progress
      LOG_INFO << "fd[" << fd << "] connect " << ep.host() << ":" << ep.port() << " in processing";
    }
    return fd;
  }
};
namespace std {

template<>
struct hash<endpoint> {
  std::size_t operator()(const endpoint &key) const {
    in_addr  addr   = key.sockaddr()->sin_addr;
    uint32_t port   = key.port();
    uint64_t unique = *(uint32_t *)(&addr);
    unique          = (unique << 32) | port;

    return std::hash<uint64_t>()(unique);
  }
};

}  // namespace std

#endif  // KCPSS_SOCKET_H
