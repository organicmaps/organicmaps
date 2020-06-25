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

#include "FIRCLSProcess.h"
#include "FIRCLSDefines.h"
#include "FIRCLSFeatures.h"
#include "FIRCLSGlobals.h"
#include "FIRCLSProfiling.h"
#include "FIRCLSThreadState.h"
#include "FIRCLSUnwind.h"
#include "FIRCLSUtility.h"

#include <dispatch/dispatch.h>
#include <objc/message.h>
#include <pthread.h>
#include <sys/sysctl.h>

#define THREAD_NAME_BUFFER_SIZE (64)

#pragma mark Prototypes
static bool FIRCLSProcessGetThreadName(FIRCLSProcess *process,
                                       thread_t thread,
                                       char *buffer,
                                       size_t length);
static const char *FIRCLSProcessGetThreadDispatchQueueName(FIRCLSProcess *process, thread_t thread);

#pragma mark - API
bool FIRCLSProcessInit(FIRCLSProcess *process, thread_t crashedThread, void *uapVoid) {
  if (!process) {
    return false;
  }

  process->task = mach_task_self();
  process->thisThread = mach_thread_self();
  process->crashedThread = crashedThread;
  process->uapVoid = uapVoid;

  if (task_threads(process->task, &process->threads, &process->threadCount) != KERN_SUCCESS) {
    // failed to get all threads
    process->threadCount = 0;
    FIRCLSSDKLog("Error: unable to get task threads\n");

    return false;
  }

  return true;
}

bool FIRCLSProcessDestroy(FIRCLSProcess *process) {
  return false;
}

// https://developer.apple.com/library/mac/#qa/qa2004/qa1361.html
bool FIRCLSProcessDebuggerAttached(void) {
  int junk;
  int mib[4];
  struct kinfo_proc info;
  size_t size;

  // Initialize the flags so that, if sysctl fails for some bizarre
  // reason, we get a predictable result.
  info.kp_proc.p_flag = 0;

  // Initialize mib, which tells sysctl the info we want, in this case
  // we're looking for information about a specific process ID.
  mib[0] = CTL_KERN;
  mib[1] = KERN_PROC;
  mib[2] = KERN_PROC_PID;
  mib[3] = getpid();

  // Call sysctl.
  size = sizeof(info);
  junk = sysctl(mib, sizeof(mib) / sizeof(*mib), &info, &size, NULL, 0);
  if (junk != 0) {
    FIRCLSSDKLog("sysctl failed while trying to get kinfo_proc\n");
    return false;
  }

  // We're being debugged if the P_TRACED flag is set.
  return (info.kp_proc.p_flag & P_TRACED) != 0;
}

#pragma mark - Thread Support
static bool FIRCLSProcessIsCurrentThread(FIRCLSProcess *process, thread_t thread) {
  return MACH_PORT_INDEX(process->thisThread) == MACH_PORT_INDEX(thread);
}

static bool FIRCLSProcessIsCrashedThread(FIRCLSProcess *process, thread_t thread) {
  return MACH_PORT_INDEX(process->crashedThread) == MACH_PORT_INDEX(thread);
}

static uint32_t FIRCLSProcessGetThreadCount(FIRCLSProcess *process) {
  return process->threadCount;
}

static thread_t FIRCLSProcessGetThread(FIRCLSProcess *process, uint32_t index) {
  if (index >= process->threadCount) {
    return MACH_PORT_NULL;
  }

  return process->threads[index];
}

bool FIRCLSProcessSuspendAllOtherThreads(FIRCLSProcess *process) {
  mach_msg_type_number_t i;
  bool success;

  success = true;
  for (i = 0; i < process->threadCount; ++i) {
    thread_t thread;

    thread = FIRCLSProcessGetThread(process, i);

    if (FIRCLSProcessIsCurrentThread(process, thread)) {
      continue;
    }

    // FIXME: workaround to get this building on watch, but we need to suspend/resume threads!
#if CLS_CAN_SUSPEND_THREADS
    success = success && (thread_suspend(thread) == KERN_SUCCESS);
#endif
  }

  return success;
}

bool FIRCLSProcessResumeAllOtherThreads(FIRCLSProcess *process) {
  mach_msg_type_number_t i;
  bool success;

  success = true;
  for (i = 0; i < process->threadCount; ++i) {
    thread_t thread;

    thread = FIRCLSProcessGetThread(process, i);

    if (FIRCLSProcessIsCurrentThread(process, thread)) {
      continue;
    }

    // FIXME: workaround to get this building on watch, but we need to suspend/resume threads!
#if CLS_CAN_SUSPEND_THREADS
    success = success && (thread_resume(thread) == KERN_SUCCESS);
#endif
  }

  return success;
}

