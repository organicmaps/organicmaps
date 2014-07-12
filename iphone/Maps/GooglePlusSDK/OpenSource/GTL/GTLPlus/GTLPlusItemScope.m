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
//  GTLPlusItemScope.m
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
//   GTLPlusItemScope (0 custom class methods, 55 custom properties)

#import "GTLPlusItemScope.h"

// ----------------------------------------------------------------------------
//
//   GTLPlusItemScope
//

@implementation GTLPlusItemScope
@dynamic about, additionalName, address, addressCountry, addressLocality,
         addressRegion, associatedMedia, attendeeCount, attendees, audio,
         author, bestRating, birthDate, byArtist, caption, contentSize,
         contentUrl, contributor, dateCreated, dateModified, datePublished,
         descriptionProperty, duration, embedUrl, endDate, familyName, gender,
         geo, givenName, height, identifier, image, inAlbum, kind, latitude,
         location, longitude, name, partOfTVSeries, performers, playerType,
         postalCode, postOfficeBoxNumber, ratingValue, reviewRating, startDate,
         streetAddress, text, thumbnail, thumbnailUrl, tickerSymbol, type, url,
         width, worstRating;

+ (NSDictionary *)propertyToJSONKeyMap {
  NSDictionary *map =
    [NSDictionary dictionaryWithObjectsAndKeys:
      @"associated_media", @"associatedMedia",
      @"description", @"descriptionProperty",
      @"id", @"identifier",
      nil];
  return map;
}

+ (NSDictionary *)arrayPropertyToClassMap {
  NSDictionary *map =
    [NSDictionary dictionaryWithObjectsAndKeys:
      [NSString class], @"additionalName",
      [GTLPlusItemScope class], @"associated_media",
      [GTLPlusItemScope class], @"attendees",
      [GTLPlusItemScope class], @"author",
      [GTLPlusItemScope class], @"contributor",
      [GTLPlusItemScope class], @"performers",
      nil];
  return map;
}

+ (void)load {
  [self registerObjectClassForKind:@"plus#itemScope"];
}

@end
