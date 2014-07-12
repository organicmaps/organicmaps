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
//  GTLRuntimeCommon.m
//

#include <objc/runtime.h>

#import "GTLRuntimeCommon.h"

#import "GTLDateTime.h"
#import "GTLObject.h"
#import "GTLUtilities.h"

static NSString *const kReturnClassKey = @"returnClass";
static NSString *const kContainedClassKey = @"containedClass";
static NSString *const kJSONKey = @"jsonKey";

// Note: NSObject's class is used as a marker for the expected/default class
// when Discovery says it can be any type of object.

@implementation GTLRuntimeCommon

// Helper to generically convert JSON to an api object type.
+ (id)objectFromJSON:(id)json
        defaultClass:(Class)defaultClass
          surrogates:(NSDictionary *)surrogates
         isCacheable:(BOOL*)isCacheable {
  id result = nil;
  BOOL canBeCached = YES;

  // TODO(TVL): use defaultClass to validate things like expectedClass is
  // done in jsonFromAPIObject:expectedClass:isCacheable:?

  if ([json isKindOfClass:[NSDictionary class]]) {
    // If no default, or the default was any object, then default to base
    // object here (and hope there is a kind to get the right thing).
    if ((defaultClass == Nil) || [defaultClass isEqual:[NSObject class]]) {
      defaultClass = [GTLObject class];
    }
    result = [GTLObject objectForJSON:json
                         defaultClass:defaultClass
                           surrogates:surrogates
                        batchClassMap:nil];
  } else if ([json isKindOfClass:[NSArray class]]) {
    NSArray *jsonArray = json;
    // make an object for each JSON dictionary in the array
    NSMutableArray *resultArray = [NSMutableArray arrayWithCapacity:[jsonArray count]];
    for (id jsonItem in jsonArray) {
      id item = [self objectFromJSON:jsonItem
                        defaultClass:defaultClass
                          surrogates:surrogates
                         isCacheable:NULL];
      [resultArray addObject:item];
    }
    result = resultArray;
  } else if ([json isKindOfClass:[NSString class]]) {
    // DateTimes live in JSON as strings, so convert
    if ([defaultClass isEqual:[GTLDateTime class]]) {
      result = [GTLDateTime dateTimeWithRFC3339String:json];
    } else {
      result = json;
      canBeCached = NO;
    }
  } else if ([json isKindOfClass:[NSNumber class]] ||
             [json isKindOfClass:[NSNull class]]) {
    result = json;
    canBeCached = NO;
  } else {
    GTL_DEBUG_LOG(@"GTLRuntimeCommon: unsupported class '%s' in objectFromJSON",
                  class_getName([json class]));
  }

  if (isCacheable) {
    *isCacheable = canBeCached;
  }
  return result;
}

// Helper to generically convert an api object type to JSON.
// |expectedClass| is the type that was expected for |obj|.
+ (id)jsonFromAPIObject:(id)obj
          expectedClass:(Class)expectedClass
            isCacheable:(BOOL*)isCacheable {
  id result = nil;
  BOOL canBeCached = YES;
  BOOL checkExpected = (expectedClass != Nil);

  if ([obj isKindOfClass:[NSString class]]) {
    result = [[obj copy] autorelease];
    canBeCached = NO;
  } else if ([obj isKindOfClass:[NSNumber class]] ||
             [obj isKindOfClass:[NSNull class]]) {
      result = obj;
      canBeCached = NO;
  } else if ([obj isKindOfClass:[GTLObject class]]) {
    result = [obj JSON];
    if (result == nil) {
      // adding an empty object; it should have a JSON dictionary so it can
      // hold future assignments
      [obj setJSON:[NSMutableDictionary dictionary]];
      result = [obj JSON];
    }
  } else if ([obj isKindOfClass:[NSArray class]]) {
    checkExpected = NO;
    NSArray *array = obj;
    // get the JSON for each thing in the array
    NSMutableArray *resultArray = [NSMutableArray arrayWithCapacity:[array count]];
    for (id item in array) {
      id itemJSON = [self jsonFromAPIObject:item
                              expectedClass:expectedClass
                                isCacheable:NULL];
      [resultArray addObject:itemJSON];
    }
    result = resultArray;
  } else if ([obj isKindOfClass:[GTLDateTime class]]) {
    // DateTimes live in JSON as strings, so convert.
    GTLDateTime *dateTime = obj;
    result = [dateTime stringValue];
  } else {
    checkExpected = NO;
    GTL_DEBUG_LOG(@"GTLRuntimeCommon: unsupported class '%s' in jsonFromAPIObject",
                  class_getName([obj class]));
  }

  if (checkExpected) {
    // If the default was any object, then clear it to skip validation checks.
    if ([expectedClass isEqual:[NSObject class]] ||
        [obj isKindOfClass:[NSNull class]]) {
      expectedClass = nil;
    }
    if (expectedClass && ![obj isKindOfClass:expectedClass]) {
      GTL_DEBUG_LOG(@"GTLRuntimeCommon: jsonFromAPIObject expected class '%s' instead got '%s'",
                    class_getName(expectedClass), class_getName([obj class]));
    }
  }

  if (isCacheable) {
    *isCacheable = canBeCached;
  }
  return result;
}

#pragma mark JSON/Object Utilities

static NSMutableDictionary *gDispatchCache = nil;

