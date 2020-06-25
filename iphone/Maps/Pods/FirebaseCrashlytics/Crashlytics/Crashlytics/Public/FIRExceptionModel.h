// Copyright 2020 Google
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

#import "FIRStackFrame.h"

NS_ASSUME_NONNULL_BEGIN

/**
 * The Firebase Crashlytics Exception Model provides a way to report custom exceptions
 * to Crashlytics that came from a runtime environment outside of the native
 * platform Crashlytics is running in.
 */
NS_SWIFT_NAME(ExceptionModel)
@interface FIRExceptionModel : NSObject

/** :nodoc: */
- (instancetype)init NS_UNAVAILABLE;

/**
 * Initializes an Exception Model model with the given required fields.
 *
 * @param name - typically the type of the Exception class
 * @param reason - the human-readable reason the issue occurred
 */
- (instancetype)initWithName:(NSString *)name reason:(NSString *)reason;

/**
 * Creates an Exception Model model with the given required fields.
 *
 * @param name - typically the type of the Exception class
 * @param reason - the human-readable reason the issue occurred
 */
+ (instancetype)exceptionModelWithName:(NSString *)name
                                reason:(NSString *)reason NS_SWIFT_UNAVAILABLE("");

/**
 * A list of Stack Frames that make up the stack trace. The order of the stack trace is top-first,
 * so typically the "main" function is the last element in this list.
 */
@property(nonatomic, copy) NSArray<FIRStackFrame *> *stackTrace;

@end

NS_ASSUME_NONNULL_END