#pragma mark - Thread Properties
void *FIRCLSThreadGetCurrentPC(void) {
  return __builtin_return_address(0);
}

static bool FIRCLSProcessGetThreadState(FIRCLSProcess *process,
                                        thread_t thread,
                                        FIRCLSThreadContext *context) {
  if (!FIRCLSIsValidPointer(context)) {
    FIRCLSSDKLogError("invalid context supplied");
    return false;
  }

  // If the thread context we should use is non-NULL, then just assign it here.  Otherwise,
  // query the thread state
  if (FIRCLSProcessIsCrashedThread(process, thread) && FIRCLSIsValidPointer(process->uapVoid)) {
    *context = *((_STRUCT_UCONTEXT *)process->uapVoid)->uc_mcontext;
    return true;
  }

  // Here's a wild trick: emulate what thread_get_state would do. It apppears that
  // we cannot reliably unwind out of thread_get_state. So, instead of trying, setup
  // a thread context that resembles what the real thing would look like
  if (FIRCLSProcessIsCurrentThread(process, thread)) {
    FIRCLSSDKLog("Faking current thread\n");
    memset(context, 0, sizeof(FIRCLSThreadContext));

    // Compute the frame address, and then base the stack value off of that. A frame pushes
    // two pointers onto the stack, so we have to offset.
    const uintptr_t frameAddress = (uintptr_t)__builtin_frame_address(0);
    const uintptr_t stackAddress = FIRCLSUnwindStackPointerFromFramePointer(frameAddress);

#if CLS_CPU_X86_64
    context->__ss.__rip = (uintptr_t)FIRCLSThreadGetCurrentPC();
    context->__ss.__rbp = frameAddress;
    context->__ss.__rsp = stackAddress;
#elif CLS_CPU_I386
    context->__ss.__eip = (uintptr_t)FIRCLSThreadGetCurrentPC();
    context->__ss.__ebp = frameAddress;
    context->__ss.__esp = stackAddress;
#elif CLS_CPU_ARM64
    FIRCLSThreadContextSetPC(context, (uintptr_t)FIRCLSThreadGetCurrentPC());
    FIRCLSThreadContextSetFramePointer(context, frameAddress);
    FIRCLSThreadContextSetLinkRegister(context, (uintptr_t)__builtin_return_address(0));
    FIRCLSThreadContextSetStackPointer(context, stackAddress);
#elif CLS_CPU_ARM
    context->__ss.__pc = (uintptr_t)FIRCLSThreadGetCurrentPC();
    context->__ss.__r[7] = frameAddress;
    context->__ss.__lr = (uintptr_t)__builtin_return_address(0);
    context->__ss.__sp = stackAddress;
#endif

    return true;
  }

#if !TARGET_OS_WATCH
  // try to get the value by querying the thread state
  mach_msg_type_number_t stateCount = FIRCLSThreadStateCount;
  if (thread_get_state(thread, FIRCLSThreadState, (thread_state_t)(&(context->__ss)),
                       &stateCount) != KERN_SUCCESS) {
    FIRCLSSDKLogError("failed to get thread state\n");
    return false;
  }

  return true;
#else
  return false;
#endif
}

static bool FIRCLSProcessGetThreadName(FIRCLSProcess *process,
                                       thread_t thread,
                                       char *buffer,
                                       size_t length) {
  pthread_t pthread;

  if (!buffer || length <= 0) {
    return false;
  }

  pthread = pthread_from_mach_thread_np(thread);

  return pthread_getname_np(pthread, buffer, length) == 0;
}