static CFStringRef SelectorKeyCopyDescriptionCallBack(const void *key) {
  // Make a CFString from the key
  NSString *name = NSStringFromSelector((SEL) key);
  CFStringRef str = CFStringCreateCopy(kCFAllocatorDefault, (CFStringRef) name);
  return str;
}

// Save the dispatch details for the specified class and selector
+ (void)setStoredDispatchForClass:(Class<GTLRuntimeCommon>)dispatchClass
                         selector:(SEL)sel
                      returnClass:(Class)returnClass
                   containedClass:(Class)containedClass
                          jsonKey:(NSString *)jsonKey {
  // cache structure:
  //   class ->
  //     selector ->
  //       returnClass
  //       containedClass
  //       jsonKey
  @synchronized([GTLRuntimeCommon class]) {
    if (gDispatchCache == nil) {
      gDispatchCache = [GTLUtilities newStaticDictionary];
    }

    CFMutableDictionaryRef classDict =
      (CFMutableDictionaryRef) [gDispatchCache objectForKey:dispatchClass];
    if (classDict == nil) {
      // We create a CFDictionary since the keys are raw selectors rather than
      // NSStrings
      const CFDictionaryKeyCallBacks keyCallBacks = {
        .version = 0,
        .retain = NULL,
        .release = NULL,
        .copyDescription = SelectorKeyCopyDescriptionCallBack,
        .equal = NULL,  // defaults to pointer comparison
        .hash = NULL    // defaults to the pointer value
      };
      const CFIndex capacity = 0; // no limit
      classDict = CFDictionaryCreateMutable(kCFAllocatorDefault, capacity,
                                            &keyCallBacks,
                                            &kCFTypeDictionaryValueCallBacks);
      [gDispatchCache setObject:(id)classDict
                         forKey:(id<NSCopying>)dispatchClass];
      CFRelease(classDict);
    }

    NSDictionary *selDict = (NSDictionary *)CFDictionaryGetValue(classDict, sel);
    if (selDict == nil) {
      selDict = [NSDictionary dictionaryWithObjectsAndKeys:
                 jsonKey, kJSONKey,
                 returnClass, kReturnClassKey, // can be nil (primitive types)
                 containedClass, kContainedClassKey, // may be nil
                 nil];
      CFDictionarySetValue(classDict, sel, selDict);
    } else {
      // we already have a dictionary for this selector on this class, which is
      // surprising
      GTL_DEBUG_LOG(@"Storing duplicate dispatch for %@ selector %@",
            dispatchClass, NSStringFromSelector(sel));
    }
  }
}

+ (BOOL)getStoredDispatchForClass:(Class<GTLRuntimeCommon>)dispatchClass
                         selector:(SEL)sel
                      returnClass:(Class *)outReturnClass
                   containedClass:(Class *)outContainedClass
                          jsonKey:(NSString **)outJsonKey {
  @synchronized([GTLRuntimeCommon class]) {
    // walk from this class up the hierarchy to the ancestor class
    Class<GTLRuntimeCommon> topClass = class_getSuperclass([dispatchClass ancestorClass]);
    for (Class currClass = dispatchClass;
         currClass != topClass;
         currClass = class_getSuperclass(currClass)) {

      CFMutableDictionaryRef classDict =
        (CFMutableDictionaryRef) [gDispatchCache objectForKey:currClass];
      if (classDict) {
        NSMutableDictionary *selDict =
          (NSMutableDictionary *) CFDictionaryGetValue(classDict, sel);
        if (selDict) {
          if (outReturnClass) {
            *outReturnClass = [selDict objectForKey:kReturnClassKey];
          }
          if (outContainedClass) {
            *outContainedClass = [selDict objectForKey:kContainedClassKey];
          }
          if (outJsonKey) {
            *outJsonKey = [selDict objectForKey:kJSONKey];
          }
          return YES;
        }
      }
    }
  }
  GTL_DEBUG_LOG(@"Failed to find stored dispatch info for %@ %s",
        dispatchClass, sel_getName(sel));
  return NO;
}

#pragma mark IMPs - getters and setters for specific object types

#if !__LP64__

// NSInteger on 32bit
static NSInteger DynamicInteger32Getter(id self, SEL sel) {
  // get an NSInteger (NSNumber) from the JSON dictionary
  NSString *jsonKey = nil;
  Class<GTLRuntimeCommon> selfClass = [self class];
  if ([GTLRuntimeCommon getStoredDispatchForClass:selfClass
                                         selector:sel
                                      returnClass:NULL
                                   containedClass:NULL
                                          jsonKey:&jsonKey]) {
    NSNumber *num = [self JSONValueForKey:jsonKey];
    num = GTL_EnsureNSNumber(num);
    NSInteger result = [num integerValue];
    return result;
  }
  return 0;
}

static void DynamicInteger32Setter(id self, SEL sel, NSInteger val) {
  // save an NSInteger (NSNumber) into the JSON dictionary
  NSString *jsonKey = nil;
  Class<GTLRuntimeCommon> selfClass = [self class];
  if ([GTLRuntimeCommon getStoredDispatchForClass:selfClass
                                         selector:sel
                                      returnClass:NULL
                                   containedClass:NULL
                                          jsonKey:&jsonKey]) {
    NSNumber *num = [NSNumber numberWithInteger:val];
    [self setJSONValue:num forKey:jsonKey];
  }
}

