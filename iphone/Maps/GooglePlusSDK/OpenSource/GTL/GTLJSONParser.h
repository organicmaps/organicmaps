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
//  GTLJSONParser.h
//

// This class is a thin wrapper around the JSON parser.  It uses
// NSJSONSerialization when available, and SBJSON otherwise.

#import <Foundation/Foundation.h>

#import "GTLDefines.h"

@interface GTLJSONParser : NSObject
+ (NSString*)stringWithObject:(id)value
                humanReadable:(BOOL)humanReadable
                        error:(NSError**)error;

+ (NSData *)dataWithObject:(id)obj
             humanReadable:(BOOL)humanReadable
                     error:(NSError**)error;

+ (id)objectWithString:(NSString *)jsonStr
                 error:(NSError **)error;

+ (id)objectWithData:(NSData *)jsonData
               error:(NSError **)error;
@end
