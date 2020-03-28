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

#ifndef KCPSS_UDP_H
#define KCPSS_UDP_H

#include "public.h"
#include "Reactor.h"
#include "socket.h"

struct SessionHeader {
  int           sid;
  int           size;
  unsigned char data[];
};

class udp {
public:
  using SessionCallbck         = std::function<int(int, unsigned char *buffer, int size)>;
  const static int MTU         = 1376;  // The mss of default kcp
  const static int MAX_PAYLOAD = MTU - sizeof(SessionHeader);

public:
  udp(Reactor *reactor, const char *addr, const char *remote_addr = nullptr);
  virtual ~udp();

  int fd() const { return fd_; }

  void set_session_callback(SessionCallbck &cb);

  int send(int sid, unsigned char *buffer, int size);
  int write(unsigned char *buffer, int size);

protected:
  ikcpcb *crtete_kcp(int conv);

  int  read_socket(int fd);
  void on_read(unsigned char *buffer, int size, sockaddr_in *target);

private:
  Reactor *       reactor_;
  int             fd_;
  int             ms_;
  ikcpcb *        kcp_;
  endpoint *      target_;
  SessionCallbck *cb_;
  SessionHeader * segment_;
  unsigned char * recv_bufffer_;
};

#endif  // KCPSS_UDP_H
