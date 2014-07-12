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
//  GTLPlusActivity.m
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
//   GTLPlusActivity (0 custom class methods, 19 custom properties)
//   GTLPlusActivityActor (0 custom class methods, 5 custom properties)
//   GTLPlusActivityObject (0 custom class methods, 10 custom properties)
//   GTLPlusActivityProvider (0 custom class methods, 1 custom properties)
//   GTLPlusActivityActorImage (0 custom class methods, 1 custom properties)
//   GTLPlusActivityActorName (0 custom class methods, 2 custom properties)
//   GTLPlusActivityObjectActor (0 custom class methods, 4 custom properties)
//   GTLPlusActivityObjectAttachmentsItem (0 custom class methods, 9 custom properties)
//   GTLPlusActivityObjectPlusoners (0 custom class methods, 2 custom properties)
//   GTLPlusActivityObjectReplies (0 custom class methods, 2 custom properties)
//   GTLPlusActivityObjectResharers (0 custom class methods, 2 custom properties)
//   GTLPlusActivityObjectActorImage (0 custom class methods, 1 custom properties)
//   GTLPlusActivityObjectAttachmentsItemEmbed (0 custom class methods, 2 custom properties)
//   GTLPlusActivityObjectAttachmentsItemFullImage (0 custom class methods, 4 custom properties)
//   GTLPlusActivityObjectAttachmentsItemImage (0 custom class methods, 4 custom properties)
//   GTLPlusActivityObjectAttachmentsItemThumbnailsItem (0 custom class methods, 3 custom properties)
//   GTLPlusActivityObjectAttachmentsItemThumbnailsItemImage (0 custom class methods, 4 custom properties)

#import "GTLPlusActivity.h"

#import "GTLPlusAcl.h"

// ----------------------------------------------------------------------------
//
//   GTLPlusActivity
//

@implementation GTLPlusActivity
@dynamic access, actor, address, annotation, crosspostSource, ETag, geocode,
         identifier, kind, object, placeId, placeName, provider, published,
         radius, title, updated, url, verb;

+ (NSDictionary *)propertyToJSONKeyMap {
  NSDictionary *map =
    [NSDictionary dictionaryWithObjectsAndKeys:
      @"etag", @"ETag",
      @"id", @"identifier",
      nil];
  return map;
}

+ (void)load {
  [self registerObjectClassForKind:@"plus#activity"];
}

@end


// ----------------------------------------------------------------------------
//
//   GTLPlusActivityActor
//

@implementation GTLPlusActivityActor
@dynamic displayName, identifier, image, name, url;

+ (NSDictionary *)propertyToJSONKeyMap {
  NSDictionary *map =
    [NSDictionary dictionaryWithObject:@"id"
                                forKey:@"identifier"];
  return map;
}

@end


// ----------------------------------------------------------------------------
//
//   GTLPlusActivityObject
//

@implementation GTLPlusActivityObject
@dynamic actor, attachments, content, identifier, objectType, originalContent,
         plusoners, replies, resharers, url;

+ (NSDictionary *)propertyToJSONKeyMap {
  NSDictionary *map =
    [NSDictionary dictionaryWithObject:@"id"
                                forKey:@"identifier"];
  return map;
}

+ (NSDictionary *)arrayPropertyToClassMap {
  NSDictionary *map =
    [NSDictionary dictionaryWithObject:[GTLPlusActivityObjectAttachmentsItem class]
                                forKey:@"attachments"];
  return map;
}

@end


// ----------------------------------------------------------------------------
//
//   GTLPlusActivityProvider
//

@implementation GTLPlusActivityProvider
@dynamic title;
@end


// ----------------------------------------------------------------------------
//
//   GTLPlusActivityActorImage
//

@implementation GTLPlusActivityActorImage
@dynamic url;
@end


// ----------------------------------------------------------------------------
//
//   GTLPlusActivityActorName
//

@implementation GTLPlusActivityActorName
@dynamic familyName, givenName;
@end


// ----------------------------------------------------------------------------
//
//   GTLPlusActivityObjectActor
//

@implementation GTLPlusActivityObjectActor
@dynamic displayName, identifier, image, url;

+ (NSDictionary *)propertyToJSONKeyMap {
  NSDictionary *map =
    [NSDictionary dictionaryWithObject:@"id"
                                forKey:@"identifier"];
  return map;
}

@end


// ----------------------------------------------------------------------------
//
//   GTLPlusActivityObjectAttachmentsItem
//

@implementation GTLPlusActivityObjectAttachmentsItem
@dynamic content, displayName, embed, fullImage, identifier, image, objectType,
         thumbnails, url;

+ (NSDictionary *)propertyToJSONKeyMap {
  NSDictionary *map =
    [NSDictionary dictionaryWithObject:@"id"
                                forKey:@"identifier"];
  return map;
}

+ (NSDictionary *)arrayPropertyToClassMap {
  NSDictionary *map =
    [NSDictionary dictionaryWithObject:[GTLPlusActivityObjectAttachmentsItemThumbnailsItem class]
                                forKey:@"thumbnails"];
  return map;
}

@end


// ----------------------------------------------------------------------------
//
//   GTLPlusActivityObjectPlusoners
//

@implementation GTLPlusActivityObjectPlusoners
@dynamic selfLink, totalItems;
@end


// ----------------------------------------------------------------------------
//
//   GTLPlusActivityObjectReplies
//

@implementation GTLPlusActivityObjectReplies
@dynamic selfLink, totalItems;
@end


// ----------------------------------------------------------------------------
//
//   GTLPlusActivityObjectResharers
//

@implementation GTLPlusActivityObjectResharers
@dynamic selfLink, totalItems;
@end


// ----------------------------------------------------------------------------
//
//   GTLPlusActivityObjectActorImage
//

@implementation GTLPlusActivityObjectActorImage
@dynamic url;
@end


// ----------------------------------------------------------------------------
//
//   GTLPlusActivityObjectAttachmentsItemEmbed
//

@implementation GTLPlusActivityObjectAttachmentsItemEmbed
@dynamic type, url;
@end


// ----------------------------------------------------------------------------
//
//   GTLPlusActivityObjectAttachmentsItemFullImage
//

@implementation GTLPlusActivityObjectAttachmentsItemFullImage
@dynamic height, type, url, width;
@end


// ----------------------------------------------------------------------------
//
//   GTLPlusActivityObjectAttachmentsItemImage
//

@implementation GTLPlusActivityObjectAttachmentsItemImage
@dynamic height, type, url, width;
@end


// ----------------------------------------------------------------------------
//
//   GTLPlusActivityObjectAttachmentsItemThumbnailsItem
//

@implementation GTLPlusActivityObjectAttachmentsItemThumbnailsItem
@dynamic descriptionProperty, image, url;

+ (NSDictionary *)propertyToJSONKeyMap {
  NSDictionary *map =
    [NSDictionary dictionaryWithObject:@"description"
                                forKey:@"descriptionProperty"];
  return map;
}

@end


// ----------------------------------------------------------------------------
//
//   GTLPlusActivityObjectAttachmentsItemThumbnailsItemImage
//

@implementation GTLPlusActivityObjectAttachmentsItemThumbnailsItemImage
@dynamic height, type, url, width;
@end
