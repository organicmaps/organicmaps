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
//  GTLObject.m
//

#define GTLOBJECT_DEFINE_GLOBALS 1

#include <objc/runtime.h>

#import "GTLObject.h"
#import "GTLRuntimeCommon.h"
#import "GTLJSONParser.h"

static NSString *const kUserDataPropertyKey = @"_userData";

@interface GTLObject () <GTLRuntimeCommon>
+ (NSMutableArray *)allDeclaredProperties;
+ (NSArray *)allKnownKeys;

+ (NSArray *)fieldsElementsForJSON:(NSDictionary *)targetJSON;
+ (NSString *)fieldsDescriptionForJSON:(NSDictionary *)targetJSON;

+ (NSMutableDictionary *)patchDictionaryForJSON:(NSDictionary *)newJSON
                               fromOriginalJSON:(NSDictionary *)originalJSON;
@end

@implementation GTLObject

@synthesize JSON = json_,
            surrogates = surrogates_,
            userProperties = userProperties_;

+ (id)object {
  return [[[self alloc] init] autorelease];
}

+ (id)objectWithJSON:(NSMutableDictionary *)dict {
  GTLObject *obj = [self object];
  obj.JSON = dict;
  return obj;
}

+ (NSDictionary *)propertyToJSONKeyMap {
  return nil;
}

+ (NSDictionary *)arrayPropertyToClassMap {
  return nil;
}

+ (Class)classForAdditionalProperties {
  return Nil;
}

- (BOOL)isEqual:(GTLObject *)other {
  if (self == other) return YES;
  if (other == nil) return NO;

  // The objects should be the same class, or one should be a subclass of the
  // other's class
  if (![other isKindOfClass:[self class]]
      && ![self isKindOfClass:[other class]]) return NO;

  // What we're not comparing here:
  //   properties
  return GTL_AreEqualOrBothNil(json_, [other JSON]);
}

// By definition, for two objects to potentially be considered equal,
// they must have the same hash value.  The hash is mostly ignored,
// but removeObjectsInArray: in Leopard does seem to check the hash,
// and NSObject's default hash method just returns the instance pointer.
// We'll define hash here for all of our GTLObjects.
- (NSUInteger)hash {
  return (NSUInteger) (void *) [GTLObject class];
}

- (id)copyWithZone:(NSZone *)zone {
  GTLObject* newObject = [[[self class] allocWithZone:zone] init];
  CFPropertyListRef ref = CFPropertyListCreateDeepCopy(kCFAllocatorDefault,
                    json_, kCFPropertyListMutableContainers);
  GTL_DEBUG_ASSERT(ref != NULL, @"GTLObject: copy failed (probably a non-plist type in the JSON)");
  newObject.JSON = [NSMakeCollectable(ref) autorelease];
  newObject.surrogates = self.surrogates;

  // What we're not copying:
  //   userProperties
  return newObject;
}

- (NSString *)descriptionWithLocale:(id)locale {
  return [self description];
}

- (void)dealloc {
  [json_ release];
  [surrogates_ release];
  [childCache_ release];
  [userProperties_ release];

  [super dealloc];
}

#pragma mark JSON values

- (void)setJSONValue:(id)obj forKey:(NSString *)key {
  NSMutableDictionary *dict = self.JSON;
  if (dict == nil && obj != nil) {
    dict = [NSMutableDictionary dictionaryWithCapacity:1];
    self.JSON = dict;
  }
  [dict setValue:obj forKey:key];
}

- (id)JSONValueForKey:(NSString *)key {
  id obj = [self.JSON objectForKey:key];
  return obj;
}

- (NSString *)JSONString {
  NSError *error = nil;
  NSString *str = [GTLJSONParser stringWithObject:[self JSON]
                                    humanReadable:YES
                                            error:&error];
  if (error) {
    return [error description];
  }
  return str;
}

