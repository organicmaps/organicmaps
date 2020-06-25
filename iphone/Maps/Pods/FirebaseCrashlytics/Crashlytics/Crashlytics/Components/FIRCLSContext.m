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

#include "FIRCLSContext.h"

#include <stdlib.h>
#include <string.h>

#import "FIRCLSFileManager.h"
#import "FIRCLSInstallIdentifierModel.h"
#import "FIRCLSInternalReport.h"
#import "FIRCLSSettings.h"

#include "FIRCLSApplication.h"
#include "FIRCLSCrashedMarkerFile.h"
#include "FIRCLSDefines.h"
#include "FIRCLSFeatures.h"
#include "FIRCLSGlobals.h"
#include "FIRCLSProcess.h"
#include "FIRCLSUtility.h"

// The writable size is our handler stack plus whatever scratch we need.  We have to use this space
// extremely carefully, however, because thread stacks always needs to be page-aligned.  Only the
// first allocation is gauranteed to be page-aligned.
//
// CLS_SIGNAL_HANDLER_STACK_SIZE and CLS_MACH_EXCEPTION_HANDLER_STACK_SIZE are platform dependant,
// defined as 0 for tv/watch.
#define CLS_MINIMUM_READWRITE_SIZE                                         \
  (CLS_SIGNAL_HANDLER_STACK_SIZE + CLS_MACH_EXCEPTION_HANDLER_STACK_SIZE + \
   sizeof(FIRCLSReadWriteContext))

// We need enough space here for the context, plus storage for strings.
#define CLS_MINIMUM_READABLE_SIZE (sizeof(FIRCLSReadOnlyContext) + 4096 * 4)

static const int64_t FIRCLSContextInitWaitTime = 5LL * NSEC_PER_SEC;

static bool FIRCLSContextRecordMetadata(const char* path, const FIRCLSContextInitData* initData);
static const char* FIRCLSContextAppendToRoot(NSString* root, NSString* component);
static void FIRCLSContextAllocate(FIRCLSContext* context);

FIRCLSContextInitData FIRCLSContextBuildInitData(FIRCLSInternalReport* report,
                                                 FIRCLSSettings* settings,
                                                 FIRCLSInstallIdentifierModel* installIDModel,
                                                 FIRCLSFileManager* fileManager) {
  // Because we need to start the crash reporter right away,
  // it starts up either with default settings, or cached settings
  // from the last time they were fetched

  FIRCLSContextInitData initData;

  memset(&initData, 0, sizeof(FIRCLSContextInitData));

  initData.customBundleId = nil;
  initData.installId = [installIDModel.installID UTF8String];
  initData.sessionId = [[report identifier] UTF8String];
  initData.rootPath = [[report path] UTF8String];
  initData.previouslyCrashedFileRootPath = [[fileManager rootPath] UTF8String];
  initData.errorsEnabled = [settings errorReportingEnabled];
  initData.customExceptionsEnabled = [settings customExceptionsEnabled];
  initData.maxCustomExceptions = [settings maxCustomExceptions];
  initData.maxErrorLogSize = [settings errorLogBufferSize];
  initData.maxLogSize = [settings logBufferSize];
  initData.maxKeyValues = [settings maxCustomKeys];

  // If this is set, then we could attempt to do a synchronous submission for certain kinds of
  // events (exceptions). This is a very cool feature, but adds complexity to the backend. For now,
  // we're going to leave this disabled. It does work in the exception case, but will ultimtely
  // result in the following crash to be discared. Usually that crash isn't interesting. But, if it
  // was, we'd never have a chance to see it.
  initData.delegate = nil;

#if CLS_MACH_EXCEPTION_SUPPORTED
  __block exception_mask_t mask = 0;

  // TODO(b/141241224) This if statement was hardcoded to no, so this block was never run
  //  FIRCLSSignalEnumerateHandledSignals(^(int idx, int signal) {
  //    if ([self.delegate ensureDeliveryOfUnixSignal:signal]) {
  //      mask |= FIRCLSMachExceptionMaskForSignal(signal);
  //    }
  //  });

  initData.machExceptionMask = mask;
#endif

  return initData;
}

