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

#import "GTLUtilities.h"

#include <objc/runtime.h>

@implementation GTLUtilities

#pragma mark String encoding

// URL Encoding

+ (NSString *)stringByURLEncodingString:(NSString *)str {
  NSString *result = [str stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding];
  return result;
}

// NSURL's stringByAddingPercentEscapesUsingEncoding: does not escape
// some characters that should be escaped in URL parameters, like / and ?;
// we'll use CFURL to force the encoding of those
//
// Reference: http://www.ietf.org/rfc/rfc3986.txt

const CFStringRef kCharsToForceEscape = CFSTR("!*'();:@&=+$,/?%#[]");

+ (NSString *)stringByURLEncodingForURI:(NSString *)str {

  NSString *resultStr = str;

  CFStringRef originalString = (CFStringRef) str;
  CFStringRef leaveUnescaped = NULL;

  CFStringRef escapedStr;
  escapedStr = CFURLCreateStringByAddingPercentEscapes(kCFAllocatorDefault,
                                                       originalString,
                                                       leaveUnescaped,
                                                       kCharsToForceEscape,
                                                       kCFStringEncodingUTF8);
  if (escapedStr) {
    resultStr = [(id)CFMakeCollectable(escapedStr) autorelease];
  }
  return resultStr;
}

+ (NSString *)stringByURLEncodingStringParameter:(NSString *)str {
  // For parameters, we'll explicitly leave spaces unescaped now, and replace
  // them with +'s
  NSString *resultStr = str;

  CFStringRef originalString = (CFStringRef) str;
  CFStringRef leaveUnescaped = CFSTR(" ");

  CFStringRef escapedStr;
  escapedStr = CFURLCreateStringByAddingPercentEscapes(kCFAllocatorDefault,
                                                       originalString,
                                                       leaveUnescaped,
                                                       kCharsToForceEscape,
                                                       kCFStringEncodingUTF8);

  if (escapedStr) {
    NSMutableString *mutableStr = [NSMutableString stringWithString:(NSString *)escapedStr];
    CFRelease(escapedStr);

    // replace spaces with plusses
    [mutableStr replaceOccurrencesOfString:@" "
                                withString:@"+"
                                   options:0
                                     range:NSMakeRange(0, [mutableStr length])];
    resultStr = mutableStr;
  }
  return resultStr;
}

+ (NSString *)stringByPercentEncodingUTF8ForString:(NSString *)inputStr {

  // Encode per http://bitworking.org/projects/atom/rfc5023.html#rfc.section.9.7
  //
  // This is used for encoding upload slug headers
  //
  // Step through the string as UTF-8, and replace characters outside 20..7E
  // (and the percent symbol itself, 25) with percent-encodings
  //
  // We avoid creating an encoding string unless we encounter some characters
  // that require it
  const char* utf8 = [inputStr UTF8String];
  if (utf8 == NULL) {
    return nil;
  }

  NSMutableString *encoded = nil;

  for (unsigned int idx = 0; utf8[idx] != '\0'; idx++) {

    unsigned char currChar = (unsigned char)utf8[idx];
    if (currChar < 0x20 || currChar == 0x25 || currChar > 0x7E) {

      if (encoded == nil) {
        // Start encoding and catch up on the character skipped so far
        encoded = [[[NSMutableString alloc] initWithBytes:utf8
                                                   length:idx
                                                 encoding:NSUTF8StringEncoding] autorelease];
      }

      // append this byte as a % and then uppercase hex
      [encoded appendFormat:@"%%%02X", currChar];

    } else {
      // This character does not need encoding
      //
      // Encoded is nil here unless we've encountered a previous character
      // that needed encoding
      [encoded appendFormat:@"%c", currChar];
    }
  }

  if (encoded) {
    return encoded;
  }

  return inputStr;
}

#pragma mark Key-Value Coding Searches in an Array

