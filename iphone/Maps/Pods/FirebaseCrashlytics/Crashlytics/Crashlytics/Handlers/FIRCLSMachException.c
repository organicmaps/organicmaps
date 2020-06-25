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

#include "FIRCLSDefines.h"
#include "FIRCLSFeatures.h"

#if CLS_MACH_EXCEPTION_SUPPORTED

#include "FIRCLSGlobals.h"
#include "FIRCLSHandler.h"
#include "FIRCLSMachException.h"
#include "FIRCLSProcess.h"
#include "FIRCLSSignal.h"
#include "FIRCLSUtility.h"

#include <errno.h>
#include <mach/mach.h>
#include <pthread.h>
#include <unistd.h>

#pragma mark Prototypes
static exception_mask_t FIRCLSMachExceptionMask(void);
static void* FIRCLSMachExceptionServer(void* argument);
static bool FIRCLSMachExceptionThreadStart(FIRCLSMachExceptionReadContext* context);
static bool FIRCLSMachExceptionReadMessage(FIRCLSMachExceptionReadContext* context,
                                           MachExceptionMessage* message);
static kern_return_t FIRCLSMachExceptionDispatchMessage(FIRCLSMachExceptionReadContext* context,
                                                        MachExceptionMessage* message);
static bool FIRCLSMachExceptionReply(FIRCLSMachExceptionReadContext* context,
                                     MachExceptionMessage* message,
                                     kern_return_t result);
static bool FIRCLSMachExceptionRegister(FIRCLSMachExceptionReadContext* context,
                                        exception_mask_t ignoreMask);
static bool FIRCLSMachExceptionUnregister(FIRCLSMachExceptionOriginalPorts* originalPorts,
                                          exception_mask_t mask);
static bool FIRCLSMachExceptionRecord(FIRCLSMachExceptionReadContext* context,
                                      MachExceptionMessage* message);

#pragma mark - Initialization
void FIRCLSMachExceptionInit(FIRCLSMachExceptionReadContext* context, exception_mask_t ignoreMask) {
  if (!FIRCLSUnlinkIfExists(context->path)) {
    FIRCLSSDKLog("Unable to reset the mach exception file %s\n", strerror(errno));
  }

  if (!FIRCLSMachExceptionRegister(context, ignoreMask)) {
    FIRCLSSDKLog("Unable to register mach exception handler\n");
    return;
  }

  if (!FIRCLSMachExceptionThreadStart(context)) {
    FIRCLSSDKLog("Unable to start thread\n");
    FIRCLSMachExceptionUnregister(&context->originalPorts, context->mask);
  }
}

void FIRCLSMachExceptionCheckHandlers(void) {
  if (_firclsContext.readonly->debuggerAttached) {
    return;
  }

  // It isn't really critical that this be done, as its extremely uncommon to run into
  // preexisting handlers.
  // Can use task_get_exception_ports for this.
}

static exception_mask_t FIRCLSMachExceptionMask(void) {
  exception_mask_t mask;

  // EXC_BAD_ACCESS
  // EXC_BAD_INSTRUCTION
  // EXC_ARITHMETIC
  // EXC_EMULATION - non-failure
  // EXC_SOFTWARE - non-failure
  // EXC_BREAKPOINT - trap instructions, from the debugger and code. Needs special treatment.
  // EXC_SYSCALL - non-failure
  // EXC_MACH_SYSCALL - non-failure
  // EXC_RPC_ALERT - non-failure
  // EXC_CRASH - see below
  // EXC_RESOURCE - non-failure, happens when a process exceeds a resource limit
  // EXC_GUARD - see below
  //
  // EXC_CRASH is a special kind of exception.  It is handled by launchd, and treated special by
  // the kernel.  Seems that we cannot safely catch it - our handler will never be called.  This
  // is a confirmed kernel bug.  Lacking access to EXC_CRASH means we must use signal handlers to
  // cover all types of crashes.
  // EXC_GUARD is relatively new, and isn't available on all OS versions. You have to be careful,
  // becuase you cannot succesfully register hanlders if there are any unrecognized masks. We've
  // dropped support for old OS versions that didn't have EXC_GUARD (iOS 5 and below, macOS 10.6 and
  // below) so we always add it now

  mask = EXC_MASK_BAD_ACCESS | EXC_MASK_BAD_INSTRUCTION | EXC_MASK_ARITHMETIC |
         EXC_MASK_BREAKPOINT | EXC_MASK_GUARD;

  return mask;
}