bool FIRCLSContextInitialize(FIRCLSInternalReport* report,
                             FIRCLSSettings* settings,
                             FIRCLSInstallIdentifierModel* installIDModel,
                             FIRCLSFileManager* fileManager) {
  FIRCLSContextInitData initDataObj =
      FIRCLSContextBuildInitData(report, settings, installIDModel, fileManager);
  FIRCLSContextInitData* initData = &initDataObj;

  if (!initData) {
    return false;
  }

  FIRCLSContextBaseInit();

  dispatch_group_t group = dispatch_group_create();
  dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);

  if (!FIRCLSIsValidPointer(initData->rootPath)) {
    return false;
  }

  NSString* rootPath = [NSString stringWithUTF8String:initData->rootPath];

  // setup our SDK log file synchronously, because other calls may depend on it
  _firclsContext.readonly->logPath = FIRCLSContextAppendToRoot(rootPath, @"sdk.log");
  if (!FIRCLSUnlinkIfExists(_firclsContext.readonly->logPath)) {
    FIRCLSErrorLog(@"Unable to write initialize SDK write paths %s", strerror(errno));
  }

  // some values that aren't tied to particular subsystem
  _firclsContext.readonly->debuggerAttached = FIRCLSProcessDebuggerAttached();
  _firclsContext.readonly->delegate = initData->delegate;

  dispatch_group_async(group, queue, ^{
    FIRCLSHostInitialize(&_firclsContext.readonly->host);
  });

  dispatch_group_async(group, queue, ^{
    _firclsContext.readonly->logging.errorStorage.maxSize = 0;
    _firclsContext.readonly->logging.errorStorage.maxEntries =
        initData->errorsEnabled ? initData->maxCustomExceptions : 0;
    _firclsContext.readonly->logging.errorStorage.restrictBySize = false;
    _firclsContext.readonly->logging.errorStorage.entryCount =
        &_firclsContext.writable->logging.errorsCount;
    _firclsContext.readonly->logging.errorStorage.aPath =
        FIRCLSContextAppendToRoot(rootPath, FIRCLSReportErrorAFile);
    _firclsContext.readonly->logging.errorStorage.bPath =
        FIRCLSContextAppendToRoot(rootPath, FIRCLSReportErrorBFile);

    _firclsContext.readonly->logging.logStorage.maxSize = initData->maxLogSize;
    _firclsContext.readonly->logging.logStorage.maxEntries = 0;
    _firclsContext.readonly->logging.logStorage.restrictBySize = true;
    _firclsContext.readonly->logging.logStorage.entryCount = NULL;
    _firclsContext.readonly->logging.logStorage.aPath =
        FIRCLSContextAppendToRoot(rootPath, FIRCLSReportLogAFile);
    _firclsContext.readonly->logging.logStorage.bPath =
        FIRCLSContextAppendToRoot(rootPath, FIRCLSReportLogBFile);
    _firclsContext.readonly->logging.customExceptionStorage.aPath =
        FIRCLSContextAppendToRoot(rootPath, FIRCLSReportCustomExceptionAFile);
    _firclsContext.readonly->logging.customExceptionStorage.bPath =
        FIRCLSContextAppendToRoot(rootPath, FIRCLSReportCustomExceptionBFile);
    _firclsContext.readonly->logging.customExceptionStorage.maxSize = 0;
    _firclsContext.readonly->logging.customExceptionStorage.restrictBySize = false;
    _firclsContext.readonly->logging.customExceptionStorage.maxEntries =
        initData->maxCustomExceptions;
    _firclsContext.readonly->logging.customExceptionStorage.entryCount =
        &_firclsContext.writable->exception.customExceptionCount;

    _firclsContext.readonly->logging.userKVStorage.maxCount = initData->maxKeyValues;
    _firclsContext.readonly->logging.userKVStorage.incrementalPath =
        FIRCLSContextAppendToRoot(rootPath, FIRCLSReportUserIncrementalKVFile);
    _firclsContext.readonly->logging.userKVStorage.compactedPath =
        FIRCLSContextAppendToRoot(rootPath, FIRCLSReportUserCompactedKVFile);

    _firclsContext.readonly->logging.internalKVStorage.maxCount = 32;  // Hardcode = bad
    _firclsContext.readonly->logging.internalKVStorage.incrementalPath =
        FIRCLSContextAppendToRoot(rootPath, FIRCLSReportInternalIncrementalKVFile);
    _firclsContext.readonly->logging.internalKVStorage.compactedPath =
        FIRCLSContextAppendToRoot(rootPath, FIRCLSReportInternalCompactedKVFile);

    FIRCLSUserLoggingInit(&_firclsContext.readonly->logging, &_firclsContext.writable->logging);
  });

  dispatch_group_async(group, queue, ^{
    _firclsContext.readonly->binaryimage.path =
        FIRCLSContextAppendToRoot(rootPath, FIRCLSReportBinaryImageFile);

    FIRCLSBinaryImageInit(&_firclsContext.readonly->binaryimage,
                          &_firclsContext.writable->binaryImage);
  });

  dispatch_group_async(group, queue, ^{
    NSString* rootPath = [NSString stringWithUTF8String:initData->previouslyCrashedFileRootPath];
    NSString* fileName = [NSString stringWithUTF8String:FIRCLSCrashedMarkerFileName];
    _firclsContext.readonly->previouslyCrashedFileFullPath =
        FIRCLSContextAppendToRoot(rootPath, fileName);
  });

  if (!_firclsContext.readonly->debuggerAttached) {
    dispatch_group_async(group, queue, ^{
      _firclsContext.readonly->signal.path =
          FIRCLSContextAppendToRoot(rootPath, FIRCLSReportSignalFile);

      FIRCLSSignalInitialize(&_firclsContext.readonly->signal);
    });

#if CLS_MACH_EXCEPTION_SUPPORTED
    dispatch_group_async(group, queue, ^{
      _firclsContext.readonly->machException.path =
          FIRCLSContextAppendToRoot(rootPath, FIRCLSReportMachExceptionFile);

      FIRCLSMachExceptionInit(&_firclsContext.readonly->machException, initData->machExceptionMask);
    });
#endif

    dispatch_group_async(group, queue, ^{
      _firclsContext.readonly->exception.path =
          FIRCLSContextAppendToRoot(rootPath, FIRCLSReportExceptionFile);
      _firclsContext.readonly->exception.maxCustomExceptions =
          initData->customExceptionsEnabled ? initData->maxCustomExceptions : 0;

      FIRCLSExceptionInitialize(&_firclsContext.readonly->exception,
                                &_firclsContext.writable->exception, initData->delegate);
    });
  } else {
    FIRCLSSDKLog("Debugger present - not installing handlers\n");
  }

  dispatch_group_async(group, queue, ^{
    const char* metaDataPath = [[rootPath stringByAppendingPathComponent:FIRCLSReportMetadataFile]
        fileSystemRepresentation];
    if (!FIRCLSContextRecordMetadata(metaDataPath, initData)) {
      FIRCLSSDKLog("Unable to record context metadata\n");
    }
  });

  // At this point we need to do two things. First, we need to do our memory protection *only* after
  // all of these initialization steps are really done. But, we also want to wait as long as
  // possible for these to be complete. If we do not, there's a chance that we will not be able to
  // correctly report a crash shortly after start.

  // Note at this will retain the group, so its totally fine to release the group here.
  dispatch_group_notify(group, queue, ^{
    _firclsContext.readonly->initialized = true;
    __sync_synchronize();

    if (!FIRCLSAllocatorProtect(_firclsContext.allocator)) {
      FIRCLSSDKLog("Error: Memory protection failed\n");
    }
  });

  if (dispatch_group_wait(group, dispatch_time(DISPATCH_TIME_NOW, FIRCLSContextInitWaitTime)) !=
      0) {
    FIRCLSSDKLog("Error: Delayed initialization\n");
  }

  return true;
}

