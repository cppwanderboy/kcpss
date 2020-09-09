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

#include "Acceptor.h"
#include "timeUtility.h"
#include <cerrno>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <sys/types.h>

std::set<Channel *> Channel::channels_;
unsigned char *Channel::recv_buffer_ = new unsigned char[SIZE_4M];

Acceptor::Acceptor(Reactor *reactor)
  : listenFd_(0)
  , reactor_(reactor)
  , my_reactor_(nullptr)
  , connect_cb_(nullptr)
  , disconnect_cb_(nullptr) {
  if (!reactor_) {
    my_reactor_ = new Reactor;
    reactor_    = my_reactor_;
  }
}

Acceptor::~Acceptor() {
  if (connect_cb_) {
    delete connect_cb_;
    connect_cb_ = nullptr;
  }
  if (disconnect_cb_) {
    delete disconnect_cb_;
    disconnect_cb_ = nullptr;
  }
  if (my_reactor_) {
    delete my_reactor_;
  }
}

int Acceptor::listen(int port) {
  struct sockaddr_in me {};
  listenFd_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (listenFd_ < 0) {
    printf("Can not create socket for tcp CServer");
  }
  int on = 1;
  setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));
  memset(&me, 0, sizeof(me));
  me.sin_family = AF_INET;
  me.sin_port   = htons(port);
  if (bind(listenFd_, (struct sockaddr *)&me, sizeof(me)) < 0) {
    printf("Can not bind port for tcp CServer");
  }
  while (true) {
    on = 1;
    if (ioctl(listenFd_, FIONBIO, (char *)&on) < 0) {
      if (errno == EINTR)
        continue;

      printf("Can not set FIONBIO for socket");
      close(listenFd_);
      return -1;
    }
    break;
  }
  if (::listen(listenFd_, 5) < 0) {
    printf("Acceptor can not listen");
  }
  Reactor::Callback cb = std::bind(&Acceptor::accept, this);
  reactor_->RegisterIO(cb, listenFd_);
  return 0;
}

int Acceptor::start() {
  if (reactor_) {
    reactor_->Run();
  }
  return 0;
}

int Acceptor::accept() {
  struct sockaddr_in it {};
  socklen_t          nameLen   = sizeof(it);
  auto               channelFd = ::accept(listenFd_, (struct sockaddr *)&it, &nameLen);

  auto *channel = new Channel(reactor_, channelFd, -1);
  on_connect(channel);

  return 0;
}

bool Acceptor::set_connect_callback(Channel::Callback &cb) {
  connect_cb_ = new Channel::Callback(cb);
  return true;
}

int Acceptor::on_connect(Channel *channel) {
  if (connect_cb_) {
    (*connect_cb_)(channel);
  }
  return 0;
}

int Acceptor::on_disconnect(Channel *channel) {
  if (disconnect_cb_) {
    (*disconnect_cb_)(channel);
  }
  return 0;
}

bool Acceptor::set_disconnect_callback(Channel::Callback &cb) {
  disconnect_cb_ = new Channel::Callback(cb);
  return true;
}

int socket_output(const char *buf, int size, ikcpcb *kcp, void *fd) {
  return ::write(*(int *)fd, buf, size);
}

Channel::Channel(Reactor *reactor, int fd, int kcpConv)
  : reactor_(reactor)
  , fd_(fd)
  , read_cb_(nullptr)
  , disconnect_cb_(nullptr)
  , kcpConv_(kcpConv)
  , kcp_(nullptr)
  , bytes_write_(0)
  , bytes_read_(0)
  , connected_(false)
  , reconnect_count_(0) {
  LOG_INFO << "fd:" << fd_ << " create new channel";

  send_buffer_.reserve(SIZE_1M);
  channels_.insert(this);

  Reactor::Callback cb = std::bind(&Channel::read, this, std::placeholders::_1);
  reactor_->RegisterIO(cb, fd_);

  on_connect(kcpConv);
}

int Channel::on_connect(int kcpConv) {
  struct timeval tval {};
  fd_set         wset;
  FD_ZERO(&wset);
  FD_SET(fd_, &wset);
  tval.tv_sec  = 0;
  tval.tv_usec = 100;
  int error    = ::select(fd_ + 1, nullptr, &wset, nullptr, &tval);
  if (error == 0) {
    LOG_INFO << "select timeout, fd=" << fd_;
  }
  if (!FD_ISSET(fd_, &wset)) {
    if (++reconnect_count_ > 10) {
      reactor_->RemoveTimer(1000 + fd_);
      on_disconnect();
      return -1;
    }
    Reactor::Callback cb = std::bind(&Channel::on_connect, this, kcpConv);
    reactor_->RegisterTimer(cb, 1000 + fd_, 0, 0.1);
    return 0;
  }
  LOG_INFO << "fd[" << fd_ << "] connected to the server success";
  if (reconnect_count_ > 0) {
    reactor_->RemoveTimer(1000 + fd_);
  }
  int flags = fcntl(fd_, F_GETFL, 0);
  flags     = ~O_NONBLOCK & flags;
  fcntl(fd_, F_SETFL, flags);

  connected_ = true;

  if (!send_buffer_.empty()) {
    write(send_buffer_.data(), send_buffer_.size());
    send_buffer_.clear();
  }
  return 0;
}

