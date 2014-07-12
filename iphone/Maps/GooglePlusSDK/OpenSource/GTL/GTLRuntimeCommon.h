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
//  GTLRuntimeCommon.h
//


#import <Foundation/Foundation.h>

#import "GTLDefines.h"

// This protocol and support class are an internal implementation detail so
// GTLObject and GTLQuery can share some code.

@protocol GTLRuntimeCommon <NSObject>
@required
// Get/Set properties
- (void)setJSONValue:(id)obj forKey:(NSString *)key;
- (id)JSONValueForKey:(NSString *)key;
// Child cache
- (void)setCacheChild:(id)obj forKey:(NSString *)key;
- (id)cacheChildForKey:(NSString *)key;
// Surrogate class mappings.
- (NSDictionary *)surrogates;
// Key map
+ (NSDictionary *)propertyToJSONKeyMapForClass:(Class<GTLRuntimeCommon>)aClass;
// Array item types
+ (NSDictionary *)arrayPropertyToClassMapForClass:(Class<GTLRuntimeCommon>)aClass;
// The parent class for dynamic support
+ (Class<GTLRuntimeCommon>)ancestorClass;
@end

@interface GTLRuntimeCommon : NSObject
// Wire things up.
+ (BOOL)resolveInstanceMethod:(SEL)sel onClass:(Class)onClass;
// Helpers
+ (id)objectFromJSON:(id)json
        defaultClass:(Class)defaultClass
          surrogates:(NSDictionary *)surrogates
         isCacheable:(BOOL*)isCacheable;
+ (id)jsonFromAPIObject:(id)obj
          expectedClass:(Class)expectedClass
            isCacheable:(BOOL*)isCacheable;
@end