- (NSArray *)additionalJSONKeys {
  NSArray *knownKeys = [[self class] allKnownKeys];
  NSMutableArray *result = [NSMutableArray arrayWithArray:[json_ allKeys]];
  [result removeObjectsInArray:knownKeys];
  // Return nil instead of an empty array.
  if ([result count] == 0) {
    result = nil;
  }
  return result;
}

#pragma mark Partial - Fields

- (NSString *)fieldsDescription {
  NSString *str = [GTLObject fieldsDescriptionForJSON:self.JSON];
  return str;
}

+ (NSString *)fieldsDescriptionForJSON:(NSDictionary *)targetJSON {
  // Internal routine: recursively generate a string field description
  // by joining elements
  NSArray *array = [self fieldsElementsForJSON:targetJSON];
  NSString *str = [array componentsJoinedByString:@","];
  return str;
}

+ (NSArray *)fieldsElementsForJSON:(NSDictionary *)targetJSON {
  // Internal routine: recursively generate an array of field description
  // element strings
  NSMutableArray *resultFields = [NSMutableArray array];

  // Sorting the dictionary keys gives us deterministic results when iterating
  NSArray *sortedKeys = [[targetJSON allKeys] sortedArrayUsingSelector:@selector(caseInsensitiveCompare:)];
  for (NSString *key in sortedKeys) {
    // We'll build a comma-separated list of fields
    id value = [targetJSON objectForKey:key];
    if ([value isKindOfClass:[NSString class]]
        || [value isKindOfClass:[NSNumber class]]) {
      // Basic type (string, number), so the key is what we want
      [resultFields addObject:key];
    } else if ([value isKindOfClass:[NSDictionary class]]) {
      // Object (dictionary): "parent/child1,parent/child2,parent/child3"
      NSArray *subElements = [self fieldsElementsForJSON:value];
      for (NSString *subElem in subElements) {
        NSString *prepended = [NSString stringWithFormat:@"%@/%@",
                               key, subElem];
        [resultFields addObject:prepended];
      }
    } else if ([value isKindOfClass:[NSArray class]]) {
      // Array; we'll generate from the first array entry:
      // "parent(child1,child2,child3)"
      //
      // Open question: should this instead create the union of elements for
      // all items in the array, rather than just get fields from the first
      // array object?
      if ([(NSArray *)value count] > 0) {
        id firstObj = [value objectAtIndex:0];
        if ([firstObj isKindOfClass:[NSDictionary class]]) {
          // An array of objects
          NSString *contentsStr = [self fieldsDescriptionForJSON:firstObj];
          NSString *encapsulated = [NSString stringWithFormat:@"%@(%@)",
                                    key, contentsStr];
          [resultFields addObject:encapsulated];
        } else {
          // An array of some basic type, or of arrays
          [resultFields addObject:key];
        }
      }
    } else {
      GTL_ASSERT(0, @"GTLObject unknown field element for %@ (%@)",
                 key, NSStringFromClass([value class]));
    }
  }
  return resultFields;
}

#pragma mark Partial - Patch

- (id)patchObjectFromOriginal:(GTLObject *)original {
  id resultObj;
  NSMutableDictionary *resultJSON = [GTLObject patchDictionaryForJSON:self.JSON
                                                     fromOriginalJSON:original.JSON];
  if ([resultJSON count] > 0) {
    resultObj = [[self class] objectWithJSON:resultJSON];
  } else {
    // Client apps should not attempt to patch with an object containing
    // empty JSON
    resultObj = nil;
  }
  return resultObj;
}

