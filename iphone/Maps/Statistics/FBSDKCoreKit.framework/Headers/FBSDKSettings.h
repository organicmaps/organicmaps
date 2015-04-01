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

#import <UIKit/UIKit.h>

#import <FBSDKCoreKit/FBSDKMacros.h>

/*
 * Constants defining logging behavior.  Use with <[FBSDKSettings setLoggingBehavior]>.
 */

/*! Include access token in logging. */
FBSDK_EXTERN NSString *const FBSDKLoggingBehaviorAccessTokens;

/*! Log performance characteristics */
FBSDK_EXTERN NSString *const FBSDKLoggingBehaviorPerformanceCharacteristics;

/*! Log FBSDKAppEvents interactions */
FBSDK_EXTERN NSString *const FBSDKLoggingBehaviorAppEvents;

/*! Log Informational occurrences */
FBSDK_EXTERN NSString *const FBSDKLoggingBehaviorInformational;

/*! Log cache errors. */
FBSDK_EXTERN NSString *const FBSDKLoggingBehaviorCacheErrors;

/*! Log errors from SDK UI controls */
FBSDK_EXTERN NSString *const FBSDKLoggingBehaviorUIControlErrors;

/*! Log debug warnings from API response, i.e. when friends fields requested, but user_friends permission isn't granted. */
FBSDK_EXTERN NSString *const FBSDKLoggingBehaviorGraphAPIDebugWarning;

/*! Log warnings from API response, i.e. when requested feature will be deprecated in next version of API.
 Info is the lowest level of severity, using it will result in logging all previously mentioned levels.
 */
FBSDK_EXTERN NSString *const FBSDKLoggingBehaviorGraphAPIDebugInfo;

/*! Log errors from SDK network requests */
FBSDK_EXTERN NSString *const FBSDKLoggingBehaviorNetworkRequests;

/*! Log errors likely to be preventable by the developer. This is in the default set of enabled logging behaviors. */
FBSDK_EXTERN NSString *const FBSDKLoggingBehaviorDeveloperErrors;

@interface FBSDKSettings : NSObject

/*!
 @abstract Get the Facebook App ID used by the SDK.
 @discussion If not explicitly set, the default will be read from the application's plist (FacebookAppID).
 */
+ (NSString *)appID;

/*!
 @abstract Set the Facebook App ID to be used by the SDK.
 @param appID The Facebook App ID to be used by the SDK.
 */
+ (void)setAppID:(NSString *)appID;

/*!
 @abstract Get the default url scheme suffix used for sessions.
 @discussion If not explicitly set, the default will be read from the application's plist (FacebookUrlSchemeSuffix).
 */
+ (NSString *)appURLSchemeSuffix;

/*!
 @abstract Set the app url scheme suffix used by the SDK.
 @param appURLSchemeSuffix The url scheme suffix to be used by the SDK.
 */
+ (void)setAppURLSchemeSuffix:(NSString *)appURLSchemeSuffix;

/*!
 @abstract Retrieve the Client Token that has been set via [FBSDKSettings setClientToken].
 @discussion If not explicitly set, the default will be read from the application's plist (FacebookClientToken).
 */
+ (NSString *)clientToken;

/*!
 @abstract Sets the Client Token for the Facebook App.
 @discussion This is needed for certain API calls when made anonymously, without a user-based access token.
 @param clientToken The Facebook App's "client token", which, for a given appid can be found in the Security
 section of the Advanced tab of the Facebook App settings found at <https://developers.facebook.com/apps/[your-app-id]>
 */
+ (void)setClientToken:(NSString *)clientToken;

/*!
 @abstract A convenient way to toggle error recovery for all FBSDKGraphRequest instances created after this is set.
 @param disableGraphErrorRecovery YES or NO.
 */
+ (void)setGraphErrorRecoveryDisabled:(BOOL)disableGraphErrorRecovery;

/*!
 @abstract Get the Facebook Display Name used by the SDK.
 @discussion If not explicitly set, the default will be read from the application's plist (FacebookDisplayName).
 */
+ (NSString *)displayName;

/*!
 @abstract Set the default Facebook Display Name to be used by the SDK.
 @discussion  This should match the Display Name that has been set for the app with the corresponding Facebook App ID,
 in the Facebook App Dashboard.
 @param displayName The Facebook Display Name to be used by the SDK.
 */
