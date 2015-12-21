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
#import <Parse/PFUser.h>

#import <FBSDKCoreKit/FBSDKAccessToken.h>

#if TARGET_OS_IOS
#import <FBSDKLoginKit/FBSDKLoginManager.h>
#endif

NS_ASSUME_NONNULL_BEGIN

/**
 The `PFFacebookUtils` class provides utility functions for using Facebook authentication with `PFUser`s.

 @warning This class supports official Facebook iOS SDK v4.0+ and is available only on iOS.
 */
@interface PFFacebookUtils : NSObject

///--------------------------------------
/// @name Interacting With Facebook
///--------------------------------------

/**
 Initializes Parse Facebook Utils.

 You must provide your Facebook application ID as the value for FacebookAppID in your bundle's plist file
 as described here: https://developers.facebook.com/docs/getting-started/facebook-sdk-for-ios/

 @warning You must invoke this in order to use the Facebook functionality in Parse.

 @param launchOptions The launchOptions as passed to [UIApplicationDelegate application:didFinishLaunchingWithOptions:].
 */
+ (void)initializeFacebookWithApplicationLaunchOptions:(nullable NSDictionary *)launchOptions;

#if TARGET_OS_IOS
/**
 `FBSDKLoginManager` provides methods for configuring login behavior, default audience
 and managing Facebook Access Token.
 
 @warning This method is available only on iOS.

 @return An instance of `FBSDKLoginManager` that is used by `PFFacebookUtils`.
 */
+ (FBSDKLoginManager *)facebookLoginManager;
#endif

///--------------------------------------
/// @name Logging In
///--------------------------------------

/**
 *Asynchronously* logs in a user using Facebook with read permissions.

 This method delegates to the Facebook SDK to authenticate the user,
 and then automatically logs in (or creates, in the case where it is a new user) a `PFUser`.

 @param permissions Array of read permissions to use.

 @return The task that has will a have `result` set to `PFUser` if operation succeeds.
 */
+ (BFTask PF_GENERIC(PFUser *)*)logInInBackgroundWithReadPermissions:(nullable NSArray PF_GENERIC(NSString *)*)permissions;

/**
 *Asynchronously* logs in a user using Facebook with read permissions.

 This method delegates to the Facebook SDK to authenticate the user,
 and then automatically logs in (or creates, in the case where it is a new user) a `PFUser`.

 @param permissions Array of read permissions to use.
 @param block       The block to execute when the log in completes.
 It should have the following signature: `^(PFUser *user, NSError *error)`.
 */
+ (void)logInInBackgroundWithReadPermissions:(nullable NSArray PF_GENERIC(NSString *)*)permissions
                                       block:(nullable PFUserResultBlock)block;

/**
 *Asynchronously* logs in a user using Facebook with publish permissions.

 This method delegates to the Facebook SDK to authenticate the user,
 and then automatically logs in (or creates, in the case where it is a new user) a `PFUser`.

 @param permissions Array of publish permissions to use.

 @return The task that has will a have `result` set to `PFUser` if operation succeeds.
 */
+ (BFTask PF_GENERIC(PFUser *)*)logInInBackgroundWithPublishPermissions:(nullable NSArray PF_GENERIC(NSString *)*)permissions;

/**
 *Asynchronously* logs in a user using Facebook with publish permissions.

 This method delegates to the Facebook SDK to authenticate the user,
 and then automatically logs in (or creates, in the case where it is a new user) a `PFUser`.

 @param permissions Array of publish permissions to use.
 @param block       The block to execute when the log in completes.
 It should have the following signature: `^(PFUser *user, NSError *error)`.
 */
+ (void)logInInBackgroundWithPublishPermissions:(nullable NSArray PF_GENERIC(NSString *)*)permissions
                                          block:(nullable PFUserResultBlock)block;

/**
 *Asynchronously* logs in a user using given Facebook Acess Token.

 This method delegates to the Facebook SDK to authenticate the user,
 and then automatically logs in (or creates, in the case where it is a new user) a `PFUser`.

 @param accessToken An instance of `FBSDKAccessToken` to use when logging in.

 @return The task that has will a have `result` set to `PFUser` if operation succeeds.
 */
+ (BFTask PF_GENERIC(PFUser *)*)logInInBackgroundWithAccessToken:(FBSDKAccessToken *)accessToken;

/**
 *Asynchronously* logs in a user using given Facebook Acess Token.

 This method delegates to the Facebook SDK to authenticate the user,
 and then automatically logs in (or creates, in the case where it is a new user) a `PFUser`.

 @param accessToken An instance of `FBSDKAccessToken` to use when logging in.
 @param block       The block to execute when the log in completes.
 It should have the following signature: `^(PFUser *user, NSError *error)`.
 */
+ (void)logInInBackgroundWithAccessToken:(FBSDKAccessToken *)accessToken
                                   block:(nullable PFUserResultBlock)block;

///--------------------------------------
/// @name Linking Users
///--------------------------------------

