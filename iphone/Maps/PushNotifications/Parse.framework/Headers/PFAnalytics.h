//
//  PFAnalytics.h
//
//  Copyright 2011-present Parse Inc. All rights reserved.
//

#import <Foundation/Foundation.h>

#if TARGET_OS_IPHONE
#import <Parse/PFConstants.h>
#else
#import <ParseOSX/PFConstants.h>
#endif

PF_ASSUME_NONNULL_BEGIN

@class BFTask;

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
+ (BFTask *)trackAppOpenedWithLaunchOptions:(PF_NULLABLE NSDictionary *)launchOptions;

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
+ (void)trackAppOpenedWithLaunchOptionsInBackground:(PF_NULLABLE NSDictionary *)launchOptions
                                              block:(PF_NULLABLE PFBooleanResultBlock)block;

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
+ (BFTask *)trackAppOpenedWithRemoteNotificationPayload:(PF_NULLABLE NSDictionary *)userInfo;

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
+ (void)trackAppOpenedWithRemoteNotificationPayloadInBackground:(PF_NULLABLE NSDictionary *)userInfo
                                                          block:(PF_NULLABLE PFBooleanResultBlock)block;

///--------------------------------------
/// @name Custom Analytics
///--------------------------------------

/*!
 @abstract Tracks the occurrence of a custom event.

 @discussion Parse will store a data point at the time of invocation with the given event name.

 @param name The name of the custom event to report to Parse as having happened.

 @returns Returns the task encapsulating the work being done.
 */
+ (BFTask *)trackEvent:(NSString *)name;

/*!
 @abstract Tracks the occurrence of a custom event. Parse will store a data point at the
 time of invocation with the given event name. The event will be sent at some
 unspecified time in the future, even if Parse is currently inaccessible.

 @param name The name of the custom event to report to Parse as having happened.
 @param block The block to execute on server response.
 It should have the following argument signature: `^(BOOL succeeded, NSError *error)`
 */
+ (void)trackEventInBackground:(NSString *)name block:(PF_NULLABLE PFBooleanResultBlock)block;

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
+ (BFTask *)trackEvent:(NSString *)name dimensions:(PF_NULLABLE NSDictionary *)dimensions;

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
                    dimensions:(PF_NULLABLE NSDictionary *)dimensions
                         block:(PF_NULLABLE PFBooleanResultBlock)block;

@end

PF_ASSUME_NONNULL_END