// NSUInteger on 32bit
static NSUInteger DynamicUInteger32Getter(id self, SEL sel) {
  // get an NSUInteger (NSNumber) from the JSON dictionary
  NSString *jsonKey = nil;
  Class<GTLRuntimeCommon> selfClass = [self class];
  if ([GTLRuntimeCommon getStoredDispatchForClass:selfClass
                                         selector:sel
                                      returnClass:NULL
                                   containedClass:NULL
                                          jsonKey:&jsonKey]) {
    NSNumber *num = [self JSONValueForKey:jsonKey];
    num = GTL_EnsureNSNumber(num);
    NSUInteger result = [num unsignedIntegerValue];
    return result;
  }
  return 0;
}

static void DynamicUInteger32Setter(id self, SEL sel, NSUInteger val) {
  // save an NSUInteger (NSNumber) into the JSON dictionary
  NSString *jsonKey = nil;
  Class<GTLRuntimeCommon> selfClass = [self class];
  if ([GTLRuntimeCommon getStoredDispatchForClass:selfClass
                                         selector:sel
                                      returnClass:NULL
                                   containedClass:NULL
                                          jsonKey:&jsonKey]) {
    NSNumber *num = [NSNumber numberWithUnsignedInteger:val];
    [self setJSONValue:num forKey:jsonKey];
  }
}

#endif  // !__LP64__

// NSInteger on 64bit, long long on 32bit and 64bit
static long long DynamicLongLongGetter(id self, SEL sel) {
  // get a long long (NSNumber) from the JSON dictionary
  NSString *jsonKey = nil;
  Class<GTLRuntimeCommon> selfClass = [self class];
  if ([GTLRuntimeCommon getStoredDispatchForClass:selfClass
                                         selector:sel
                                      returnClass:NULL
                                   containedClass:NULL
                                          jsonKey:&jsonKey]) {
    NSNumber *num = [self JSONValueForKey:jsonKey];
    num = GTL_EnsureNSNumber(num);
    long long result = [num longLongValue];
    return result;
  }
  return 0;
}

static void DynamicLongLongSetter(id self, SEL sel, long long val) {
  // save a long long (NSNumber) into the JSON dictionary
  NSString *jsonKey = nil;
  Class<GTLRuntimeCommon> selfClass = [self class];
  if ([GTLRuntimeCommon getStoredDispatchForClass:selfClass
                                         selector:sel
                                      returnClass:NULL
                                   containedClass:NULL
                                          jsonKey:&jsonKey]) {
    NSNumber *num = [NSNumber numberWithLongLong:val];
    [self setJSONValue:num forKey:jsonKey];
  }
}

// NSUInteger on 64bit, unsiged long long on 32bit and 64bit
static unsigned long long DynamicULongLongGetter(id self, SEL sel) {
  // get an unsigned long long (NSNumber) from the JSON dictionary
  NSString *jsonKey = nil;
  Class<GTLRuntimeCommon> selfClass = [self class];
  if ([GTLRuntimeCommon getStoredDispatchForClass:selfClass
                                         selector:sel
                                      returnClass:NULL
                                   containedClass:NULL
                                          jsonKey:&jsonKey]) {
    NSNumber *num = [self JSONValueForKey:jsonKey];
    num = GTL_EnsureNSNumber(num);
    unsigned long long result = [num unsignedLongLongValue];
    return result;
  }
  return 0;
}

static void DynamicULongLongSetter(id self, SEL sel, unsigned long long val) {
  // save an unsigned long long (NSNumber) into the JSON dictionary
  NSString *jsonKey = nil;
  Class<GTLRuntimeCommon> selfClass = [self class];
  if ([GTLRuntimeCommon getStoredDispatchForClass:selfClass
                                         selector:sel
                                      returnClass:NULL
                                   containedClass:NULL
                                          jsonKey:&jsonKey]) {
    NSNumber *num = [NSNumber numberWithUnsignedLongLong:val];
    [self setJSONValue:num forKey:jsonKey];
  }
}

// float
static float DynamicFloatGetter(id self, SEL sel) {
  // get a float (NSNumber) from the JSON dictionary
  NSString *jsonKey = nil;
  Class<GTLRuntimeCommon> selfClass = [self class];
  if ([GTLRuntimeCommon getStoredDispatchForClass:selfClass
                                         selector:sel
                                      returnClass:NULL
                                   containedClass:NULL
                                          jsonKey:&jsonKey]) {
    NSNumber *num = [self JSONValueForKey:jsonKey];
    num = GTL_EnsureNSNumber(num);
    float result = [num floatValue];
    return result;
  }
  return 0.0f;
}

static void DynamicFloatSetter(id self, SEL sel, float val) {
  // save a float (NSNumber) into the JSON dictionary
  NSString *jsonKey = nil;
  Class<GTLRuntimeCommon> selfClass = [self class];
  if ([GTLRuntimeCommon getStoredDispatchForClass:selfClass
                                         selector:sel
                                      returnClass:NULL
                                   containedClass:NULL
                                          jsonKey:&jsonKey]) {
    NSNumber *num = [NSNumber numberWithFloat:val];
    [self setJSONValue:num forKey:jsonKey];
  }
}

