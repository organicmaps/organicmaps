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

#import <CoreLocation/CoreLocation.h>
#import <Foundation/Foundation.h>

#import <FBSDKCoreKit/FBSDKCoreKit.h>

#import "FBSDKPlacesKitConstants.h"

/**
 Completion block for aysnchronous place request generation.

 @param graphRequest An `FBSDKGraphRequest` with the parameters supplied to the
 original method call.

 @param location A CLLocation representing the current location of the device at the time
 of the method call, which you can cache for later use.

 @param error An error indicating a failure in a location services request.
 */
typedef void (^FBSDKPlaceGraphRequestCompletion)(FBSDKGraphRequest *_Nullable graphRequest, CLLocation *_Nullable location, NSError *_Nullable error);

/**
 Completion block for aysnchronous current place request generation.

 @param graphRequest An `FBSDKGraphRequest` with the parameters supplied to the
 original method call.

 @param error An error indicating a failure in a location services request.
 */
typedef void (^FBSDKCurrentPlaceGraphRequestCompletion)(FBSDKGraphRequest *_Nullable graphRequest, NSError *_Nullable error);

/**
 `FBSDKPlacesManager` provides methods for searching for nearby places, as well as for
 determining the current place where a person might be.

 To use some features of the `FBSDKPlacesManager`, your app must be configured to allow
 either the `CoreLocation` permission for  `kCLAuthorizationStatusAuthorizedWhenInUse`,
 or for `kCLAuthorizationStatusAuthorizedAlways`. For information about enabling these
 for your app, see:
 https://developer.apple.com/library/content/documentation/UserExperience/Conceptual/LocationAwarenessPG/CoreLocation/CoreLocation.html.
 */
@interface FBSDKPlacesManager : NSObject


/**
 Method for generating a graph request for searching the Places Graph using the device's
 current location. This is an asynchronous call, due to the need to fetch the current
 location from the device.

 @param searchTerm The term to search for in the Places Graph.

 @param categories The categories for the place. Each string in this array must be a
 category recognized by the SDK. See `FBSDKPlacesKitConstants.h` for the categories
 exposed by the SDK, and see https://developers.facebook.com/docs/places/web/search#categories
 for the most up to date list.

 @param fields A list of fields that you might want the request to return. See
 `FBSDKPlacesKitConstants.h` for the fields exposed by the SDK, and see
 https://developers.facebook.com/docs/places/fields" for the most up to date list.

 @param distance The search radius. For an unlimited radius, use 0.

 @param cursor A pagination cursor.

 @param completion An `FBSDKPlaceGraphRequestCompletion` block. Note that this block will
 return the location, which you can choose to cache and use on calls to the synchronous
 `placesGraphRequestForLocation` method.
 */
- (void)generatePlaceSearchRequestForSearchTerm:(nullable NSString *)searchTerm
                                     categories:(nullable NSArray<NSString *> *)categories
                                         fields:(nullable NSArray<NSString *> *)fields
                                       distance:(CLLocationDistance)distance
                                         cursor:(nullable NSString *)cursor
                                     completion:(nonnull FBSDKPlaceGraphRequestCompletion)completion;

/**
 Method for generating a graph request for searching the Places API.

 @param location The location for the search. If you do not wish to provide a location,
 you must provide a search term.

 @param searchTerm The term to search the Places Graph for.

 @param categories The categories of the place. Each string in this array must be a
 category recognized by the SDK. See `FBSDKPlacesKitConstants.h` for the categories
 exposed by the SDK, and see https://developers.facebook.com/docs/places/web/search#categories
 for the most up to date list.

 @param fields A list of fields that you might want the request to return. See
 `FBSDKPlacesKitConstants.h` for the fields exposed by the SDK, and see https://developers.facebook.com/docs/places/fields
 for the most up to date list.

 @param distance The search radius. For an unlimited radius, use 0.

 @param cursor A pagination cursor

 @return An `FBSDKGraphRequest` for the given parameters.
 */
