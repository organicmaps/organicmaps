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

#include "FIRCLSProfiling.h"

#include <mach/mach_time.h>
#include <stdio.h>

FIRCLSProfileMark FIRCLSProfilingStart(void) {
  return mach_absolute_time();
}

double FIRCLSProfileEnd(FIRCLSProfileMark mark) {
  uint64_t duration = mach_absolute_time() - mark;

  mach_timebase_info_data_t info;
  mach_timebase_info(&info);

  if (info.denom == 0) {
    return 0.0;
  }

  // Convert to nanoseconds
  duration *= info.numer;
  duration /= info.denom;

  return (double)duration / (double)NSEC_PER_MSEC;  // return time in milliseconds
}

void FIRCLSProfileBlock(const char* label, void (^block)(void)) {
  FIRCLSProfileMark mark = FIRCLSProfilingStart();

  block();

  fprintf(stderr, "[Profile] %s: %f ms\n", label, FIRCLSProfileEnd(mark));
}