void writelog(const char *log, struct IKCPCB *kcp, void *user) {
  LOG_INFO << kcp->conv << "," << log;
}

void Channel::init_kcp(int conv) {
  LOG_INFO << "init kcp channel, kcpConv = " << conv;
  kcp_                    = ikcp_create(conv, &fd_);
  Reactor::Callback kcpCb = std::bind(&Channel::update_kcp, this, std::placeholders::_1);
  kcp_->output            = socket_output;
  kcp_->logmask           = 15;
  kcp_->writelog          = writelog;
  ikcp_nodelay(kcp_, 1, 1, 2, 1);
  reactor_->RegisterTimer(kcpCb, 19890, 10);
}

int Channel::read(int fd) {
  while (connected_) {
    ssize_t ret = ::read(fd_, recv_buffer_, BUF_SIZE);
    if (ret <= 0) {
      on_disconnect();
      break;
    } else if (ret < BUF_SIZE) {
      on_read(recv_buffer_, ret);
      break;
    } else if (ret == BUF_SIZE) {
      on_read(recv_buffer_, ret);
    }
  }
  return 0;
}

int check_connections(int flag, Channel *channel) {
  if (channel && !channel->connected_) {
    LOG_INFO << "Channel[fd=" << channel->fd() << "] disconnected";
    delete channel;
  }
  return 0;
}

int Channel::on_disconnect() {
  LOG_INFO << "fd:" << fd_ << " disconnected";
  if (disconnect_cb_) {
    (*disconnect_cb_)(this);
  }
  connected_                = false;
  Reactor::Callback checkCb = std::bind(&check_connections, std::placeholders::_1, this);
  reactor_->RegisterTimer(checkCb, 2000 + fd_, 0, 0.01);
  return 0;
}

bool Channel::set_disconnect_callback(Channel::Callback &cb) {
  //  fprintf(stderr, "Channel::%s fd[%d]\n", __FUNCTION__, fd_);
  if (!disconnect_cb_) {
    disconnect_cb_ = new Callback(cb);
  }
  return true;
}

bool Channel::set_read_callback(Channel::ReadCallbck &cb) {
  //  fprintf(stderr, "Channel::%s fd[%d]\n", __FUNCTION__, fd_);
  if (!read_cb_) {
    read_cb_ = new ReadCallbck(cb);
  }
  return true;
}

void Channel::on_read(unsigned char *buffer, int size) {
  LOG_INFO << "fd:" << fd() << " read " << size << " bytes";
  if (read_cb_) {
    if (kcp_ == nullptr) {
      bytes_read_ += size;
      (*read_cb_)(buffer, size);
      return;
    }
    ikcp_input(kcp_, (char *)buffer, size);
    int payload_size = 0;
    do {
      payload_size = ikcp_recv(kcp_, (char *)recv_buffer_, BUF_SIZE);
      if (payload_size > 0) {
        bytes_read_ += payload_size;
        (*read_cb_)(recv_buffer_, payload_size);
      }
    } while (payload_size >= 0);
  }
}

int Channel::write(unsigned char *buf, int size) {
  if (!connected_) {
    LOG_INFO << "write but fd[" << fd_ << "] not connected, push to buffer";
    send_buffer_.insert(send_buffer_.end(), buf, buf + size);
    return 0;
  }
  bytes_write_ += size;
  if (kcp_) {
    return ikcp_send(kcp_, (char *)buf, size);
  } else {
    int flags = fcntl(fd_, F_GETFL, 0);
    LOG_INFO << "fd:" << fd_ << " send " << size << " bytes";
    return ::write(fd_, buf, size);
  }
}

Channel::~Channel() {
  LOG_INFO << "destruct channel fd=" << fd_;
  if (kcp_) {
    LOG_INFO << "ikcp_release conv=" << kcp_->conv;
    reactor_->RemoveTimer(19890);
    ikcp_release(kcp_);
  }
  reactor_->RemoveIO(fd_);
  channels_.erase(this);
  ::close(fd_);
  if (read_cb_) {
    delete read_cb_;
    read_cb_ = nullptr;
  }
  if (disconnect_cb_) {
    delete disconnect_cb_;
    disconnect_cb_ = nullptr;
  }
}

int Channel::update_kcp(int elapse) {
  if (kcp_) {
    ikcp_update(kcp_, now_ms());
  }
  return 0;
}

int Channel::write(Channel *channel, unsigned char *buf, int size) {
  if (channels_.find(channel) == channels_.end()) {
    return 0;
  }
  return channel->write(buf, size);
}