+ (NSMutableDictionary *)patchDictionaryForJSON:(NSDictionary *)newJSON
                               fromOriginalJSON:(NSDictionary *)originalJSON {
  // Internal recursive routine to create an object suitable for
  // our patch semantics
  NSMutableDictionary *resultJSON = [NSMutableDictionary dictionary];

  // Iterate through keys present in the old object
  NSArray *originalKeys = [originalJSON allKeys];
  for (NSString *key in originalKeys) {
    id originalValue = [originalJSON objectForKey:key];
    id newValue = [newJSON valueForKey:key];
    if (newValue == nil) {
      // There is no new value for this key, so set the value to NSNull
      [resultJSON setValue:[NSNull null] forKey:key];
    } else if (!GTL_AreEqualOrBothNil(originalValue, newValue)) {
      // The values for this key differ
      if ([originalValue isKindOfClass:[NSDictionary class]]
          && [newValue isKindOfClass:[NSDictionary class]]) {
        // Both are objects; recurse
        NSMutableDictionary *subDict = [self patchDictionaryForJSON:newValue
                                                   fromOriginalJSON:originalValue];
        [resultJSON setValue:subDict forKey:key];
      } else {
        // They are non-object values; the new replaces the old. Per the
        // documentation for patch, this replaces entire arrays.
        [resultJSON setValue:newValue forKey:key];
      }
    } else {
      // The values are the same; omit this key-value pair
    }
  }

  // Iterate through keys present only in the new object, and add them to the
  // result
  NSMutableArray *newKeys = [NSMutableArray arrayWithArray:[newJSON allKeys]];
  [newKeys removeObjectsInArray:originalKeys];

  for (NSString *key in newKeys) {
    id value = [newJSON objectForKey:key];
    [resultJSON setValue:value forKey:key];
  }
  return resultJSON;
}

+ (id)nullValue {
  return [NSNull null];
}

#pragma mark Additional Properties

- (id)additionalPropertyForName:(NSString *)name {
  // Return the cached object, if any, before creating one.
  id result = [self cacheChildForKey:name];
  if (result != nil) {
    return result;
  }

  Class defaultClass = [[self class] classForAdditionalProperties];
  id jsonObj = [self JSONValueForKey:name];
  BOOL shouldCache = NO;
  if (jsonObj != nil) {
    NSDictionary *surrogates = self.surrogates;
    result = [GTLRuntimeCommon objectFromJSON:jsonObj
                                 defaultClass:defaultClass
                                   surrogates:surrogates
                                  isCacheable:&shouldCache];
  }

  [self setCacheChild:(shouldCache ? result : nil)
               forKey:name];
  return result;
}

- (void)setAdditionalProperty:(id)obj forName:(NSString *)name {
  BOOL shouldCache = NO;
  Class defaultClass = [[self class] classForAdditionalProperties];
  id json = [GTLRuntimeCommon jsonFromAPIObject:obj
                                  expectedClass:defaultClass
                                    isCacheable:&shouldCache];
  [self setJSONValue:json forKey:name];
  [self setCacheChild:(shouldCache ? obj : nil)
               forKey:name];
}

- (NSDictionary *)additionalProperties {
  NSMutableDictionary *result = [NSMutableDictionary dictionary];

  NSArray *propertyNames = [self additionalJSONKeys];
  for (NSString *name in propertyNames) {
    id obj = [self additionalPropertyForName:name];
    [result setObject:obj forKey:name];
  }

  return result;
}

#pragma mark Child Cache methods

// There is no property for childCache_ as there shouldn't be KVC/KVO
// support for it, it's an implementation detail.

- (void)setCacheChild:(id)obj forKey:(NSString *)key {
  if (childCache_ == nil && obj != nil) {
    childCache_ = [[NSMutableDictionary alloc] initWithObjectsAndKeys:
                   obj, key, nil];
  } else {
    [childCache_ setValue:obj forKey:key];
  }
}

- (id)cacheChildForKey:(NSString *)key {
  id obj = [childCache_ objectForKey:key];
  return obj;
}

#pragma mark userData and user properties

- (void)setUserData:(id)userData {
  [self setProperty:userData forKey:kUserDataPropertyKey];
}

- (id)userData {
  // be sure the returned pointer has the life of the autorelease pool,
  // in case self is released immediately
  return [[[self propertyForKey:kUserDataPropertyKey] retain] autorelease];
}

- (void)setProperty:(id)obj forKey:(NSString *)key {
  if (obj == nil) {
    // user passed in nil, so delete the property
    [userProperties_ removeObjectForKey:key];
  } else {
    // be sure the property dictionary exists
    if (userProperties_ == nil) {
      self.userProperties = [NSMutableDictionary dictionary];
    }
    [userProperties_ setObject:obj forKey:key];
  }
}

