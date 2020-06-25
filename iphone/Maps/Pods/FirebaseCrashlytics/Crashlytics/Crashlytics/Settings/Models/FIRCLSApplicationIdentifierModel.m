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

#import "FIRCLSApplicationIdentifierModel.h"

#import "FIRCLSApplication.h"
#import "FIRCLSByteUtility.h"
#import "FIRCLSDefines.h"
#import "FIRCLSMachO.h"
#import "FIRCLSUUID.h"

@interface FIRCLSApplicationIdentifierModel ()

@property(nonatomic, copy, readwrite) NSDictionary *architectureUUIDMap;
@property(nonatomic, copy, readwrite) NSString *buildInstanceID;
@property(nonatomic, readonly) FIRCLSMachOVersion builtSDK;
@property(nonatomic, readonly) FIRCLSMachOVersion minimumSDK;

@end

@implementation FIRCLSApplicationIdentifierModel

- (nullable instancetype)init {
  self = [super init];
  if (!self) {
    return nil;
  }

  if (![self computeExecutableInfo]) {
    return nil;
  }

  [self computeInstanceIdentifier];

  return self;
}

- (NSString *)bundleID {
  return FIRCLSApplicationGetBundleIdentifier();
}

- (NSString *)displayName {
  return FIRCLSApplicationGetName();
}

- (NSString *)platform {
  return FIRCLSApplicationGetPlatform();
}

- (NSString *)buildVersion {
  return FIRCLSApplicationGetBundleVersion();
}

- (NSString *)displayVersion {
  return FIRCLSApplicationGetShortBundleVersion();
}

- (NSString *)synthesizedVersion {
  return [NSString stringWithFormat:@"%@ (%@)", self.displayVersion, self.buildVersion];
}

- (FIRCLSApplicationInstallationSourceType)installSource {
  return FIRCLSApplicationInstallationSource();
}

- (NSString *)builtSDKString {
  return FIRCLSMachOFormatVersion(&_builtSDK);
}

- (NSString *)minimumSDKString {
  return FIRCLSMachOFormatVersion(&_minimumSDK);
}

- (BOOL)computeExecutableInfo {
  struct FIRCLSMachOFile file;

  if (!FIRCLSMachOFileInitWithCurrent(&file)) {
    return NO;
  }

  NSMutableDictionary *executables = [NSMutableDictionary dictionary];

  FIRCLSMachOFileEnumerateSlices(&file, ^(FIRCLSMachOSliceRef fileSlice) {
    NSString *arch;

    arch = [NSString stringWithUTF8String:FIRCLSMachOSliceGetArchitectureName(fileSlice)];

    FIRCLSMachOSliceEnumerateLoadCommands(
        fileSlice, ^(uint32_t type, uint32_t size, const struct load_command *cmd) {
          if (type == LC_UUID) {
            const uint8_t *uuid;

            uuid = FIRCLSMachOGetUUID(cmd);

            [executables setObject:FIRCLSUUIDToNSString(uuid) forKey:arch];
          } else if (type == LC_VERSION_MIN_MACOSX || type == LC_VERSION_MIN_IPHONEOS) {
            self->_minimumSDK = FIRCLSMachOGetMinimumOSVersion(cmd);
            self->_builtSDK = FIRCLSMachOGetLinkedSDKVersion(cmd);
          }
        });
  });

  FIRCLSMachOFileDestroy(&file);

  _architectureUUIDMap = executables;

  return YES;
}

- (void)computeInstanceIdentifier {
  // build up the components of the instance identifier
  NSMutableString *string = [NSMutableString string];

  // first, the uuids, sorted by architecture name
  for (NSString *key in
       [[_architectureUUIDMap allKeys] sortedArrayUsingSelector:@selector(compare:)]) {
    [string appendString:[self.architectureUUIDMap objectForKey:key]];
  }

  // TODO: the instance identifier calculation needs to match Beta's expectation. So, we have to
  // continue generating a less-correct value for now. One day, we should encorporate a hash of the
  // Info.plist and icon data.

  _buildInstanceID = FIRCLSHashNSData([string dataUsingEncoding:NSUTF8StringEncoding]);
}

@end
