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

#pragma once

#include <Foundation/Foundation.h>

#define FIRCLSURLSESSION_REQUIRED (!TARGET_OS_WATCH && !TARGET_OS_TV)

// These macros generate a function to force a symbol for the containing .o, to work around an issue
// where strip will not strip debug information without a symbol to strip.
#define CONCAT_EXPANDED(a, b) a##b
#define CONCAT(a, b) CONCAT_EXPANDED(a, b)
#define DUMMY_FUNCTION_NAME(x) CONCAT(fircls_strip_this_, x)
#define INJECT_STRIP_SYMBOL(x)        \
  void DUMMY_FUNCTION_NAME(x)(void) { \
  }
