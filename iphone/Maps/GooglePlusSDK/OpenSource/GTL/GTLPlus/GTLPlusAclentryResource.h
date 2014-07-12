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
//  GTLPlusAclentryResource.h
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
//   GTLPlusAclentryResource (0 custom class methods, 3 custom properties)

#if GTL_BUILT_AS_FRAMEWORK
  #import "GTL/GTLObject.h"
#else
  #import "GTLObject.h"
#endif

// ----------------------------------------------------------------------------
//
//   GTLPlusAclentryResource
//

@interface GTLPlusAclentryResource : GTLObject

// A descriptive name for this entry. Suitable for display.
@property (copy) NSString *displayName;

// The ID of the entry. For entries of type "person" or "circle", this is the ID
// of the resource. For other types, this property is not set.
// identifier property maps to 'id' in JSON (to avoid Objective C's 'id').
@property (copy) NSString *identifier;

// The type of entry describing to whom access is granted. Possible values are:
// - "person" - Access to an individual.
// - "circle" - Access to members of a circle.
// - "myCircles" - Access to members of all the person's circles.
// - "extendedCircles" - Access to members of everyone in a person's circles,
// plus all of the people in their circles.
// - "public" - Access to anyone on the web.
@property (copy) NSString *type;

@end
