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

#ifndef KCPSS_CODEC_H
#define KCPSS_CODEC_H

#include "public.h"

class codec {
public:
  virtual void encode(unsigned char *buffer, int size) = 0;
  virtual void decode(unsigned char *buffer, int size) = 0;
};

class null_codec : public codec {
public:
  void encode(unsigned char *buffer, int size) override {}
  void decode(unsigned char *buffer, int size) override {}
};

class fast_codec : public codec {
public:
  void encode(unsigned char *buffer, int size) override {
    int i = 0;
    for (; i + 8 < size; i += 8) {
      auto *   ch    = (uint64_t *)(&buffer[i]);
      uint64_t left  = (*ch & 0xf0f0f0f0f0f0f0f0U) >> 4U;
      uint64_t right = (*ch & 0x0f0f0f0f0f0f0f0fU) << 4U;
      *ch            = left | right;
    }
    for (; i + 4 < size; i += 4) {
      auto *   ch    = (uint32_t *)(&buffer[i]);
      uint32_t left  = (*ch & 0xf0f0f0f0U) >> 4U;
      uint32_t right = (*ch & 0x0f0f0f0fU) << 4U;
      *ch            = left | right;
    }
    for (; i < size; ++i) {
      uint8_t ch = buffer[i];
      ch         = (ch << 4U) | (ch >> 4U);
      buffer[i]  = ch;
    }
  }
  void decode(unsigned char *buffer, int size) override { encode(buffer, size); }
};

#endif  // KCPSS_CODEC_H
