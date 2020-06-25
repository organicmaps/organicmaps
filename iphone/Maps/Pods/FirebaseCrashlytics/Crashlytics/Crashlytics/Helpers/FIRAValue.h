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

typedef NS_ENUM(NSInteger, FIRAValueType) {
  kFIRAValueTypeDouble = 0,
  kFIRAValueTypeInteger,
  kFIRAValueTypeString,
};

@interface FIRAValue : NSObject <NSCopying>

/// The type of the value.
@property(nonatomic, readonly) FIRAValueType valueType;

#pragma mark - Double type.

/// Indicates whether the FIRAValue instance is a floating point.
@property(nonatomic, readonly) BOOL isDouble;

/// Float value. Check valueType to see if this attribute has float value.
@property(nonatomic, readonly) double doubleValue;

#pragma mark - Integer type.

/// Indicates whether the FIRAValue instance is an integer.
@property(nonatomic, readonly) BOOL isInt64;

/// Int64 value. Check valueType to see if this attribute has int64 value.
@property(nonatomic, readonly) int64_t int64Value;

#pragma mark - String type.

/// Indicates whether the FIRAValue instance is a string.
@property(nonatomic, readonly) BOOL isString;

/// String value. Check valueType to see if this attribute has string value.
@property(nonatomic, readonly) NSString *stringValue;

#pragma mark - Initializers.

/// Creates a @c FIRAValue if |object| is of type NSString or NSNumber. Returns |object| if it's
/// already a FIRAValue. Returns nil otherwise.
+ (instancetype)valueFromObject:(id)object;

/// Creates a @c FIRAValue with double value.
- (instancetype)initWithDouble:(double)value;

/// Creates a @c FIRAValue with int64 value.
- (instancetype)initWithInt64:(int64_t)value;

/// Creates a @c FIRAValue with string value.
- (instancetype)initWithString:(NSString *)stringValue;

- (instancetype)init NS_UNAVAILABLE;

@end