- (id)propertyForKey:(NSString *)key {
  id obj = [userProperties_ objectForKey:key];

  // be sure the returned pointer has the life of the autorelease pool,
  // in case self is released immediately
  return [[obj retain] autorelease];
}

#pragma mark Support methods

+ (NSMutableArray *)allDeclaredProperties {
  NSMutableArray *array = [NSMutableArray array];

  // walk from this class up the hierarchy to GTLObject
  Class topClass = class_getSuperclass([GTLObject class]);
  for (Class currClass = self;
       currClass != topClass;
       currClass = class_getSuperclass(currClass)) {
    // step through this class's properties, and add the property names to the
    // array
    objc_property_t *properties = class_copyPropertyList(currClass, NULL);
    if (properties) {
      for (objc_property_t *prop = properties;
           *prop != NULL;
           ++prop) {
        const char *propName = property_getName(*prop);
        // We only want dynamic properties; their attributes contain ",D".
        const char *attr = property_getAttributes(*prop);
        const char *dynamicMarker = strstr(attr, ",D");
        if (dynamicMarker &&
            (dynamicMarker[2] == 0 || dynamicMarker[2] == ',' )) {
          [array addObject:[NSString stringWithUTF8String:propName]];
        }
      }
      free(properties);
    }
  }
  return array;
}

+ (NSArray *)allKnownKeys {
  NSArray *allProps = [self allDeclaredProperties];
  NSMutableArray *knownKeys = [NSMutableArray arrayWithArray:allProps];

  NSDictionary *propMap = [GTLObject propertyToJSONKeyMapForClass:[self class]];

  NSUInteger idx = 0;
  for (NSString *propName in allProps) {
    NSString *jsonKey = [propMap objectForKey:propName];
    if (jsonKey) {
      [knownKeys replaceObjectAtIndex:idx
                           withObject:jsonKey];
    }
    ++idx;
  }
  return knownKeys;
}

- (NSString *)description {
  // find the list of declared and otherwise known JSON keys for this class
  NSArray *knownKeys = [[self class] allKnownKeys];

  NSMutableString *descStr = [NSMutableString string];

  NSString *spacer = @"";
  for (NSString *key in json_) {
    NSString *value = nil;
    // show question mark for JSON keys not supported by a declared property:
    //   foo?:"Hi mom."
    NSString *qmark = [knownKeys containsObject:key] ? @"" : @"?";

    // determine property value to dislay
    id rawValue = [json_ valueForKey:key];
    if ([rawValue isKindOfClass:[NSDictionary class]]) {
      // for dictionaries, show the list of keys:
      //   {key1,key2,key3}
      NSString *subkeyList = [[rawValue allKeys] componentsJoinedByString:@","];
      value = [NSString stringWithFormat:@"{%@}", subkeyList];
    } else if ([rawValue isKindOfClass:[NSArray class]]) {
      // for arrays, show the number of items in the array:
      //   [3]
      value = [NSString stringWithFormat:@"[%lu]", (unsigned long)[(NSArray *)rawValue count]];
    } else if ([rawValue isKindOfClass:[NSString class]]) {
      // for strings, show the string in quotes:
      //   "Hi mom."
      value = [NSString stringWithFormat:@"\"%@\"", rawValue];
    } else {
      // for numbers, show just the number
      value = [rawValue description];
    }
    [descStr appendFormat:@"%@%@%@:%@", spacer, key, qmark, value];
    spacer = @" ";
  }

  NSString *str = [NSString stringWithFormat:@"%@ %p: {%@}",
                   [self class], self, descStr];
  return str;
}

#pragma mark Class Registration

static NSMutableDictionary *gKindMap = nil;

+ (Class)registeredObjectClassForKind:(NSString *)kind {
  Class resultClass = [gKindMap objectForKey:kind];
  return resultClass;
}

