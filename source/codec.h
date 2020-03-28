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

class simple_codec : public codec {
public:
  void encode(unsigned char *buffer, int size) override {
    for (int i = 0; i < size; ++i) {
      uint8_t ch = buffer[i];
      buffer[i]  = g_encoder[ch];
    }
  }

  void decode(unsigned char *buffer, int size) override {
    for (int i = 0; i < size; ++i) {
      unsigned char ch = buffer[i];
      buffer[i]        = g_decoder[ch];
    }
  }

private:
  static const uint8_t g_encoder[];
  static const uint8_t g_decoder[];
};

#endif  // KCPSS_CODEC_H
