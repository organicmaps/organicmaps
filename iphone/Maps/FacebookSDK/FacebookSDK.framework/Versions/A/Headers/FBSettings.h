/*
 * Copyright 2010-present Facebook.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#import <Foundation/Foundation.h>
#import <CoreGraphics/CGBase.h>

/*
 * Constants defining logging behavior.  Use with <[FBSettings setLoggingBehavior]>.
 */

/*! Log requests from FBRequest* classes */
extern NSString *const FBLoggingBehaviorFBRequests;

/*! Log requests from FBURLConnection* classes */
extern NSString *const FBLoggingBehaviorFBURLConnections;

/*! Include access token in logging. */
extern NSString *const FBLoggingBehaviorAccessTokens;

/*! Log session state transitions. */
extern NSString *const FBLoggingBehaviorSessionStateTransitions;

/*! Log performance characteristics */
extern NSString *const FBLoggingBehaviorPerformanceCharacteristics;

/*! Log FBAppEvents interactions */
extern NSString *const FBLoggingBehaviorAppEvents;

/*! Log Informational occurrences */
extern NSString *const FBLoggingBehaviorInformational;

/*! Log errors likely to be preventable by the developer. This is in the default set of enabled logging behaviors. */
extern NSString *const FBLoggingBehaviorDeveloperErrors;

@class FBGraphObject;

/*!
 @typedef

 @abstract Block type used to get install data that is returned by server when publishInstall is called
 @discussion
 */
typedef void (^FBInstallResponseDataHandler)(FBGraphObject *response, NSError *error);

/*!
 @typedef

 @abstract A list of beta features that can be enabled for the SDK. Beta features are for evaluation only,
 and are therefore only enabled for DEBUG builds. Beta features should not be enabled
 in release builds.
 */
typedef enum : NSUInteger {
  FBBetaFeaturesNone                 = 0,
#if defined(DEBUG) || defined(FB_BUILD_ONLY)
  FBBetaFeaturesShareDialog          = 1 << 0,
  FBBetaFeaturesOpenGraphShareDialog = 1 << 1,
#endif
} FBBetaFeatures;

@interface FBSettings : NSObject

/*!
 @method

 @abstract Retrieve the current iOS SDK version.

 */
+ (NSString *)sdkVersion;

/*!
 @method

 @abstract Retrieve the current Facebook SDK logging behavior.

 */
+ (NSSet *)loggingBehavior;

/*!
 @method

 @abstract Set the current Facebook SDK logging behavior.  This should consist of strings defined as
 constants with FBLogBehavior*, and can be constructed with, e.g., [NSSet initWithObjects:].

 @param loggingBehavior A set of strings indicating what information should be logged.  If nil is provided, the logging
 behavior is reset to the default set of enabled behaviors.  Set in an empty set in order to disable all logging.
 */
+ (void)setLoggingBehavior:(NSSet *)loggingBehavior;

/*! @abstract deprecated method */
+ (BOOL)shouldAutoPublishInstall __attribute__ ((deprecated));

/*! @abstract deprecated method */
+ (void)setShouldAutoPublishInstall:(BOOL)autoPublishInstall __attribute__ ((deprecated));

/*!
 @method

 @abstract This method has been replaced by [FBAppEvents activateApp] */
+ (void)publishInstall:(NSString *)appID __attribute__ ((deprecated("use [FBAppEvents activateApp] instead")));

/*!
 @method

 @abstract Manually publish an attributed install to the Facebook graph, and return the server response back in
 the supplied handler.  Calling this method will implicitly turn off auto-publish.  This method acquires the
 current attribution id from the facebook application, queries the graph API to determine if the application
 has install attribution enabled, publishes the id, and records success to avoid reporting more than once.

 @param appID   A specific appID to publish an install for.  If nil, uses [FBSession defaultAppID].
 @param handler A block to call with the server's response.
 */
+ (void)publishInstall:(NSString *)appID
           withHandler:(FBInstallResponseDataHandler)handler __attribute__ ((deprecated));


/*!
 @method

 @abstract
 Gets the application version to the provided string.  `FBAppEvents`, for instance, attaches the app version to
 events that it logs, which are then available in App Insights.
 */
+ (NSString *)appVersion;

/*!
 @method

 @abstract
 Sets the application version to the provided string.  `FBAppEvents`, for instance, attaches the app version to
 events that it logs, which are then available in App Insights.

 @param appVersion  The version identifier of the iOS app.
 */
+ (void)setAppVersion:(NSString *)appVersion;

/*!
 @method

 @abstract Retrieve the Client Token that has been set via [FBSettings setClientToken]
 */
+ (NSString *)clientToken;

