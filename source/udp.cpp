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

#include "udp.h"
#include "socket.h"

udp::udp(Reactor *reactor, const char *addr, const char *remote_addr)
  : reactor_(reactor), ms_(0), kcp_(nullptr), target_(nullptr), cb_(nullptr) {
  fd_ = socket::create_udp(addr);

  Reactor::Callback cb = std::bind(&udp::read_socket, this);
  reactor_->RegisterIO(cb, fd_);

  if (remote_addr) {
    target_ = new endpoint(remote_addr);
  }
  std::random_device         rd;
  std::default_random_engine e(rd());
  kcp_ = crtete_kcp(e());

  segment_      = reinterpret_cast<SessionHeader *>(new char[MTU]);
  recv_bufffer_ = new unsigned char[SIZE_4M];
}

udp::~udp() {
  if (target_) {
    delete target_;
    target_ = nullptr;
  }
  if (cb_) {
    delete cb_;
    cb_ = nullptr;
  }
  delete[](char *) segment_;
  delete[] recv_bufffer_;
}

int udp_socket_output(const char *buf, int size, ikcpcb *kcp, void *fd) {
  auto *channel = reinterpret_cast<udp *>(fd);
  return channel->write(kcp->conv, (unsigned char *)buf, size);
}

ikcpcb *udp::crtete_kcp(int conv) {
  LOG_INFO << "init kcp channel, kcpConv = " << conv;
  ikcpcb *kcp = ikcp_create(conv, this);
  kcp->output = udp_socket_output;
  ikcp_nodelay(kcp, 1, 10, 2, 1);
  ikcp_wndsize(kcp, 4096, 4096);

  Reactor::Callback kcpCb = [=](int elapse) -> int {
    if (kcp) {
      ikcp_update(kcp, ms_ += 10);
    }
    return 0;
  };
  reactor_->RegisterTimer(kcpCb, 19890, 10);
  return kcp;
}

int udp::send(int conv, int sid, unsigned char *buffer, int size) {
  segment_->sid  = sid;
  segment_->size = MAX_PAYLOAD;
  while (size > MAX_PAYLOAD) {
    memcpy(segment_->data, buffer, MAX_PAYLOAD);
    ikcp_send(kcp_, (const char *)segment_, MTU);
    size -= MAX_PAYLOAD;
    buffer += MAX_PAYLOAD;
  }
  segment_->size = size;
  memcpy(segment_->data, buffer, size);
  return ikcp_send(kcp_, (const char *)segment_, size + SessionHeaderSize);
}

int udp::write(int conv, unsigned char *buffer, int size) {
  LOG_INFO << "conv[" << conv << "] write " << size << " bytes";
  int len = sizeof(sockaddr);
  return ::sendto(fd_, buffer, size, 0, (struct sockaddr *)target_->sockaddr(), len);
}

int udp::read_socket() {
  while (true) {
    sockaddr_in target{};
    socklen_t   len = sizeof(target);
    ssize_t     ret = ::recvfrom(fd_, recv_bufffer_, SIZE_4M, 0, (sockaddr *)&target, &len);
    if (target_ == nullptr) {
      target_ = new endpoint(target);
    }
    if (ret < 0) {
      LOG_CRIT << "read udp socket error, fd = " << fd_ << ", ret = " << (int)ret;
    } else if (ret == 0) {
      break;
    } else if (ret < SIZE_4M) {
      on_read(recv_bufffer_, ret, &target);
      break;
    } else if (ret == SIZE_4M) {
      on_read(recv_bufffer_, ret, &target);
    }
  }
  return 0;
}

void udp::on_read(unsigned char *buffer, int size, sockaddr_in *target) {
  LOG_DBUG << "fd[" << fd_ << "] on read " << size;
  if (kcp_) {
    ikcp_input(kcp_, reinterpret_cast<const char *>(buffer), size);
    int payload_size;
    do {
      payload_size = ikcp_recv(kcp_, (char *)segment_, MTU);
      if (payload_size > 0) {
        if (cb_) {
          (*cb_)(kcp_->conv, segment_->sid, segment_->data, segment_->size);
        }
      }
    } while (payload_size >= 0);
  }
}

void udp::set_session_callback(udp::SessionCallbck &cb) {
  if (!cb_) {
    cb_ = new SessionCallbck(cb);
  }
}

void udp_server::on_read(unsigned char *buffer, int size, sockaddr_in *target) {
  endpoint ep(*target);
  ikcpcb * kcp;
  int      conv = ikcp_getconv(buffer);
  LOG_INFO << "read " << size << " bytes from " << ep.host() << ":" << ep.port() << " conv[" << conv
           << "] fd[" << fd() << "]";
  if (!connected_client(conv)) {
    if (client_conv_.find(ep) != client_conv_.end()) {
      auto *old_kcp  = client_conv_[ep];
      auto  old_conv = old_kcp->conv;
      LOG_INFO << "Release old kcp conv=" << old_conv << ", target=" << ep.host() << ":"
               << ep.port();
      ikcp_release(old_kcp);
      conv_ep_.erase(old_conv);
      conv_kcp_.erase(old_conv);
    }
    kcp = crtete_kcp(conv);
    LOG_INFO << "new kcp conv[" << conv << "] from " << ep.host() << ":" << ep.port();
    client_conv_[ep] = kcp;
    conv_kcp_[conv]  = kcp;
    conv_ep_[conv]   = ep;
  } else {
    kcp         = conv_kcp_[conv];
    auto old_ep = conv_ep_[conv];
    if (!(old_ep == ep)) {
      LOG_INFO << "end point changed";
      conv_ep_[conv]   = ep;
      client_conv_[ep] = kcp;
    }
  }
  if (kcp) {
    ikcp_input(kcp, reinterpret_cast<const char *>(buffer), size);
    int payload_size;
    do {
      payload_size = ikcp_recv(kcp, (char *)segment_, MTU);
      if (payload_size > 0) {
        if (cb_) {
          (*cb_)(kcp->conv, segment_->sid, segment_->data, segment_->size);
        }
      } else {
        LOG_INFO << "invalid payload size " << payload_size;
      }
    } while (payload_size >= 0);
  } else {
    LOG_CRIT << "no kcp found for " << ep.host() << ":" << ep.port() << " conv[" << conv << "] fd["
             << fd() << "]";
  }
}
bool udp_server::connected_client(int conv) {
  auto it = conv_kcp_.find(conv);
  return !(it == conv_kcp_.end());
}
int udp_server::write(int conv, unsigned char *buffer, int size) {
  int len = sizeof(sockaddr);
  return ::sendto(fd_, buffer, size, 0, (struct sockaddr *)conv_ep_[conv].sockaddr(), len);
}
int udp_server::send(int conv, int sid, unsigned char *buffer, int size) {
  segment_->sid  = sid;
  segment_->size = MAX_PAYLOAD;
  if (conv_kcp_.find(conv) == conv_kcp_.end()) {
    LOG_CRIT << "no kcp find for "
             << " conv[" << conv << "] sid[" << sid << "]";
    return -1;
  }
  ikcpcb *kcp = conv_kcp_[conv];
  while (size > MAX_PAYLOAD) {
    memcpy(segment_->data, buffer, MAX_PAYLOAD);
    ikcp_send(kcp, (const char *)segment_, MTU);
    size -= MAX_PAYLOAD;
    buffer += MAX_PAYLOAD;
  }
  segment_->size = size;
  memcpy(segment_->data, buffer, size);
  return ikcp_send(kcp, (const char *)segment_, size + SessionHeaderSize);
}
