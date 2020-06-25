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

#include "FIRCLSUtility.h"

#include <mach/mach.h>

#include <dlfcn.h>

#include "FIRCLSFeatures.h"
#include "FIRCLSFile.h"
#include "FIRCLSGlobals.h"

#import "FIRCLSByteUtility.h"
#import "FIRCLSUUID.h"

#import <CommonCrypto/CommonHMAC.h>

void FIRCLSLookupFunctionPointer(void* ptr, void (^block)(const char* name, const char* lib)) {
  Dl_info info;

  if (dladdr(ptr, &info) == 0) {
    block(NULL, NULL);
    return;
  }

  const char* name = "unknown";
  const char* lib = "unknown";

  if (info.dli_sname) {
    name = info.dli_sname;
  }

  if (info.dli_fname) {
    lib = info.dli_fname;
  }

  block(name, lib);
}

uint8_t FIRCLSNybbleFromChar(char c) {
  if (c >= '0' && c <= '9') {
    return c - '0';
  }

  if (c >= 'a' && c <= 'f') {
    return c - 'a' + 10;
  }

  if (c >= 'A' && c <= 'F') {
    return c - 'A' + 10;
  }

  return FIRCLSInvalidCharNybble;
}

bool FIRCLSReadMemory(vm_address_t src, void* dest, size_t len) {
  if (!FIRCLSIsValidPointer(src)) {
    return false;
  }

  vm_size_t readSize = len;

  return vm_read_overwrite(mach_task_self(), src, len, (pointer_t)dest, &readSize) == KERN_SUCCESS;
}

bool FIRCLSReadString(vm_address_t src, char** dest, size_t maxlen) {
  char c;
  vm_address_t address;

  if (!dest) {
    return false;
  }

  // Walk the entire string.  Not certain this is perfect...
  for (address = src; address < src + maxlen; ++address) {
    if (!FIRCLSReadMemory(address, &c, 1)) {
      return false;
    }

    if (c == 0) {
      break;
    }
  }

  *dest = (char*)src;

  return true;
}

const char* FIRCLSDupString(const char* string) {
#if CLS_MEMORY_PROTECTION_ENABLED
  char* buffer;
  size_t length;

  if (!string) {
    return NULL;
  }

  length = strlen(string);
  buffer = FIRCLSAllocatorSafeAllocate(_firclsContext.allocator, length + 1, CLS_READONLY);

  memcpy(buffer, string, length);

  buffer[length] = 0;  // null-terminate

  return buffer;
#else
  return strdup(string);
#endif
}

void FIRCLSDispatchAfter(float timeInSeconds, dispatch_queue_t queue, dispatch_block_t block) {
  dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(timeInSeconds * NSEC_PER_SEC)), queue,
                 block);
}

bool FIRCLSUnlinkIfExists(const char* path) {
  if (unlink(path) != 0) {
    if (errno != ENOENT) {
      return false;
    }
  }

  return true;
}

/*
NSString* FIRCLSGenerateUUID(void) {
  NSString* string;

  CFUUIDRef uuid = CFUUIDCreate(kCFAllocatorDefault);
  string = CFBridgingRelease(CFUUIDCreateString(kCFAllocatorDefault, uuid));
  CFRelease(uuid);

  return string;
}
*/

NSString* FIRCLSNormalizeUUID(NSString* value) {
  return [[value stringByReplacingOccurrencesOfString:@"-" withString:@""] lowercaseString];
}

NSString* FIRCLSGenerateNormalizedUUID(void) {
  return FIRCLSNormalizeUUID(FIRCLSGenerateUUID());
}

NSString* FIRCLSNSDataToNSString(NSData* data) {
  NSString* string;
  char* buffer;
  size_t size;
  NSUInteger length;

  // we need 2 hex char for every byte of data, plus one more spot for a
  // null terminator
  length = [data length];
  size = (length * 2) + 1;
  buffer = malloc(sizeof(char) * size);

  if (!buffer) {
    FIRCLSErrorLog(@"Unable to malloc in FIRCLSNSDataToNSString");
    return nil;
  }

  FIRCLSSafeHexToString([data bytes], length, buffer);

  string = [NSString stringWithUTF8String:buffer];

  free(buffer);

  return string;
}

/*
NSString* FIRCLSHashBytes(const void* bytes, size_t length) {
  uint8_t digest[CC_SHA1_DIGEST_LENGTH] = {0};
  CC_SHA1(bytes, (CC_LONG)length, digest);

  NSData* result = [NSData dataWithBytes:digest length:CC_SHA1_DIGEST_LENGTH];

  return FIRCLSNSDataToNSString(result);
}

NSString* FIRCLSHashNSData(NSData* data) {
  return FIRCLSHashBytes([data bytes], [data length]);
}
*/

void FIRCLSAddOperationAfter(float timeInSeconds, NSOperationQueue* queue, void (^block)(void)) {
  dispatch_queue_t afterQueue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
  FIRCLSDispatchAfter(timeInSeconds, afterQueue, ^{
    [queue addOperationWithBlock:block];
  });
}

#if DEBUG
void FIRCLSPrintAUUID(const uint8_t* value) {
  CFUUIDRef uuid = CFUUIDCreateFromUUIDBytes(kCFAllocatorDefault, *(CFUUIDBytes*)value);

  NSString* string = CFBridgingRelease(CFUUIDCreateString(kCFAllocatorDefault, uuid));

  CFRelease(uuid);

  FIRCLSDebugLog(@"%@", [[string stringByReplacingOccurrencesOfString:@"-"
                                                           withString:@""] lowercaseString]);
}
#endif