// double
static double DynamicDoubleGetter(id self, SEL sel) {
  // get a double (NSNumber) from the JSON dictionary
  NSString *jsonKey = nil;
  Class<GTLRuntimeCommon> selfClass = [self class];
  if ([GTLRuntimeCommon getStoredDispatchForClass:selfClass
                                         selector:sel
                                      returnClass:NULL
                                   containedClass:NULL
                                          jsonKey:&jsonKey]) {
    NSNumber *num = [self JSONValueForKey:jsonKey];
    num = GTL_EnsureNSNumber(num);
    double result = [num doubleValue];
    return result;
  }
  return 0.0;
}

static void DynamicDoubleSetter(id self, SEL sel, double val) {
  // save a double (NSNumber) into the JSON dictionary
  NSString *jsonKey = nil;
  Class<GTLRuntimeCommon> selfClass = [self class];
  if ([GTLRuntimeCommon getStoredDispatchForClass:selfClass
                                         selector:sel
                                      returnClass:NULL
                                   containedClass:NULL
                                          jsonKey:&jsonKey]) {
    NSNumber *num = [NSNumber numberWithDouble:val];
    [self setJSONValue:num forKey:jsonKey];
  }
}

// BOOL
static BOOL DynamicBooleanGetter(id self, SEL sel) {
  // get a BOOL (NSNumber) from the JSON dictionary
  NSString *jsonKey = nil;
  Class<GTLRuntimeCommon> selfClass = [self class];
  if ([GTLRuntimeCommon getStoredDispatchForClass:selfClass
                                         selector:sel
                                      returnClass:NULL
                                   containedClass:NULL
                                          jsonKey:&jsonKey]) {
    NSNumber *num = [self JSONValueForKey:jsonKey];
    BOOL flag = [num boolValue];
    return flag;
  }
  return NO;
}

static void DynamicBooleanSetter(id self, SEL sel, BOOL val) {
  // save a BOOL (NSNumber) into the JSON dictionary
  NSString *jsonKey = nil;
  Class<GTLRuntimeCommon> selfClass = [self class];
  if ([GTLRuntimeCommon getStoredDispatchForClass:selfClass
                                         selector:sel
                                      returnClass:NULL
                                   containedClass:NULL
                                          jsonKey:&jsonKey]) {
    NSNumber *num = [NSNumber numberWithBool:val];
    [self setJSONValue:num forKey:jsonKey];
  }
}

// NSString
static NSString *DynamicStringGetter(id<GTLRuntimeCommon> self, SEL sel) {
  // get an NSString from the JSON dictionary
  NSString *jsonKey = nil;
  Class<GTLRuntimeCommon> selfClass = [self class];
  if ([GTLRuntimeCommon getStoredDispatchForClass:selfClass
                                         selector:sel
                                      returnClass:NULL
                                   containedClass:NULL
                                          jsonKey:&jsonKey]) {

    NSString *str = [self JSONValueForKey:jsonKey];
    return str;
  }
  return nil;
}

static void DynamicStringSetter(id<GTLRuntimeCommon> self, SEL sel,
                                NSString *str) {
  // save an NSString into the JSON dictionary
  NSString *jsonKey = nil;
  Class<GTLRuntimeCommon> selfClass = [self class];
  if ([GTLRuntimeCommon getStoredDispatchForClass:selfClass
                                         selector:sel
                                      returnClass:NULL
                                   containedClass:NULL
                                          jsonKey:&jsonKey]) {
    NSString *copiedStr = [str copy];
    [self setJSONValue:copiedStr forKey:jsonKey];
    [copiedStr release];
  }
}

// GTLDateTime
static GTLDateTime *DynamicDateTimeGetter(id<GTLRuntimeCommon> self, SEL sel) {
  // get a GTLDateTime from the JSON dictionary
  NSString *jsonKey = nil;
  Class<GTLRuntimeCommon> selfClass = [self class];
  if ([GTLRuntimeCommon getStoredDispatchForClass:selfClass
                                         selector:sel
                                      returnClass:NULL
                                   containedClass:NULL
                                          jsonKey:&jsonKey]) {

    // Return the cached object before creating on demand.
    GTLDateTime *cachedDateTime = [self cacheChildForKey:jsonKey];
    if (cachedDateTime != nil) {
      return cachedDateTime;
    }
    NSString *str = [self JSONValueForKey:jsonKey];
    id cacheValue, resultValue;
    if (![str isKindOfClass:[NSNull class]]) {
      GTLDateTime *dateTime = [GTLDateTime dateTimeWithRFC3339String:str];

      cacheValue = dateTime;
      resultValue = dateTime;
    } else {
      cacheValue = nil;
      resultValue = [NSNull null];
    }
    [self setCacheChild:cacheValue forKey:jsonKey];
    return resultValue;
  }
  return nil;
}

static void DynamicDateTimeSetter(id<GTLRuntimeCommon> self, SEL sel,
                                  GTLDateTime *dateTime) {
  // save an GTLDateTime into the JSON dictionary
  NSString *jsonKey = nil;
  Class<GTLRuntimeCommon> selfClass = [self class];
  if ([GTLRuntimeCommon getStoredDispatchForClass:selfClass
                                         selector:sel
                                      returnClass:NULL
                                   containedClass:NULL
                                          jsonKey:&jsonKey]) {
    id cacheValue, jsonValue;
    if (![dateTime isKindOfClass:[NSNull class]]) {
      jsonValue = [dateTime stringValue];
      cacheValue = dateTime;
    } else {
      jsonValue = [NSNull null];
      cacheValue = nil;
    }

    [self setJSONValue:jsonValue forKey:jsonKey];
    [self setCacheChild:cacheValue forKey:jsonKey];
  }
}

