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

#include "FIRCLSUnwind.h"
#include "FIRCLSBinaryImage.h"
#include "FIRCLSCompactUnwind.h"
#include "FIRCLSFeatures.h"
#include "FIRCLSGlobals.h"
#include "FIRCLSUtility.h"

#include <mach/mach.h>
#include <signal.h>
#include <stdio.h>

// Without a limit on the number of frames we unwind, there's a real possibility
// we'll get stuck in an infinite loop. But, we still need pretty big limits,
// because stacks can get quite big. Also, the stacks are different on the platforms.
// These values were empirically determined (~525000 on OS X, ~65000 on iOS).
#if TARGET_OS_EMBEDDED
const uint32_t FIRCLSUnwindMaxFrames = 100000;
#else
const uint32_t FIRCLSUnwindMaxFrames = 600000;
#endif

const uint32_t FIRCLSUnwindInfiniteRecursionCountThreshold = 10;

#pragma mark Prototypes
static bool FIRCLSUnwindNextFrameUsingAllStrategies(FIRCLSUnwindContext* context);
#if CLS_COMPACT_UNWINDING_SUPPORTED
static bool FIRCLSUnwindWithCompactUnwindInfo(FIRCLSUnwindContext* context);
#endif
bool FIRCLSUnwindContextHasValidPCAndSP(FIRCLSUnwindContext* context);

#pragma mark - API
bool FIRCLSUnwindInit(FIRCLSUnwindContext* context, FIRCLSThreadContext threadContext) {
  if (!context) {
    return false;
  }

  memset(context, 0, sizeof(FIRCLSUnwindContext));

  context->registers = threadContext;

  return true;
}

bool FIRCLSUnwindNextFrame(FIRCLSUnwindContext* context) {
  if (!FIRCLSIsValidPointer(context)) {
    FIRCLSSDKLog("Error: invalid inputs\n");
    return false;
  }

  if (!FIRCLSUnwindContextHasValidPCAndSP(context)) {
    // This is a special-case. It is possible to try to unwind a thread that has no stack (ie, is
    // executing zero functions. I believe this happens when a thread has exited, but before the
    // kernel has actually cleaned it up. This situation can only apply to the first frame. So, in
    // that case, we don't count it as an error. But, if it happens mid-unwind, it's a problem.

    if (context->frameCount == 0) {
      FIRCLSSDKLog("Cancelling unwind for thread with invalid PC/SP\n");
    } else {
      FIRCLSSDKLog("Error: thread PC/SP invalid before unwind\n");
    }

    return false;
  }

  if (!FIRCLSUnwindNextFrameUsingAllStrategies(context)) {
    FIRCLSSDKLogError("Failed to advance to the next frame\n");
    return false;
  }

  uintptr_t pc = FIRCLSUnwindGetPC(context);
  uintptr_t sp = FIRCLSUnwindGetStackPointer(context);

  // Unwinding will complete when this is no longer a valid value
  if (!FIRCLSIsValidPointer(pc)) {
    return false;
  }

  // after unwinding, validate that we have a sane register value
  if (!FIRCLSIsValidPointer(sp)) {
    FIRCLSSDKLog("Error: SP (%p) isn't a valid pointer\n", (void*)sp);
    return false;
  }

  // track repeating frames
  if (context->lastFramePC == pc) {
    context->repeatCount += 1;
  } else {
    context->repeatCount = 0;
  }

  context->frameCount += 1;
  context->lastFramePC = pc;

  return true;
}

#pragma mark - Register Accessors
uintptr_t FIRCLSUnwindGetPC(FIRCLSUnwindContext* context) {
  if (!FIRCLSIsValidPointer(context)) {
    return 0;
  }

  return FIRCLSThreadContextGetPC(&context->registers);
}

uintptr_t FIRCLSUnwindGetStackPointer(FIRCLSUnwindContext* context) {
  if (!FIRCLSIsValidPointer(context)) {
    return 0;
  }

  return FIRCLSThreadContextGetStackPointer(&context->registers);
}

static uintptr_t FIRCLSUnwindGetFramePointer(FIRCLSUnwindContext* context) {
  if (!FIRCLSIsValidPointer(context)) {
    return 0;
  }

  return FIRCLSThreadContextGetFramePointer(&context->registers);
}

uint32_t FIRCLSUnwindGetFrameRepeatCount(FIRCLSUnwindContext* context) {
  if (!FIRCLSIsValidPointer(context)) {
    return 0;
  }

  return context->repeatCount;
}

