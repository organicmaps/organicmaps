// Copyright (c) 2014-present, Facebook, Inc. All rights reserved.
//
// You are hereby granted a non-exclusive, worldwide, royalty-free license to use,
// copy, modify, and distribute this software in source code or binary form for use
// in connection with the web services and APIs provided by Facebook.
//
// As with any software that integrates with the Facebook platform, your use of
// this software is subject to the Facebook Developer Principles and Policies
// [http://developers.facebook.com/policy/]. This copyright notice shall be
// included in all copies or substantial portions of the software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#import <Foundation/Foundation.h>

/**
 These are the fields currently exposed by FBSDKPlacesKit. They map to the fields on
 Place objects returned by the Graph API, which can be found here:
 https://developers.facebook.com/docs/places. Should fields be added to the Graph API in
 the future, you can use strings found in the online documenation in addition to
 these string constants.
 */

/**
 Field Key for information about the Place.
 */
extern NSString *const FBSDKPlacesFieldKeyAbout;

/**
 Field Key for AppLinks for the Place.
 */
extern NSString *const FBSDKPlacesFieldKeyAppLinks;

/**
 Field Key for the Place's categories.
 */
extern NSString *const FBSDKPlacesFieldKeyCategories;

/**
 Field Key for the number of checkins at the Place.
 */
extern NSString *const FBSDKPlacesFieldKeyCheckins;

/**
 Field Key for the OpenGraphContext of the place, including friends who were at this
 place, or who liked to its page. This field requires authentication with a user token.
 An error will be thrown if this field is requested using a client token.
 */
extern NSString *const FBSDKPlacesFieldKeyContext;

/**
 Field Key for the confidence level for a current place estimate candidate.
 */
extern NSString *const FBSDKPlacesFieldKeyConfidence;

/**
 Field Key for the Place's cover photo. Note that this is not the actual photo data,
 but rather URLs and other metadata.
 */
extern NSString *const FBSDKPlacesFieldKeyCoverPhoto;

/**
 Field Key for the description of the Place.
 */
extern NSString *const FBSDKPlacesFieldKeyDescription;

/**
 Field Key for the social sentence and like count information for this place. This is
 the same information used for the Like button.
 */
extern NSString *const FBSDKPlacesFieldKeyEngagement;

/**
 Field Key for the number of people who like the place.
 */
extern NSString *const FBSDKPlacesFieldKeyFanCount;

/**
 Field Key for hour ranges for when the Place is open. Each day can have two different
 hours ranges. The keys in the dictionary are in the form of {day}_{number}_{status}.
 {day} should be the first 3 characters of the day of the week, {number} should be
 either 1 or 2 to allow for the two different hours ranges per day. {status} should be
 either open or close, to delineate the start or end of a time range. An example would
 be mon_1_open with value 17:00 and mon_1_close with value 21:15 which would represent
 a single opening range of 5 PM to 9:15 PM on Mondays. You can find an example of hours
 being parsed out in the Sample App.
 */
extern NSString *const FBSDKPlacesFieldKeyHours;

/**
 Field Key for a value indicating whether this place is always open.
 */
extern NSString *const FBSDKPlacesFieldKeyIsAlwaysOpen;

/**
 Field Key for a value indicating whether this place is permanently closed.
 */
extern NSString *const FBSDKPlacesFieldKeyIsPermanentlyClosed;

/**
 Pages with a large number of followers can be manually verified by Facebook as having an
 authentic identity. This field indicates whether the page is verified by this process.
 */
extern NSString *const FBSDKPlacesFieldKeyIsVerified;

/**
 Field Key for address and latitude/longitude information for the place.
 */
extern NSString *const FBSDKPlacesFieldKeyLocation;

/**
 Field Key for a link to Place's Facebook page.
 */
extern NSString *const FBSDKPlacesFieldKeyLink;

/**
 Field Key for the name of the place.
 */
extern NSString *const FBSDKPlacesFieldKeyName;

/**
 Field Key for the overall page rating based on rating surveys from users on a scale
 from 1-5. This value is normalized, and is not guaranteed to be a strict average of
 user ratings.
 */
extern NSString *const FBSDKPlacesFieldKeyOverallStarRating;

/**
 Field Key for PageParking information for the Place.
 */
extern NSString *const FBSDKPlacesFieldKeyParking;

/**
 Field Key for available payment options.
 */
extern NSString *const FBSDKPlacesFieldKeyPaymentOptions;

/**
 Field Key for the unique Facebook ID of the place.
 */