+ (void)setDisplayName:(NSString *)displayName;

/*!
 @abstract Get the Facebook domain part.
 @discussion If not explicitly set, the default will be read from the application's plist (FacebookDomainPart).
 */
+ (NSString *)facebookDomainPart;

/*!
 @abstract Set the subpart of the Facebook domain.
 @discussion This can be used to change the Facebook domain (e.g. @"beta") so that requests will be sent to
 graph.beta.facebook.com
 @param facebookDomainPart The domain part to be inserted into facebook.com.
 */
+ (void)setFacebookDomainPart:(NSString *)facebookDomainPart;

/*!
 @abstract The quality of JPEG images sent to Facebook from the SDK.
 @discussion If not explicitly set, the default is 0.9.
 @see [UIImageJPEGRepresentation](https://developer.apple.com/library/ios/documentation/UIKit/Reference/UIKitFunctionReference/#//apple_ref/c/func/UIImageJPEGRepresentation) */
+ (CGFloat)JPEGCompressionQuality;

/*!
 @abstract Set the quality of JPEG images sent to Facebook from the SDK.
 @param JPEGCompressionQuality The quality for JPEG images, expressed as a value from 0.0 to 1.0.
 @see [UIImageJPEGRepresentation](https://developer.apple.com/library/ios/documentation/UIKit/Reference/UIKitFunctionReference/#//apple_ref/c/func/UIImageJPEGRepresentation) */
+ (void)setJPEGCompressionQuality:(CGFloat)JPEGCompressionQuality;

/*!
 @abstract
 Gets whether data such as that generated through FBSDKAppEvents and sent to Facebook should be restricted from being used for other than analytics and conversions.  Defaults to NO.  This value is stored on the device and persists across app launches.
 */
+ (BOOL)limitEventAndDataUsage;

/*!
 @abstract
 Sets whether data such as that generated through FBSDKAppEvents and sent to Facebook should be restricted from being used for other than analytics and conversions.  Defaults to NO.  This value is stored on the device and persists across app launches.

 @param limitEventAndDataUsage   The desired value.
 */
+ (void)setLimitEventAndDataUsage:(BOOL)limitEventAndDataUsage;

/*!
 @abstract Retrieve the current iOS SDK version.
 */
+ (NSString *)sdkVersion;

/*!
 @abstract Retrieve the current Facebook SDK logging behavior.
 */
+ (NSSet *)loggingBehavior;

/*!
 @abstract Set the current Facebook SDK logging behavior.  This should consist of strings defined as
 constants with FBSDKLoggingBehavior*.

 @param loggingBehavior A set of strings indicating what information should be logged.  If nil is provided, the logging
 behavior is reset to the default set of enabled behaviors.  Set to an empty set in order to disable all logging.

 @discussion You can also define this via an array in your app plist with key "FacebookLoggingBehavior" or add and remove individual values via enableLoggingBehavior: or disableLogginBehavior:
 */
+ (void)setLoggingBehavior:(NSSet *)loggingBehavior;

/*!
 @abstract Enable a particular Facebook SDK logging behavior.

 @param loggingBehavior The LoggingBehavior to enable. This should be a string defined as a constant with FBSDKLoggingBehavior*.
 */
+ (void)enableLoggingBehavior:(NSString *)loggingBehavior;

/*!
 @abstract Disable a particular Facebook SDK logging behavior.

 @param loggingBehavior The LoggingBehavior to disable. This should be a string defined as a constant with FBSDKLoggingBehavior*.
 */
+ (void)disableLoggingBehavior:(NSString *)loggingBehavior;

/*!
 @abstract Set the user defaults key used by legacy token caches.

 @param tokenInformationKeyName the key used by legacy token caches.

 @discussion Use this only if you customized FBSessionTokenCachingStrategy in v3.x of
  the Facebook SDK for iOS.
*/
+ (void)setLegacyUserDefaultTokenInformationKeyName:(NSString *)tokenInformationKeyName;

/*!
 @abstract Get the user defaults key used by legacy token caches.
*/
+ (NSString *)legacyUserDefaultTokenInformationKeyName;

@end