static const char *FIRCLSProcessGetThreadDispatchQueueName(FIRCLSProcess *process,
                                                           thread_t thread) {
  thread_identifier_info_data_t info;
  mach_msg_type_number_t infoCount;
  dispatch_queue_t *queueAddress;
  dispatch_queue_t queue;
  const char *string;

  infoCount = THREAD_IDENTIFIER_INFO_COUNT;
  if (thread_info(thread, THREAD_IDENTIFIER_INFO, (thread_info_t)&info, &infoCount) !=
      KERN_SUCCESS) {
    FIRCLSSDKLog("unable to get thread info\n");
    return NULL;
  }

  queueAddress = (dispatch_queue_t *)info.dispatch_qaddr;
  if (queueAddress == NULL) {
    return "";
  }

  // Sometimes a queue address is invalid.  I cannot explain why this is, but
  // it can cause a crash.
  if (!FIRCLSReadMemory((vm_address_t)queueAddress, &queue, sizeof(void *))) {
    return "";
  }

  // here, we know it is safe to de-reference this address, so attempt to get the queue name
  if (!queue) {
    return "";
  }

  string = dispatch_queue_get_label(queue);

  // but, we still don't if the entire string is valid, so check that too
  if (!FIRCLSReadString((vm_address_t)string, (char **)&string, 128)) {
    return "";
  }

  return string;
}

