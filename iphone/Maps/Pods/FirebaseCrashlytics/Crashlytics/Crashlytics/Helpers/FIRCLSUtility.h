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

#pragma once

#include <mach/vm_types.h>
#include <stdbool.h>
#include <stdio.h>
#include "FIRCLSGlobals.h"

#define FIRCLSIsValidPointer(x) ((uintptr_t)x >= 4096)
#define FIRCLSInvalidCharNybble (255)

__BEGIN_DECLS

void FIRCLSLookupFunctionPointer(void* ptr, void (^block)(const char* name, const char* lib));

void FIRCLSHexFromByte(uint8_t c, char output[]);
uint8_t FIRCLSNybbleFromChar(char c);

bool FIRCLSReadMemory(vm_address_t src, void* dest, size_t len);
bool FIRCLSReadString(vm_address_t src, char** dest, size_t maxlen);

const char* FIRCLSDupString(const char* string);

bool FIRCLSUnlinkIfExists(const char* path);

#if __OBJC__
void FIRCLSDispatchAfter(float timeInSeconds, dispatch_queue_t queue, dispatch_block_t block);

NSString* FIRCLSNormalizeUUID(NSString* value);
NSString* FIRCLSGenerateNormalizedUUID(void);

NSString* FIRCLSNSDataToNSString(NSData* data);

void FIRCLSAddOperationAfter(float timeInSeconds, NSOperationQueue* queue, void (^block)(void));
#endif

#if DEBUG
void FIRCLSPrintAUUID(const uint8_t* value);
#endif

__END_DECLS