+ (NSArray *)objectsFromArray:(NSArray *)sourceArray
                    withValue:(id)desiredValue
                   forKeyPath:(NSString *)keyPath {
  // Step through all entries, get the value from
  // the key path, and see if it's equal to the
  // desired value
  NSMutableArray *results = [NSMutableArray array];

  for(id obj in sourceArray) {
    id val = [obj valueForKeyPath:keyPath];
    if (GTL_AreEqualOrBothNil(val, desiredValue)) {

      // found a match; add it to the results array
      [results addObject:obj];
    }
  }
  return results;
}

+ (id)firstObjectFromArray:(NSArray *)sourceArray
                 withValue:(id)desiredValue
                forKeyPath:(NSString *)keyPath {
  for (id obj in sourceArray) {
    id val = [obj valueForKeyPath:keyPath];
    if (GTL_AreEqualOrBothNil(val, desiredValue)) {
      // found a match; return it
      return obj;
    }
  }
  return nil;
}

#pragma mark Version helpers

// compareVersion compares two strings in 1.2.3.4 format
// missing fields are interpreted as zeros, so 1.2 = 1.2.0.0
+ (NSComparisonResult)compareVersion:(NSString *)ver1 toVersion:(NSString *)ver2 {

  static NSCharacterSet* dotSet = nil;
  if (dotSet == nil) {
    dotSet = [[NSCharacterSet characterSetWithCharactersInString:@"."] retain];
  }

  if (ver1 == nil) ver1 = @"";
  if (ver2 == nil) ver2 = @"";

  NSScanner* scanner1 = [NSScanner scannerWithString:ver1];
  NSScanner* scanner2 = [NSScanner scannerWithString:ver2];

  [scanner1 setCharactersToBeSkipped:dotSet];
  [scanner2 setCharactersToBeSkipped:dotSet];

  int partA1 = 0, partA2 = 0, partB1 = 0, partB2 = 0;
  int partC1 = 0, partC2 = 0, partD1 = 0, partD2 = 0;

  if ([scanner1 scanInt:&partA1] && [scanner1 scanInt:&partB1]
      && [scanner1 scanInt:&partC1] && [scanner1 scanInt:&partD1]) {
  }
  if ([scanner2 scanInt:&partA2] && [scanner2 scanInt:&partB2]
      && [scanner2 scanInt:&partC2] && [scanner2 scanInt:&partD2]) {
  }

  if (partA1 != partA2) return ((partA1 < partA2) ? NSOrderedAscending : NSOrderedDescending);
  if (partB1 != partB2) return ((partB1 < partB2) ? NSOrderedAscending : NSOrderedDescending);
  if (partC1 != partC2) return ((partC1 < partC2) ? NSOrderedAscending : NSOrderedDescending);
  if (partD1 != partD2) return ((partD1 < partD2) ? NSOrderedAscending : NSOrderedDescending);
  return NSOrderedSame;
}

#pragma mark URL builder

+ (NSURL *)URLWithString:(NSString *)urlString
         queryParameters:(NSDictionary *)queryParameters {
  if ([urlString length] == 0) return nil;

  NSString *fullURLString;
  if ([queryParameters count] > 0) {
    NSMutableArray *queryItems = [NSMutableArray arrayWithCapacity:[queryParameters count]];

    // sort the custom parameter keys so that we have deterministic parameter
    // order for unit tests
    NSArray *queryKeys = [queryParameters allKeys];
    NSArray *sortedQueryKeys = [queryKeys sortedArrayUsingSelector:@selector(caseInsensitiveCompare:)];

    for (NSString *paramKey in sortedQueryKeys) {
      NSString *paramValue = [queryParameters valueForKey:paramKey];

      NSString *paramItem = [NSString stringWithFormat:@"%@=%@",
                             [self stringByURLEncodingStringParameter:paramKey],
                             [self stringByURLEncodingStringParameter:paramValue]];

      [queryItems addObject:paramItem];
    }

    NSString *paramStr = [queryItems componentsJoinedByString:@"&"];

    BOOL hasQMark = ([urlString rangeOfString:@"?"].location == NSNotFound);
    char joiner = hasQMark ? '?' : '&';
    fullURLString = [NSString stringWithFormat:@"%@%c%@",
                     urlString, joiner, paramStr];
  } else {
    fullURLString = urlString;
  }
  NSURL *result = [NSURL URLWithString:fullURLString];
  return result;
}

