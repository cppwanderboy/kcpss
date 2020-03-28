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

#include "codec.h"

const uint8_t simple_codec::g_encoder[] = {
  61,  169, 202, 2,   24,  140, 113, 203, 70,  237, 139, 195, 255, 18,  60,  164, 138, 103, 13,
  123, 248, 181, 214, 95,  253, 33,  185, 239, 198, 64,  14,  77,  159, 168, 233, 59,  69,  151,
  106, 174, 41,  142, 58,  32,  42,  125, 20,  121, 145, 254, 49,  235, 153, 186, 141, 27,  11,
  177, 130, 100, 172, 97,  112, 128, 166, 39,  146, 136, 196, 224, 230, 219, 178, 162, 3,   28,
  12,  0,   204, 221, 240, 188, 90,  229, 104, 167, 1,   216, 15,  245, 82,  96,  71,  108, 76,
  6,   62,  26,  194, 105, 222, 199, 65,  182, 228, 155, 9,   144, 67,  201, 242, 35,  52,  152,
  98,  175, 184, 244, 154, 55,  110, 251, 19,  156, 133, 25,  54,  234, 134, 37,  137, 236, 118,
  215, 48,  119, 85,  209, 120, 57,  92,  208, 180, 86,  250, 56,  193, 211, 212, 74,  29,  135,
  91,  34,  87,  205, 190, 176, 83,  197, 17,  93,  99,  150, 217, 5,   45,  161, 192, 111, 68,
  225, 249, 243, 38,  220, 207, 231, 206, 147, 173, 75,  158, 109, 40,  115, 47,  179, 107, 126,
  46,  117, 16,  160, 7,   129, 247, 114, 226, 149, 81,  157, 238, 84,  132, 165, 50,  78,  127,
  89,  189, 187, 246, 122, 23,  31,  94,  163, 36,  10,  88,  200, 53,  213, 252, 30,  191, 66,
  218, 183, 79,  63,  22,  170, 8,   124, 232, 80,  51,  223, 101, 210, 21,  116, 73,  72,  43,
  44,  131, 148, 241, 143, 4,   171, 102, 227};

const uint8_t simple_codec::g_decoder[] = {
  77,  86,  3,   74,  252, 165, 95,  194, 234, 106, 219, 56,  76,  18,  30,  88,  192, 160, 13,
  122, 46,  242, 232, 214, 4,   125, 97,  55,  75,  150, 225, 215, 43,  25,  153, 111, 218, 129,
  174, 65,  184, 40,  44,  246, 247, 166, 190, 186, 134, 50,  206, 238, 112, 222, 126, 119, 145,
  139, 42,  35,  14,  0,   96,  231, 29,  102, 227, 108, 170, 36,  8,   92,  245, 244, 149, 181,
  94,  31,  207, 230, 237, 200, 90,  158, 203, 136, 143, 154, 220, 209, 82,  152, 140, 161, 216,
  23,  91,  61,  114, 162, 59,  240, 254, 17,  84,  99,  38,  188, 93,  183, 120, 169, 62,  6,
  197, 185, 243, 191, 132, 135, 138, 47,  213, 19,  235, 45,  189, 208, 63,  195, 58,  248, 204,
  124, 128, 151, 67,  130, 16,  10,  5,   54,  41,  251, 107, 48,  66,  179, 249, 199, 163, 37,
  113, 52,  118, 105, 123, 201, 182, 32,  193, 167, 73,  217, 15,  205, 64,  85,  33,  1,   233,
  253, 60,  180, 39,  115, 157, 57,  72,  187, 142, 21,  103, 229, 116, 26,  53,  211, 81,  210,
  156, 226, 168, 146, 98,  11,  68,  159, 28,  101, 221, 109, 2,   7,   78,  155, 178, 176, 141,
  137, 241, 147, 148, 223, 22,  133, 87,  164, 228, 71,  175, 79,  100, 239, 69,  171, 198, 255,
  104, 83,  70,  177, 236, 34,  127, 51,  131, 9,   202, 27,  80,  250, 110, 173, 117, 89,  212,
  196, 20,  172, 144, 121, 224, 24,  49,  12};