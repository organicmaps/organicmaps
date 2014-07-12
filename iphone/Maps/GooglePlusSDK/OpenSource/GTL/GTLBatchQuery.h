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
//  GTLBatchQuery.h
//

//  Batch query documentation:
//  https://code.google.com/p/google-api-objectivec-client/wiki/Introduction#Batch_Operations

#import "GTLQuery.h"

@interface GTLBatchQuery : NSObject <GTLQueryProtocol> {
 @private
  NSMutableArray *queries_;
  NSMutableDictionary *requestIDMap_;
  BOOL skipAuthorization_;
  NSDictionary *additionalHTTPHeaders_;
}

// Queries included in this batch.  Each query should have a unique requestID.
@property (retain) NSArray *queries;

// Clients may set this to YES to disallow authorization. Defaults to NO.
@property (assign) BOOL shouldSkipAuthorization;

// Any additional HTTP headers for this batch.
//
// These headers override the same keys from the service object's
// additionalHTTPHeaders.
@property (copy) NSDictionary *additionalHTTPHeaders;

+ (id)batchQuery;
+ (id)batchQueryWithQueries:(NSArray *)array;

- (void)addQuery:(GTLQuery *)query GTL_NONNULL((1));

- (GTLQuery *)queryForRequestID:(NSString *)requestID GTL_NONNULL((1));

@end