/*!
 @method

 @abstract Sets the Client Token for the Facebook App.  This is needed for certain API calls when made anonymously,
 without a user-based Session.

 @param clientToken  The Facebook App's "client token", which, for a given appid can be found in the Security
 section of the Advanced tab of the Facebook App settings found at <https://developers.facebook.com/apps/[your-app-id]>

 */
+ (void)setClientToken:(NSString *)clientToken;

/*!
 @method

 @abstract Set the default Facebook Display Name to be used by the SDK. This should match
 the Display Name that has been set for the app with the corresponding Facebook App ID, in
 the Facebook App Dashboard

 @param displayName The default Facebook Display Name to be used by the SDK.
 */
+ (void)setDefaultDisplayName:(NSString *)displayName;

/*!
 @method

 @abstract Get the default Facebook Display Name used by the SDK. If not explicitly
 set, the default will be read from the application's plist.
 */
+ (NSString *)defaultDisplayName;

/*!
 @method

 @abstract Set the default Facebook App ID to use for sessions. The SDK allows the appID
 to be overridden per instance in certain cases (e.g. per instance of FBSession)

 @param appID The default Facebook App ID to be used by the SDK.
 */
+ (void)setDefaultAppID:(NSString*)appID;

/*!
 @method

 @abstract Get the default Facebook App ID used by the SDK. If not explicitly
 set, the default will be read from the application's plist. The SDK allows the appID
 to be overridden per instance in certain cases (e.g. per instance of FBSession)
 */
+ (NSString*)defaultAppID;

/*!
 @method

 @abstract Set the default url scheme suffix used by the SDK.

 @param urlSchemeSuffix The default url scheme suffix to be used by the SDK.
 */
+ (void)setDefaultUrlSchemeSuffix:(NSString*)urlSchemeSuffix;

/*!
 @method

 @abstract Get the default url scheme suffix used for sessions.  If not
 explicitly set, the default will be read from the application's plist value for 'FacebookUrlSchemeSuffix'.
 */
+ (NSString*)defaultUrlSchemeSuffix;

/*!
 @method

 @abstract Set the bundle name from the SDK will try and load overrides of images and text

 @param bundleName The name of the bundle (MyFBBundle).
 */
+ (void)setResourceBundleName:(NSString*)bundleName;

/*!
 @method

 @abstract Get the name of the bundle to override the SDK images and text
 */
+ (NSString*)resourceBundleName;

/*!
 @method

 @abstract Set the subpart of the facebook domain (e.g. @"beta") so that requests will be sent to graph.beta.facebook.com

 @param facebookDomainPart The domain part to be inserted into facebook.com
 */
+ (void)setFacebookDomainPart:(NSString*)facebookDomainPart;

/*!
 @method

 @abstract Get the Facebook domain part
 */
+ (NSString*)facebookDomainPart;

/*!
 @method

 @abstract Enables the specified beta features. Beta features are for evaluation only,
 and are therefore only enabled for debug builds. Beta features should not be enabled
 in release builds.

 @param betaFeatures The beta features to enable (expects a bitwise OR of FBBetaFeatures)
 */
+ (void)enableBetaFeatures:(NSUInteger)betaFeatures;

/*!
 @method

 @abstract Enables a beta feature. Beta features are for evaluation only,
 and are therefore only enabled for debug builds. Beta features should not be enabled
 in release builds.

 @param betaFeature The beta feature to enable.
 */
+ (void)enableBetaFeature:(FBBetaFeatures)betaFeature;

/*!
 @method

 @abstract Disables a beta feature.

 @param betaFeature The beta feature to disable.
 */
+ (void)disableBetaFeature:(FBBetaFeatures)betaFeature;

/*!
 @method

 @abstract Determines whether a beta feature is enabled or not.

 @param betaFeature The beta feature to check.

 @return YES if the beta feature is enabled, NO if not.
 */
+ (BOOL)isBetaFeatureEnabled:(FBBetaFeatures)betaFeature;

/*!
 @method
 
 @abstract
 Gets whether data such as that generated through FBAppEvents and sent to Facebook should be restricted from being used for other than analytics and conversions.  Defaults to NO.  This value is stored on the device and persists across app launches.
 */
+ (BOOL)limitEventAndDataUsage;

/*!
 @method
 
 @abstract
 Sets whether data such as that generated through FBAppEvents and sent to Facebook should be restricted from being used for other than analytics and conversions.  Defaults to NO.  This value is stored on the device and persists across app launches.
 
 @param limitEventAndDataUsage   The desired value.
 */
+ (void)setLimitEventAndDataUsage:(BOOL)limitEventAndDataUsage;

@end