#pragma mark - Unwind Strategies
static bool FIRCLSUnwindNextFrameUsingAllStrategies(FIRCLSUnwindContext* context) {
  if (!FIRCLSIsValidPointer(context)) {
    FIRCLSSDKLogError("Arguments invalid\n");
    return false;
  }

  if (context->frameCount >= FIRCLSUnwindMaxFrames) {
    FIRCLSSDKLogWarn("Exceeded maximum number of frames\n");
    return false;
  }

  uintptr_t pc = FIRCLSUnwindGetPC(context);

  // Ok, what's going on here? libunwind's UnwindCursor<A,R>::setInfoBasedOnIPRegister has a
  // parameter that, if true, does this subtraction. Despite the comments in the code
  // (of 35.1), I found that the parameter was almost always set to true.
  //
  // I then ran into a problem when unwinding from _pthread_start -> thread_start. This
  // is a common transition, which happens in pretty much every report. An extra frame
  // was being generated, because the PC we get for _pthread_start was mapping to exactly
  // one greater than the function's last byte, according to the compact unwind info. This
  // resulted in using the wrong compact encoding, and picking the next function, which
  // turned out to be dwarf instead of a frame pointer.

  // So, the moral is - do the subtraction for all frames except the first. I haven't found
  // a case where it produces an incorrect result. Also note that at first, I thought this would
  // subtract one from the final addresses too. But, the end of this function will *compute* PC,
  // so this value is used only to look up unwinding data.

  if (context->frameCount > 0) {
    --pc;
    if (!FIRCLSThreadContextSetPC(&context->registers, pc)) {
      FIRCLSSDKLogError("Unable to set PC\n");
      return false;
    }
  }

  if (!FIRCLSIsValidPointer(pc)) {
    FIRCLSSDKLogError("PC is invalid\n");
    return false;
  }

  // the first frame is special - as the registers we need
  // are already loaded by definition
  if (context->frameCount == 0) {
    return true;
  }

#if CLS_COMPACT_UNWINDING_SUPPORTED
  // attempt to advance to the next frame using compact unwinding, and
  // only fall back to the frame pointer if that fails
  if (FIRCLSUnwindWithCompactUnwindInfo(context)) {
    return true;
  }
#endif

  // If the frame pointer is zero, we cannot use an FP-based unwind and we can reasonably
  // assume that we've just gotten to the end of the stack.
  if (FIRCLSUnwindGetFramePointer(context) == 0) {
    FIRCLSSDKLogWarn("FP is zero, aborting unwind\n");
    // make sure to set the PC to zero, to indicate the unwind is complete
    return FIRCLSThreadContextSetPC(&context->registers, 0);
  }

  // Only allow stack scanning (as a last resort) if we're on the first frame. All others
  // are too likely to screw up.
  if (FIRCLSUnwindWithFramePointer(&context->registers, context->frameCount == 1)) {
    return true;
  }

  FIRCLSSDKLogError("Unable to use frame pointer\n");

  return false;
}

#if CLS_COMPACT_UNWINDING_SUPPORTED
static bool FIRCLSUnwindWithCompactUnwindInfo(FIRCLSUnwindContext* context) {
  if (!context) {
    return false;
  }

  // step one - find the image the current pc is within
  FIRCLSBinaryImageRuntimeNode image;

  uintptr_t pc = FIRCLSUnwindGetPC(context);

  if (!FIRCLSBinaryImageSafeFindImageForAddress(pc, &image)) {
    FIRCLSSDKLogWarn("Unable to find binary for %p\n", (void*)pc);
    return false;
  }

#if CLS_BINARY_IMAGE_RUNTIME_NODE_RECORD_NAME
  FIRCLSSDKLogDebug("Binary image for %p at %p => %s\n", (void*)pc, image.baseAddress, image.name);
#else
  FIRCLSSDKLogDebug("Binary image for %p at %p\n", (void*)pc, image.baseAddress);
#endif

  if (!FIRCLSBinaryImageSafeHasUnwindInfo(&image)) {
    FIRCLSSDKLogInfo("Binary image at %p has no unwind info\n", image.baseAddress);
    return false;
  }

  if (!FIRCLSCompactUnwindInit(&context->compactUnwindState, image.unwindInfo, image.ehFrame,
                               (uintptr_t)image.baseAddress)) {
    FIRCLSSDKLogError("Unable to read unwind info\n");
    return false;
  }

  // this function will actually attempt to find compact unwind info for the current PC,
  // and use it to mutate the context register state
  return FIRCLSCompactUnwindLookupAndCompute(&context->compactUnwindState, &context->registers);
}
#endif

#pragma mark - Utility Functions
bool FIRCLSUnwindContextHasValidPCAndSP(FIRCLSUnwindContext* context) {
  return FIRCLSIsValidPointer(FIRCLSUnwindGetPC(context)) &&
         FIRCLSIsValidPointer(FIRCLSUnwindGetStackPointer(context));
}

#if CLS_CPU_64BIT
#define BASIC_INFO_TYPE vm_region_basic_info_64_t
#define BASIC_INFO VM_REGION_BASIC_INFO_64
#define BASIC_INFO_COUNT VM_REGION_BASIC_INFO_COUNT_64
#define vm_region_query_fn vm_region_64
#else
#define BASIC_INFO_TYPE vm_region_basic_info_t
#define BASIC_INFO VM_REGION_BASIC_INFO
#define BASIC_INFO_COUNT VM_REGION_BASIC_INFO_COUNT
#define vm_region_query_fn vm_region
#endif
bool FIRCLSUnwindIsAddressExecutable(vm_address_t address) {
#if CLS_COMPACT_UNWINDING_SUPPORTED
  FIRCLSBinaryImageRuntimeNode unusedNode;

  return FIRCLSBinaryImageSafeFindImageForAddress(address, &unusedNode);
#else
  return true;
#endif
}

bool FIRCLSUnwindFirstExecutableAddress(vm_address_t start,
                                        vm_address_t end,
                                        vm_address_t* foundAddress) {
  // This function walks up the data on the stack, looking for the first value that is an address on
  // an exectuable page.  This is a heurestic, and can hit false positives.

  *foundAddress = 0;  // write in a 0

  do {
    vm_address_t address;

    FIRCLSSDKLogDebug("Checking address %p => %p\n", (void*)start, (void*)*(uintptr_t*)start);

    // if start isn't a valid pointer, don't even bother trying
    if (FIRCLSIsValidPointer(start)) {
      if (!FIRCLSReadMemory(start, &address, sizeof(void*))) {
        // if we fail to read from the stack, we're done
        return false;
      }

      FIRCLSSDKLogDebug("Checking for executable %p\n", (void*)address);
      // when we find an exectuable address, we're finished
      if (FIRCLSUnwindIsAddressExecutable(address)) {
        *foundAddress = address;
        return true;
      }
    }

    start += sizeof(void*);  // move back up the stack

  } while (start < end);

  return false;
}