#pragma mark - Data Recording
static bool FIRCLSProcessRecordThreadRegisters(FIRCLSThreadContext context, FIRCLSFile *file) {
#if CLS_CPU_ARM
  FIRCLSFileWriteHashEntryUint64(file, "r0", context.__ss.__r[0]);
  FIRCLSFileWriteHashEntryUint64(file, "r1", context.__ss.__r[1]);
  FIRCLSFileWriteHashEntryUint64(file, "r2", context.__ss.__r[2]);
  FIRCLSFileWriteHashEntryUint64(file, "r3", context.__ss.__r[3]);
  FIRCLSFileWriteHashEntryUint64(file, "r4", context.__ss.__r[4]);
  FIRCLSFileWriteHashEntryUint64(file, "r5", context.__ss.__r[5]);
  FIRCLSFileWriteHashEntryUint64(file, "r6", context.__ss.__r[6]);
  FIRCLSFileWriteHashEntryUint64(file, "r7", context.__ss.__r[7]);
  FIRCLSFileWriteHashEntryUint64(file, "r8", context.__ss.__r[8]);
  FIRCLSFileWriteHashEntryUint64(file, "r9", context.__ss.__r[9]);
  FIRCLSFileWriteHashEntryUint64(file, "r10", context.__ss.__r[10]);
  FIRCLSFileWriteHashEntryUint64(file, "r11", context.__ss.__r[11]);
  FIRCLSFileWriteHashEntryUint64(file, "ip", context.__ss.__r[12]);
  FIRCLSFileWriteHashEntryUint64(file, "sp", context.__ss.__sp);
  FIRCLSFileWriteHashEntryUint64(file, "lr", context.__ss.__lr);
  FIRCLSFileWriteHashEntryUint64(file, "pc", context.__ss.__pc);
  FIRCLSFileWriteHashEntryUint64(file, "cpsr", context.__ss.__cpsr);
#elif CLS_CPU_ARM64
  FIRCLSFileWriteHashEntryUint64(file, "x0", context.__ss.__x[0]);
  FIRCLSFileWriteHashEntryUint64(file, "x1", context.__ss.__x[1]);
  FIRCLSFileWriteHashEntryUint64(file, "x2", context.__ss.__x[2]);
  FIRCLSFileWriteHashEntryUint64(file, "x3", context.__ss.__x[3]);
  FIRCLSFileWriteHashEntryUint64(file, "x4", context.__ss.__x[4]);
  FIRCLSFileWriteHashEntryUint64(file, "x5", context.__ss.__x[5]);
  FIRCLSFileWriteHashEntryUint64(file, "x6", context.__ss.__x[6]);
  FIRCLSFileWriteHashEntryUint64(file, "x7", context.__ss.__x[7]);
  FIRCLSFileWriteHashEntryUint64(file, "x8", context.__ss.__x[8]);
  FIRCLSFileWriteHashEntryUint64(file, "x9", context.__ss.__x[9]);
  FIRCLSFileWriteHashEntryUint64(file, "x10", context.__ss.__x[10]);
  FIRCLSFileWriteHashEntryUint64(file, "x11", context.__ss.__x[11]);
  FIRCLSFileWriteHashEntryUint64(file, "x12", context.__ss.__x[12]);
  FIRCLSFileWriteHashEntryUint64(file, "x13", context.__ss.__x[13]);
  FIRCLSFileWriteHashEntryUint64(file, "x14", context.__ss.__x[14]);
  FIRCLSFileWriteHashEntryUint64(file, "x15", context.__ss.__x[15]);
  FIRCLSFileWriteHashEntryUint64(file, "x16", context.__ss.__x[16]);
  FIRCLSFileWriteHashEntryUint64(file, "x17", context.__ss.__x[17]);
  FIRCLSFileWriteHashEntryUint64(file, "x18", context.__ss.__x[18]);
  FIRCLSFileWriteHashEntryUint64(file, "x19", context.__ss.__x[19]);
  FIRCLSFileWriteHashEntryUint64(file, "x20", context.__ss.__x[20]);
  FIRCLSFileWriteHashEntryUint64(file, "x21", context.__ss.__x[21]);
  FIRCLSFileWriteHashEntryUint64(file, "x22", context.__ss.__x[22]);
  FIRCLSFileWriteHashEntryUint64(file, "x23", context.__ss.__x[23]);
  FIRCLSFileWriteHashEntryUint64(file, "x24", context.__ss.__x[24]);
  FIRCLSFileWriteHashEntryUint64(file, "x25", context.__ss.__x[25]);
  FIRCLSFileWriteHashEntryUint64(file, "x26", context.__ss.__x[26]);
  FIRCLSFileWriteHashEntryUint64(file, "x27", context.__ss.__x[27]);
  FIRCLSFileWriteHashEntryUint64(file, "x28", context.__ss.__x[28]);
  FIRCLSFileWriteHashEntryUint64(file, "fp", FIRCLSThreadContextGetFramePointer(&context));
  FIRCLSFileWriteHashEntryUint64(file, "sp", FIRCLSThreadContextGetStackPointer(&context));
  FIRCLSFileWriteHashEntryUint64(file, "lr", FIRCLSThreadContextGetLinkRegister(&context));
  FIRCLSFileWriteHashEntryUint64(file, "pc", FIRCLSThreadContextGetPC(&context));
  FIRCLSFileWriteHashEntryUint64(file, "cpsr", context.__ss.__cpsr);
#elif CLS_CPU_I386
  FIRCLSFileWriteHashEntryUint64(file, "eax", context.__ss.__eax);
  FIRCLSFileWriteHashEntryUint64(file, "ebx", context.__ss.__ebx);
  FIRCLSFileWriteHashEntryUint64(file, "ecx", context.__ss.__ecx);
  FIRCLSFileWriteHashEntryUint64(file, "edx", context.__ss.__edx);
  FIRCLSFileWriteHashEntryUint64(file, "edi", context.__ss.__edi);
  FIRCLSFileWriteHashEntryUint64(file, "esi", context.__ss.__esi);
  FIRCLSFileWriteHashEntryUint64(file, "ebp", context.__ss.__ebp);
  FIRCLSFileWriteHashEntryUint64(file, "esp", context.__ss.__esp);
  FIRCLSFileWriteHashEntryUint64(file, "ss", context.__ss.__ss);
  FIRCLSFileWriteHashEntryUint64(file, "eflags", context.__ss.__eflags);
  FIRCLSFileWriteHashEntryUint64(file, "eip", context.__ss.__eip);
  FIRCLSFileWriteHashEntryUint64(file, "cs", context.__ss.__cs);
  FIRCLSFileWriteHashEntryUint64(file, "ds", context.__ss.__ds);
  FIRCLSFileWriteHashEntryUint64(file, "es", context.__ss.__es);
  FIRCLSFileWriteHashEntryUint64(file, "fs", context.__ss.__fs);
  FIRCLSFileWriteHashEntryUint64(file, "gs", context.__ss.__gs);

  // how do we get the cr2 register?
#elif CLS_CPU_X86_64
  FIRCLSFileWriteHashEntryUint64(file, "rax", context.__ss.__rax);
  FIRCLSFileWriteHashEntryUint64(file, "rbx", context.__ss.__rbx);
  FIRCLSFileWriteHashEntryUint64(file, "rcx", context.__ss.__rcx);
  FIRCLSFileWriteHashEntryUint64(file, "rdx", context.__ss.__rdx);
  FIRCLSFileWriteHashEntryUint64(file, "rdi", context.__ss.__rdi);
  FIRCLSFileWriteHashEntryUint64(file, "rsi", context.__ss.__rsi);
  FIRCLSFileWriteHashEntryUint64(file, "rbp", context.__ss.__rbp);
  FIRCLSFileWriteHashEntryUint64(file, "rsp", context.__ss.__rsp);
  FIRCLSFileWriteHashEntryUint64(file, "r8", context.__ss.__r8);
  FIRCLSFileWriteHashEntryUint64(file, "r9", context.__ss.__r9);
  FIRCLSFileWriteHashEntryUint64(file, "r10", context.__ss.__r10);
  FIRCLSFileWriteHashEntryUint64(file, "r11", context.__ss.__r11);
  FIRCLSFileWriteHashEntryUint64(file, "r12", context.__ss.__r12);
  FIRCLSFileWriteHashEntryUint64(file, "r13", context.__ss.__r13);
  FIRCLSFileWriteHashEntryUint64(file, "r14", context.__ss.__r14);
  FIRCLSFileWriteHashEntryUint64(file, "r15", context.__ss.__r15);
  FIRCLSFileWriteHashEntryUint64(file, "rip", context.__ss.__rip);
  FIRCLSFileWriteHashEntryUint64(file, "rflags", context.__ss.__rflags);
  FIRCLSFileWriteHashEntryUint64(file, "cs", context.__ss.__cs);
  FIRCLSFileWriteHashEntryUint64(file, "fs", context.__ss.__fs);
  FIRCLSFileWriteHashEntryUint64(file, "gs", context.__ss.__gs);
#endif

  return true;
}