/**
 *Asynchronously* links Facebook with read permissions to an existing `PFUser`.

 This method delegates to the Facebook SDK to authenticate
 the user, and then automatically links the account to the `PFUser`.
 It will also save any unsaved changes that were made to the `user`.

 @param user        User to link to Facebook.
 @param permissions Array of read permissions to use when logging in with Facebook.

 @return The task that will have a `result` set to `@YES` if operation succeeds.
 */
+ (BFTask PF_GENERIC(NSNumber *)*)linkUserInBackground:(PFUser *)user
                                   withReadPermissions:(nullable NSArray PF_GENERIC(NSString *)*)permissions;

/**
 *Asynchronously* links Facebook with read permissions to an existing `PFUser`.

 This method delegates to the Facebook SDK to authenticate
 the user, and then automatically links the account to the `PFUser`.
 It will also save any unsaved changes that were made to the `user`.

 @param user        User to link to Facebook.
 @param permissions Array of read permissions to use.
 @param block       The block to execute when the linking completes.
 It should have the following signature: `^(BOOL succeeded, NSError *error)`.
 */
+ (void)linkUserInBackground:(PFUser *)user
         withReadPermissions:(nullable NSArray PF_GENERIC(NSString *)*)permissions
                       block:(nullable PFBooleanResultBlock)block;

/**
 *Asynchronously* links Facebook with publish permissions to an existing `PFUser`.

 This method delegates to the Facebook SDK to authenticate
 the user, and then automatically links the account to the `PFUser`.
 It will also save any unsaved changes that were made to the `user`.

 @param user        User to link to Facebook.
 @param permissions Array of publish permissions to use.

 @return The task that will have a `result` set to `@YES` if operation succeeds.
 */
+ (BFTask PF_GENERIC(NSNumber *)*)linkUserInBackground:(PFUser *)user
                                withPublishPermissions:(NSArray PF_GENERIC(NSString *)*)permissions;

/**
 *Asynchronously* links Facebook with publish permissions to an existing `PFUser`.

 This method delegates to the Facebook SDK to authenticate
 the user, and then automatically links the account to the `PFUser`.
 It will also save any unsaved changes that were made to the `user`.

 @param user        User to link to Facebook.
 @param permissions Array of publish permissions to use.
 @param block       The block to execute when the linking completes.
 It should have the following signature: `^(BOOL succeeded, NSError *error)`.
 */
+ (void)linkUserInBackground:(PFUser *)user
      withPublishPermissions:(NSArray PF_GENERIC(NSString *)*)permissions
                       block:(nullable PFBooleanResultBlock)block;

/**
 *Asynchronously* links Facebook Access Token to an existing `PFUser`.

 This method delegates to the Facebook SDK to authenticate
 the user, and then automatically links the account to the `PFUser`.
 It will also save any unsaved changes that were made to the `user`.

 @param user        User to link to Facebook.
 @param accessToken An instance of `FBSDKAccessToken` to use.

 @return The task that will have a `result` set to `@YES` if operation succeeds.
 */
+ (BFTask PF_GENERIC(NSNumber *)*)linkUserInBackground:(PFUser *)user withAccessToken:(FBSDKAccessToken *)accessToken;

/**
 *Asynchronously* links Facebook Access Token to an existing `PFUser`.

 This method delegates to the Facebook SDK to authenticate
 the user, and then automatically links the account to the `PFUser`.
 It will also save any unsaved changes that were made to the `user`.

 @param user        User to link to Facebook.
 @param accessToken An instance of `FBSDKAccessToken` to use.
 @param block       The block to execute when the linking completes.
 It should have the following signature: `^(BOOL succeeded, NSError *error)`.
 */
+ (void)linkUserInBackground:(PFUser *)user
             withAccessToken:(FBSDKAccessToken *)accessToken
                       block:(nullable PFBooleanResultBlock)block;

///--------------------------------------
/// @name Unlinking Users
///--------------------------------------

/**
 Unlinks the `PFUser` from a Facebook account *asynchronously*.

 @param user User to unlink from Facebook.
 @return The task, that encapsulates the work being done.
 */
+ (BFTask PF_GENERIC(NSNumber *)*)unlinkUserInBackground:(PFUser *)user;

/**
 Unlinks the `PFUser` from a Facebook account *asynchronously*.

 @param user User to unlink from Facebook.
 @param block The block to execute.
 It should have the following argument signature: `^(BOOL succeeded, NSError *error)`.
 */
+ (void)unlinkUserInBackground:(PFUser *)user block:(nullable PFBooleanResultBlock)block;

///--------------------------------------
/// @name Getting Linked State
///--------------------------------------

/**
 Whether the user has their account linked to Facebook.

 @param user User to check for a facebook link. The user must be logged in on this device.

 @return `YES` if the user has their account linked to Facebook, otherwise `NO`.
 */
+ (BOOL)isLinkedWithUser:(PFUser *)user;

@end

NS_ASSUME_NONNULL_END
