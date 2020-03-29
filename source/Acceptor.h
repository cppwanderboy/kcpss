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

#ifndef KCPSS_ACCEPTOR_H
#define KCPSS_ACCEPTOR_H

#include "public.h"
#include "Reactor.h"

class Channel {
public:
  using Callback    = std::function<int(Channel *)>;
  using ReadCallbck = std::function<int(unsigned char *buffer, int size)>;

public:
  Channel(Reactor *reactor, int fd, int kcpConv = -1);
  virtual ~Channel();

  int fd() const { return fd_; }

  static int write(Channel *channel, unsigned char *buf, int size);
  // Callbacks
  bool set_disconnect_callback(Callback &cb);
  bool set_read_callback(ReadCallbck &cb);

  bool connected_;

protected:
  virtual int  read(int fd);
  int          write(unsigned char *buf, int size);
  virtual int  on_connect(int kcpConv);
  virtual int  on_disconnect();
  virtual void on_read(unsigned char *buffer, int size);

  void        init_kcp(int conv);
  virtual int update_kcp(int flag);

protected:
  Reactor *                  reactor_;
  Callback *                 disconnect_cb_;
  ReadCallbck *              read_cb_;
  int                        fd_;
  int                        kcpConv_;
  ikcpcb *                   kcp_;
  int                        ms_;
  size_t                     bytes_read_;
  size_t                     bytes_write_;
  static std::set<Channel *> channels_;
  unsigned char *            recv_buffer_;
  std::vector<unsigned char> send_buffer_;
  int                        reconnect_count_;
};

class Acceptor {
public:
  Acceptor(Reactor *reactor = nullptr);
  virtual ~Acceptor();

  virtual int listen(int port);
  virtual int start();

  // Callbacks
  bool set_connect_callback(Channel::Callback &cb);
  bool set_disconnect_callback(Channel::Callback &cb);

protected:
  virtual int accept();
  virtual int on_connect(Channel *channel);
  virtual int on_disconnect(Channel *channel);

private:
  int                listenFd_;
  Reactor *          reactor_;
  Channel::Callback *connect_cb_;
  Channel::Callback *disconnect_cb_;
};

#endif  // KCPSS_ACCEPTOR_H
