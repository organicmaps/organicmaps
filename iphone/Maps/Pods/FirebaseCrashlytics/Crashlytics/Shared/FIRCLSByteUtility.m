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

#import "FIRCLSByteUtility.h"

#import <CommonCrypto/CommonDigest.h>
#import <CommonCrypto/CommonHMAC.h>

#pragma mark Private functions

static const char FIRCLSHexMap[] = "0123456789abcdef";

void FIRCLSHexFromByte(uint8_t c, char output[]) {
  if (!output) {
    return;
  }

  output[0] = FIRCLSHexMap[c >> 4];
  output[1] = FIRCLSHexMap[c & 0x0f];
}

void FIRCLSSafeHexToString(const uint8_t *value, size_t length, char *outputBuffer) {
  if (!outputBuffer) {
    return;
  }

  memset(outputBuffer, 0, (length * 2) + 1);

  if (!value) {
    return;
  }

  for (size_t i = 0; i < length; ++i) {
    uint8_t c = value[i];

    FIRCLSHexFromByte(c, &outputBuffer[i * 2]);
  }
}

NSString *FIRCLSNSDataPrettyDescription(NSData *data) {
  NSString *string;
  char *buffer;
  size_t size;
  NSUInteger length;

  // we need 2 hex char for every byte of data, plus one more spot for a
  // null terminator
  length = data.length;
  size = (length * 2) + 1;
  buffer = malloc(sizeof(char) * size);

  if (!buffer) {
    return nil;
  }

  FIRCLSSafeHexToString(data.bytes, length, buffer);

  string = [NSString stringWithUTF8String:buffer];

  free(buffer);

  return string;
}

#pragma mark Public functions

NSString *FIRCLSHashBytes(const void *bytes, size_t length) {
  uint8_t digest[CC_SHA1_DIGEST_LENGTH] = {0};
  CC_SHA1(bytes, (CC_LONG)length, digest);

  NSData *result = [NSData dataWithBytes:digest length:CC_SHA1_DIGEST_LENGTH];

  return FIRCLSNSDataPrettyDescription(result);
}

NSString *FIRCLSHashNSData(NSData *data) {
  return FIRCLSHashBytes(data.bytes, data.length);
}

NSString *FIRCLS256HashBytes(const void *bytes, size_t length) {
  uint8_t digest[CC_SHA256_DIGEST_LENGTH] = {0};
  CC_SHA256(bytes, (CC_LONG)length, digest);

  NSData *result = [NSData dataWithBytes:digest length:CC_SHA256_DIGEST_LENGTH];

  return FIRCLSNSDataPrettyDescription(result);
}

NSString *FIRCLS256HashNSData(NSData *data) {
  return FIRCLS256HashBytes(data.bytes, data.length);
}

void FIRCLSEnumerateByteRangesOfNSDataUsingBlock(
    NSData *data, void (^block)(const void *bytes, NSRange byteRange, BOOL *stop)) {
  if ([data respondsToSelector:@selector(enumerateByteRangesUsingBlock:)]) {
    [data enumerateByteRangesUsingBlock:^(const void *bytes, NSRange byteRange, BOOL *stop) {
      block(bytes, byteRange, stop);
    }];

    return;
  }

  // Fall back to the less-efficient mechanism for older OSes. Safe
  // to ignore the return value of stop, since we'll only ever
  // call this once anyways
  BOOL stop = NO;

  block(data.bytes, NSMakeRange(0, data.length), &stop);
}
