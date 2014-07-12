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
//  GTLBatchResult.m
//

#import "GTLBatchResult.h"

#import "GTLErrorObject.h"

@implementation GTLBatchResult

@synthesize successes = successes_,
            failures = failures_;

- (id)copyWithZone:(NSZone *)zone {
  GTLBatchResult* newObject = [super copyWithZone:zone];
  newObject.successes = [[self.successes mutableCopyWithZone:zone] autorelease];
  newObject.failures = [[self.failures mutableCopyWithZone:zone] autorelease];
  return newObject;
}

- (void)dealloc {
  [successes_ release];
  [failures_ release];

  [super dealloc];
}

- (NSString *)description {
  return [NSString stringWithFormat:@"%@ %p (successes:%lu failures:%lu)",
          [self class], self,
          (unsigned long) [self.successes count],
          (unsigned long) [self.failures count]];
}

#pragma mark -

- (void)createItemsWithClassMap:(NSDictionary *)batchClassMap {
  // This is called by GTLObject objectForJSON:defaultClass:
  // JSON is defined to be a dictionary, but for batch results, it really
  // is any array.
  id json = self.JSON;
  GTL_DEBUG_ASSERT([json isKindOfClass:[NSArray class]],
                   @"didn't get an array for the batch results");
  NSArray *jsonArray = json;

  NSMutableDictionary *successes = [NSMutableDictionary dictionary];
  NSMutableDictionary *failures = [NSMutableDictionary dictionary];

  for (NSMutableDictionary *rpcResponse in jsonArray) {
    NSString *responseID = [rpcResponse objectForKey:@"id"];

    NSMutableDictionary *errorJSON = [rpcResponse objectForKey:@"error"];
    if (errorJSON) {
      GTLErrorObject *errorObject = [GTLErrorObject objectWithJSON:errorJSON];
      [failures setValue:errorObject forKey:responseID];
    } else {
      NSMutableDictionary *resultJSON = [rpcResponse objectForKey:@"result"];

      NSDictionary *surrogates = self.surrogates;
      Class defaultClass = [batchClassMap objectForKey:responseID];

      id resultObject = [[self class] objectForJSON:resultJSON
                                       defaultClass:defaultClass
                                         surrogates:surrogates
                                      batchClassMap:nil];
      if (resultObject == nil) {
        // methods like delete return no object
        resultObject = [NSNull null];
      }
      [successes setValue:resultObject forKey:responseID];
    }
  }
  self.successes = successes;
  self.failures = failures;
}

@end