// NSNumber
static NSNumber *DynamicNumberGetter(id<GTLRuntimeCommon> self, SEL sel) {
  // get an NSNumber from the JSON dictionary
  NSString *jsonKey = nil;
  Class<GTLRuntimeCommon> selfClass = [self class];
  if ([GTLRuntimeCommon getStoredDispatchForClass:selfClass
                                         selector:sel
                                      returnClass:NULL
                                   containedClass:NULL
                                          jsonKey:&jsonKey]) {

    NSNumber *num = [self JSONValueForKey:jsonKey];
    num = GTL_EnsureNSNumber(num);
    return num;
  }
  return nil;
}

static void DynamicNumberSetter(id<GTLRuntimeCommon> self, SEL sel,
                                NSNumber *num) {
  // save an NSNumber into the JSON dictionary
  NSString *jsonKey = nil;
  Class<GTLRuntimeCommon> selfClass = [self class];
  if ([GTLRuntimeCommon getStoredDispatchForClass:selfClass
                                         selector:sel
                                      returnClass:NULL
                                   containedClass:NULL
                                          jsonKey:&jsonKey]) {
    [self setJSONValue:num forKey:jsonKey];
  }
}

// GTLObject
static GTLObject *DynamicObjectGetter(id<GTLRuntimeCommon> self, SEL sel) {
  // get a GTLObject from the JSON dictionary
  NSString *jsonKey = nil;
  Class returnClass = nil;
  Class<GTLRuntimeCommon> selfClass = [self class];
  if ([GTLRuntimeCommon getStoredDispatchForClass:selfClass
                                         selector:sel
                                      returnClass:&returnClass
                                   containedClass:NULL
                                          jsonKey:&jsonKey]) {

    // Return the cached object before creating on demand.
    GTLObject *cachedObj = [self cacheChildForKey:jsonKey];
    if (cachedObj != nil) {
      return cachedObj;
    }
    NSMutableDictionary *dict = [self JSONValueForKey:jsonKey];
    if ([dict isKindOfClass:[NSMutableDictionary class]]) {
      // get the class of the object being returned, and instantiate it
      if (returnClass == Nil) {
        returnClass = [GTLObject class];
      }

      NSDictionary *surrogates = self.surrogates;
      GTLObject *obj = [GTLObject objectForJSON:dict
                                   defaultClass:returnClass
                                     surrogates:surrogates
                                  batchClassMap:nil];
      [self setCacheChild:obj forKey:jsonKey];
      return obj;
    } else if ([dict isKindOfClass:[NSNull class]]) {
      [self setCacheChild:nil forKey:jsonKey];
      return (id) [NSNull null];
    } else if (dict != nil) {
      // unexpected; probably got a string -- let the caller figure it out
      GTL_DEBUG_LOG(@"GTLObject: unexpected JSON: %@.%@ should be a dictionary, actually is a %@:\n%@",
                    NSStringFromClass(selfClass), NSStringFromSelector(sel),
                    NSStringFromClass([dict class]), dict);
      return (GTLObject *)dict;
    }
  }
  return nil;
}

static void DynamicObjectSetter(id<GTLRuntimeCommon> self, SEL sel,
                                GTLObject *obj) {
  // save a GTLObject into the JSON dictionary
  NSString *jsonKey = nil;
  Class<GTLRuntimeCommon> selfClass = [self class];
  if ([GTLRuntimeCommon getStoredDispatchForClass:selfClass
                                         selector:sel
                                      returnClass:NULL
                                   containedClass:NULL
                                          jsonKey:&jsonKey]) {
    id cacheValue, jsonValue;
    if (![obj isKindOfClass:[NSNull class]]) {
      NSMutableDictionary *dict = [obj JSON];
      if (dict == nil && obj != nil) {
        // adding an empty object; it should have a JSON dictionary so it can
        // hold future assignments
        obj.JSON = [NSMutableDictionary dictionary];
        jsonValue = obj.JSON;
      } else {
        jsonValue = dict;
      }
      cacheValue = obj;
    } else {
      jsonValue = [NSNull null];
      cacheValue = nil;
    }
    [self setJSONValue:jsonValue forKey:jsonKey];
    [self setCacheChild:cacheValue forKey:jsonKey];
  }
}