static bool FIRCLSMachExceptionThreadStart(FIRCLSMachExceptionReadContext* context) {
  pthread_attr_t attr;

  if (pthread_attr_init(&attr) != 0) {
    FIRCLSSDKLog("pthread_attr_init %s\n", strerror(errno));
    return false;
  }

  if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) != 0) {
    FIRCLSSDKLog("pthread_attr_setdetachstate %s\n", strerror(errno));
    return false;
  }

  // Use to pre-allocate a stack for this thread
  // The stack must be page-aligned
  if (pthread_attr_setstack(&attr, _firclsContext.readonly->machStack,
                            CLS_MACH_EXCEPTION_HANDLER_STACK_SIZE) != 0) {
    FIRCLSSDKLog("pthread_attr_setstack %s\n", strerror(errno));
    return false;
  }

  if (pthread_create(&context->thread, &attr, FIRCLSMachExceptionServer, context) != 0) {
    FIRCLSSDKLog("pthread_create %s\n", strerror(errno));
    return false;
  }

  pthread_attr_destroy(&attr);

  return true;
}

exception_mask_t FIRCLSMachExceptionMaskForSignal(int signal) {
  switch (signal) {
    case SIGTRAP:
      return EXC_MASK_BREAKPOINT;
    case SIGSEGV:
      return EXC_MASK_BAD_ACCESS;
    case SIGBUS:
      return EXC_MASK_BAD_ACCESS;
    case SIGILL:
      return EXC_MASK_BAD_INSTRUCTION;
    case SIGABRT:
      return EXC_MASK_CRASH;
    case SIGSYS:
      return EXC_MASK_CRASH;
    case SIGFPE:
      return EXC_MASK_ARITHMETIC;
  }

  return 0;
}

#pragma mark - Message Handling
static void* FIRCLSMachExceptionServer(void* argument) {
  FIRCLSMachExceptionReadContext* context = argument;

  pthread_setname_np("com.google.firebase.crashlytics.MachExceptionServer");

  while (1) {
    MachExceptionMessage message;

    // read the exception message
    if (!FIRCLSMachExceptionReadMessage(context, &message)) {
      break;
    }

    // handle it, and possibly forward
    kern_return_t result = FIRCLSMachExceptionDispatchMessage(context, &message);

    // and now, reply
    if (!FIRCLSMachExceptionReply(context, &message, result)) {
      break;
    }
  }

  FIRCLSSDKLog("Mach exception server thread exiting\n");

  return NULL;
}

