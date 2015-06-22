/*******************************************************************************
The MIT License (MIT)

Copyright (c) 2015 Alexander Zolotarev <me@alex.bio> from Minsk, Belarus

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*******************************************************************************/

#ifndef TEST_DEFINES_H
#define TEST_DEFINES_H

#include <cstdlib>
#include <exception>
#include <iostream>

#define TEST_EQUAL(x, y)                                                                                        \
  {                                                                                                             \
    auto vx = (x);                                                                                              \
    auto vy = (y);                                                                                              \
    if (vx != vy) {                                                                                             \
      std::cerr << __FILE__ << ':' << __FUNCTION__ << ':' << __LINE__ << " Test failed: " << #x << " != " << #y \
                << " (" << vx << " != " << vy << ")" << std::endl;                                              \
      std::exit(-1);                                                                                            \
    }                                                                                                           \
  }

#define TEST_ALMOST_EQUAL(x, y, epsilon)                                                            \
  {                                                                                                 \
    auto vx = (x);                                                                                  \
    auto vy = (y);                                                                                  \
    if (fabs(vx - vy) > epsilon) {                                                                  \
      cerr << "Test failed: " << #x << " ~!= " << #y << " (" << vx << " ~!= " << vy << ")" << endl; \
      return -1;                                                                                    \
    }                                                                                               \
  }

#define TEST_EXCEPTION(ex, op)                                                                                   \
  {                                                                                                              \
    bool has_fired = false;                                                                                      \
    try {                                                                                                        \
      op;                                                                                                        \
    } catch (const std::exception & exc) {                                                                       \
      has_fired = true;                                                                                          \
      if (typeid(ex) != typeid(exc)) {                                                                           \
        std::cerr << __FILE__ << ':' << __FUNCTION__ << ':' << __LINE__ << " Test failed: " << typeid(ex).name() \
                  << " != " << typeid(exc).name() << std::endl;                                                  \
        std::exit(-1);                                                                                           \
      }                                                                                                          \
    }                                                                                                            \
    if (!has_fired) {                                                                                            \
      std::cerr << __FILE__ << ':' << __FUNCTION__ << ':' << __LINE__ << " Test failed: "                        \
                << "Exception " << typeid(ex).name() << "Was not thrown." << std::endl;                          \
      std::exit(-1);                                                                                             \
    }                                                                                                            \
  }

// Generates unique temporary file name or empty string on error.
inline static std::string GenerateTemporaryFileName() {
#ifdef _MSC_VER
  char tmp_file[L_tmpnam];
  if (0 == ::tmpnam_s(tmp_file, L_tmpnam)) {
    return tmp_file;
  }
#else
  char tmp_file[] = "/tmp/alohalytics_file_manager-XXXXXX";
  if (::mktemp(tmp_file)) {
    return tmp_file;
  }
#endif
  return std::string();
}

#endif  // TEST_DEFINES_H