void FIRCLSContextUpdateMetadata(FIRCLSInternalReport* report,
                                 FIRCLSSettings* settings,
                                 FIRCLSInstallIdentifierModel* installIDModel,
                                 FIRCLSFileManager* fileManager) {
  FIRCLSContextInitData initDataObj =
      FIRCLSContextBuildInitData(report, settings, installIDModel, fileManager);
  FIRCLSContextInitData* initData = &initDataObj;

  NSString* rootPath = [NSString stringWithUTF8String:initData->rootPath];

  const char* metaDataPath =
      [[rootPath stringByAppendingPathComponent:FIRCLSReportMetadataFile] fileSystemRepresentation];

  if (!FIRCLSContextRecordMetadata(metaDataPath, initData)) {
    FIRCLSErrorLog(@"Unable to update context metadata");
  }
}

void FIRCLSContextBaseInit(void) {
  NSString* sdkBundleID = FIRCLSApplicationGetSDKBundleID();

  NSString* loggingQueueName = [sdkBundleID stringByAppendingString:@".logging"];
  NSString* binaryImagesQueueName = [sdkBundleID stringByAppendingString:@".binary-images"];
  NSString* exceptionQueueName = [sdkBundleID stringByAppendingString:@".exception"];

  _firclsLoggingQueue = dispatch_queue_create([loggingQueueName UTF8String], DISPATCH_QUEUE_SERIAL);
  _firclsBinaryImageQueue =
      dispatch_queue_create([binaryImagesQueueName UTF8String], DISPATCH_QUEUE_SERIAL);
  _firclsExceptionQueue =
      dispatch_queue_create([exceptionQueueName UTF8String], DISPATCH_QUEUE_SERIAL);

  FIRCLSContextAllocate(&_firclsContext);

  _firclsContext.writable->internalLogging.logFd = -1;
  _firclsContext.writable->internalLogging.logLevel = FIRCLSInternalLogLevelDebug;
  _firclsContext.writable->crashOccurred = false;

  _firclsContext.readonly->initialized = false;

  __sync_synchronize();
}

