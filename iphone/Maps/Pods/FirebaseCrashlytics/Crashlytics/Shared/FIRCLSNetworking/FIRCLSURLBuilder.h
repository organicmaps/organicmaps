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

/**
 * This is a convenience class to ease constructing NSURLs.
 */
@interface FIRCLSURLBuilder : NSObject

/**
 * Convenience method that returns a FIRCLSURLBuilder instance with the input base URL appended to
 * it.
 */
+ (instancetype)URLWithBase:(NSString *)base;
/**
 * Appends the component to the URL being built by FIRCLSURLBuilder instance
 */
- (void)appendComponent:(NSString *)component;
/**
 * Escapes and appends the component to the URL being built by FIRCLSURLBuilder instance
 */
- (void)escapeAndAppendComponent:(NSString *)component;
/**
 * Adds a query and value to the URL being built
 */
- (void)appendValue:(id)value forQueryParam:(NSString *)param;
/**
 * Returns the built URL
 */
- (NSURL *)URL;

@end
