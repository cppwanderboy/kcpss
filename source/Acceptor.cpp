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
#include <cerrno>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <sys/types.h>

std::set<Channel *> Channel::channels_;

Acceptor::Acceptor(Reactor *reactor, EKcpDirection kcpDirection)
  : kcpDirection_(kcpDirection)
  , listenFd_(0)
  , reactor_(reactor)
  , connect_cb_(nullptr)
  , disconnect_cb_(nullptr) {
  if (!reactor_) {
    reactor_ = new Reactor;
  }
}

Acceptor::~Acceptor() = default;

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

  int kcpConv = -1;
  if (kcpDirection_ == OutKcp || kcpDirection_ == BothKcp) {
    kcpConv = 0;
  }
  auto *channel = new Channel(reactor_, channelFd, kcpConv);
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
  , ms_(0)
  , bytes_write_(0)
  , bytes_read_(0)
  , kcp_mode_(EKcpMode::None)
  , connected_(true) {
  LOG_INFO << __FUNCTION__ << " fd = " << fd_;
  Reactor::Callback cb = std::bind(&Channel::read, this, std::placeholders::_1);
  reactor_->RegisterIO(cb, fd_);

  if (kcpConv > 0) {
    kcp_mode_ = EKcpMode::Initiative;
    init_kcp(kcpConv_);
  } else if (kcpConv == 0) {
    kcp_mode_ = EKcpMode::Passive;
  }
  channels_.insert(this);

  recv_buffer_ = new unsigned char[SIZE_16M];
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
  ikcp_nodelay(kcp_, 10, 20, 2, 1);
  reactor_->RegisterTimer(kcpCb, 19890, 10);
}

int Channel::read(int fd) {
  const static int BUF_SIZE = SIZE_16M;
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
  LOG_INFO << " Channel[fd=" << fd_ << "] disconnected";
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
  if (disconnect_cb_) {
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
  LOG_DBUG << "fd[" << fd() << "] on read " << size;
  if (kcp_mode_ == Passive && bytes_read_ == 0) {
    kcpConv_ = *(int *)buffer;
    LOG_INFO << "[fd=" << fd_ << "] recv negotiate kcpConv=" << kcpConv_;
    init_kcp(kcpConv_);
    buffer += sizeof(int);
    size -= sizeof(int);
    bytes_read_ += size;
    (*read_cb_)(buffer, size);
    return;
  }
  if (read_cb_) {
    if (kcp_ == nullptr) {
      bytes_read_ += size;
      (*read_cb_)(buffer, size);
      return;
    }
    ikcp_input(kcp_, (char *)buffer, size);
    int payload_size = 0;
    do {
      payload_size = ikcp_recv(kcp_, (char *)recv_buffer_, SIZE_16M);
      if (payload_size > 0) {
        bytes_read_ += payload_size;
        (*read_cb_)(recv_buffer_, payload_size);
      }
    } while (payload_size >= 0);
  }
}

int Channel::write(unsigned char *buf, int size) {
  if (kcp_mode_ == Initiative && (bytes_write_ == 0)) {
    LOG_INFO << "[fd=" << fd_ << "] send negotiate kcpConv=" << kcpConv_;
    auto *data = new unsigned char[size + sizeof(int)];
    int * conv = (int *)data;
    *conv      = kcpConv_;
    memcpy(data + sizeof(int), buf, size);
    bytes_write_ += size;
    size += sizeof(int);
    LOG_INFO << "[fd=" << fd_ << "] send " << size;
    int send = ::write(fd_, data, size);
    delete[] data;
    return send;
  }
  bytes_write_ += size;
  if (kcp_) {
    return ikcp_send(kcp_, (char *)buf, size);
  } else {
    LOG_INFO << "[fd=" << fd_ << "] send " << size;
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
  delete[] recv_buffer_;
  ::close(fd_);
}

int Channel::update_kcp(int elapse) {
  if (kcp_) {
    ikcp_update(kcp_, ms_ += 10);
  }
  return 0;
}

int Channel::write(Channel *channel, unsigned char *buf, int size) {
  if (channels_.find(channel) == channels_.end()) {
    return 0;
  }
  return channel->write(buf, size);
}
