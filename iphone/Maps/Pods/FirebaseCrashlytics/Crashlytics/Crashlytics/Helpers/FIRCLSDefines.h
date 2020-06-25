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

#include "TargetConditionals.h"

// macro trickiness
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#define CONCAT_EXPANDED(a, b) a##b
#define CONCAT(a, b) CONCAT_EXPANDED(a, b)

// These macros generate a function to force a symbol for the containing .o, to work around an issue
// where strip will not strip debug information without a symbol to strip.
#define DUMMY_FUNCTION_NAME(x) CONCAT(fircls_strip_this_, x)
#define INJECT_STRIP_SYMBOL(x)        \
  void DUMMY_FUNCTION_NAME(x)(void) { \
  }

// These make some target os types available to previous versions of xcode that do not yet have them
// in their SDKs
#ifndef TARGET_OS_IOS
#define TARGET_OS_IOS TARGET_OS_IPHONE
#endif

#ifndef TARGET_OS_WATCH
#define TARGET_OS_WATCH 0
#endif

#ifndef TARGET_OS_TV
#define TARGET_OS_TV 0
#endif

// These help compile based on availability of technologies/frameworks.
#define CLS_TARGET_OS_OSX (TARGET_OS_MAC && !TARGET_OS_IPHONE)
#define CLS_TARGET_OS_HAS_UIKIT (TARGET_OS_IOS || TARGET_OS_TV)

#define CLS_SDK_DISPLAY_VERSION STR(DISPLAY_VERSION)

#define CLS_SDK_GENERATOR_NAME (STR(CLS_SDK_NAME) "/" CLS_SDK_DISPLAY_VERSION)

// arch definitions
#if defined(__arm__) || defined(__arm64__) || defined(__arm64e__)
#include <arm/arch.h>
#endif

#if defined(__arm__)
#define CLS_CPU_ARM 1
#endif
#if defined(__arm64__) || defined(__arm64e__)
#define CLS_CPU_ARM64 1
#endif
#if defined(__ARM_ARCH_7S__)
#define CLS_CPU_ARMV7S 1
#endif
#if defined(_ARM_ARCH_7)
#define CLS_CPU_ARMV7 1
#endif
#if defined(_ARM_ARCH_6)
#define CLS_CPU_ARMV6 1
#endif
#if defined(__i386__)
#define CLS_CPU_I386 1
#endif
#if defined(__x86_64__)
#define CLS_CPU_X86_64 1
#endif
#define CLS_CPU_X86 (CLS_CPU_I386 || CLS_CPU_X86_64)
#define CLS_CPU_64BIT (CLS_CPU_X86_64 || CLS_CPU_ARM64)
