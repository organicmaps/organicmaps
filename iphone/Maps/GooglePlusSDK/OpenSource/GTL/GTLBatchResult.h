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
//  GTLBatchResult.h
//

#import "GTLObject.h"

@interface GTLBatchResult : GTLObject <GTLBatchItemCreationProtocol> {
 @private
  NSMutableDictionary *successes_;
  NSMutableDictionary *failures_;
}

// Dictionaries of results for all queries in the batch
//
// Dictionary keys are requestID strings; objects are results or
// GTLErrorObjects.
//
// For successes with no returned object (such as from delete operations),
// the object for the dictionary entry is NSNull.
//
//
// The original query for each result is available from the service ticket,
// for example
//
// NSDictionary *successes = batchResults.successes;
// for (NSString *requestID in successes) {
//   GTLObject *obj = [successes objectForKey:requestID];
//   GTLQuery *query = [ticket queryForRequestID:requestID];
//   NSLog(@"Query %@ returned object %@", query, obj);
// }
//
// NSDictionary *failures = batchResults.failures;
// for (NSString *requestID in failures) {
//   GTLErrorObject *errorObj = [failures objectForKey:requestID];
//   GTLQuery *query = [ticket queryForRequestID:requestID];
//   NSLog(@"Query %@ failed with error %@", query, errorObj);
// }
//

@property (retain) NSMutableDictionary *successes;
@property (retain) NSMutableDictionary *failures;

@end
