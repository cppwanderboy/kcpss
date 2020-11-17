# kcpss 
![CI](https://github.com/cppwanderboy/kcpss/workflows/CI/badge.svg?branch=master)
[![CodeFactor](https://www.codefactor.io/repository/github/cppwanderboy/kcpss/badge/master)](https://www.codefactor.io/repository/github/cppwanderboy/kcpss/overview/master)
[![Language grade: C/C++](https://img.shields.io/lgtm/grade/cpp/g/cppwanderboy/kcpss.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/cppwanderboy/kcpss/context:cpp)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](https://github.com/technote-space/release-github-actions/blob/master/LICENSE)

A simple socks5 proxy server and cient based libev and kcptun.

## Install & Run
Download the latest release and modify config file as below.
``` ini
[client]
local = tcp://0.0.0.0:1088
remote = udp://192.168.1.3:4088

[server]
local = udp://192.168.1.3:4088
```
Run as server through `./kcpss -s` or as client through `./kcpss -c`.

## Notes
* feel free to modify `source/codec.h` to encrypt you messages
* not support windows yet
