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

#include "config.h"
#include "ini.h"
#include "udp.h"
#include "public.h"
#include "codec.h"
#include "Acceptor.h"
#include "snappy.h"

struct config {
  std::string local;
  std::string remote;
  codec *     remote_codec{};
};

class udp_client {
public:
  udp_client(const char *local, const char *remote, Reactor *reactor, codec *remote_codec = nullptr)
    : udp_(reactor, local, remote), codec_(remote_codec), global_sid_(0) {
    udp::SessionCallbck cb = std::bind(&udp_client::remote_in, this, _1, _2, _3);
    udp_.set_session_callback(cb);
  }

  int remote_in(int sid, unsigned char *buffer, int size) {
    LOG_INFO << "SID[" << sid << "] local in " << size;
    if (codec_)
      codec_->decode(buffer, size);
    if (channels_.find(sid) != channels_.end()) {
      return Channel::write(channels_[sid], buffer, size);
    }
    return -1;
  }

  int local_in(int sid, unsigned char *buffer, int size) {
    LOG_INFO << "SID[" << sid << "] local in " << size;
    if (size == 3 && buffer[0] == 5 && buffer[1] == 1) {
      // fast return to skip socks5 negotiate & reduce 1 RTT time.
      buffer[1] = 0;
      size      = 2;
      return Channel::write(channels_[sid], buffer, size);
    }
    if (codec_)
      codec_->encode(buffer, size);
    return udp_.send(sid, buffer, size);
  }

  int accepted(Channel *channel) {
    int sid = global_sid_++;
    LOG_INFO << "Connection accept, fd=" << channel->fd() << ", sid=" << sid;
    channels_[sid] = channel;

    Channel::ReadCallbck local_in_cb = std::bind(&udp_client::local_in, this, sid, _1, _2);
    channel->set_read_callback(local_in_cb);

    return 0;
  }

private:
  udp                                udp_;
  codec *                            codec_;
  std::unordered_map<int, Channel *> channels_;
  int                                global_sid_;
};

class udp_server {
public:
  udp_server(const char *local, const char *remote, Reactor *reactor, codec *remote_codec = nullptr)
    : udp_(reactor, local), remote_(remote), reactor_(reactor), codec_(remote_codec) {
    udp::SessionCallbck cb = std::bind(&udp_server::loacl_in, this, _1, _2, _3);
    udp_.set_session_callback(cb);
  }

  int loacl_in(int sid, unsigned char *buffer, int size) {
    LOG_INFO << "SID[" << sid << "] local in " << size;
    if (codec_)
      codec_->decode(buffer, size);
    if (channels_.find(sid) != channels_.end()) {
      return Channel::write(channels_[sid], buffer, size);
    } else {
      auto *remote_channel = handle_socks5(buffer, size);
      if (!remote_channel) {
        unsigned char rsp[] = {0x05, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        if (codec_) {
          codec_->encode(rsp, 10);
        }
        return udp_.send(sid, const_cast<unsigned char *>(rsp), 10);
      }
      Channel::ReadCallbck remote_in_cb = std::bind(&udp_server::remote_in, this, _1, _2, sid);
      remote_channel->set_read_callback(remote_in_cb);
      channels_[sid]      = remote_channel;
      unsigned char rsp[] = {0x05, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
      if (codec_) {
        codec_->encode(rsp, 10);
      }
      return udp_.send(sid, const_cast<unsigned char *>(rsp), 10);
    }
  }

  int remote_in(unsigned char *buffer, int size, int sid) {
    LOG_INFO << "SID[" << sid << "] remote in " << size;
    if (codec_) {
      codec_->encode(buffer, size);
    }
    return udp_.send(sid, buffer, size);
  }

protected:
  struct socks5_header {
    uint8_t version;
    uint8_t cmd;
    uint8_t reserved;
    uint8_t addr_type;
    char    addr[];
  };

  Channel *handle_socks5(unsigned char *buffer, int size) {
    auto *header = reinterpret_cast<socks5_header *>(buffer);
    if (header->cmd != 1) {
      return nullptr;
    }
    char *          ip   = header->addr + 1;
    uint8_t         high = *(uint8_t *)(buffer + size - 2);
    uint8_t         low  = *(uint8_t *)(buffer + size - 1);
    int             port = high << 8 | low;
    struct hostent *host;
    switch (header->addr_type) {
      case 1:  // ipv4
        break;
      case 3:  // domain
        *(uint8_t *)(buffer + size - 2) = 0;
        host                            = gethostbyname(ip);
        if (host == nullptr) {  // unknown host
          return nullptr;
        }
        ip = inet_ntoa(*(struct in_addr *)((host)->h_addr_list[0]));
        break;
      case 4:  // ipv6
        *(ip + 16) = 0;
        break;
      default:
        return nullptr;
    }
    int fd = socket::create_tcp(endpoint("tcp", ip, port));
    return new Channel(reactor_, fd, -1);
  }

private:
  endpoint                           remote_;
  udp                                udp_;
  Reactor *                          reactor_;
  codec *                            codec_;
  std::unordered_map<int, Channel *> channels_;
};

void start_server(const config &proxy_config) {
  auto *     reactor = new Reactor;
  udp_server rsp(
    proxy_config.local.c_str(), proxy_config.remote.c_str(), reactor, proxy_config.remote_codec);
  reactor->Run();
}

void start_client(const config &proxy_config) {
  auto *reactor = new Reactor;
  auto *server  = new Acceptor(reactor, NoKcp);
  server->listen(endpoint(proxy_config.local.c_str()).port());
  udp_client rsp(
    proxy_config.local.c_str(), proxy_config.remote.c_str(), reactor, proxy_config.remote_codec);
  Channel::Callback cb = std::bind(&udp_client::accepted, &rsp, _1);
  server->set_connect_callback(cb);
  server->start();
}

config parse_config(const char *config_file, std::string &modeString) {
  inipp::Ini<char> iniConfig;
  std::ifstream    is(config_file);
  if (!is.is_open()) {
    fprintf(stderr, "[Exit] Can't open config file %s\n", config_file);
    exit(0);
  }
  iniConfig.parse(is);

  config run_config;
  if (modeString == "server") {
    run_config.local  = iniConfig.sections["server"]["local"];
    run_config.remote = "";
  } else if (modeString == "client") {
    run_config.local  = iniConfig.sections["client"]["local"];
    run_config.remote = iniConfig.sections["client"]["remote"];
  }
  run_config.remote_codec = new fast_codec;
  return run_config;
}

const char *usage = "Usage: %s [-s] [-c] [-h] [-v]\n";

int main(int argc, char **argv) {
  signal(SIGPIPE, SIG_IGN);

  const char *configFile = "./kcpss.ini";

  std::string modeString;
  int         opt;
  while ((opt = getopt(argc, argv, "scvh?")) != -1) {
    switch (opt) {
      case 's':
        modeString = "server";
        break;
      case 'c':
        modeString = "client";
        break;
      case 'v':
        printf("kcpss version %s\n", PROJECT_VERSION);
        printf("commit [%s]@%s %s\n", GIT_HASH, GIT_BRANCH, GIT_MESSAGE);
        exit(0);
      default: /* '?' */
        fprintf(stderr, usage, argv[0]);
        exit(EXIT_FAILURE);
    }
  }

  mlog::set_level(mlog::LogLevel::CRIT);

  config run_config = parse_config(configFile, modeString);
  if (modeString == "server") {
    start_server(run_config);
  } else if (modeString == "client") {
    start_client(run_config);
  }
}