static bool FIRCLSMachExceptionReadMessage(FIRCLSMachExceptionReadContext* context,
                                           MachExceptionMessage* message) {
  mach_msg_return_t r;

  memset(message, 0, sizeof(MachExceptionMessage));

  r = mach_msg(&message->head, MACH_RCV_MSG | MACH_RCV_LARGE, 0, sizeof(MachExceptionMessage),
               context->port, MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
  if (r != MACH_MSG_SUCCESS) {
    FIRCLSSDKLog("Error receving mach_msg (%d)\n", r);
    return false;
  }

  FIRCLSSDKLog("Accepted mach exception message\n");

  return true;
}

static kern_return_t FIRCLSMachExceptionDispatchMessage(FIRCLSMachExceptionReadContext* context,
                                                        MachExceptionMessage* message) {
  FIRCLSSDKLog("Mach exception: 0x%x, count: %d, code: 0x%llx 0x%llx\n", message->exception,
               message->codeCnt, message->codeCnt > 0 ? message->code[0] : -1,
               message->codeCnt > 1 ? message->code[1] : -1);

  // This will happen if a child process raises an exception, as the exception ports are
  // inherited.
  if (message->task.name != mach_task_self()) {
    FIRCLSSDKLog("Mach exception task mis-match, returning failure\n");
    return KERN_FAILURE;
  }

  FIRCLSSDKLog("Unregistering handler\n");
  if (!FIRCLSMachExceptionUnregister(&context->originalPorts, context->mask)) {
    FIRCLSSDKLog("Failed to unregister\n");
    return KERN_FAILURE;
  }

  FIRCLSSDKLog("Restoring original signal handlers\n");
  if (!FIRCLSSignalSafeInstallPreexistingHandlers(&_firclsContext.readonly->signal)) {
    FIRCLSSDKLog("Failed to restore signal handlers\n");
    return KERN_FAILURE;
  }

  FIRCLSSDKLog("Recording mach exception\n");
  if (!FIRCLSMachExceptionRecord(context, message)) {
    FIRCLSSDKLog("Failed to record mach exception\n");
    return KERN_FAILURE;
  }

  return KERN_SUCCESS;
}

static bool FIRCLSMachExceptionReply(FIRCLSMachExceptionReadContext* context,
                                     MachExceptionMessage* message,
                                     kern_return_t result) {
  MachExceptionReply reply;
  mach_msg_return_t r;

  // prepare the reply
  reply.head.msgh_bits = MACH_MSGH_BITS(MACH_MSGH_BITS_REMOTE(message->head.msgh_bits), 0);
  reply.head.msgh_remote_port = message->head.msgh_remote_port;
  reply.head.msgh_size = (mach_msg_size_t)sizeof(MachExceptionReply);
  reply.head.msgh_local_port = MACH_PORT_NULL;
  reply.head.msgh_id = message->head.msgh_id + 100;

  reply.NDR = NDR_record;

  reply.retCode = result;

  FIRCLSSDKLog("Sending exception reply\n");

  // send it
  r = mach_msg(&reply.head, MACH_SEND_MSG, reply.head.msgh_size, 0, MACH_PORT_NULL,
               MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
  if (r != MACH_MSG_SUCCESS) {
    FIRCLSSDKLog("mach_msg reply failed (%d)\n", r);
    return false;
  }

  FIRCLSSDKLog("Exception reply delivered\n");

  return true;
}

#pragma mark - Registration
static bool FIRCLSMachExceptionRegister(FIRCLSMachExceptionReadContext* context,
                                        exception_mask_t ignoreMask) {
  mach_port_t task = mach_task_self();

  kern_return_t kr = mach_port_allocate(task, MACH_PORT_RIGHT_RECEIVE, &context->port);
  if (kr != KERN_SUCCESS) {
    FIRCLSSDKLog("Error: mach_port_allocate failed %d\n", kr);
    return false;
  }

  kr = mach_port_insert_right(task, context->port, context->port, MACH_MSG_TYPE_MAKE_SEND);
  if (kr != KERN_SUCCESS) {
    FIRCLSSDKLog("Error: mach_port_insert_right failed %d\n", kr);
    mach_port_deallocate(task, context->port);
    return false;
  }

  // Get the desired mask, which covers all the mach exceptions we are capable of handling,
  // but clear out any that are in our ignore list.  We do this by ANDing with the bitwise
  // negation.  Because we are only clearing bits, there's no way to set an incorrect mask
  // using ignoreMask.
  context->mask = FIRCLSMachExceptionMask() & ~ignoreMask;

  // ORing with MACH_EXCEPTION_CODES will produce 64-bit exception data
  kr = task_swap_exception_ports(task, context->mask, context->port,
                                 EXCEPTION_DEFAULT | MACH_EXCEPTION_CODES, THREAD_STATE_NONE,
                                 context->originalPorts.masks, &context->originalPorts.count,
                                 context->originalPorts.ports, context->originalPorts.behaviors,
                                 context->originalPorts.flavors);
  if (kr != KERN_SUCCESS) {
    FIRCLSSDKLog("Error: task_swap_exception_ports %d\n", kr);
    return false;
  }

  for (int i = 0; i < context->originalPorts.count; ++i) {
    FIRCLSSDKLog("original 0x%x 0x%x 0x%x 0x%x\n", context->originalPorts.ports[i],
                 context->originalPorts.masks[i], context->originalPorts.behaviors[i],
                 context->originalPorts.flavors[i]);
  }

  return true;
}

static bool FIRCLSMachExceptionUnregister(FIRCLSMachExceptionOriginalPorts* originalPorts,
                                          exception_mask_t mask) {
  kern_return_t kr;

  // Re-register all the old ports.
  for (mach_msg_type_number_t i = 0; i < originalPorts->count; ++i) {
    // clear the bits from this original mask
    mask &= ~originalPorts->masks[i];

    kr =
        task_set_exception_ports(mach_task_self(), originalPorts->masks[i], originalPorts->ports[i],
                                 originalPorts->behaviors[i], originalPorts->flavors[i]);
    if (kr != KERN_SUCCESS) {
      FIRCLSSDKLog("unable to restore original port: %d", originalPorts->ports[i]);
    }
  }

  // Finally, mark any masks we registered for that do not have an original port as unused.
  kr = task_set_exception_ports(mach_task_self(), mask, MACH_PORT_NULL,
                                EXCEPTION_DEFAULT | MACH_EXCEPTION_CODES, THREAD_STATE_NONE);
  if (kr != KERN_SUCCESS) {
    FIRCLSSDKLog("unable to unset unregistered mask: 0x%x", mask);
    return false;
  }

  return true;
}

#pragma mark - Recording
static void FIRCLSMachExceptionNameLookup(exception_type_t number,
                                          mach_exception_data_type_t code,
                                          const char** name,
                                          const char** codeName) {
  if (!name || !codeName) {
    return;
  }

  *name = NULL;
  *codeName = NULL;

  switch (number) {
    case EXC_BAD_ACCESS:
      *name = "EXC_BAD_ACCESS";
      switch (code) {
        case KERN_INVALID_ADDRESS:
          *codeName = "KERN_INVALID_ADDRESS";
          break;
        case KERN_PROTECTION_FAILURE:
          *codeName = "KERN_PROTECTION_FAILURE";
          break;
      }

      break;
    case EXC_BAD_INSTRUCTION:
      *name = "EXC_BAD_INSTRUCTION";
#if CLS_CPU_X86
      *codeName = "EXC_I386_INVOP";
#endif
      break;
    case EXC_ARITHMETIC:
      *name = "EXC_ARITHMETIC";
#if CLS_CPU_X86
      switch (code) {
        case EXC_I386_DIV:
          *codeName = "EXC_I386_DIV";
          break;
        case EXC_I386_INTO:
          *codeName = "EXC_I386_INTO";
          break;
        case EXC_I386_NOEXT:
          *codeName = "EXC_I386_NOEXT";
          break;
        case EXC_I386_EXTOVR:
          *codeName = "EXC_I386_EXTOVR";
          break;
        case EXC_I386_EXTERR:
          *codeName = "EXC_I386_EXTERR";
          break;
        case EXC_I386_EMERR:
          *codeName = "EXC_I386_EMERR";
          break;
        case EXC_I386_BOUND:
          *codeName = "EXC_I386_BOUND";
          break;
        case EXC_I386_SSEEXTERR:
          *codeName = "EXC_I386_SSEEXTERR";
          break;
      }
#endif
      break;
    case EXC_BREAKPOINT:
      *name = "EXC_BREAKPOINT";
#if CLS_CPU_X86
      switch (code) {
        case EXC_I386_DIVERR:
          *codeName = "EXC_I386_DIVERR";
          break;
        case EXC_I386_SGLSTP:
          *codeName = "EXC_I386_SGLSTP";
          break;
        case EXC_I386_NMIFLT:
          *codeName = "EXC_I386_NMIFLT";
          break;
        case EXC_I386_BPTFLT:
          *codeName = "EXC_I386_BPTFLT";
          break;
        case EXC_I386_INTOFLT:
          *codeName = "EXC_I386_INTOFLT";
          break;
        case EXC_I386_BOUNDFLT:
          *codeName = "EXC_I386_BOUNDFLT";
          break;
        case EXC_I386_INVOPFLT:
          *codeName = "EXC_I386_INVOPFLT";
          break;
        case EXC_I386_NOEXTFLT:
          *codeName = "EXC_I386_NOEXTFLT";
          break;
        case EXC_I386_EXTOVRFLT:
          *codeName = "EXC_I386_EXTOVRFLT";
          break;
        case EXC_I386_INVTSSFLT:
          *codeName = "EXC_I386_INVTSSFLT";
          break;
        case EXC_I386_SEGNPFLT:
          *codeName = "EXC_I386_SEGNPFLT";
          break;
        case EXC_I386_STKFLT:
          *codeName = "EXC_I386_STKFLT";
          break;
        case EXC_I386_GPFLT:
          *codeName = "EXC_I386_GPFLT";
          break;
        case EXC_I386_PGFLT:
          *codeName = "EXC_I386_PGFLT";
          break;
        case EXC_I386_EXTERRFLT:
          *codeName = "EXC_I386_EXTERRFLT";
          break;
        case EXC_I386_ALIGNFLT:
          *codeName = "EXC_I386_ALIGNFLT";
          break;
        case EXC_I386_ENDPERR:
          *codeName = "EXC_I386_ENDPERR";
          break;
        case EXC_I386_ENOEXTFLT:
          *codeName = "EXC_I386_ENOEXTFLT";
          break;
      }
#endif
      break;
    case EXC_GUARD:
      *name = "EXC_GUARD";
      break;
  }
}

static bool FIRCLSMachExceptionRecord(FIRCLSMachExceptionReadContext* context,
                                      MachExceptionMessage* message) {
  if (!context || !message) {
    return false;
  }

  if (FIRCLSContextMarkAndCheckIfCrashed()) {
    FIRCLSSDKLog("Error: aborting mach exception handler because crash has already occurred\n");
    exit(1);
    return false;
  }

  FIRCLSFile file;

  if (!FIRCLSFileInitWithPath(&file, context->path, false)) {
    FIRCLSSDKLog("Unable to open mach exception file\n");
    return false;
  }

  FIRCLSFileWriteSectionStart(&file, "mach_exception");

  FIRCLSFileWriteHashStart(&file);

  FIRCLSFileWriteHashEntryUint64(&file, "exception", message->exception);

  // record the codes
  FIRCLSFileWriteHashKey(&file, "codes");
  FIRCLSFileWriteArrayStart(&file);
  for (mach_msg_type_number_t i = 0; i < message->codeCnt; ++i) {
    FIRCLSFileWriteArrayEntryUint64(&file, message->code[i]);
  }
  FIRCLSFileWriteArrayEnd(&file);

  const char* name = NULL;
  const char* codeName = NULL;

  FIRCLSMachExceptionNameLookup(message->exception, message->codeCnt > 0 ? message->code[0] : 0,
                                &name, &codeName);

  FIRCLSFileWriteHashEntryString(&file, "name", name);
  FIRCLSFileWriteHashEntryString(&file, "code_name", codeName);

  FIRCLSFileWriteHashEntryUint64(&file, "original_ports", context->originalPorts.count);
  FIRCLSFileWriteHashEntryUint64(&file, "time", time(NULL));

  FIRCLSFileWriteHashEnd(&file);

  FIRCLSFileWriteSectionEnd(&file);

  FIRCLSHandler(&file, message->thread.name, NULL);

  FIRCLSFileClose(&file);

  return true;
}

#else

INJECT_STRIP_SYMBOL(cls_mach_exception)

#endif
