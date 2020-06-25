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

#include "FIRCLSSignal.h"
#include "FIRCLSGlobals.h"
#include "FIRCLSHandler.h"
#include "FIRCLSUtility.h"

#include <dlfcn.h>
#include <stdlib.h>

static const int FIRCLSFatalSignals[FIRCLSSignalCount] = {SIGABRT, SIGBUS, SIGFPE, SIGILL,
                                                          SIGSEGV, SIGSYS, SIGTRAP};

#if CLS_USE_SIGALTSTACK
static void FIRCLSSignalInstallAltStack(FIRCLSSignalReadContext *roContext);
#endif
static void FIRCLSSignalInstallHandlers(FIRCLSSignalReadContext *roContext);
static void FIRCLSSignalHandler(int signal, siginfo_t *info, void *uapVoid);

void FIRCLSSignalInitialize(FIRCLSSignalReadContext *roContext) {
  if (!FIRCLSUnlinkIfExists(roContext->path)) {
    FIRCLSSDKLog("Unable to reset the signal log file %s\n", strerror(errno));
  }

#if CLS_USE_SIGALTSTACK
  FIRCLSSignalInstallAltStack(roContext);
#endif
  FIRCLSSignalInstallHandlers(roContext);
#if TARGET_IPHONE_SIMULATOR
  // prevent the OpenGL stack (by way of OpenGLES.framework/libLLVMContainer.dylib) from installing
  // signal handlers that do not chain back
  // TODO: I don't believe this is necessary as of recent iOS releases
  bool *ptr = dlsym(RTLD_DEFAULT, "_ZN4llvm23DisablePrettyStackTraceE");
  if (ptr) {
    *ptr = true;
  }
#endif
}

void FIRCLSSignalEnumerateHandledSignals(void (^block)(int idx, int signal)) {
  for (int i = 0; i < FIRCLSSignalCount; ++i) {
    block(i, FIRCLSFatalSignals[i]);
  }
}

#if CLS_USE_SIGALTSTACK

static void FIRCLSSignalInstallAltStack(FIRCLSSignalReadContext *roContext) {
  stack_t signalStack;
  stack_t originalStack;

  signalStack.ss_sp = _firclsContext.readonly->signalStack;
  signalStack.ss_size = CLS_SIGNAL_HANDLER_STACK_SIZE;
  signalStack.ss_flags = 0;

  if (sigaltstack(&signalStack, &originalStack) != 0) {
    FIRCLSSDKLog("Unable to setup stack %s\n", strerror(errno));

    return;
  }

  roContext->originalStack.ss_sp = NULL;
  roContext->originalStack = originalStack;
}

#endif

static void FIRCLSSignalInstallHandlers(FIRCLSSignalReadContext *roContext) {
  FIRCLSSignalEnumerateHandledSignals(^(int idx, int signal) {
    struct sigaction action;
    struct sigaction previousAction;

    action.sa_sigaction = FIRCLSSignalHandler;
    // SA_RESETHAND seems like it would be great, but it doesn't appear to
    // work correctly.  After taking a signal, causing another identical signal in
    // the handler will *not* cause the default handler to be invokved (which should
    // terminate the process).  I've found some evidence that others have seen this
    // behavior on MAC OS X.
    action.sa_flags = SA_SIGINFO | SA_ONSTACK;

    sigemptyset(&action.sa_mask);

    previousAction.sa_sigaction = NULL;
    if (sigaction(signal, &action, &previousAction) != 0) {
      FIRCLSSDKLog("Unable to install handler for %d (%s)\n", signal, strerror(errno));
    }

    // store the last action, so it can be recalled
    roContext->originalActions[idx].sa_sigaction = NULL;

    if (previousAction.sa_sigaction) {
      roContext->originalActions[idx] = previousAction;
    }
  });
}

void FIRCLSSignalCheckHandlers(void) {
  if (_firclsContext.readonly->debuggerAttached) {
    return;
  }

  FIRCLSSignalEnumerateHandledSignals(^(int idx, int signal) {
    struct sigaction previousAction;
    Dl_info info;
    void *ptr;

    if (sigaction(signal, 0, &previousAction) != 0) {
      fprintf(stderr, "Unable to read signal handler\n");
      return;
    }

    ptr = previousAction.__sigaction_u.__sa_handler;
    const char *signalName = NULL;
    const char *codeName = NULL;

    FIRCLSSignalNameLookup(signal, 0, &signalName, &codeName);

    if (ptr == FIRCLSSignalHandler) {
      return;
    }

    const char *name = NULL;
    if (dladdr(ptr, &info) != 0) {
      name = info.dli_sname;
    }

    fprintf(stderr,
            "[Crashlytics] The signal %s has a non-Crashlytics handler (%s).  This will interfere "
            "with reporting.\n",
            signalName, name);
  });
}

void FIRCLSSignalSafeRemoveHandlers(bool includingAbort) {
  for (int i = 0; i < FIRCLSSignalCount; ++i) {
    struct sigaction sa;

    if (!includingAbort && (FIRCLSFatalSignals[i] == SIGABRT)) {
      continue;
    }

    sa.sa_handler = SIG_DFL;
    sigemptyset(&sa.sa_mask);

    if (sigaction(FIRCLSFatalSignals[i], &sa, NULL) != 0)
      FIRCLSSDKLog("Unable to set default handler for %d (%s)\n", i, strerror(errno));
  }
}

