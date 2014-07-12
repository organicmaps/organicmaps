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
//  GTLErrorObject.m
//

#import "GTLErrorObject.h"
#import "GTLService.h"

@implementation GTLErrorObject

@dynamic code;
@dynamic message;
@dynamic data;

+ (NSDictionary *)arrayPropertyToClassMap {
  NSDictionary *map = [NSDictionary dictionaryWithObject:[GTLErrorObjectData class]
                                                  forKey:@"data"];
  return map;
}

- (NSError *)foundationError {
  NSMutableDictionary *userInfo;

  // This structured GTLErrorObject will be available in the error's userInfo
  // dictionary
  userInfo = [NSMutableDictionary dictionaryWithObject:self
                                                forKey:kGTLStructuredErrorKey];

  NSString *reasonStr = self.message;
  if (reasonStr) {
    // We always store an error in the userInfo key "error"
    [userInfo setObject:reasonStr
                 forKey:kGTLServerErrorStringKey];

    // Store a user-readable "reason" to show up when an error is logged,
    // in parentheses like NSError does it
    NSString *parenthesized = [NSString stringWithFormat:@"(%@)", reasonStr];
    [userInfo setObject:parenthesized
                 forKey:NSLocalizedFailureReasonErrorKey];
  }

  NSInteger code = [self.code integerValue];
  NSError *error = [NSError errorWithDomain:kGTLJSONRPCErrorDomain
                                       code:code
                                   userInfo:userInfo];
  return error;
}

+ (GTLErrorObject *)underlyingObjectForError:(NSError *)foundationError {
  NSDictionary *userInfo = [foundationError userInfo];
  GTLErrorObject *errorObj = [userInfo objectForKey:kGTLStructuredErrorKey];
  return errorObj;
}

@end

@implementation GTLErrorObjectData
@dynamic domain;
@dynamic reason;
@dynamic message;
@dynamic location;
@end


