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

NS_ASSUME_NONNULL_BEGIN

/**
 * The Firebase Crashlytics Stack Frame provides a way to construct the lines of
 * a stack trace for reporting along with a recorded Exception Model.
 */
NS_SWIFT_NAME(StackFrame)
@interface FIRStackFrame : NSObject

/** :nodoc: */
- (instancetype)init NS_UNAVAILABLE;

/**
 * Initializes a symbolicated Stack Frame with the given required fields. Symbolicated
 * Stack Frames will appear in the Crashlytics dashboard as reported in these fields.
 *
 * @param symbol - The function or method name
 * @param file - the file where the exception occurred
 * @param line - the line number
 */
- (instancetype)initWithSymbol:(NSString *)symbol file:(NSString *)file line:(NSInteger)line;

/**
 * Creates a symbolicated Stack Frame with the given required fields. Symbolicated
 * Stack Frames will appear in the Crashlytics dashboard as reported in these fields. *
 *
 * @param symbol - The function or method name
 * @param file - the file where the exception occurred
 * @param line - the line number
 */
+ (instancetype)stackFrameWithSymbol:(NSString *)symbol
                                file:(NSString *)file
                                line:(NSInteger)line NS_SWIFT_UNAVAILABLE("");

@end

NS_ASSUME_NONNULL_END