// get an NSArray of GTLObjects, NSStrings, or NSNumbers from the
// JSON dictionary for this object
static NSMutableArray *DynamicArrayGetter(id<GTLRuntimeCommon> self, SEL sel) {
  NSString *jsonKey = nil;
  Class containedClass = nil;
  Class selfClass = [self class];
  if ([GTLRuntimeCommon getStoredDispatchForClass:selfClass
                                         selector:sel
                                      returnClass:NULL
                                   containedClass:&containedClass
                                          jsonKey:&jsonKey]) {

    // Return the cached array before creating on demand.
    NSMutableArray *cachedArray = [self cacheChildForKey:jsonKey];
    if (cachedArray != nil) {
      return cachedArray;
    }
    NSMutableArray *result = nil;
    NSArray *array = [self JSONValueForKey:jsonKey];
    if (array != nil) {
      if ([array isKindOfClass:[NSArray class]]) {
        NSDictionary *surrogates = self.surrogates;
        result = [GTLRuntimeCommon objectFromJSON:array
                                     defaultClass:containedClass
                                       surrogates:surrogates
                                      isCacheable:NULL];
      } else {
#if DEBUG
        if (![array isKindOfClass:[NSNull class]]) {
          GTL_DEBUG_LOG(@"GTLObject: unexpected JSON: %@.%@ should be an array, actually is a %@:\n%@",
                        NSStringFromClass(selfClass), NSStringFromSelector(sel),
                        NSStringFromClass([array class]), array);
        }
#endif
        result = (NSMutableArray *)array;
      }
    }

    [self setCacheChild:result forKey:jsonKey];
    return result;
  }
  return nil;
}

static void DynamicArraySetter(id<GTLRuntimeCommon> self, SEL sel,
                               NSMutableArray *array) {
  // save an array of GTLObjects objects into the JSON dictionary
  NSString *jsonKey = nil;
  Class selfClass = [self class];
  Class containedClass = nil;
  if ([GTLRuntimeCommon getStoredDispatchForClass:selfClass
                                         selector:sel
                                      returnClass:NULL
                                   containedClass:&containedClass
                                          jsonKey:&jsonKey]) {
    id json = [GTLRuntimeCommon jsonFromAPIObject:array
                                    expectedClass:containedClass
                                      isCacheable:NULL];
    [self setJSONValue:json forKey:jsonKey];
    [self setCacheChild:array forKey:jsonKey];
  }
}

// type 'id'
static id DynamicNSObjectGetter(id<GTLRuntimeCommon> self, SEL sel) {
  NSString *jsonKey = nil;
  Class returnClass = nil;
  Class selfClass = [self class];
  if ([GTLRuntimeCommon getStoredDispatchForClass:selfClass
                                         selector:sel
                                      returnClass:&returnClass
                                   containedClass:NULL
                                          jsonKey:&jsonKey]) {

    // Return the cached object before creating on demand.
    id cachedObj = [self cacheChildForKey:jsonKey];
    if (cachedObj != nil) {
      return cachedObj;
    }

    id jsonObj = [self JSONValueForKey:jsonKey];
    if (jsonObj != nil) {
      BOOL shouldCache = NO;
      NSDictionary *surrogates = self.surrogates;
      id result = [GTLRuntimeCommon objectFromJSON:jsonObj
                                      defaultClass:nil
                                        surrogates:surrogates
                                       isCacheable:&shouldCache];

      [self setCacheChild:(shouldCache ? result : nil)
                   forKey:jsonKey];
      return result;
    }
  }
  return nil;
}

static void DynamicNSObjectSetter(id<GTLRuntimeCommon> self, SEL sel, id obj) {
  NSString *jsonKey = nil;
  Class selfClass = [self class];
  if ([GTLRuntimeCommon getStoredDispatchForClass:selfClass
                                         selector:sel
                                      returnClass:NULL
                                   containedClass:NULL
                                          jsonKey:&jsonKey]) {
    BOOL shouldCache = NO;
    id json = [GTLRuntimeCommon jsonFromAPIObject:obj
                                    expectedClass:Nil
                                      isCacheable:&shouldCache];
    [self setJSONValue:json forKey:jsonKey];
    [self setCacheChild:(shouldCache ? obj : nil)
                 forKey:jsonKey];
  }
}

#pragma mark Runtime lookup support

static objc_property_t PropertyForSel(Class<GTLRuntimeCommon> startClass,
                                      SEL sel, BOOL isSetter,
                                      Class<GTLRuntimeCommon> *outFoundClass) {
  const char *baseName = sel_getName(sel);
  size_t baseNameLen = strlen(baseName);
  if (isSetter) {
    baseName += 3;    // skip "set"
    baseNameLen -= 4; // subtract "set" and the final colon
  }

  // walk from this class up the hierarchy to the ancestor class
  Class<GTLRuntimeCommon> topClass = class_getSuperclass([startClass ancestorClass]);
  for (Class currClass = startClass;
       currClass != topClass;
       currClass = class_getSuperclass(currClass)) {
    // step through this class's properties
    objc_property_t foundProp = NULL;
    objc_property_t *properties = class_copyPropertyList(currClass, NULL);
    if (properties) {
      for (objc_property_t *prop = properties; *prop != NULL; ++prop) {
        const char *propName = property_getName(*prop);
        size_t propNameLen = strlen(propName);

        // search for an exact-name match (a getter), but case-insensitive on the
        // first character (in case baseName comes from a setter)
        if (baseNameLen == propNameLen
            && strncasecmp(baseName, propName, 1) == 0
            && (baseNameLen <= 1
                || strncmp(baseName + 1, propName + 1, baseNameLen - 1) == 0)) {
              // return the actual property name
              foundProp = *prop;

              // if requested, return the class containing the property
              if (outFoundClass) *outFoundClass = currClass;
              break;
            }
      }
      free(properties);
    }
    if (foundProp) return foundProp;
  }

  // not found; this occasionally happens when the system looks for a method
  // like "getFoo" or "descriptionWithLocale:indent:"
  return NULL;
}

