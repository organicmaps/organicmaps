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

/**
 * Returns a SHA1 Hash of the input NSData
 */
NSString *FIRCLSHashNSData(NSData *data);
/**
 * Returns a SHA256 Hash of the input NSData
 */
NSString *FIRCLS256HashNSData(NSData *data);
/**
 * Returns a SHA1 Hash of the input bytes
 */
NSString *FIRCLSHashBytes(const void *bytes, size_t length);
/**
 * Populates a Hex value conversion of value into outputBuffer.
 * If value is nil, then outputBuffer is not modified.
 */
void FIRCLSSafeHexToString(const uint8_t *value, size_t length, char *outputBuffer);

/**
 * Iterates through the raw bytes of NSData in a way that is similar to
 * -[NSData enumerateByteRangesUsingBlock:], but is safe to call from older
 * OSes that do not support it.
 */
void FIRCLSEnumerateByteRangesOfNSDataUsingBlock(
    NSData *data, void (^block)(const void *bytes, NSRange byteRange, BOOL *stop));
