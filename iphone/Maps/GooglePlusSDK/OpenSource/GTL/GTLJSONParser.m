/* Copyright (c) 2011 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//
//  GTLJSONParser.m
//

#import "GTLJSONParser.h"

// We can assume NSJSONSerialization is present on Mac OS X 10.7 and iOS 5
#if !defined(GTL_REQUIRES_NSJSONSERIALIZATION)
#if (!TARGET_OS_IPHONE && (MAC_OS_X_VERSION_MIN_REQUIRED >= 1070)) || \
    (TARGET_OS_IPHONE && (__IPHONE_OS_VERSION_MIN_REQUIRED >= 50000))
#define GTL_REQUIRES_NSJSONSERIALIZATION 1
#endif
#endif

// If GTMNSJSONSerialization is available, it is used for parsing and
// formatting JSON
#if !GTL_REQUIRES_NSJSONSERIALIZATION
@interface GTMNSJSONSerialization : NSObject
+ (NSData *)dataWithJSONObject:(id)obj options:(NSUInteger)opt error:(NSError **)error;
+ (id)JSONObjectWithData:(NSData *)data options:(NSUInteger)opt error:(NSError **)error;
@end

// As a fallback, SBJSON is used for parsing and formatting JSON
@interface GTLSBJSON
- (void)setHumanReadable:(BOOL)flag;
- (NSString*)stringWithObject:(id)value error:(NSError**)error;
- (id)objectWithString:(NSString*)jsonrep error:(NSError**)error;
@end
#endif // !GTL_REQUIRES_NSJSONSERIALIZATION

@implementation GTLJSONParser

#if DEBUG && !GTL_REQUIRES_NSJSONSERIALIZATION
// When compiling for iOS 4 compatibility, SBJSON must be available
+ (void)load {
  Class writer = NSClassFromString(@"SBJsonWriter");
  Class parser = NSClassFromString(@"SBJsonParser");
  Class oldParser = NSClassFromString(@"SBJSON");
  GTL_ASSERT((oldParser != Nil)
             || (writer != Nil && parser != Nil),
             @"No parsing class found");
}
#endif // DEBUG && !GTL_REQUIRES_NSJSONSERIALIZATION

+ (NSString*)stringWithObject:(id)obj
                humanReadable:(BOOL)humanReadable
                        error:(NSError**)error {
  NSData *data = [self dataWithObject:obj
                        humanReadable:humanReadable
                                error:error];
  if (data) {
    NSString *jsonStr = [[[NSString alloc] initWithData:data
                                               encoding:NSUTF8StringEncoding] autorelease];
    return jsonStr;
  }
  return nil;
}

+ (NSData *)dataWithObject:(id)obj
             humanReadable:(BOOL)humanReadable
                     error:(NSError**)error {
  const NSUInteger kOpts = humanReadable ? (1UL << 0) : 0; // NSJSONWritingPrettyPrinted

#if GTL_REQUIRES_NSJSONSERIALIZATION
  NSData *data = [NSJSONSerialization dataWithJSONObject:obj
                                                 options:kOpts
                                                   error:error];
  return data;
#else
  Class serializer = NSClassFromString(@"NSJSONSerialization");
  if (serializer) {
    NSData *data = [serializer dataWithJSONObject:obj
                                          options:kOpts
                                            error:error];
    return data;
  } else {
    Class jsonWriteClass = NSClassFromString(@"SBJsonWriter");
    if (!jsonWriteClass) {
      jsonWriteClass = NSClassFromString(@"SBJSON");
    }

    if (error) *error = nil;

    GTLSBJSON *writer = [[[jsonWriteClass alloc] init] autorelease];
    [writer setHumanReadable:humanReadable];
    NSString *jsonStr = [writer stringWithObject:obj
                                           error:error];
    NSData *data = [jsonStr dataUsingEncoding:NSUTF8StringEncoding];
    return data;
  }
#endif
}

+ (id)objectWithString:(NSString *)jsonStr
                 error:(NSError **)error {
  NSData *data = [jsonStr dataUsingEncoding:NSUTF8StringEncoding];
  return [self objectWithData:data
                        error:error];
}

+ (id)objectWithData:(NSData *)jsonData
               error:(NSError **)error {
#if GTL_REQUIRES_NSJSONSERIALIZATION
  NSMutableDictionary *obj = [NSJSONSerialization JSONObjectWithData:jsonData
                                                             options:NSJSONReadingMutableContainers
                                                               error:error];
  return obj;
#else
  Class serializer = NSClassFromString(@"NSJSONSerialization");
  if (serializer) {
    const NSUInteger kOpts = (1UL << 0); // NSJSONReadingMutableContainers
    NSMutableDictionary *obj = [serializer JSONObjectWithData:jsonData
                                                      options:kOpts
                                                        error:error];
    return obj;
  } else {
    Class jsonParseClass = NSClassFromString(@"SBJsonParser");
    if (!jsonParseClass) {
      jsonParseClass = NSClassFromString(@"SBJSON");
    }

    if (error) *error = nil;

    GTLSBJSON *parser = [[[jsonParseClass alloc] init] autorelease];

    NSString *jsonrep = [[[NSString alloc] initWithData:jsonData
                                               encoding:NSUTF8StringEncoding] autorelease];
    id obj = [parser objectWithString:jsonrep
                                error:error];
    return obj;
  }
#endif
}

@end