typedef struct {
  const char *attributePrefix;

  const char *setterEncoding;
  IMP         setterFunction;
  const char *getterEncoding;
  IMP         getterFunction;

  // These are the "fixed" return classes, but some properties will require
  // looking up the return class instead (because it is a subclass of
  // GTLObject).
  const char *returnClassName;
  Class       returnClass;
  BOOL extractReturnClass;

} GTLDynamicImpInfo;

static const GTLDynamicImpInfo *DynamicImpInfoForProperty(objc_property_t prop,
                                                          Class *outReturnClass) {

  if (outReturnClass) *outReturnClass = nil;

  // dynamic method resolution:
  // http://developer.apple.com/library/ios/#documentation/Cocoa/Conceptual/ObjCRuntimeGuide/Articles/ocrtDynamicResolution.html
  //
  // property runtimes:
  // http://developer.apple.com/library/ios/#documentation/Cocoa/Conceptual/ObjCRuntimeGuide/Articles/ocrtPropertyIntrospection.html

  // Get and parse the property attributes, which look something like
  //   T@"NSString",&,D,P
  //   Ti,D -- NSInteger on 32bit
  //   Tq,D -- NSInteger on 64bit, long long on 32bit & 64bit
  //   Tc,D -- BOOL comes as char
  //   T@"NSString",D
  //   T@"GTLLink",D
  //   T@"NSArray",D


  static GTLDynamicImpInfo kImplInfo[] = {
#if !__LP64__
    { // NSInteger on 32bit
      "Ti",
      "v@:i", (IMP)DynamicInteger32Setter,
      "i@:", (IMP)DynamicInteger32Getter,
      nil, nil,
      NO
    },
    { // NSUInteger on 32bit
      "TI",
      "v@:I", (IMP)DynamicUInteger32Setter,
      "I@:", (IMP)DynamicUInteger32Getter,
      nil, nil,
      NO
    },
#endif
    { // NSInteger on 64bit, long long on 32bit and 64bit.
      "Tq",
      "v@:q", (IMP)DynamicLongLongSetter,
      "q@:", (IMP)DynamicLongLongGetter,
      nil, nil,
      NO
    },
    { // NSUInteger on 64bit, long long on 32bit and 64bit.
      "TQ",
      "v@:Q", (IMP)DynamicULongLongSetter,
      "Q@:", (IMP)DynamicULongLongGetter,
      nil, nil,
      NO
    },
    { // float
      "Tf",
      "v@:f", (IMP)DynamicFloatSetter,
      "f@:", (IMP)DynamicFloatGetter,
      nil, nil,
      NO
    },
    { // double
      "Td",
      "v@:d", (IMP)DynamicDoubleSetter,
      "d@:", (IMP)DynamicDoubleGetter,
      nil, nil,
      NO
    },
    { // BOOL
      "Tc",
      "v@:c", (IMP)DynamicBooleanSetter,
      "c@:", (IMP)DynamicBooleanGetter,
      nil, nil,
      NO
    },
    { // NSString
      "T@\"NSString\"",
      "v@:@", (IMP)DynamicStringSetter,
      "@@:", (IMP)DynamicStringGetter,
      "NSString", nil,
      NO
    },
    { // NSNumber
      "T@\"NSNumber\"",
      "v@:@", (IMP)DynamicNumberSetter,
      "@@:", (IMP)DynamicNumberGetter,
      "NSNumber", nil,
      NO
    },
    { // GTLDateTime
#if !defined(GTL_TARGET_NAMESPACE)
      "T@\"GTLDateTime\"",
      "v@:@", (IMP)DynamicDateTimeSetter,
      "@@:", (IMP)DynamicDateTimeGetter,
      "GTLDateTime", nil,
      NO
#else
      "T@\"" GTL_TARGET_NAMESPACE_STRING "_" "GTLDateTime\"",
      "v@:@", (IMP)DynamicDateTimeSetter,
      "@@:", (IMP)DynamicDateTimeGetter,
      GTL_TARGET_NAMESPACE_STRING "_" "GTLDateTime", nil,
      NO
#endif
    },
    { // NSArray with type
      "T@\"NSArray\"",
      "v@:@", (IMP)DynamicArraySetter,
      "@@:", (IMP)DynamicArrayGetter,
      "NSArray", nil,
      NO
    },
    { // id (any of the objects above)
      "T@,",
      "v@:@", (IMP)DynamicNSObjectSetter,
      "@@:", (IMP)DynamicNSObjectGetter,
      "NSObject", nil,
      NO
    },
    { // GTLObject - Last, cause it's a special case and prefix is general
      "T@\"",
      "v@:@", (IMP)DynamicObjectSetter,
      "@@:", (IMP)DynamicObjectGetter,
      nil, nil,
      YES
    },
  };

  static BOOL hasLookedUpClasses = NO;
  if (!hasLookedUpClasses) {
    // Unfortunately, you can't put [NSString class] into the static structure,
    // so this lookup has to be done at runtime.
    hasLookedUpClasses = YES;
    for (uint32_t idx = 0; idx < sizeof(kImplInfo)/sizeof(kImplInfo[0]); ++idx) {
      if (kImplInfo[idx].returnClassName) {
        kImplInfo[idx].returnClass = objc_getClass(kImplInfo[idx].returnClassName);
        NSCAssert1(kImplInfo[idx].returnClass != nil,
                   @"GTLRuntimeCommon: class lookup failed: %s", kImplInfo[idx].returnClassName);
      }
    }
  }

  const char *attr = property_getAttributes(prop);

  const char *dynamicMarker = strstr(attr, ",D");
  if (!dynamicMarker ||
      (dynamicMarker[2] != 0 && dynamicMarker[2] != ',' )) {
    GTL_DEBUG_LOG(@"GTLRuntimeCommon: property %s isn't dynamic, attributes %s",
                  property_getName(prop), attr ? attr : "(nil)");
    return NULL;
  }

  const GTLDynamicImpInfo *result = NULL;

  // Cycle over the list

  for (uint32_t idx = 0; idx < sizeof(kImplInfo)/sizeof(kImplInfo[0]); ++idx) {
    const char *attributePrefix = kImplInfo[idx].attributePrefix;
    if (strncmp(attr, attributePrefix, strlen(attributePrefix)) == 0) {
      result = &kImplInfo[idx];
      if (outReturnClass) *outReturnClass = result->returnClass;
      break;
    }
  }

  if (result == NULL) {
    GTL_DEBUG_LOG(@"GTLRuntimeCommon: unexpected attributes %s for property %s",
                  attr ? attr : "(nil)", property_getName(prop));
    return NULL;
  }

  if (result->extractReturnClass && outReturnClass) {

    // add a null at the next quotation mark
    char *attrCopy = strdup(attr);
    char *classNameStart = attrCopy + 3;
    char *classNameEnd = strstr(classNameStart, "\"");
    if (classNameEnd) {
      *classNameEnd = '\0';

      // Lookup the return class
      *outReturnClass = objc_getClass(classNameStart);
      if (*outReturnClass == nil) {
        GTL_DEBUG_LOG(@"GTLRuntimeCommon: did not find class with name \"%s\" "
                      "for property \"%s\" with attributes \"%s\"",
                      classNameStart, property_getName(prop), attr);
      }
    } else {
      GTL_DEBUG_LOG(@"GTLRuntimeCommon: Failed to find end of class name for "
                    "property \"%s\" with attributes \"%s\"",
                    property_getName(prop), attr);
    }
    free(attrCopy);
  }

  return result;
}

