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

#import <Foundation/Foundation.h>

#ifndef SKIP_GTL_DEFINES
  #import "GTLDefines.h"
#endif

// helper functions for implementing isEqual:
BOOL GTL_AreEqualOrBothNil(id obj1, id obj2);
BOOL GTL_AreBoolsEqual(BOOL b1, BOOL b2);

// Helper to ensure a number is a number.
//
// The GoogleAPI servers will send numbers >53 bits as strings to avoid
// bugs in some JavaScript implementations.  Work around this by catching
// the string and turning it back into a number.
NSNumber *GTL_EnsureNSNumber(NSNumber *num);

@interface GTLUtilities : NSObject

//
// String encoding
//

// URL encoding, different for parts of URLs and parts of URL parameters
//
// +stringByURLEncodingString just makes a string legal for a URL
//
// +stringByURLEncodingForURI also encodes some characters that are legal in
// URLs but should not be used in URIs,
// per http://bitworking.org/projects/atom/rfc5023.html#rfc.section.9.7
//
// +stringByURLEncodingStringParameter is like +stringByURLEncodingForURI but
// replaces space characters with + characters rather than percent-escaping them
//
+ (NSString *)stringByURLEncodingString:(NSString *)str;
+ (NSString *)stringByURLEncodingForURI:(NSString *)str;
+ (NSString *)stringByURLEncodingStringParameter:(NSString *)str;

// Percent-encoded UTF-8
+ (NSString *)stringByPercentEncodingUTF8ForString:(NSString *)str;

// Key-value coding searches in an array
//
// Utilities to get from an array objects having a known value (or nil)
// at a keyPath

+ (NSArray *)objectsFromArray:(NSArray *)sourceArray
                    withValue:(id)desiredValue
                   forKeyPath:(NSString *)keyPath;

+ (id)firstObjectFromArray:(NSArray *)sourceArray
                 withValue:(id)desiredValue
                forKeyPath:(NSString *)keyPath;

//
// Version helpers
//

+ (NSComparisonResult)compareVersion:(NSString *)ver1 toVersion:(NSString *)ver2;

//
// URL builder
//

// If there are already query parameters on urlString, the new ones are simple
// appended after them.
+ (NSURL *)URLWithString:(NSString *)urlString
         queryParameters:(NSDictionary *)queryParameters;

// Allocate a global dictionary
+ (NSMutableDictionary *)newStaticDictionary;

// Walk up the class tree merging dictionaries and return the result.
+ (NSDictionary *)mergedClassDictionaryForSelector:(SEL)selector
                                        startClass:(Class)startClass
                                     ancestorClass:(Class)ancestorClass
                                             cache:(NSMutableDictionary *)cache;
@end
