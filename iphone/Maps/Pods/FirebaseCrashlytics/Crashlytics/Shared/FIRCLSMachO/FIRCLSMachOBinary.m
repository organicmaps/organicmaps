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

#import "FIRCLSMachOBinary.h"

#import "FIRCLSMachOSlice.h"

#import <CommonCrypto/CommonHMAC.h>

static void FIRCLSSafeHexToString(const uint8_t* value, size_t length, char* outputBuffer);
static NSString* FIRCLSNSDataToNSString(NSData* data);
static NSString* FIRCLSHashBytes(const void* bytes, size_t length);
static NSString* FIRCLSHashNSString(NSString* value);

@interface FIRCLSMachOBinary ()

+ (NSString*)hashNSString:(NSString*)value;

@end

@implementation FIRCLSMachOBinary

+ (id)MachOBinaryWithPath:(NSString*)path {
  return [[self alloc] initWithURL:[NSURL fileURLWithPath:path]];
}

@synthesize slices = _slices;

- (id)initWithURL:(NSURL*)url {
  self = [super init];
  if (self) {
    _url = [url copy];

    if (!FIRCLSMachOFileInitWithPath(&_file, [[_url path] fileSystemRepresentation])) {
      return nil;
    }

    _slices = [NSMutableArray new];
    FIRCLSMachOFileEnumerateSlices(&_file, ^(FIRCLSMachOSliceRef slice) {
      FIRCLSMachOSlice* sliceObject;

      sliceObject = [[FIRCLSMachOSlice alloc] initWithSlice:slice];

      [self->_slices addObject:sliceObject];
    });
  }

  return self;
}

- (void)dealloc {
  FIRCLSMachOFileDestroy(&_file);
}

- (NSURL*)URL {
  return _url;
}

- (NSString*)path {
  return [_url path];
}

- (NSString*)instanceIdentifier {
  if (_instanceIdentifier) {
    return _instanceIdentifier;
  }

  NSMutableString* prehashedString = [NSMutableString new];

  // sort the slices by architecture
  NSArray* sortedSlices =
      [_slices sortedArrayUsingComparator:^NSComparisonResult(id obj1, id obj2) {
        return [[obj1 architectureName] compare:[obj2 architectureName]];
      }];

  // append them all into a big string
  for (FIRCLSMachOSlice* slice in sortedSlices) {
    [prehashedString appendString:[slice uuid]];
  }

  _instanceIdentifier = [FIRCLSHashNSString(prehashedString) copy];

  return _instanceIdentifier;
}

- (void)enumerateUUIDs:(void (^)(NSString* uuid, NSString* architecture))block {
  for (FIRCLSMachOSlice* slice in _slices) {
    block([slice uuid], [slice architectureName]);
  }
}

- (FIRCLSMachOSlice*)sliceForArchitecture:(NSString*)architecture {
  for (FIRCLSMachOSlice* slice in [self slices]) {
    if ([[slice architectureName] isEqualToString:architecture]) {
      return slice;
    }
  }

  return nil;
}

+ (NSString*)hashNSString:(NSString*)value {
  return FIRCLSHashNSString(value);
}

@end

// TODO: Functions copied from the SDK.  We should figure out a way to share this.
static void FIRCLSSafeHexToString(const uint8_t* value, size_t length, char* outputBuffer) {
  const char hex[] = "0123456789abcdef";

  if (!value) {
    outputBuffer[0] = '\0';
    return;
  }

  for (size_t i = 0; i < length; ++i) {
    unsigned char c = value[i];
    outputBuffer[i * 2] = hex[c >> 4];
    outputBuffer[i * 2 + 1] = hex[c & 0x0F];
  }

  outputBuffer[length * 2] = '\0';  // null terminate
}

static NSString* FIRCLSNSDataToNSString(NSData* data) {
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
    return nil;
  }

  FIRCLSSafeHexToString([data bytes], length, buffer);

  string = [NSString stringWithUTF8String:buffer];

  free(buffer);

  return string;
}

static NSString* FIRCLSHashBytes(const void* bytes, size_t length) {
  uint8_t digest[CC_SHA1_DIGEST_LENGTH] = {0};
  CC_SHA1(bytes, (CC_LONG)length, digest);

  NSData* result = [NSData dataWithBytes:digest length:CC_SHA1_DIGEST_LENGTH];

  return FIRCLSNSDataToNSString(result);
}

static NSString* FIRCLSHashNSString(NSString* value) {
  const char* s = [value cStringUsingEncoding:NSUTF8StringEncoding];

  return FIRCLSHashBytes(s, strlen(s));
}