#pragma mark Runtime - wiring point

+ (BOOL)resolveInstanceMethod:(SEL)sel onClass:(Class<GTLRuntimeCommon>)onClass {
  // dynamic method resolution:
  // http://developer.apple.com/library/ios/#documentation/Cocoa/Conceptual/ObjCRuntimeGuide/Articles/ocrtDynamicResolution.html
  //
  // property runtimes:
  // http://developer.apple.com/library/ios/#documentation/Cocoa/Conceptual/ObjCRuntimeGuide/Articles/ocrtPropertyIntrospection.html

  const char *selName = sel_getName(sel);
  size_t selNameLen = strlen(selName);
  char lastChar = selName[selNameLen - 1];
  BOOL isSetter = (lastChar == ':');

  // look for a declared property matching this selector name exactly
  Class<GTLRuntimeCommon> foundClass = nil;

  objc_property_t prop = PropertyForSel(onClass, sel, isSetter, &foundClass);
  if (prop != NULL && foundClass != nil) {

    Class returnClass = nil;
    const GTLDynamicImpInfo *implInfo = DynamicImpInfoForProperty(prop,
                                                                  &returnClass);
    if (implInfo == NULL) {
      GTL_DEBUG_LOG(@"GTLRuntimeCommon: unexpected return type class %s for "
                      "property \"%s\" of class \"%s\"",
                      returnClass ? class_getName(returnClass) : "<nil>",
                      property_getName(prop),
                      class_getName(onClass));
    }

    if (implInfo != NULL) {
      IMP imp = ( isSetter ? implInfo->setterFunction : implInfo->getterFunction );
      const char *encoding = ( isSetter ? implInfo->setterEncoding : implInfo->getterEncoding );

      class_addMethod(foundClass, sel, imp, encoding);

      const char *propName = property_getName(prop);
      NSString *propStr = [NSString stringWithUTF8String:propName];

      // replace the property name with the proper JSON key if it's
      // special-cased with a map in the found class; otherwise, the property
      // name is the JSON key
      NSDictionary *keyMap =
        [[foundClass ancestorClass] propertyToJSONKeyMapForClass:foundClass];
      NSString *jsonKey = [keyMap objectForKey:propStr];
      if (jsonKey == nil) {
        jsonKey = propStr;
      }

      Class containedClass = nil;

      // For arrays we need to look up what the contained class is.
      if (imp == (IMP)DynamicArraySetter || imp == (IMP)DynamicArrayGetter) {
        NSDictionary *classMap =
          [[foundClass ancestorClass] arrayPropertyToClassMapForClass:foundClass];
        containedClass = [classMap objectForKey:jsonKey];
        if (containedClass == Nil) {
          GTL_DEBUG_LOG(@"GTLRuntimeCommon: expected array item class for "
                        "property \"%s\" of class \"%s\"",
                        property_getName(prop), class_getName(foundClass));
        }
      }

      // save the dispatch info to the cache
      [GTLRuntimeCommon setStoredDispatchForClass:foundClass
                                         selector:sel
                                      returnClass:returnClass
                                   containedClass:containedClass
                                          jsonKey:jsonKey];
      return YES;
    }
  }

  return NO;
}

@end