static void FIRCLSContextAllocate(FIRCLSContext* context) {
  // create the allocator, and the contexts
  // The ordering here is really important, because the "stack" variable must be
  // page-aligned.  There's no mechanism to ask the allocator to do alignment, but we
  // do know the very first allocation in a region is aligned to a page boundary.

  context->allocator = FIRCLSAllocatorCreate(CLS_MINIMUM_READWRITE_SIZE, CLS_MINIMUM_READABLE_SIZE);

  context->readonly =
      FIRCLSAllocatorSafeAllocate(context->allocator, sizeof(FIRCLSReadOnlyContext), CLS_READONLY);
  memset(context->readonly, 0, sizeof(FIRCLSReadOnlyContext));

#if CLS_MEMORY_PROTECTION_ENABLED
#if CLS_MACH_EXCEPTION_SUPPORTED
  context->readonly->machStack = FIRCLSAllocatorSafeAllocate(
      context->allocator, CLS_MACH_EXCEPTION_HANDLER_STACK_SIZE, CLS_READWRITE);
#endif
#if CLS_USE_SIGALTSTACK
  context->readonly->signalStack =
      FIRCLSAllocatorSafeAllocate(context->allocator, CLS_SIGNAL_HANDLER_STACK_SIZE, CLS_READWRITE);
#endif
#else
#if CLS_MACH_EXCEPTION_SUPPORTED
  context->readonly->machStack = valloc(CLS_MACH_EXCEPTION_HANDLER_STACK_SIZE);
#endif
#if CLS_USE_SIGALTSTACK
  context->readonly->signalStack = valloc(CLS_SIGNAL_HANDLER_STACK_SIZE);
#endif
#endif

#if CLS_MACH_EXCEPTION_SUPPORTED
  memset(_firclsContext.readonly->machStack, 0, CLS_MACH_EXCEPTION_HANDLER_STACK_SIZE);
#endif
#if CLS_USE_SIGALTSTACK
  memset(_firclsContext.readonly->signalStack, 0, CLS_SIGNAL_HANDLER_STACK_SIZE);
#endif

  context->writable = FIRCLSAllocatorSafeAllocate(context->allocator,
                                                  sizeof(FIRCLSReadWriteContext), CLS_READWRITE);
  memset(context->writable, 0, sizeof(FIRCLSReadWriteContext));
}

void FIRCLSContextBaseDeinit(void) {
  _firclsContext.readonly->initialized = false;

  FIRCLSAllocatorDestroy(_firclsContext.allocator);
}

bool FIRCLSContextIsInitialized(void) {
  __sync_synchronize();
  if (!FIRCLSIsValidPointer(_firclsContext.readonly)) {
    return false;
  }

  return _firclsContext.readonly->initialized;
}

