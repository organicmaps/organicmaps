//----------------------------------------------------------------------------------
// File:            libs\jni\nv_event\scoped_profiler.h
// Samples Version: NVIDIA Android Lifecycle samples 1_0beta 
// Email:           tegradev@nvidia.com
// Web:             http://developer.nvidia.com/category/zone/mobile-development
//
// Copyright 2009-2011 NVIDIA® Corporation 
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
//----------------------------------------------------------------------------------

#ifndef SCOPED_PROFILER_H
#define SCOPED_PROFILER_H

#define PERF_STMTS 0
#if PERF_STMTS == 1
#include "../nv_time/nv_time.hpp"
#include "stdio.h"
#include "stdlib.h"

static char s_bigString[4096];
static int s_bigStringSize;

static char s_tmpBuf[1024];

class ScopedProfiler
{
public:
  ScopedProfiler(const char* text)
  {
    _text = text;
    _startTime = nvGetSystemTime();
    __last = this;
  }
  ~ScopedProfiler()
  {
    stop();
  }
  inline void stop()
  {
    if(_text)
    {
      int size = snprintf(s_tmpBuf, dimof(s_tmpBuf)-1, "%d ms spent in %s" , (int)(nvGetSystemTime() - _startTime), _text);
      strcat(s_bigString + s_bigStringSize, s_tmpBuf);
      s_bigStringSize += size;
      _text = 0;
  }
  static void stopLast()
  {
    if(__last)
      __last->stop();
    __last = 0;
  }
  const char* _text;
  long _startTime;
  static ScopedProfiler* __last;
};
ScopedProfiler* ScopedProfiler::__last = 0;
	
#define STRINGIFIER(s) #s
#define CONCAT_(a,b) a ## b
#define CONCAT(a,b) CONCAT_(a,b)
#define PERFBLURB(s) static const char CONCAT(___str,__LINE__)[] = s "\n"; ScopedProfiler CONCAT(SCOPED_PROFILER,__LINE__)(CONCAT(___str,__LINE__));
#define RESET_PROFILING()  {  DEBUG_D("%s", s_bigString);  s_bigString[0] = 0;  s_bigStringSize = 0;  }
#else
#define PERFBLURB(s)
#define RESET_PROFILING()
#endif

#endif // SCOPED_PROFILER_H

