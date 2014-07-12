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
//  GTLBatchQuery.m
//

#import "GTLBatchQuery.h"

@implementation GTLBatchQuery

@synthesize shouldSkipAuthorization = skipAuthorization_,
            additionalHTTPHeaders = additionalHTTPHeaders_;

+ (id)batchQuery {
  GTLBatchQuery *obj = [[[self alloc] init] autorelease];
  return obj;
}

+ (id)batchQueryWithQueries:(NSArray *)queries {
  GTLBatchQuery *obj = [self batchQuery];
  obj.queries = queries;
  return obj;
}

- (id)copyWithZone:(NSZone *)zone {
  // Deep copy the list of queries
  NSArray *copiesOfQueries = [[[NSArray alloc] initWithArray:self.queries
                                                   copyItems:YES] autorelease];
  GTLBatchQuery *newBatch = [[[self class] allocWithZone:zone] init];
  newBatch.queries = copiesOfQueries;
  newBatch.shouldSkipAuthorization = self.shouldSkipAuthorization;
  newBatch.additionalHTTPHeaders = self.additionalHTTPHeaders;
  return newBatch;
}

- (void)dealloc {
  [queries_ release];
  [additionalHTTPHeaders_ release];
  [requestIDMap_ release];

  [super dealloc];
}

- (NSString *)description {
  NSArray *queries = self.queries;
  NSArray *methodNames = [queries valueForKey:@"methodName"];
  NSArray *dedupedNames = [[NSSet setWithArray:methodNames] allObjects];
  NSString *namesStr = [dedupedNames componentsJoinedByString:@","];

  return [NSString stringWithFormat:@"%@ %p (queries:%lu methods:%@)",
          [self class], self, (unsigned long) [queries count], namesStr];
}

#pragma mark -

- (BOOL)isBatchQuery {
  return YES;
}

- (GTLUploadParameters *)uploadParameters {
  // File upload is not supported for batches
  return nil;
}

- (void)executionDidStop {
  NSArray *queries = self.queries;
  [queries makeObjectsPerformSelector:@selector(executionDidStop)];
}

- (GTLQuery *)queryForRequestID:(NSString *)requestID {
  GTLQuery *result = [requestIDMap_ objectForKey:requestID];
  if (result) return result;

  // We've not before tried to look up a query, or the map is stale
  [requestIDMap_ release];
  requestIDMap_ = [[NSMutableDictionary alloc] init];

  for (GTLQuery *query in queries_) {
    [requestIDMap_ setObject:query forKey:query.requestID];
  }

  result = [requestIDMap_ objectForKey:requestID];
  return result;
}

#pragma mark -

- (void)setQueries:(NSArray *)array {
#if DEBUG
  for (id obj in array) {
    GTLQuery *query = obj;
    GTL_DEBUG_ASSERT([query isKindOfClass:[GTLQuery class]],
                     @"unexpected query class: %@", [obj class]);
    GTL_DEBUG_ASSERT(query.uploadParameters == nil,
                     @"batch may not contain upload: %@", query);
  }
#endif

  [queries_ autorelease];
  queries_ = [array mutableCopy];
}

- (NSArray *)queries {
  return queries_;
}

- (void)addQuery:(GTLQuery *)query {
  GTL_DEBUG_ASSERT([query isKindOfClass:[GTLQuery class]],
                   @"unexpected query class: %@", [query class]);
  GTL_DEBUG_ASSERT(query.uploadParameters == nil,
                   @"batch may not contain upload: %@", query);

  if (queries_ == nil) {
    queries_ = [[NSMutableArray alloc] init];
  }

  [queries_ addObject:query];
}

@end
