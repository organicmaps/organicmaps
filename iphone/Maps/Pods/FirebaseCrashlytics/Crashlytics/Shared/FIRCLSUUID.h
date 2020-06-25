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

#import <Foundation/Foundation.h>
#import "FIRCLSConstants.h"

/**
 * Generates and returns a UUID
 * This is also used by used by Answers to generate UUIDs.
 */
NSString *FIRCLSGenerateUUID(void);

/**
 * Converts the input uint8_t UUID to NSString
 */
NSString *FIRCLSUUIDToNSString(const uint8_t *uuid);