static bool FIRCLSProcessRecordThread(FIRCLSProcess *process, thread_t thread, FIRCLSFile *file) {
  FIRCLSUnwindContext unwindContext;
  FIRCLSThreadContext context;

  if (!FIRCLSProcessGetThreadState(process, thread, &context)) {
    FIRCLSSDKLogError("unable to get thread state");
    return false;
  }

  if (!FIRCLSUnwindInit(&unwindContext, context)) {
    FIRCLSSDKLog("unable to init unwind context\n");

    return false;
  }

  FIRCLSFileWriteHashStart(file);

  // registers
  FIRCLSFileWriteHashKey(file, "registers");
  FIRCLSFileWriteHashStart(file);

  FIRCLSProcessRecordThreadRegisters(context, file);

  FIRCLSFileWriteHashEnd(file);

  // stacktrace
  FIRCLSFileWriteHashKey(file, "stacktrace");

  // stacktrace is an array of integers
  FIRCLSFileWriteArrayStart(file);

  uint32_t repeatedPCCount = 0;
  uint64_t repeatedPC = 0;
  const FIRCLSInternalLogLevel level = _firclsContext.writable->internalLogging.logLevel;

  while (FIRCLSUnwindNextFrame(&unwindContext)) {
    const uintptr_t pc = FIRCLSUnwindGetPC(&unwindContext);
    const uint32_t frameCount = FIRCLSUnwindGetFrameRepeatCount(&unwindContext);

    if (repeatedPC == pc && repeatedPC != 0) {
      // actively counting a recursion
      repeatedPCCount = frameCount;
      continue;
    }

    if (frameCount >= FIRCLSUnwindInfiniteRecursionCountThreshold && repeatedPC == 0) {
      repeatedPC = pc;
      FIRCLSSDKLogWarn("Possible infinite recursion - suppressing logging\n");
      _firclsContext.writable->internalLogging.logLevel = FIRCLSInternalLogLevelWarn;
      continue;
    }

    if (repeatedPC != 0) {
      // at this point, we've recorded a repeated PC, but it is now no longer
      // repeating, so we can restore the logging
      _firclsContext.writable->internalLogging.logLevel = level;
    }

    FIRCLSFileWriteArrayEntryUint64(file, pc);
  }

  FIRCLSFileWriteArrayEnd(file);

  // crashed?
  if (FIRCLSProcessIsCrashedThread(process, thread)) {
    FIRCLSFileWriteHashEntryBoolean(file, "crashed", true);
  }

  if (repeatedPC != 0) {
    FIRCLSFileWriteHashEntryUint64(file, "repeated_pc", repeatedPC);
    FIRCLSFileWriteHashEntryUint64(file, "repeat_count", repeatedPCCount);
  }

  // Just for extra safety, restore the logging level again. The logic
  // above is fairly tricky, this is cheap, and no logging is a real pain.
  _firclsContext.writable->internalLogging.logLevel = level;

  // end thread info
  FIRCLSFileWriteHashEnd(file);

  return true;
}

