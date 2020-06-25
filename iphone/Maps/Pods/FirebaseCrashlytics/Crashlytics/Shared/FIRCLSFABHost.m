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

#include "FIRCLSFABHost.h"

#if TARGET_OS_WATCH
#import <WatchKit/WatchKit.h>
#elif TARGET_OS_IPHONE
#import <UIKit/UIKit.h>
#endif

#include <sys/sysctl.h>

#define FIRCLS_HOST_SYSCTL_BUFFER_SIZE (128)

#pragma mark - OS Versions

#pragma mark Private

static NSString *FIRCLSHostSysctlEntry(const char *sysctlKey) {
  char buffer[FIRCLS_HOST_SYSCTL_BUFFER_SIZE];
  size_t bufferSize = FIRCLS_HOST_SYSCTL_BUFFER_SIZE;
  if (sysctlbyname(sysctlKey, buffer, &bufferSize, NULL, 0) != 0) {
    return nil;
  }
  return [NSString stringWithUTF8String:buffer];
}

#pragma mark Public

NSOperatingSystemVersion FIRCLSHostGetOSVersion(void) {
  // works on macos(10.10), ios(8.0), watchos(2.0), tvos(9.0)
  if ([NSProcessInfo.processInfo respondsToSelector:@selector(operatingSystemVersion)]) {
    return [NSProcessInfo.processInfo operatingSystemVersion];
  }

  NSOperatingSystemVersion version = {0, 0, 0};

#if TARGET_OS_IPHONE

#if TARGET_OS_WATCH
  NSString *versionString = [[WKInterfaceDevice currentDevice] systemVersion];
#else
  NSString *versionString = [[UIDevice currentDevice] systemVersion];
#endif

  NSArray *parts = [versionString componentsSeparatedByString:@"."];

  if (parts.count > 0) {
    version.majorVersion = [[parts objectAtIndex:0] integerValue];
  }

  if ([parts count] > 1) {
    version.minorVersion = [[parts objectAtIndex:1] integerValue];
  }

  if ([parts count] > 2) {
    version.patchVersion = [[parts objectAtIndex:2] integerValue];
  }

#endif

  return version;
}

NSString *FIRCLSHostOSBuildVersion(void) {
  return FIRCLSHostSysctlEntry("kern.osversion");
}

NSString *FIRCLSHostOSDisplayVersion(void) {
  NSOperatingSystemVersion version = FIRCLSHostGetOSVersion();
  return [NSString stringWithFormat:@"%ld.%ld.%ld", (long)version.majorVersion,
                                    (long)version.minorVersion, (long)version.patchVersion];
}

#pragma mark - Host Models

#pragma mark Public

NSString *FIRCLSHostModelInfo(void) {
  NSString *model = nil;

#if TARGET_OS_SIMULATOR
#if TARGET_OS_WATCH
  model = @"watchOS Simulator";
#elif TARGET_OS_TV
  model = @"tvOS Simulator";
#elif TARGET_OS_IPHONE
  switch (UI_USER_INTERFACE_IDIOM()) {
    case UIUserInterfaceIdiomPhone:
      model = @"iOS Simulator (iPhone)";
      break;
    case UIUserInterfaceIdiomPad:
      model = @"iOS Simulator (iPad)";
      break;
    default:
      model = @"iOS Simulator (Unknown)";
      break;
  }
#endif
#elif TARGET_OS_EMBEDDED
  model = FIRCLSHostSysctlEntry("hw.machine");
#else
  model = FIRCLSHostSysctlEntry("hw.model");
#endif

  return model;
}
