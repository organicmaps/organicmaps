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

#include "FIRCLSHost.h"

#include <mach/mach.h>
#include <sys/mount.h>
#include <sys/sysctl.h>

#import "FIRCLSApplication.h"
#include "FIRCLSDefines.h"
#import "FIRCLSFABHost.h"
#include "FIRCLSFile.h"
#include "FIRCLSGlobals.h"
#include "FIRCLSUtility.h"

#if TARGET_OS_IPHONE
#import <UIKit/UIKit.h>
#else
#import <Cocoa/Cocoa.h>
#endif

#define CLS_HOST_SYSCTL_BUFFER_SIZE (128)

#if CLS_CPU_ARM64
#define CLS_MAX_NATIVE_PAGE_SIZE (1024 * 16)
#else
// return 4K, which is correct for all platforms except arm64, currently
#define CLS_MAX_NATIVE_PAGE_SIZE (1024 * 4)
#endif
#define CLS_MIN_NATIVE_PAGE_SIZE (1024 * 4)

#pragma mark Prototypes
static void FIRCLSHostWriteSysctlEntry(
    FIRCLSFile* file, const char* key, const char* sysctlKey, void* buffer, size_t bufferSize);
static void FIRCLSHostWriteModelInfo(FIRCLSFile* file);
static void FIRCLSHostWriteOSVersionInfo(FIRCLSFile* file);

#pragma mark - API
void FIRCLSHostInitialize(FIRCLSHostReadOnlyContext* roContext) {
  _firclsContext.readonly->host.pageSize = FIRCLSHostGetPageSize();
  _firclsContext.readonly->host.documentDirectoryPath = NULL;

  // determine where the document directory is mounted, so we can get file system statistics later
  NSArray* paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
  if ([paths count]) {
    _firclsContext.readonly->host.documentDirectoryPath =
        FIRCLSDupString([[paths objectAtIndex:0] fileSystemRepresentation]);
  }
}

vm_size_t FIRCLSHostGetPageSize(void) {
  size_t size;
  int pageSize;

  // hw.pagesize is defined as HW_PAGESIZE, which is an int. It's important to match
  // these types. Turns out that sysctl will not init the data to zero, but it appears
  // that sysctlbyname does. This API is nicer, but that's important to keep in mind.

  pageSize = 0;
  size = sizeof(pageSize);
  if (sysctlbyname("hw.pagesize", &pageSize, &size, NULL, 0) != 0) {
    FIRCLSSDKLog("sysctlbyname failed while trying to get hw.pagesize\n");

    return CLS_MAX_NATIVE_PAGE_SIZE;
  }

  // if the returned size is not the expected value, abort
  if (size != sizeof(pageSize)) {
    return CLS_MAX_NATIVE_PAGE_SIZE;
  }

  // put in some guards to make sure our size is reasonable
  if (pageSize > CLS_MAX_NATIVE_PAGE_SIZE) {
    return CLS_MAX_NATIVE_PAGE_SIZE;
  }

  if (pageSize < CLS_MIN_NATIVE_PAGE_SIZE) {
    return CLS_MIN_NATIVE_PAGE_SIZE;
  }

  return pageSize;
}

static void FIRCLSHostWriteSysctlEntry(
    FIRCLSFile* file, const char* key, const char* sysctlKey, void* buffer, size_t bufferSize) {
  if (sysctlbyname(sysctlKey, buffer, &bufferSize, NULL, 0) != 0) {
    FIRCLSFileWriteHashEntryString(file, key, "(failed)");
    return;
  }

  FIRCLSFileWriteHashEntryString(file, key, buffer);
}

static void FIRCLSHostWriteModelInfo(FIRCLSFile* file) {
  FIRCLSFileWriteHashEntryString(file, "model", [FIRCLSHostModelInfo() UTF8String]);

  // allocate a static buffer for the sysctl values, which are typically
  // quite short
  char buffer[CLS_HOST_SYSCTL_BUFFER_SIZE];

#if TARGET_OS_EMBEDDED
  FIRCLSHostWriteSysctlEntry(file, "machine", "hw.model", buffer, CLS_HOST_SYSCTL_BUFFER_SIZE);
#else
  FIRCLSHostWriteSysctlEntry(file, "machine", "hw.machine", buffer, CLS_HOST_SYSCTL_BUFFER_SIZE);
  FIRCLSHostWriteSysctlEntry(file, "cpu", "machdep.cpu.brand_string", buffer,
                             CLS_HOST_SYSCTL_BUFFER_SIZE);
#endif
}

static void FIRCLSHostWriteOSVersionInfo(FIRCLSFile* file) {
  FIRCLSFileWriteHashEntryString(file, "os_build_version", [FIRCLSHostOSBuildVersion() UTF8String]);
  FIRCLSFileWriteHashEntryString(file, "os_display_version",
                                 [FIRCLSHostOSDisplayVersion() UTF8String]);
  FIRCLSFileWriteHashEntryString(file, "platform", [FIRCLSApplicationGetPlatform() UTF8String]);
}

bool FIRCLSHostRecord(FIRCLSFile* file) {
  FIRCLSFileWriteSectionStart(file, "host");

  FIRCLSFileWriteHashStart(file);

  FIRCLSHostWriteModelInfo(file);
  FIRCLSHostWriteOSVersionInfo(file);
  FIRCLSFileWriteHashEntryString(file, "locale",
                                 [[[NSLocale currentLocale] localeIdentifier] UTF8String]);

  FIRCLSFileWriteHashEnd(file);

  FIRCLSFileWriteSectionEnd(file);

  return true;
}

void FIRCLSHostWriteDiskUsage(FIRCLSFile* file) {
  struct statfs tStats;

  FIRCLSFileWriteSectionStart(file, "storage");

  FIRCLSFileWriteHashStart(file);

  if (statfs(_firclsContext.readonly->host.documentDirectoryPath, &tStats) == 0) {
    FIRCLSFileWriteHashEntryUint64(file, "free", tStats.f_bavail * tStats.f_bsize);
    FIRCLSFileWriteHashEntryUint64(file, "total", tStats.f_blocks * tStats.f_bsize);
  }

  FIRCLSFileWriteHashEnd(file);

  FIRCLSFileWriteSectionEnd(file);
}