bool FIRCLSProcessRecordAllThreads(FIRCLSProcess *process, FIRCLSFile *file) {
  uint32_t threadCount;
  uint32_t i;

  threadCount = FIRCLSProcessGetThreadCount(process);

  FIRCLSFileWriteSectionStart(file, "threads");

  FIRCLSFileWriteArrayStart(file);

  for (i = 0; i < threadCount; ++i) {
    thread_t thread;

    thread = FIRCLSProcessGetThread(process, i);

    FIRCLSSDKLogInfo("recording thread %d data\n", i);
    if (!FIRCLSProcessRecordThread(process, thread, file)) {
      return false;
    }
  }

  FIRCLSFileWriteArrayEnd(file);

  FIRCLSFileWriteSectionEnd(file);

  FIRCLSSDKLogInfo("completed recording all thread data\n");

  return true;
}

void FIRCLSProcessRecordThreadNames(FIRCLSProcess *process, FIRCLSFile *file) {
  uint32_t threadCount;
  uint32_t i;

  FIRCLSFileWriteSectionStart(file, "thread_names");

  FIRCLSFileWriteArrayStart(file);

  threadCount = FIRCLSProcessGetThreadCount(process);
  for (i = 0; i < threadCount; ++i) {
    thread_t thread;
    char name[THREAD_NAME_BUFFER_SIZE];

    thread = FIRCLSProcessGetThread(process, i);

    name[0] = 0;  // null-terminate, just in case nothing is written

    FIRCLSProcessGetThreadName(process, thread, name, THREAD_NAME_BUFFER_SIZE);

    FIRCLSFileWriteArrayEntryString(file, name);
  }

  FIRCLSFileWriteArrayEnd(file);
  FIRCLSFileWriteSectionEnd(file);
}

void FIRCLSProcessRecordDispatchQueueNames(FIRCLSProcess *process, FIRCLSFile *file) {
  uint32_t threadCount;
  uint32_t i;

  FIRCLSFileWriteSectionStart(file, "dispatch_queue_names");

  FIRCLSFileWriteArrayStart(file);

  threadCount = FIRCLSProcessGetThreadCount(process);
  for (i = 0; i < threadCount; ++i) {
    thread_t thread;
    const char *name;

    thread = FIRCLSProcessGetThread(process, i);

    name = FIRCLSProcessGetThreadDispatchQueueName(process, thread);

    FIRCLSFileWriteArrayEntryString(file, name);
  }

  FIRCLSFileWriteArrayEnd(file);
  FIRCLSFileWriteSectionEnd(file);
}

#pragma mark - Othe Process Info
bool FIRCLSProcessGetMemoryUsage(uint64_t *active,
                                 uint64_t *inactive,
                                 uint64_t *wired,
                                 uint64_t *freeMem) {
  mach_port_t hostPort;
  mach_msg_type_number_t hostSize;
  vm_size_t pageSize;
  vm_statistics_data_t vmStat;

  hostPort = mach_host_self();

  hostSize = sizeof(vm_statistics_data_t) / sizeof(integer_t);

  pageSize = _firclsContext.readonly->host.pageSize;

  if (host_statistics(hostPort, HOST_VM_INFO, (host_info_t)&vmStat, &hostSize) != KERN_SUCCESS) {
    FIRCLSSDKLog("Failed to get vm statistics\n");
    return false;
  }

  if (!(active && inactive && wired && freeMem)) {
    FIRCLSSDKLog("Invalid pointers\n");
    return false;
  }

  // compute the sizes in bytes and return the values
  *active = vmStat.active_count * pageSize;
  *inactive = vmStat.inactive_count * pageSize;
  *wired = vmStat.wire_count * pageSize;
  *freeMem = vmStat.free_count * pageSize;

  return true;
}

bool FIRCLSProcessGetInfo(FIRCLSProcess *process,
                          uint64_t *virtualSize,
                          uint64_t *residentSize,
                          time_value_t *userTime,
                          time_value_t *systemTime) {
  struct task_basic_info_64 taskInfo;
  mach_msg_type_number_t count;

  count = TASK_BASIC_INFO_64_COUNT;
  if (task_info(process->task, TASK_BASIC_INFO_64, (task_info_t)&taskInfo, &count) !=
      KERN_SUCCESS) {
    FIRCLSSDKLog("Failed to get task info\n");
    return false;
  }

  if (!(virtualSize && residentSize && userTime && systemTime)) {
    FIRCLSSDKLog("Invalid pointers\n");
    return false;
  }

  *virtualSize = taskInfo.virtual_size;
  *residentSize = taskInfo.resident_size;
  *userTime = taskInfo.user_time;
  *systemTime = taskInfo.system_time;

  return true;
}