#pragma mark Collections

+ (NSMutableDictionary *)newStaticDictionary {
  NSMutableDictionary *dict = [[NSMutableDictionary alloc] init];

  // make the dictionary ineligible for garbage collection
#if !GTL_IPHONE
  [[NSGarbageCollector defaultCollector] disableCollectorForPointer:dict];
#endif
  return dict;
}

+ (NSDictionary *)mergedClassDictionaryForSelector:(SEL)selector
                                        startClass:(Class)startClass
                                     ancestorClass:(Class)ancestorClass
                                             cache:(NSMutableDictionary *)cache {
  NSDictionary *result;
  @synchronized(cache) {
    result = [cache objectForKey:startClass];
    if (result == nil) {
      // Collect the class's dictionary.
      NSDictionary *classDict = [startClass performSelector:selector];

      // Collect the parent class's merged dictionary.
      NSDictionary *parentClassMergedDict;
      if ([startClass isEqual:ancestorClass]) {
        parentClassMergedDict = nil;
      } else {
        Class parentClass = class_getSuperclass(startClass);
        parentClassMergedDict =
          [GTLUtilities mergedClassDictionaryForSelector:selector
                                              startClass:parentClass
                                           ancestorClass:ancestorClass
                                                   cache:cache];
      }

      // Merge this class's into the parent's so things properly override.
      NSMutableDictionary *mergeDict;
      if (parentClassMergedDict != nil) {
        mergeDict =
          [NSMutableDictionary dictionaryWithDictionary:parentClassMergedDict];
      } else {
        mergeDict = [NSMutableDictionary dictionary];
      }
      if (classDict != nil) {
        [mergeDict addEntriesFromDictionary:classDict];
      }

      // Make an immutable version.
      result = [NSDictionary dictionaryWithDictionary:mergeDict];

      // Save it.
      [cache setObject:result forKey:(id<NSCopying>)startClass];
    }
  }
  return result;
}

@end

// isEqual: has the fatal flaw that it doesn't deal well with the receiver
// being nil. We'll use this utility instead.
BOOL GTL_AreEqualOrBothNil(id obj1, id obj2) {
  if (obj1 == obj2) {
    return YES;
  }
  if (obj1 && obj2) {
    BOOL areEqual = [obj1 isEqual:obj2];
    return areEqual;
  }
  return NO;
}

BOOL GTL_AreBoolsEqual(BOOL b1, BOOL b2) {
  // avoid comparison problems with boolean types by negating
  // both booleans
  return (!b1 == !b2);
}

NSNumber *GTL_EnsureNSNumber(NSNumber *num) {
  // If the server returned a string object where we expect a number, try
  // to make a number object.
  if ([num isKindOfClass:[NSString class]]) {
    NSNumber *newNum;
    NSString *str = (NSString *)num;
    if ([str rangeOfString:@"."].location != NSNotFound) {
      // This is a floating-point number.
      // Force the parser to use '.' as the decimal separator.
      static NSLocale *usLocale = nil;
      @synchronized([GTLUtilities class]) {
        if (usLocale == nil) {
          usLocale = [[NSLocale alloc] initWithLocaleIdentifier:@"en_US"];
        }
        newNum = [NSDecimalNumber decimalNumberWithString:(NSString*)num
                                                   locale:(id)usLocale];
      }
    } else {
      // NSDecimalNumber +decimalNumberWithString:locale:
      // does not correctly create an NSNumber for large values like
      // 71100000000007780.
      if ([str hasPrefix:@"-"]) {
        newNum = [NSNumber numberWithLongLong:[str longLongValue]];
      } else {
        const char *utf8 = [str UTF8String];
        unsigned long long ull = strtoull(utf8, NULL, 10);
        newNum = [NSNumber numberWithUnsignedLongLong:ull];
      }
    }
    if (newNum) {
      num = newNum;
    }
  }
  return num;
}
