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
//  GTLQuery.h
//

// Query documentation:
// https://code.google.com/p/google-api-objectivec-client/wiki/Introduction#Query_Operations

#import "GTLObject.h"
#import "GTLUploadParameters.h"

@protocol GTLQueryProtocol <NSObject, NSCopying>
- (BOOL)isBatchQuery;
- (BOOL)shouldSkipAuthorization;
- (void)executionDidStop;
- (NSDictionary *)additionalHTTPHeaders;
- (GTLUploadParameters *)uploadParameters;
@end

@protocol GTLQueryCollectionProtocol
@optional
@property (retain) NSString *pageToken;
@property (retain) NSNumber *startIndex;
@end

@class GTLServiceTicket;

@interface GTLQuery : NSObject <GTLQueryProtocol> {
 @private
  NSString *methodName_;
  NSMutableDictionary *json_;
  GTLObject *bodyObject_;
  NSMutableDictionary *childCache_;
  NSString *requestID_;
  GTLUploadParameters *uploadParameters_;
  NSDictionary *urlQueryParameters_;
  NSDictionary *additionalHTTPHeaders_;
  Class expectedObjectClass_;
  BOOL skipAuthorization_;
#if NS_BLOCKS_AVAILABLE
  void (^completionBlock_)(GTLServiceTicket *ticket, id object, NSError *error);
#elif !__LP64__
  // Placeholders: for 32-bit builds, keep the size of the object's ivar section
  // the same with and without blocks
  id completionPlaceholder_;
#endif
}

// The rpc method name.
@property (readonly) NSString *methodName;

// The JSON dictionary of all the parameters set on this query.
@property (retain) NSMutableDictionary *JSON;

// The object set to be uploaded with the query.
@property (retain) GTLObject *bodyObject;

// Each query must have a request ID string. The user may replace the
// default assigned request ID with a custom string, provided that if
// used in a batch query, all request IDs in the batch must be unique.
@property (copy) NSString *requestID;

// For queries which support file upload, the MIME type and file handle
// or data must be provided.
@property (copy) GTLUploadParameters *uploadParameters;

// Any url query parameters to add to the query (useful for debugging with some
// services).
@property (copy) NSDictionary *urlQueryParameters;

// Any additional HTTP headers for this query.  Not valid when this query
// is added to a batch.
//
// These headers override the same keys from the service object's
// additionalHTTPHeaders.
@property (copy) NSDictionary *additionalHTTPHeaders;

// The GTLObject subclass expected for results (used if the result doesn't
// include a kind attribute).
@property (assign) Class expectedObjectClass;

// Clients may set this to YES to disallow authorization. Defaults to NO.
@property (assign) BOOL shouldSkipAuthorization;

#if NS_BLOCKS_AVAILABLE
// Clients may provide an optional callback block to be called immediately
// before the executeQuery: callback.
//
// The completionBlock property is particularly useful for queries executed
// in a batch.
//
// Errors passed to the completionBlock will have an "underlying" GTLErrorObject
// when the server returned an error for this specific query:
//
//   GTLErrorObject *errorObj = [GTLErrorObject underlyingObjectForError:error];
//   if (errorObj) {
//     // the server returned this error for this specific query
//   } else {
//     // the batch execution failed
//   }
@property (copy) void (^completionBlock)(GTLServiceTicket *ticket, id object, NSError *error);
#endif

// methodName is the RPC method name to use.
+ (id)queryWithMethodName:(NSString *)methodName GTL_NONNULL((1));

// methodName is the RPC method name to use.
- (id)initWithMethodName:(NSString *)method GTL_NONNULL((1));

// If you need to set a parameter that is not listed as a property for a
// query class, you can do so via this api.  If you need to clear it after
// setting, pass nil for obj.
- (void)setCustomParameter:(id)obj forKey:(NSString *)key GTL_NONNULL((2));

// Auto-generated request IDs
+ (NSString *)nextRequestID;

// Methods for subclasses to override.
+ (NSDictionary *)parameterNameMap;
+ (NSDictionary *)arrayPropertyToClassMap;
@end