void FIRCLSProcessRecordStats(FIRCLSProcess *process, FIRCLSFile *file) {
  uint64_t active;
  uint64_t inactive;
  uint64_t virtualSize;
  uint64_t residentSize;
  uint64_t wired;
  uint64_t freeMem;
  time_value_t userTime;
  time_value_t systemTime;

  if (!FIRCLSProcessGetMemoryUsage(&active, &inactive, &wired, &freeMem)) {
    FIRCLSSDKLog("Unable to get process memory usage\n");
    return;
  }

  if (!FIRCLSProcessGetInfo(process, &virtualSize, &residentSize, &userTime, &systemTime)) {
    FIRCLSSDKLog("Unable to get process stats\n");
    return;
  }

  FIRCLSFileWriteSectionStart(file, "process_stats");

  FIRCLSFileWriteHashStart(file);

  FIRCLSFileWriteHashEntryUint64(file, "active", active);
  FIRCLSFileWriteHashEntryUint64(file, "inactive", inactive);
  FIRCLSFileWriteHashEntryUint64(file, "wired", wired);
  FIRCLSFileWriteHashEntryUint64(file, "freeMem", freeMem);  // Intentionally left in, for now. Arg.
  FIRCLSFileWriteHashEntryUint64(file, "free_mem", freeMem);
  FIRCLSFileWriteHashEntryUint64(file, "virtual", virtualSize);
  FIRCLSFileWriteHashEntryUint64(file, "resident", active);
  FIRCLSFileWriteHashEntryUint64(file, "user_time",
                                 (userTime.seconds * 1000 * 1000) + userTime.microseconds);
  FIRCLSFileWriteHashEntryUint64(file, "sys_time",
                                 (systemTime.seconds * 1000 * 1000) + systemTime.microseconds);

  FIRCLSFileWriteHashEnd(file);

  FIRCLSFileWriteSectionEnd(file);
}

#pragma mark - Runtime Info
#define OBJC_MSG_SEND_START ((vm_address_t)objc_msgSend)
#define OBJC_MSG_SEND_SUPER_START ((vm_address_t)objc_msgSendSuper)
#define OBJC_MSG_SEND_END (OBJC_MSG_SEND_START + 66)
#define OBJC_MSG_SEND_SUPER_END (OBJC_MSG_SEND_SUPER_START + 66)

#if !CLS_CPU_ARM64
#define OBJC_MSG_SEND_STRET_START ((vm_address_t)objc_msgSend_stret)
#define OBJC_MSG_SEND_SUPER_STRET_START ((vm_address_t)objc_msgSendSuper_stret)
#define OBJC_MSG_SEND_STRET_END (OBJC_MSG_SEND_STRET_START + 66)
#define OBJC_MSG_SEND_SUPER_STRET_END (OBJC_MSG_SEND_SUPER_STRET_START + 66)
#endif

#if CLS_CPU_X86
#define OBJC_MSG_SEND_FPRET_START ((vm_address_t)objc_msgSend_fpret)
#define OBJC_MSG_SEND_FPRET_END (OBJC_MSG_SEND_FPRET_START + 66)
#endif