bool FIRCLSContextHasCrashed(void) {
  if (!FIRCLSContextIsInitialized()) {
    return false;
  }

  // we've already run a full barrier above, so this read is ok
  return _firclsContext.writable->crashOccurred;
}

void FIRCLSContextMarkHasCrashed(void) {
  if (!FIRCLSContextIsInitialized()) {
    return;
  }

  _firclsContext.writable->crashOccurred = true;
  __sync_synchronize();
}

bool FIRCLSContextMarkAndCheckIfCrashed(void) {
  if (!FIRCLSContextIsInitialized()) {
    return false;
  }

  if (_firclsContext.writable->crashOccurred) {
    return true;
  }

  _firclsContext.writable->crashOccurred = true;
  __sync_synchronize();

  return false;
}

static const char* FIRCLSContextAppendToRoot(NSString* root, NSString* component) {
  return FIRCLSDupString(
      [[root stringByAppendingPathComponent:component] fileSystemRepresentation]);
}

static bool FIRCLSContextRecordIdentity(FIRCLSFile* file, const FIRCLSContextInitData* initData) {
  FIRCLSFileWriteSectionStart(file, "identity");

  FIRCLSFileWriteHashStart(file);

  FIRCLSFileWriteHashEntryString(file, "generator", CLS_SDK_GENERATOR_NAME);
  FIRCLSFileWriteHashEntryString(file, "display_version", CLS_SDK_DISPLAY_VERSION);
  FIRCLSFileWriteHashEntryString(file, "build_version", CLS_SDK_DISPLAY_VERSION);
  FIRCLSFileWriteHashEntryUint64(file, "started_at", time(NULL));

  FIRCLSFileWriteHashEntryString(file, "session_id", initData->sessionId);
  FIRCLSFileWriteHashEntryString(file, "install_id", initData->installId);
  FIRCLSFileWriteHashEntryString(file, "beta_token", initData->betaToken);
  FIRCLSFileWriteHashEntryBoolean(file, "absolute_log_timestamps", true);

  FIRCLSFileWriteHashEnd(file);
  FIRCLSFileWriteSectionEnd(file);

  return true;
}

static bool FIRCLSContextRecordApplication(FIRCLSFile* file, const char* customBundleId) {
  FIRCLSFileWriteSectionStart(file, "application");

  FIRCLSFileWriteHashStart(file);

  FIRCLSFileWriteHashEntryString(file, "bundle_id",
                                 [FIRCLSApplicationGetBundleIdentifier() UTF8String]);
  FIRCLSFileWriteHashEntryString(file, "custom_bundle_id", customBundleId);
  FIRCLSFileWriteHashEntryString(file, "build_version",
                                 [FIRCLSApplicationGetBundleVersion() UTF8String]);
  FIRCLSFileWriteHashEntryString(file, "display_version",
                                 [FIRCLSApplicationGetShortBundleVersion() UTF8String]);
  FIRCLSFileWriteHashEntryString(file, "extension_id",
                                 [FIRCLSApplicationExtensionPointIdentifier() UTF8String]);

  FIRCLSFileWriteHashEnd(file);
  FIRCLSFileWriteSectionEnd(file);

  return true;
}

static bool FIRCLSContextRecordMetadata(const char* path, const FIRCLSContextInitData* initData) {
  if (!FIRCLSUnlinkIfExists(path)) {
    FIRCLSSDKLog("Unable to unlink existing metadata file %s\n", strerror(errno));
  }

  FIRCLSFile file;

  if (!FIRCLSFileInitWithPath(&file, path, false)) {
    FIRCLSSDKLog("Unable to open metadata file %s\n", strerror(errno));
    return false;
  }

  if (!FIRCLSContextRecordIdentity(&file, initData)) {
    FIRCLSSDKLog("Unable to write out identity metadata\n");
  }

  if (!FIRCLSHostRecord(&file)) {
    FIRCLSSDKLog("Unable to write out host metadata\n");
  }

  if (!FIRCLSContextRecordApplication(&file, initData->customBundleId)) {
    FIRCLSSDKLog("Unable to write out application metadata\n");
  }

  if (!FIRCLSBinaryImageRecordMainExecutable(&file)) {
    FIRCLSSDKLog("Unable to write out executable metadata\n");
  }

  FIRCLSFileClose(&file);

  return true;
}
