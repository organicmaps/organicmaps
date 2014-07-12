/* Copyright (c) 2013 Google Inc.
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
//  GTLPlusPeopleFeed.h
//

// ----------------------------------------------------------------------------
// NOTE: This file is generated from Google APIs Discovery Service.
// Service:
//   Google+ API (plus/v1)
// Description:
//   The Google+ API enables developers to build on top of the Google+ platform.
// Documentation:
//   https://developers.google.com/+/api/
// Classes:
//   GTLPlusPeopleFeed (0 custom class methods, 7 custom properties)

#if GTL_BUILT_AS_FRAMEWORK
  #import "GTL/GTLObject.h"
#else
  #import "GTLObject.h"
#endif

@class GTLPlusPerson;

// ----------------------------------------------------------------------------
//
//   GTLPlusPeopleFeed
//

// This class supports NSFastEnumeration over its "items" property. It also
// supports -itemAtIndex: to retrieve individual objects from "items".

@interface GTLPlusPeopleFeed : GTLCollectionObject

// ETag of this response for caching purposes.
@property (copy) NSString *ETag;

// The people in this page of results. Each item includes the id, displayName,
// image, and url for the person. To retrieve additional profile data, see the
// people.get method.
@property (retain) NSArray *items;  // of GTLPlusPerson

// Identifies this resource as a collection of people. Value: "plus#peopleFeed".
@property (copy) NSString *kind;

// The continuation token, which is used to page through large result sets.
// Provide this value in a subsequent request to return the next page of
// results.
@property (copy) NSString *nextPageToken;

// Link to this resource.
@property (copy) NSString *selfLink;

// The title of this collection of people.
@property (copy) NSString *title;

// The total number of people available in this list. The number of people in a
// response might be smaller due to paging. This might not be set for all
// collections.
@property (retain) NSNumber *totalItems;  // intValue

@end
