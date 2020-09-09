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
#ifndef KCPSS_SOCKS5_H
#define KCPSS_SOCKS5_H

class socks5 {
public:
  static bool is_hello(const unsigned char *buffer, int size) {
    return size == 3 && buffer[0] == 5 && buffer[1] == 1;
  }
  static void echo_hello(unsigned char *buffer, int *size) {
    buffer[0] = 5;
    buffer[1] = 0;
    *size     = 2;
  }

  static endpoint parser_endpoint_from_request(unsigned char *buffer, int size) {
    auto *header = reinterpret_cast<socks5_header *>(buffer);
    if (header->cmd != 1) {
      return endpoint::null();
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
          return endpoint::null();
        }
        ip = inet_ntoa(*(struct in_addr *)((host)->h_addr_list[0]));
        break;
      case 4:  // ipv6
        *(ip + 16) = 0;
        break;
      default:
        return endpoint::null();
    }
    return endpoint("tcp", ip, port);
  }

  static int prepare_response(unsigned char *buffer, bool success) {
    static const unsigned char rsp[] = {0x05, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    static const int           rsp_size = 10;
    memcpy(buffer, rsp, rsp_size);
    success ? buffer[1] = 0x00 : buffer[1] = 0x01;
    return rsp_size;
  }

protected:
  struct socks5_header {
    uint8_t version;
    uint8_t cmd;
    uint8_t reserved;
    uint8_t addr_type;
    char    addr[];
  };

private:
};
#endif  // KCPSS_SOCKS5_H