+ (void)registerObjectClassForKind:(NSString *)kind {
  // there's no autorelease pool in place at +load time, so we'll create our own
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

  if (gKindMap == nil) {
    gKindMap = [GTLUtilities newStaticDictionary];
  }

  Class selfClass = [self class];

#if DEBUG
  // ensure this is a unique registration
  if ([gKindMap objectForKey:kind] != nil ) {
    GTL_DEBUG_LOG(@"%@ (%@) registration conflicts with %@",
                  selfClass, kind, [gKindMap objectForKey:kind]);
  }
  if ([[gKindMap allKeysForObject:selfClass] count] != 0) {
    GTL_DEBUG_LOG(@"%@ (%@) registration conflicts with %@",
                  selfClass, kind, [gKindMap allKeysForObject:selfClass]);
  }
#endif

  [gKindMap setValue:selfClass forKey:kind];

  // we drain here to keep the clang static analyzer quiet
  [pool drain];
}

#pragma mark Object Instantiation

+ (GTLObject *)objectForJSON:(NSMutableDictionary *)json
                defaultClass:(Class)defaultClass
                  surrogates:(NSDictionary *)surrogates
               batchClassMap:(NSDictionary *)batchClassMap {
  if ([json count] == 0 || [json isEqual:[NSNull null]]) {
    // no actual result, such as the response from a delete
    return nil;
  }

  // Determine the class to instantiate, based on the original fetch
  // request or by looking up "kind" string from the registration at
  // +load time of GTLObject subclasses
  //
  // We're letting the dynamic kind override the default class so
  // feeds of heterogenous entries can use the defaultClass as a
  // fallback
  Class classToCreate = defaultClass;
  NSString *kind = nil;
  if ([json isKindOfClass:[NSDictionary class]]) {
    kind = [json valueForKey:@"kind"];
    if ([kind isKindOfClass:[NSString class]] && [kind length] > 0) {
      Class dynamicClass = [GTLObject registeredObjectClassForKind:kind];
      if (dynamicClass) {
        classToCreate = dynamicClass;
      }
    }
  }

  // Warn the developer that no specific class of GTLObject
  // was requested with the fetch call, and no class is found
  // compiled in to match the "kind" attribute of the JSON
  // returned by the server
  GTL_ASSERT(classToCreate != nil,
             @"Could not find registered GTLObject subclass to "
             "match JSON with kind \"%@\"", kind);

  if (classToCreate == nil) {
    classToCreate = [self class];
  }

  // See if the top-level class for the JSON is listed in the surrogates;
  // if so, instantiate the surrogate class instead
  Class baseSurrogate = [surrogates objectForKey:classToCreate];
  if (baseSurrogate) {
    classToCreate = baseSurrogate;
  }

  // now instantiate the GTLObject
  GTLObject *parsedObject = [classToCreate object];

  parsedObject.surrogates = surrogates;
  parsedObject.JSON = json;

  // it's time to instantiate inner items
  if ([parsedObject conformsToProtocol:@protocol(GTLBatchItemCreationProtocol)]) {
    id <GTLBatchItemCreationProtocol> batch =
      (id <GTLBatchItemCreationProtocol>) parsedObject;
    [batch createItemsWithClassMap:batchClassMap];
  }

  return parsedObject;
}

#pragma mark Runtime Utilities

static NSMutableDictionary *gJSONKeyMapCache = nil;
static NSMutableDictionary *gArrayPropertyToClassMapCache = nil;

+ (void)initialize {
  // Note that initialize is guaranteed by the runtime to be called in a
  // thread-safe manner
  if (gJSONKeyMapCache == nil) {
    gJSONKeyMapCache = [GTLUtilities newStaticDictionary];
  }
  if (gArrayPropertyToClassMapCache == nil) {
    gArrayPropertyToClassMapCache = [GTLUtilities newStaticDictionary];
  }
}

