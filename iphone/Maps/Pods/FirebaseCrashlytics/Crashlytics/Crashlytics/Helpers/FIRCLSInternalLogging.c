// Copyright 2019 Google
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "FIRCLSInternalLogging.h"
#include "FIRCLSContext.h"
#include "FIRCLSGlobals.h"
#include "FIRCLSUtility.h"

void FIRCLSSDKFileLog(FIRCLSInternalLogLevel level, const char* format, ...) {
  if (!_firclsContext.readonly || !_firclsContext.writable) {
    return;
  }

  const char* path = _firclsContext.readonly->logPath;
  if (!FIRCLSIsValidPointer(path)) {
    return;
  }

  if (_firclsContext.writable->internalLogging.logLevel > level) {
    return;
  }

  if (_firclsContext.writable->internalLogging.logFd == -1) {
    _firclsContext.writable->internalLogging.logFd = open(path, O_WRONLY | O_CREAT | O_APPEND, 0644);
  }

  const int fd = _firclsContext.writable->internalLogging.logFd;
  if (fd < 0) {
    return;
  }

  va_list args;
  va_start(args, format);

#if DEBUG && 0
  // It's nice to use printf here, so all the formatting works. However, its possible to hit a
  // deadlock if you call vfprintf in a crash handler. So, this code is handy to keep, just in case,
  // if there's a really tough thing to debug.
  FILE* file = fopen(path, "a+");
  vfprintf(file, format, args);
  fclose(file);
#else
  size_t formatLength = strlen(format);
  for (size_t idx = 0; idx < formatLength; ++idx) {
    if (format[idx] != '%') {
      write(fd, &format[idx], 1);
      continue;
    }

    idx++;  // move to the format char
    switch (format[idx]) {
      case 'd': {
        int value = va_arg(args, int);
        FIRCLSFileFDWriteInt64(fd, value);
      } break;
      case 'u': {
        uint32_t value = va_arg(args, uint32_t);
        FIRCLSFileFDWriteUInt64(fd, value, false);
      } break;
      case 'p': {
        uintptr_t value = va_arg(args, uintptr_t);
        write(fd, "0x", 2);
        FIRCLSFileFDWriteUInt64(fd, value, true);
      } break;
      case 's': {
        const char* string = va_arg(args, const char*);
        if (!string) {
          string = "(null)";
        }

        write(fd, string, strlen(string));
      } break;
      case 'x': {
        unsigned int value = va_arg(args, unsigned int);
        FIRCLSFileFDWriteUInt64(fd, value, true);
      } break;
      default:
        // unhandled, back up to write out the percent + the format char
        write(fd, &format[idx - 1], 2);
        break;
    }
  }
#endif
  va_end(args);
}
