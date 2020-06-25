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

#import "FIRCLSMultipartMimeStreamEncoder.h"

#import "FIRCLSByteUtility.h"
#import "FIRCLSLogger.h"
#import "FIRCLSUUID.h"

@interface FIRCLSMultipartMimeStreamEncoder () <NSStreamDelegate>

@property(nonatomic) NSUInteger length;
@property(nonatomic, copy) NSString *boundary;
@property(nonatomic, copy, readonly) NSData *headerData;
@property(nonatomic, copy, readonly) NSData *footerData;
@property(nonatomic, strong) NSOutputStream *outputStream;

@end

@implementation FIRCLSMultipartMimeStreamEncoder

+ (void)populateRequest:(NSMutableURLRequest *)request
    withDataFromEncoder:(void (^)(FIRCLSMultipartMimeStreamEncoder *encoder))block {
  NSString *boundary = [self generateBoundary];

  NSOutputStream *stream = [NSOutputStream outputStreamToMemory];

  FIRCLSMultipartMimeStreamEncoder *encoder =
      [[FIRCLSMultipartMimeStreamEncoder alloc] initWithStream:stream andBoundary:boundary];

  [encoder encode:^{
    block(encoder);
  }];

  [request setValue:encoder.contentTypeHTTPHeaderValue forHTTPHeaderField:@"Content-Type"];
  [request setValue:encoder.contentLengthHTTPHeaderValue forHTTPHeaderField:@"Content-Length"];

  NSData *data = [stream propertyForKey:NSStreamDataWrittenToMemoryStreamKey];
  request.HTTPBody = data;
}

+ (NSString *)contentTypeHTTPHeaderValueWithBoundary:(NSString *)boundary {
  return [NSString stringWithFormat:@"multipart/form-data; boundary=%@", boundary];
}

+ (instancetype)encoderWithStream:(NSOutputStream *)stream andBoundary:(NSString *)boundary {
  return [[self alloc] initWithStream:stream andBoundary:boundary];
}

+ (NSString *)generateBoundary {
  return FIRCLSGenerateUUID();
}

- (instancetype)initWithStream:(NSOutputStream *)stream andBoundary:(NSString *)boundary {
  self = [super init];
  if (!self) {
    return nil;
  }

  self.outputStream = stream;

  if (!boundary) {
    boundary = [FIRCLSMultipartMimeStreamEncoder generateBoundary];
  }

  _boundary = boundary;

  return self;
}

- (void)encode:(void (^)(void))block {
  [self beginEncoding];

  block();

  [self endEncoding];
}

- (NSString *)contentTypeHTTPHeaderValue {
  return [[self class] contentTypeHTTPHeaderValueWithBoundary:self.boundary];
}

- (NSString *)contentLengthHTTPHeaderValue {
  return [NSString stringWithFormat:@"%lu", (unsigned long)_length];
}

#pragma - mark MIME part API
- (void)beginEncoding {
  _length = 0;

  [self.outputStream open];

  [self writeData:self.headerData];
}

- (void)endEncoding {
  [self writeData:self.footerData];

  [self.outputStream close];
}

- (NSData *)headerData {
  return [@"MIME-Version: 1.0\r\n" dataUsingEncoding:NSUTF8StringEncoding];
}

- (NSData *)footerData {
  return [[NSString stringWithFormat:@"--%@--\r\n", self.boundary]
      dataUsingEncoding:NSUTF8StringEncoding];
}

- (void)addFileData:(NSData *)data
           fileName:(NSString *)fileName
           mimeType:(NSString *)mimeType
          fieldName:(NSString *)name {
  if ([data length] == 0) {
    FIRCLSErrorLog(@"Unable to MIME encode data with zero length (%@)", name);
    return;
  }

  if ([name length] == 0 || [fileName length] == 0) {
    FIRCLSErrorLog(@"name (%@) or fieldname (%@) is invalid", name, fileName);
    return;
  }

  NSMutableString *string;

  string = [NSMutableString
      stringWithFormat:@"--%@\r\nContent-Disposition: form-data; name=\"%@\"; filename=\"%@\"\r\n",
                       self.boundary, name, fileName];

  if (mimeType) {
    [string appendFormat:@"Content-Type: %@\r\n", mimeType];
    [string appendString:@"Content-Transfer-Encoding: binary\r\n\r\n"];
  } else {
    [string appendString:@"Content-Type: application/octet-stream\r\n\r\n"];
  }

  [self writeData:[string dataUsingEncoding:NSUTF8StringEncoding]];

  [self writeData:data];

  [self writeData:[@"\r\n" dataUsingEncoding:NSUTF8StringEncoding]];
}

- (void)addValue:(id)value fieldName:(NSString *)name {
  if ([name length] == 0 || !value || value == NSNull.null) {
    FIRCLSErrorLog(@"name (%@) or value (%@) is invalid", name, value);
    return;
  }

  NSMutableString *string;

  string =
      [NSMutableString stringWithFormat:@"--%@\r\nContent-Disposition: form-data; name=\"%@\"\r\n",
                                        self.boundary, name];
  [string appendString:@"Content-Type: text/plain\r\n\r\n"];
  [string appendFormat:@"%@\r\n", value];

  [self writeData:[string dataUsingEncoding:NSUTF8StringEncoding]];
}

- (void)addFile:(NSURL *)fileURL
       fileName:(NSString *)fileName
       mimeType:(NSString *)mimeType
      fieldName:(NSString *)name {
  NSData *data = [NSData dataWithContentsOfURL:fileURL];

  [self addFileData:data fileName:fileName mimeType:mimeType fieldName:name];
}

- (BOOL)writeBytes:(const void *)bytes ofLength:(NSUInteger)length {
  if ([self.outputStream write:bytes maxLength:length] != length) {
    FIRCLSErrorLog(@"Failed to write bytes to stream");
    return NO;
  }

  _length += length;

  return YES;
}

- (void)writeData:(NSData *)data {
  FIRCLSEnumerateByteRangesOfNSDataUsingBlock(
      data, ^(const void *bytes, NSRange byteRange, BOOL *stop) {
        NSUInteger length = byteRange.length;

        if ([self.outputStream write:bytes maxLength:length] != length) {
          FIRCLSErrorLog(@"Failed to write data to stream");
          *stop = YES;
          return;
        }

        self->_length += length;
      });
}

@end