- (nullable FBSDKGraphRequest *)placeSearchRequestForLocation:(nullable CLLocation *)location
                                                   searchTerm:(nullable NSString *)searchTerm
                                                   categories:(nullable NSArray<NSString *> *)categories
                                                       fields:(nullable NSArray<NSString *> *)fields
                                                     distance:(CLLocationDistance)distance
                                                       cursor:(nullable NSString *)cursor;

/**
 Method for generating a graph request to query for places the device is likely
 located in. This is an asynchronous call, due to the need to fetch the current location
 from the device. Note that the results of the graph request are improved if the user has
 both Wi-Fi and Bluetooth enabled.

 @param minimumConfidence The Minimum Confidence level place estimate candidates must meet.

 @param fields A list of fields that you might want the request to return. See
 `FBSDKPlacesKitConstants.h` for the fields exposed by the SDK, and see https://developers.facebook.com/docs/places/fields
 for the most up to date list.

 @param completion A `FBSDKCurrentPlaceGraphRequestCompletion` block, that contains the graph request.
 */
- (void)generateCurrentPlaceRequestWithMinimumConfidenceLevel:(FBSDKPlaceLocationConfidence)minimumConfidence
                                                       fields:(nullable NSArray<NSString *> *)fields
                                                   completion:(nonnull FBSDKCurrentPlaceGraphRequestCompletion)completion;

/**
 Method for generating a graph request to query for places the device is likely
 located in. This method allows the user to pass in a CLLocation, as the above method
 can be real-world slow at obtaining a location from device (on the order of multiple
 seconds). This is still an asynchronous call, as the bluetooth scan still needs to
 happen, however it is relatively fast (0.5 seconds). Note that the results of the
 graph request are improved if the user has both Wi-Fi and Bluetooth enabled.

 @param currentLocation The current location of the device. If the location passed in here
 isn't actually the current location, accuracy of results will be diminished.

 @param minimumConfidence The Minimum Confidence level place estimate candidates must meet.

 @param fields A list of fields that you might want the request to return. See
 `FBSDKPlacesKitConstants.h` for the fields exposed by the SDK, and see https://developers.facebook.com/docs/places/fields
 for the most up to date list.

 @param completion A `FBSDKCurrentPlaceGraphRequestCompletion` block, that contains the graph request.
 */
- (void)generateCurrentPlaceRequestForCurrentLocation:(nonnull CLLocation *)currentLocation
                           withMinimumConfidenceLevel:(FBSDKPlaceLocationConfidence)minimumConfidence
                                               fields:(nullable NSArray<NSString *> *)fields
                                           completion:(nonnull FBSDKCurrentPlaceGraphRequestCompletion)completion;

/**
 Method for generating a graph request to provide feedback to the Places Graph about
 the accuracy of current place estimate candidates. We'd very much appreciate it if
 you'd give us feedback about our current place results. If a user indicates that a place
 returned by a current place estimate either is, or is not where they currently are,
 then this method generates a graph request to allow you to pass that feedback back to us.

 @param placeID The place ID returned by the current place request.

 @param tracking The tracking value returned by the current place request.

 @param wasHere A boolean value indicating whether the user is actually at the place.

 @return An `FBSDKGraphRequest` for the given parameters.
 */
- (nonnull FBSDKGraphRequest *)currentPlaceFeedbackRequestForPlaceID:(nonnull NSString *)placeID
                                                            tracking:(nonnull NSString *)tracking
                                                             wasHere:(BOOL)wasHere;

/**
 Method for generating a graph request to fetch additional information for a given place.

 @param placeID The ID of the place.

 @param fields A list of fields that you might want the request to return. See
 `FBSDKPlacesKitConstants.h` for the fields exposed by the SDK, and see https://developers.facebook.com/docs/places/fields
 for the most up to date list.

 @return An `FBSDKGraphRequest` for the given parameters.
 */
- (nonnull FBSDKGraphRequest *)placeInfoRequestForPlaceID:(nonnull NSString *)placeID
                                                   fields:(nullable NSArray<NSString *> *)fields;


@end
