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

#import "FIRCLSUUID.h"

#import "FIRCLSByteUtility.h"

static NSInteger const FIRCLSUUIDStringLength = 33;

#pragma mark Public methods

NSString *FIRCLSGenerateUUID(void) {
  NSString *string;

  CFUUIDRef uuid = CFUUIDCreate(kCFAllocatorDefault);
  string = CFBridgingRelease(CFUUIDCreateString(kCFAllocatorDefault, uuid));
  CFRelease(uuid);

  return string;
}

NSString *FIRCLSUUIDToNSString(const uint8_t *uuid) {
  char uuidString[FIRCLSUUIDStringLength];

  FIRCLSSafeHexToString(uuid, 16, uuidString);

  return [NSString stringWithUTF8String:uuidString];
}
