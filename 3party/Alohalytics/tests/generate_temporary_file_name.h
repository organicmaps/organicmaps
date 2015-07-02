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

#ifndef GENERATE_TEMPORARY_FILE_NAME
#define GENERATE_TEMPORARY_FILE_NAME

#include <stdlib.h>
#include <string>

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

#endif  // GENERATE_TEMPORARY_FILE_NAME
