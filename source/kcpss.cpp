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
#include "socks5.h"

struct config {
  std::string local;
  std::string remote;
  codec *     remote_codec{new null_codec};
};

constexpr int heartbeat_sid = -1989;

class proxy_client {
public:
  proxy_client(const char *local,
               const char *remote,
               Reactor *   reactor,
               codec *     codec = new null_codec)
    : udp_(reactor, local, remote), codec_(codec), max_sid_(0) {
    udp::SessionCallbck cb = std::bind(&proxy_client::remote_in, this, _1, _2, _3, _4);
    udp_.set_session_callback(cb);

    Reactor::Callback heartbeat = [=](int elapse) -> int {
      char const *kcpss{"kcpss"};
      udp_.send(-1, heartbeat_sid, (unsigned char *)kcpss, sizeof(kcpss));
      return 0;
    };
    reactor->RegisterTimer(heartbeat, 19890, 1000);
  }

  int remote_in(int conv, int sid, unsigned char *buffer, int size) {
    LOG_DBUG << "kcp://" << conv << ":" << sid << " remote in " << size << " bytes";
    if (sid == heartbeat_sid) {
      return 0;
    }
    codec_->decode(buffer, size);
    if (channels_.find(sid) != channels_.end()) {
      return Channel::write(channels_[sid], buffer, size);
    }
    return -1;
  }

  int local_in(int sid, unsigned char *buffer, int size) {
    LOG_DBUG << "kcp://NULL:" << sid << " local in " << size << " bytes";
    if (socks5::is_hello(buffer, size)) {
      // fast return to skip socks5 negotiate & reduce 1 RTT time.
      socks5::echo_hello(buffer, &size);
      return Channel::write(channels_[sid], buffer, size);
    }
    codec_->encode(buffer, size);
    return udp_.send(-1, sid, buffer, size);
  }

  int accepted(Channel *channel) {
    int sid = max_sid_++;
    LOG_INFO << "Connection accept, fd[" << channel->fd() << "], sid[" << sid << "]";
    channels_[sid] = channel;

    Channel::ReadCallbck cb = std::bind(&proxy_client::local_in, this, sid, _1, _2);
    channel->set_read_callback(cb);
    return 0;
  }

private:
  udp                                udp_;
  codec *                            codec_;
  std::unordered_map<int, Channel *> channels_;
  int                                max_sid_;
};

class proxy_server {
public:
  proxy_server(const char *local, Reactor *reactor, codec *codec = new null_codec)
    : udp_(reactor, local), reactor_(reactor), codec_(codec) {
    codec_                 = codec ? codec : new null_codec;
    udp::SessionCallbck cb = std::bind(&proxy_server::local_in, this, _1, _2, _3, _4);
    udp_.set_session_callback(cb);
  }

  int local_in(int conv, int sid, unsigned char *buffer, int size) {
    LOG_DBUG << "kcp://" << conv << ":" << sid << " local in " << size << " bytes";
    if (sid == heartbeat_sid) {
      return 0;
    }
    codec_->decode(buffer, size);
    key_t key         = (static_cast<uint64_t>(conv) << 32U) | static_cast<uint32_t>(sid);
    bool  new_request = channels_.find(key) == channels_.end();
    if (!new_request) {
      return Channel::write(channels_[key], buffer, size);
    } else {
      LOG_INFO << "create channel for conv[" << conv << "] sid[" << sid << "], "
               << "channel.size[" << channels_.size() << "]";
      auto target = socks5::parser_endpoint_from_request(buffer, size);
      bool is_ok  = !target.is_null();
      if (is_ok) {
        int   fd             = socket::create_tcp(target);
        auto *remote_channel = new Channel(reactor_, fd, -1);
        is_ok                = (remote_channel != nullptr);
        if (is_ok) {
          Channel::ReadCallbck cb = std::bind(&proxy_server::remote_in, this, conv, _1, _2, sid);
          remote_channel->set_read_callback(cb);
          Channel::Callback rmMap = [this](Channel *channel) -> int {
            for (auto pair : channels_) {
              if (pair.second == channel) {
                channels_.erase(pair.first);
                LOG_INFO << "remove proxy client channel, "
                         << "channel.size[" << channels_.size() << "]";
                break;
              }
            }
            return 0;
          };
          remote_channel->set_disconnect_callback(rmMap);
          channels_[key] = remote_channel;
        }
      }
      unsigned char rsp[16];
      int           rsp_size = socks5::prepare_response(rsp, is_ok);
      codec_->encode(rsp, rsp_size);
      return udp_.send(conv, sid, const_cast<unsigned char *>(rsp), rsp_size);
    }
  }

  int remote_in(int conv, unsigned char *buffer, int size, int sid) {
    LOG_DBUG << "kcp://" << conv << ":" << sid << " remote in " << size << " bytes";
    codec_->encode(buffer, size);
    return udp_.send(conv, sid, buffer, size);
  }

private:
  using key_t = uint64_t;
  udp_server                           udp_;
  Reactor *                            reactor_;
  codec *                              codec_;
  std::unordered_map<key_t, Channel *> channels_;
};

void start_server(const config &proxy_config) {
  auto *       reactor = new Reactor;
  proxy_server rsp(proxy_config.local.c_str(), reactor, proxy_config.remote_codec);
  reactor->Run();
}

void start_client(const config &proxy_config) {
  auto *reactor = new Reactor;
  auto *server  = new Acceptor(reactor);
  server->listen(endpoint(proxy_config.local.c_str()).port());
  proxy_client rsp(
    proxy_config.local.c_str(), proxy_config.remote.c_str(), reactor, proxy_config.remote_codec);
  Channel::Callback cb = std::bind(&proxy_client::accepted, &rsp, _1);
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

  auto logLevel = iniConfig.sections["log"]["level"];
  std::transform(logLevel.begin(), logLevel.end(), logLevel.begin(), [](unsigned char c) {
    return std::tolower(c);
  });
  mlog::LogLevel level = mlog::LogLevel::INFO;
  if (logLevel == "critical" || logLevel == "crit") {
    level = mlog::LogLevel::CRIT;
  } else if (logLevel == "warn" || logLevel == "warning") {
    level = mlog::LogLevel::WARN;
  } else if (logLevel == "info") {
    level = mlog::LogLevel::INFO;
  } else if (logLevel == "debug" || logLevel == "dbug") {
    level = mlog::LogLevel::DBUG;
  }
  mlog::set_level(level);
  LOG_CRIT << "[config] log-level = " << mlog::mlog::level_str(level);
  return run_config;
}

bool file_exist(const char *fileName) {
  return std::ifstream(fileName).good();
}

const char *usage = "Usage: %s [-s] [-c] [-h] [-v]\n";

int main(int argc, char **argv) {
  signal(SIGPIPE, SIG_IGN);

  const char *configFile = "/etc/kcpss.ini";
  if (file_exist("./kcpss.ini")) {
    configFile = "./kcpss.ini";
  }

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

  config run_config = parse_config(configFile, modeString);
  if (modeString == "server") {
    start_server(run_config);
  } else if (modeString == "client") {
    start_client(run_config);
  }
}