extern NSString *const FBSDKPlacesFieldKeyPlaceID;

/**
 Field Key for the Place's phone number.
 */
extern NSString *const FBSDKPlacesFieldKeyPhone;

/**
 Field Key for the Place's photos. Note that this is not the actual photo data, but
 rather URLs and other metadata.
 */
extern NSString *const FBSDKPlacesFieldKeyPhotos;

/**
 Field Key for the price range of the business, expressed as a string. Applicable to
 Restaurants or Nightlife. Can be one of $ (0-10), $$ (10-30), $$$ (30-50), $$$$ (50+),
 or Unspecified.
 */
extern NSString *const FBSDKPlacesFieldKeyPriceRange;

/**
 Field Key for the Place's profile photo. Note that this is not the actual photo data,
 but rather URLs and other metadata.
 */
extern NSString *const FBSDKPlacesFieldKeyProfilePhoto;

/**
 Field Key for the number of ratings for the place.
 */
extern NSString *const FBSDKPlacesFieldKeyRatingCount;

/**
 Field Key for restaurant services e.g: delivery, takeout.
 */
extern NSString *const FBSDKPlacesFieldKeyRestaurantServices;

/**
 Field Key for restaurant specialties.
 */
extern NSString *const FBSDKPlacesFieldKeyRestaurantSpecialties;

/**
 Field Key for the address in a single line.
 */
extern NSString *const FBSDKPlacesFieldKeySingleLineAddress;

/**
 Field Key for the string of the Place's website URL.
 */
extern NSString *const FBSDKPlacesFieldKeyWebsite;

/**
 Field Key for the Workflows.
 */
extern NSString *const FBSDKPlacesFieldKeyWorkflows;

/**
 Response Key for the place's city field.
 */
extern NSString *const FBSDKPlacesResponseKeyCity;

/**
 Response Key for the place's city ID field.
 */
extern NSString *const FBSDKPlacesResponseKeyCityID;

/**
 Response Key for the place's country field.
 */
extern NSString *const FBSDKPlacesResponseKeyCountry;

/**
 Response Key for the place's country code field.
 */
extern NSString *const FBSDKPlacesResponseKeyCountryCode;

/**
 Response Key for the place's latitude field.
 */
extern NSString *const FBSDKPlacesResponseKeyLatitude;

/**
 Response Key for the place's longitude field.
 */
extern NSString *const FBSDKPlacesResponseKeyLongitude;

/**
 Response Key for the place's region field.
 */
extern NSString *const FBSDKPlacesResponseKeyRegion;

/**
 Response Key for the place's region ID field.
 */
extern NSString *const FBSDKPlacesResponseKeyRegionID;

/**
 Response Key for the place's state field.
 */
extern NSString *const FBSDKPlacesResponseKeyState;

/**
 Response Key for the place's street field.
 */
extern NSString *const FBSDKPlacesResponseKeyStreet;

/**
 Response Key for the place's zip code field.
 */
extern NSString *const FBSDKPlacesResponseKeyZip;

/**
 Response Key for the categories that this place matched.
 To be used on the search request if the categories parameter is specified.
 */
extern NSString *const FBSDKPlacesResponseKeyMatchedCategories;

/**
 Response Key for the photo source dictionary.
 */
extern NSString *const FBSDKPlacesResponseKeyPhotoSource;

/**
 Response Key for response data.
 */
extern NSString *const FBSDKPlacesResponseKeyData;

/**
 Response Key for a URL.
 */
extern NSString *const FBSDKPlacesResponseKeyUrl;

/**
 Parameter Key for the current place summary.
 */
extern NSString *const FBSDKPlacesParameterKeySummary;

/**
 Summary Key for the current place tracking ID.
 */
extern NSString *const FBSDKPlacesSummaryKeyTracking;

/**
 The level of confidence the Facebook SDK has that a Place is the correct one for the
 user's current location.

 - FBSDKPlaceLocationConfidenceNotApplicable: Used to indicate that any level is
 acceptable as a minimum threshold
 - FBSDKPlaceLocationConfidenceLow: Low confidence level.
 - FBSDKPlaceLocationConfidenceMedium: Medium confidence level.
 - FBSDKPlaceLocationConfidenceHigh: High confidence level.
 */
typedef NS_ENUM(NSInteger) {
  FBSDKPlaceLocationConfidenceNotApplicable,
  FBSDKPlaceLocationConfidenceLow,
  FBSDKPlaceLocationConfidenceMedium,
  FBSDKPlaceLocationConfidenceHigh
} FBSDKPlaceLocationConfidence;