bool FIRCLSSignalSafeInstallPreexistingHandlers(FIRCLSSignalReadContext *roContext) {
  bool success;

  FIRCLSSignalSafeRemoveHandlers(true);

#if CLS_USE_SIGALTSTACK

  // re-install the original stack, if needed
  if (roContext->originalStack.ss_sp) {
    if (sigaltstack(&roContext->originalStack, 0) != 0) {
      FIRCLSSDKLog("Unable to setup stack %s\n", strerror(errno));

      return false;
    }
  }

#endif

  // re-install the original handlers, if any
  success = true;
  for (int i = 0; i < FIRCLSSignalCount; ++i) {
    if (roContext->originalActions[i].sa_sigaction == NULL) {
      continue;
    }

    if (sigaction(FIRCLSFatalSignals[i], &roContext->originalActions[i], 0) != 0) {
      FIRCLSSDKLog("Unable to install handler for %d (%s)\n", i, strerror(errno));
      success = false;
    }
  }

  return success;
}

void FIRCLSSignalNameLookup(int number, int code, const char **name, const char **codeName) {
  if (!name || !codeName) {
    return;
  }

  *codeName = NULL;

  switch (number) {
    case SIGABRT:
      *name = "SIGABRT";
      *codeName = "ABORT";
      break;
    case SIGBUS:
      *name = "SIGBUS";
      break;
    case SIGFPE:
      *name = "SIGFPE";
      break;
    case SIGILL:
      *name = "SIGILL";
      break;
    case SIGSEGV:
      *name = "SIGSEGV";
      break;
    case SIGSYS:
      *name = "SIGSYS";
      break;
    case SIGTRAP:
      *name = "SIGTRAP";
      break;
    default:
      *name = "UNKNOWN";
      break;
  }
}

static void FIRCLSSignalRecordSignal(int savedErrno, siginfo_t *info, void *uapVoid) {
  if (!_firclsContext.readonly) {
    return;
  }

  if (FIRCLSContextMarkAndCheckIfCrashed()) {
    FIRCLSSDKLog("Error: aborting signal handler because crash has already occurred");
    exit(1);
    return;
  }

  FIRCLSFile file;

  if (!FIRCLSFileInitWithPath(&file, _firclsContext.readonly->signal.path, false)) {
    FIRCLSSDKLog("Unable to open signal file\n");
    return;
  }

  FIRCLSFileWriteSectionStart(&file, "signal");

  FIRCLSFileWriteHashStart(&file);

  if (FIRCLSIsValidPointer(info)) {
    FIRCLSFileWriteHashEntryUint64(&file, "number", info->si_signo);
    FIRCLSFileWriteHashEntryUint64(&file, "code", info->si_code);
    FIRCLSFileWriteHashEntryUint64(&file, "address", (uint64_t)info->si_addr);

    const char *name = NULL;
    const char *codeName = NULL;

    FIRCLSSignalNameLookup(info->si_signo, info->si_code, &name, &codeName);

    FIRCLSFileWriteHashEntryString(&file, "name", name);
    FIRCLSFileWriteHashEntryString(&file, "code_name", codeName);
  }

  FIRCLSFileWriteHashEntryUint64(&file, "errno", savedErrno);
  FIRCLSFileWriteHashEntryUint64(&file, "time", time(NULL));

  FIRCLSFileWriteHashEnd(&file);

  FIRCLSFileWriteSectionEnd(&file);

  FIRCLSHandler(&file, mach_thread_self(), uapVoid);

  FIRCLSFileClose(&file);
}

static void FIRCLSSignalHandler(int signal, siginfo_t *info, void *uapVoid) {
  int savedErrno;
  sigset_t set;

  // save errno, both because it is interesting, and so we can restore it afterwards
  savedErrno = errno;
  errno = 0;

  FIRCLSSDKLog("Signal: %d\n", signal);

  // it is important to do this before unmasking signals, otherwise we can get
  // called in a loop
  FIRCLSSignalSafeRemoveHandlers(true);

  sigfillset(&set);
  if (sigprocmask(SIG_UNBLOCK, &set, NULL) != 0) {
    FIRCLSSDKLog("Unable to unmask signals - we risk infinite recursion here\n");
  }

  // check info and uapVoid, and set them to appropriate values if invalid.  This can happen
  // if we have been called without the SA_SIGINFO flag set
  if (!FIRCLSIsValidPointer(info)) {
    info = NULL;
  }

  if (!FIRCLSIsValidPointer(uapVoid)) {
    uapVoid = NULL;
  }

  FIRCLSSignalRecordSignal(savedErrno, info, uapVoid);

  // re-install original handlers
  if (_firclsContext.readonly) {
    FIRCLSSignalSafeInstallPreexistingHandlers(&_firclsContext.readonly->signal);
  }

  // restore errno
  errno = savedErrno;
}
