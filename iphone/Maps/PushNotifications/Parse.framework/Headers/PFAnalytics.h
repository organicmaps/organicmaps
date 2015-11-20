/**
 * Copyright (c) 2015-present, Parse, LLC.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#import <Foundation/Foundation.h>

#import <Bolts/BFTask.h>

#import <Parse/PFConstants.h>

NS_ASSUME_NONNULL_BEGIN

/*!
 `PFAnalytics` provides an interface to Parse's logging and analytics backend.

 Methods will return immediately and cache the request (+ timestamp) to be
 handled "eventually." That is, the request will be sent immediately if possible
 or the next time a network connection is available.
 */
@interface PFAnalytics : NSObject

///--------------------------------------
/// @name App-Open / Push Analytics
///--------------------------------------

/*!
 @abstract Tracks this application being launched. If this happened as the result of the
 user opening a push notification, this method sends along information to
 correlate this open with that push.

 @discussion Pass in `nil` to track a standard "application opened" event.

 @param launchOptions The `NSDictionary` indicating the reason the application was
 launched, if any. This value can be found as a parameter to various
 `UIApplicationDelegate` methods, and can be empty or `nil`.

 @returns Returns the task encapsulating the work being done.
 */
+ (BFTask PF_GENERIC(NSNumber *)*)trackAppOpenedWithLaunchOptions:(nullable NSDictionary *)launchOptions;

/*!
 @abstract Tracks this application being launched.
 If this happened as the result of the user opening a push notification,
 this method sends along information to correlate this open with that push.

 @discussion Pass in `nil` to track a standard "application opened" event.

 @param launchOptions The dictionary indicating the reason the application was
 launched, if any. This value can be found as a parameter to various
 `UIApplicationDelegate` methods, and can be empty or `nil`.
 @param block The block to execute on server response.
 It should have the following argument signature: `^(BOOL succeeded, NSError *error)`
 */
+ (void)trackAppOpenedWithLaunchOptionsInBackground:(nullable NSDictionary *)launchOptions
                                              block:(nullable PFBooleanResultBlock)block;

/*!
 @abstract Tracks this application being launched. If this happened as the result of the
 user opening a push notification, this method sends along information to
 correlate this open with that push.

 @param userInfo The Remote Notification payload, if any. This value can be
 found either under `UIApplicationLaunchOptionsRemoteNotificationKey` on `launchOptions`,
 or as a parameter to `application:didReceiveRemoteNotification:`.
 This can be empty or `nil`.

 @returns Returns the task encapsulating the work being done.
 */
+ (BFTask PF_GENERIC(NSNumber *)*)trackAppOpenedWithRemoteNotificationPayload:(nullable NSDictionary *)userInfo;

/*!
 @abstract Tracks this application being launched. If this happened as the result of the
 user opening a push notification, this method sends along information to
 correlate this open with that push.

 @param userInfo The Remote Notification payload, if any. This value can be
 found either under `UIApplicationLaunchOptionsRemoteNotificationKey` on `launchOptions`,
 or as a parameter to `application:didReceiveRemoteNotification:`. This can be empty or `nil`.
 @param block The block to execute on server response.
 It should have the following argument signature: `^(BOOL succeeded, NSError *error)`
 */
+ (void)trackAppOpenedWithRemoteNotificationPayloadInBackground:(nullable NSDictionary *)userInfo
                                                          block:(nullable PFBooleanResultBlock)block;

///--------------------------------------
/// @name Custom Analytics
///--------------------------------------

/*!
 @abstract Tracks the occurrence of a custom event.

 @discussion Parse will store a data point at the time of invocation with the given event name.

 @param name The name of the custom event to report to Parse as having happened.

 @returns Returns the task encapsulating the work being done.
 */
+ (BFTask PF_GENERIC(NSNumber *)*)trackEvent:(NSString *)name;

/*!
 @abstract Tracks the occurrence of a custom event. Parse will store a data point at the
 time of invocation with the given event name. The event will be sent at some
 unspecified time in the future, even if Parse is currently inaccessible.

 @param name The name of the custom event to report to Parse as having happened.
 @param block The block to execute on server response.
 It should have the following argument signature: `^(BOOL succeeded, NSError *error)`
 */
+ (void)trackEventInBackground:(NSString *)name block:(nullable PFBooleanResultBlock)block;

/*!
 @abstract Tracks the occurrence of a custom event with additional dimensions. Parse will
 store a data point at the time of invocation with the given event name.

 @discussion Dimensions will allow segmentation of the occurrences of this custom event.
 Keys and values should be NSStrings, and will throw otherwise.

 To track a user signup along with additional metadata, consider the following:

 NSDictionary *dimensions = @{ @"gender": @"m",
 @"source": @"web",
 @"dayType": @"weekend" };
 [PFAnalytics trackEvent:@"signup" dimensions:dimensions];

 @warning There is a default limit of 8 dimensions per event tracked.

 @param name The name of the custom event to report to Parse as having happened.
 @param dimensions The `NSDictionary` of information by which to segment this event.

 @returns Returns the task encapsulating the work being done.
 */
+ (BFTask PF_GENERIC(NSNumber *)*)trackEvent:(NSString *)name
                                  dimensions:(nullable NSDictionary PF_GENERIC(NSString *, NSString *)*)dimensions;

/*!
 @abstract Tracks the occurrence of a custom event with additional dimensions. Parse will
 store a data point at the time of invocation with the given event name. The
 event will be sent at some unspecified time in the future, even if Parse is currently inaccessible.

 @discussionDimensions will allow segmentation of the occurrences of this custom event.
 Keys and values should be NSStrings, and will throw otherwise.

 To track a user signup along with additional metadata, consider the following:
 NSDictionary *dimensions = @{ @"gender": @"m",
 @"source": @"web",
 @"dayType": @"weekend" };
 [PFAnalytics trackEvent:@"signup" dimensions:dimensions];

 There is a default limit of 8 dimensions per event tracked.

 @param name The name of the custom event to report to Parse as having happened.
 @param dimensions The `NSDictionary` of information by which to segment this event.
 @param block The block to execute on server response.
 It should have the following argument signature: `^(BOOL succeeded, NSError *error)`
 */
+ (void)trackEventInBackground:(NSString *)name
                    dimensions:(nullable NSDictionary PF_GENERIC(NSString *, NSString *)*)dimensions
                         block:(nullable PFBooleanResultBlock)block;

@end

NS_ASSUME_NONNULL_END