+ (NSDictionary *)propertyToJSONKeyMapForClass:(Class<GTLRuntimeCommon>)aClass {
  NSDictionary *resultMap =
    [GTLUtilities mergedClassDictionaryForSelector:@selector(propertyToJSONKeyMap)
                                        startClass:aClass
                                     ancestorClass:[GTLObject class]
                                             cache:gJSONKeyMapCache];
  return resultMap;
}

+ (NSDictionary *)arrayPropertyToClassMapForClass:(Class<GTLRuntimeCommon>)aClass {
  NSDictionary *resultMap =
    [GTLUtilities mergedClassDictionaryForSelector:@selector(arrayPropertyToClassMap)
                                        startClass:aClass
                                     ancestorClass:[GTLObject class]
                                             cache:gArrayPropertyToClassMapCache];
  return resultMap;
}

#pragma mark Runtime Support

+ (Class<GTLRuntimeCommon>)ancestorClass {
  return [GTLObject class];
}

+ (BOOL)resolveInstanceMethod:(SEL)sel {
  BOOL resolved = [GTLRuntimeCommon resolveInstanceMethod:sel onClass:self];
  if (resolved)
    return YES;

  return [super resolveInstanceMethod:sel];
}

@end

@implementation GTLCollectionObject
// Subclasses must implement the items method dynamically.

- (void)dealloc {
  [identifierMap_ release];
  [super dealloc];
}

- (id)itemAtIndex:(NSUInteger)idx {
  NSArray *items = [self performSelector:@selector(items)];
  if (idx < [items count]) {
    return [items objectAtIndex:idx];
  }
  return nil;
}

- (id)objectAtIndexedSubscript:(NSInteger)idx {
  if (idx >= 0) {
    return [self itemAtIndex:(NSUInteger)idx];
  }
  return nil;
}

- (id)itemForIdentifier:(NSString *)key {
  if (identifierMap_ == nil) {
    NSArray *items = [self performSelector:@selector(items)];
    NSMutableDictionary *dict =
      [NSMutableDictionary dictionaryWithCapacity:[items count]];
    for (id item in items) {
      id identifier = [item valueForKey:@"identifier"];
      if (identifier != nil && identifier != [NSNull null]) {
        if ([dict objectForKey:identifier] == nil) {
          [dict setObject:item forKey:identifier];
        }
      }
    }
    identifierMap_ = [dict copy];
  }
  return [identifierMap_ objectForKey:key];
}

- (void)resetIdentifierMap {
  [identifierMap_ release];
  identifierMap_ = nil;
}

// NSFastEnumeration protocol
- (NSUInteger)countByEnumeratingWithState:(NSFastEnumerationState *)state
                                  objects:(id *)stackbuf
                                    count:(NSUInteger)len {
  NSArray *items = [self performSelector:@selector(items)];
  NSUInteger result = [items countByEnumeratingWithState:state
                                                 objects:stackbuf
                                                   count:len];
  return result;
}

@end

@implementation GTLResultArray

- (NSArray *)itemsWithItemClass:(Class)itemClass {
  // Return the cached array before creating on demand.
  NSString *cacheKey = @"result_array_items";
  NSMutableArray *cachedArray = [self cacheChildForKey:cacheKey];
  if (cachedArray != nil) {
    return cachedArray;
  }
  NSArray *result = nil;
  NSArray *array = (NSArray *)[self JSON];
  if (array != nil) {
    if ([array isKindOfClass:[NSArray class]]) {
      NSDictionary *surrogates = self.surrogates;
      result = [GTLRuntimeCommon objectFromJSON:array
                                   defaultClass:itemClass
                                     surrogates:surrogates
                                    isCacheable:NULL];
    } else {
#if DEBUG
      if (![array isKindOfClass:[NSNull class]]) {
        GTL_DEBUG_LOG(@"GTLObject: unexpected JSON: %@ should be an array, actually is a %@:\n%@",
                      NSStringFromClass([self class]),
                      NSStringFromClass([array class]),
                      array);
      }
#endif
      result = array;
    }
  }

  [self setCacheChild:result forKey:cacheKey];
  return result;
}

@end