static const char *FIRCLSProcessGetObjCSelectorName(FIRCLSThreadContext registers) {
  void *selectorAddress;
  void *selRegister;
#if !CLS_CPU_ARM64
  void *stretSelRegister;
#endif
  vm_address_t pc;

  // First, did we crash in objc_msgSend?  The two ways I can think
  // of doing this are to use dladdr, and then comparing the strings to
  // objc_msg*, or looking up the symbols, and guessing if we are "close enough".

  selectorAddress = NULL;

#if CLS_CPU_ARM
  pc = registers.__ss.__pc;
  selRegister = (void *)registers.__ss.__r[1];
  stretSelRegister = (void *)registers.__ss.__r[2];
#elif CLS_CPU_ARM64
  pc = FIRCLSThreadContextGetPC(&registers);
  selRegister = (void *)registers.__ss.__x[1];
#elif CLS_CPU_I386
  pc = registers.__ss.__eip;
  selRegister = (void *)registers.__ss.__ecx;
  stretSelRegister = (void *)registers.__ss.__ecx;
#elif CLS_CPU_X86_64
  pc = registers.__ss.__rip;
  selRegister = (void *)registers.__ss.__rsi;
  stretSelRegister = (void *)registers.__ss.__rdx;
#endif

  if ((pc >= OBJC_MSG_SEND_START) && (pc <= OBJC_MSG_SEND_END)) {
    selectorAddress = selRegister;
  }

#if !CLS_CPU_ARM64
  if ((pc >= OBJC_MSG_SEND_SUPER_START) && (pc <= OBJC_MSG_SEND_SUPER_END)) {
    selectorAddress = selRegister;
  }

  if ((pc >= OBJC_MSG_SEND_STRET_START) && (pc <= OBJC_MSG_SEND_STRET_END)) {
    selectorAddress = stretSelRegister;
  }

  if ((pc >= OBJC_MSG_SEND_SUPER_STRET_START) && (pc <= OBJC_MSG_SEND_SUPER_STRET_END)) {
    selectorAddress = stretSelRegister;
  }

#if CLS_CPU_X86
  if ((pc >= OBJC_MSG_SEND_FPRET_START) && (pc <= OBJC_MSG_SEND_FPRET_END)) {
    selectorAddress = selRegister;
  }
#endif
#endif

  if (!selectorAddress) {
    return "";
  }

  if (!FIRCLSReadString((vm_address_t)selectorAddress, (char **)&selectorAddress, 128)) {
    FIRCLSSDKLog("Unable to read the selector string\n");
    return "";
  }

  return selectorAddress;
}

#define CRASH_ALIGN __attribute__((aligned(8)))
typedef struct {
  unsigned version CRASH_ALIGN;
  const char *message CRASH_ALIGN;
  const char *signature CRASH_ALIGN;
  const char *backtrace CRASH_ALIGN;
  const char *message2 CRASH_ALIGN;
  void *reserved CRASH_ALIGN;
  void *reserved2 CRASH_ALIGN;
} crash_info_t;

static void FIRCLSProcessRecordCrashInfo(FIRCLSFile *file) {
  // TODO: this should be abstracted into binary images, if possible
  FIRCLSBinaryImageRuntimeNode *nodes = _firclsContext.writable->binaryImage.nodes;
  if (!nodes) {
    FIRCLSSDKLogError("The node structure is NULL\n");
    return;
  }

  for (uint32_t i = 0; i < CLS_BINARY_IMAGE_RUNTIME_NODE_COUNT; ++i) {
    FIRCLSBinaryImageRuntimeNode *node = &nodes[i];

    if (!node->crashInfo) {
      continue;
    }

    crash_info_t info;

    if (!FIRCLSReadMemory((vm_address_t)node->crashInfo, &info, sizeof(crash_info_t))) {
      continue;
    }

    FIRCLSSDKLogDebug("Found crash info with version %d\n", info.version);

    // Currently support versions 0 through 5.
    // 4 was in use for a long time, but it appears that with iOS 9 / swift 2.0, the verison has
    // been bumped.
    if (info.version > 5) {
      continue;
    }

    if (!info.message) {
      continue;
    }

#if CLS_BINARY_IMAGE_RUNTIME_NODE_RECORD_NAME
    FIRCLSSDKLogInfo("Found crash info for %s\n", node->name);
#endif

    FIRCLSSDKLogDebug("attempting to read crash info string\n");

    char *string = NULL;

    if (!FIRCLSReadString((vm_address_t)info.message, &string, 256)) {
      FIRCLSSDKLogError("Failed to copy crash info string\n");
      continue;
    }

    FIRCLSFileWriteArrayEntryHexEncodedString(file, string);
  }
}

void FIRCLSProcessRecordRuntimeInfo(FIRCLSProcess *process, FIRCLSFile *file) {
  FIRCLSThreadContext mcontext;

  if (!FIRCLSProcessGetThreadState(process, process->crashedThread, &mcontext)) {
    FIRCLSSDKLogError("unable to get crashed thread state");
  }

  FIRCLSFileWriteSectionStart(file, "runtime");

  FIRCLSFileWriteHashStart(file);

  FIRCLSFileWriteHashEntryString(file, "objc_selector", FIRCLSProcessGetObjCSelectorName(mcontext));

  FIRCLSFileWriteHashKey(file, "crash_info_entries");

  FIRCLSFileWriteArrayStart(file);
  FIRCLSProcessRecordCrashInfo(file);
  FIRCLSFileWriteArrayEnd(file);

  FIRCLSFileWriteHashEnd(file);

  FIRCLSFileWriteSectionEnd(file);
}
