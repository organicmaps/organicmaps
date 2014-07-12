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
//  GTLPlusItemScope.h
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

#if GTL_BUILT_AS_FRAMEWORK
  #import "GTL/GTLObject.h"
#else
  #import "GTLObject.h"
#endif

@class GTLPlusItemScope;

// ----------------------------------------------------------------------------
//
//   GTLPlusItemScope
//

@interface GTLPlusItemScope : GTLObject

// The subject matter of the content.
@property (retain) GTLPlusItemScope *about;

// An additional name for a Person, can be used for a middle name.
@property (retain) NSArray *additionalName;  // of NSString

// Postal address.
@property (retain) GTLPlusItemScope *address;

// Address country.
@property (copy) NSString *addressCountry;

// Address locality.
@property (copy) NSString *addressLocality;

// Address region.
@property (copy) NSString *addressRegion;

// The encoding.
@property (retain) NSArray *associatedMedia;  // of GTLPlusItemScope

// Number of attendees.
@property (retain) NSNumber *attendeeCount;  // intValue

// A person attending the event.
@property (retain) NSArray *attendees;  // of GTLPlusItemScope

// From http://schema.org/MusicRecording, the audio file.
@property (retain) GTLPlusItemScope *audio;

// The person who created this scope.
@property (retain) NSArray *author;  // of GTLPlusItemScope

// Best possible rating value.
@property (copy) NSString *bestRating;

// Date of birth.
@property (copy) NSString *birthDate;

// From http://schema.org/MusicRecording, the artist that performed this
// recording.
@property (retain) GTLPlusItemScope *byArtist;

// The caption for this object.
@property (copy) NSString *caption;

// File size in (mega/kilo) bytes.
@property (copy) NSString *contentSize;

// Actual bytes of the media object, for example the image file or video file.
@property (copy) NSString *contentUrl;

// The list of contributors for this scope.
@property (retain) NSArray *contributor;  // of GTLPlusItemScope

// The date this scope was created.
@property (copy) NSString *dateCreated;

// The date this scope was last modified.
@property (copy) NSString *dateModified;

// The initial date this scope was published.
@property (copy) NSString *datePublished;

// The string describing the content of this scope.
// Remapped to 'descriptionProperty' to avoid NSObject's 'description'.
@property (copy) NSString *descriptionProperty;

// The duration of the item (movie, audio recording, event, etc.) in ISO 8601
// date format.
@property (copy) NSString *duration;

// A URL pointing to a player for a specific video. In general, this is the
// information in the src element of an embed tag and should not be the same as
// the content of the loc tag.
@property (copy) NSString *embedUrl;

// The end date and time of the event (in ISO 8601 date format).
@property (copy) NSString *endDate;

// Family name. In the U.S., the last name of an Person. This can be used along
// with givenName instead of the Name property.
@property (copy) NSString *familyName;

// Gender of the person.
@property (copy) NSString *gender;

// Geo coordinates.
@property (retain) GTLPlusItemScope *geo;

// Given name. In the U.S., the first name of a Person. This can be used along
// with familyName instead of the Name property.
@property (copy) NSString *givenName;

// The height of the media object.
@property (copy) NSString *height;

// The id for this item scope.
// identifier property maps to 'id' in JSON (to avoid Objective C's 'id').
@property (copy) NSString *identifier;

// A url to the image for this scope.
@property (copy) NSString *image;

// From http://schema.org/MusicRecording, which album a song is in.
@property (retain) GTLPlusItemScope *inAlbum;

// Identifies this resource as an itemScope.
@property (copy) NSString *kind;

// Latitude.
@property (retain) NSNumber *latitude;  // doubleValue

// The location of the event or organization.
@property (retain) GTLPlusItemScope *location;

// Longitude.
@property (retain) NSNumber *longitude;  // doubleValue

// The name of this scope.
@property (copy) NSString *name;

// Property of http://schema.org/TVEpisode indicating which series the episode
// belongs to.
@property (retain) GTLPlusItemScope *partOfTVSeries;

// The main performer or performers of the event-for example, a presenter,
// musician, or actor.
@property (retain) NSArray *performers;  // of GTLPlusItemScope

// Player type required-for example, Flash or Silverlight.
@property (copy) NSString *playerType;

// Postal code.
@property (copy) NSString *postalCode;

// Post office box number.
@property (copy) NSString *postOfficeBoxNumber;

// Rating value.
@property (copy) NSString *ratingValue;

// Review rating.
@property (retain) GTLPlusItemScope *reviewRating;

// The start date and time of the event (in ISO 8601 date format).
@property (copy) NSString *startDate;

// Street address.
@property (copy) NSString *streetAddress;

// Comment text, review text, etc.
@property (copy) NSString *text;

// Thumbnail image for an image or video.
@property (retain) GTLPlusItemScope *thumbnail;

// A url to a thumbnail image for this scope.
@property (copy) NSString *thumbnailUrl;

// The exchange traded instrument associated with a Corporation object. The
// tickerSymbol is expressed as an exchange and an instrument name separated by
// a space character. For the exchange component of the tickerSymbol attribute,
// we reccommend using the controlled vocaulary of Market Identifier Codes (MIC)
// specified in ISO15022.
@property (copy) NSString *tickerSymbol;

// The item type.
@property (copy) NSString *type;

// A URL for the item upon which the action was performed.
@property (copy) NSString *url;

// The width of the media object.
@property (copy) NSString *width;

// Worst possible rating value.
@property (copy) NSString *worstRating;

@end
