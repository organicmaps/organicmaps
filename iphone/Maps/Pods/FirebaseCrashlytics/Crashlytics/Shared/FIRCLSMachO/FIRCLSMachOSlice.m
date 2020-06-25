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

#import "FIRCLSMachOSlice.h"

#include <mach-o/loader.h>

// this is defined only if __OPEN_SOURCE__ is *not* defined in the TVOS SDK's mach-o/loader.h
// also, it has not yet made it back to the OSX SDKs, for example
#ifndef LC_VERSION_MIN_TVOS
#define LC_VERSION_MIN_TVOS 0x2F
#endif

@implementation FIRCLSMachOSlice

+ (id)runningSlice {
  struct FIRCLSMachOSlice slice;

  slice = FIRCLSMachOSliceGetCurrent();

  return [[self alloc] initWithSlice:&slice];
}

@synthesize minimumOSVersion = _minimumOSVersion;
@synthesize linkedSDKVersion = _linkedSDKVersion;

- (id)initWithSlice:(FIRCLSMachOSliceRef)sliceRef {
  self = [super init];
  if (self) {
    NSMutableArray* dylibs;

    _slice = *sliceRef;

    _minimumOSVersion.major = 0;
    _minimumOSVersion.minor = 0;
    _minimumOSVersion.bugfix = 0;

    _linkedSDKVersion.major = 0;
    _linkedSDKVersion.minor = 0;
    _linkedSDKVersion.bugfix = 0;

    dylibs = [NSMutableArray array];

    FIRCLSMachOSliceEnumerateLoadCommands(
        &_slice, ^(uint32_t type, uint32_t size, const struct load_command* cmd) {
          switch (type) {
            case LC_UUID:
              self->_uuidString =
                  [FIRCLSMachONormalizeUUID((CFUUIDBytes*)FIRCLSMachOGetUUID(cmd)) copy];
              break;
            case LC_LOAD_DYLIB:
              [dylibs addObject:[NSString stringWithUTF8String:FIRCLSMachOGetDylibPath(cmd)]];
              break;
            case LC_VERSION_MIN_IPHONEOS:
            case LC_VERSION_MIN_MACOSX:
            case LC_VERSION_MIN_WATCHOS:
            case LC_VERSION_MIN_TVOS:
              self->_minimumOSVersion = FIRCLSMachOGetMinimumOSVersion(cmd);
              self->_linkedSDKVersion = FIRCLSMachOGetLinkedSDKVersion(cmd);
              break;
          }
        });

    _linkedDylibs = [dylibs copy];
  }

  return self;
}

- (NSString*)architectureName {
  return [NSString stringWithUTF8String:FIRCLSMachOSliceGetArchitectureName(&_slice)];
}

- (NSString*)uuid {
  return _uuidString;
}

- (NSArray*)linkedDylibs {
  return _linkedDylibs;
}

@end
