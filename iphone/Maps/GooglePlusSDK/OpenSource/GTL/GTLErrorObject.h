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
//  GTLErrorObject.h
//

#import "GTLObject.h"

@class GTLErrorObjectData;

@interface GTLErrorObject : GTLObject
@property (retain) NSNumber *code;
@property (retain) NSString *message;
@property (retain) NSArray *data; // of GTLErrorObjectData

// Convenience accessor for creating an NSError from a GTLErrorObject.
@property (readonly) NSError *foundationError;

// Convenience accessor for extracting the GTLErrorObject that was used to
// create an NSError.
//
// Returns nil if the error was not originally from a GTLErrorObject.
+ (GTLErrorObject *)underlyingObjectForError:(NSError *)foundationError;

@end

@interface GTLErrorObjectData : GTLObject
@property (retain) NSString *domain;
@property (retain) NSString *reason;
@property (retain) NSString *message;
@property (retain) NSString *location;
@end
