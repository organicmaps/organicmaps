//
//  PFFacebookUtils.h
//
//  Copyright 2011-present Parse Inc. All rights reserved.
//

#import <Foundation/Foundation.h>

#import <Bolts/BFTask.h>

#import <FBSDKCoreKit/FBSDKAccessToken.h>

#import <FBSDKLoginKit/FBSDKLoginManager.h>

#import <Parse/PFConstants.h>
#import <Parse/PFNullability.h>
#import <Parse/PFUser.h>

PF_ASSUME_NONNULL_BEGIN

/*!
 The `PFFacebookUtils` class provides utility functions for using Facebook authentication with <PFUser>s.

 @warning This class supports official Facebook iOS SDK v4.0+ and is available only on iOS.
 */
@interface PFFacebookUtils : NSObject

///--------------------------------------
/// @name Interacting With Facebook
///--------------------------------------

/*!
 @abstract Initializes Parse Facebook Utils.

 @discussion You must provide your Facebook application ID as the value for FacebookAppID in your bundle's plist file
 as described here: https://developers.facebook.com/docs/getting-started/facebook-sdk-for-ios/

 @warning You must invoke this in order to use the Facebook functionality in Parse.
 
 @param launchOptions The launchOptions as passed to [UIApplicationDelegate application:didFinishLaunchingWithOptions:].
 */
+ (void)initializeFacebookWithApplicationLaunchOptions:(PF_NULLABLE NSDictionary *)launchOptions;

/*!
 @abstract `FBSDKLoginManager` provides methods for configuring login behavior, default audience
 and managing Facebook Access Token.

 @returns An instance of `FBSDKLoginManager` that is used by `PFFacebookUtils`.
 */
+ (FBSDKLoginManager *)facebookLoginManager;

///--------------------------------------
/// @name Logging In
///--------------------------------------

/*!
 @abstract *Asynchronously* logs in a user using Facebook with read permissions.

 @discussion This method delegates to the Facebook SDK to authenticate the user,
 and then automatically logs in (or creates, in the case where it is a new user) a <PFUser>.

 @param permissions Array of read permissions to use.

 @returns The task that has will a have `result` set to <PFUser> if operation succeeds.
 */
+ (BFTask *)logInInBackgroundWithReadPermissions:(PF_NULLABLE NSArray *)permissions;

/*!
 @abstract *Asynchronously* logs in a user using Facebook with read permissions.

 @discussion This method delegates to the Facebook SDK to authenticate the user,
 and then automatically logs in (or creates, in the case where it is a new user) a <PFUser>.

 @param permissions Array of read permissions to use.
 @param block       The block to execute when the log in completes.
 It should have the following signature: `^(PFUser *user, NSError *error)`.
 */
+ (void)logInInBackgroundWithReadPermissions:(PF_NULLABLE NSArray *)permissions
                                       block:(PF_NULLABLE PFUserResultBlock)block;

/*!
 @abstract *Asynchronously* logs in a user using Facebook with publish permissions.

 @discussion This method delegates to the Facebook SDK to authenticate the user,
 and then automatically logs in (or creates, in the case where it is a new user) a <PFUser>.

 @param permissions Array of publish permissions to use.

 @returns The task that has will a have `result` set to <PFUser> if operation succeeds.
 */
+ (BFTask *)logInInBackgroundWithPublishPermissions:(PF_NULLABLE NSArray *)permissions;

/*!
 @abstract *Asynchronously* logs in a user using Facebook with publish permissions.

 @discussion This method delegates to the Facebook SDK to authenticate the user,
 and then automatically logs in (or creates, in the case where it is a new user) a <PFUser>.

 @param permissions Array of publish permissions to use.
 @param block       The block to execute when the log in completes.
 It should have the following signature: `^(PFUser *user, NSError *error)`.
 */
+ (void)logInInBackgroundWithPublishPermissions:(PF_NULLABLE NSArray *)permissions
                                          block:(PF_NULLABLE PFUserResultBlock)block;

/*!
 @abstract *Asynchronously* logs in a user using given Facebook Acess Token.

 @discussion This method delegates to the Facebook SDK to authenticate the user,
 and then automatically logs in (or creates, in the case where it is a new user) a <PFUser>.

 @param accessToken An instance of `FBSDKAccessToken` to use when logging in.

 @returns The task that has will a have `result` set to <PFUser> if operation succeeds.
 */
+ (BFTask *)logInInBackgroundWithAccessToken:(FBSDKAccessToken *)accessToken;

/*!
 @abstract *Asynchronously* logs in a user using given Facebook Acess Token.

 @discussion This method delegates to the Facebook SDK to authenticate the user,
 and then automatically logs in (or creates, in the case where it is a new user) a <PFUser>.

 @param accessToken An instance of `FBSDKAccessToken` to use when logging in.
 @param block       The block to execute when the log in completes.
 It should have the following signature: `^(PFUser *user, NSError *error)`.
 */
+ (void)logInInBackgroundWithAccessToken:(FBSDKAccessToken *)accessToken
                                   block:(PF_NULLABLE PFUserResultBlock)block;

///--------------------------------------
/// @name Linking Users
///--------------------------------------

/*!
 @abstract *Asynchronously* links Facebook with read permissions to an existing <PFUser>.

 @discussion This method delegates to the Facebook SDK to authenticate
 the user, and then automatically links the account to the <PFUser>.
 It will also save any unsaved changes that were made to the `user`.

 @param user        User to link to Facebook.
 @param permissions Array of read permissions to use when logging in with Facebook.

 @returns The task that will have a `result` set to `@YES` if operation succeeds.
 */
+ (BFTask *)linkUserInBackground:(PFUser *)user withReadPermissions:(PF_NULLABLE NSArray *)permissions;

/*!
 @abstract *Asynchronously* links Facebook with read permissions to an existing <PFUser>.

 @discussion This method delegates to the Facebook SDK to authenticate
 the user, and then automatically links the account to the <PFUser>.
 It will also save any unsaved changes that were made to the `user`.

 @param user        User to link to Facebook.
 @param permissions Array of read permissions to use.
 @param block       The block to execute when the linking completes.
 It should have the following signature: `^(BOOL succeeded, NSError *error)`.
 */
+ (void)linkUserInBackground:(PFUser *)user
         withReadPermissions:(PF_NULLABLE NSArray *)permissions
                       block:(PF_NULLABLE PFBooleanResultBlock)block;

/*!
 @abstract *Asynchronously* links Facebook with publish permissions to an existing <PFUser>.

 @discussion This method delegates to the Facebook SDK to authenticate
 the user, and then automatically links the account to the <PFUser>.
 It will also save any unsaved changes that were made to the `user`.

 @param user        User to link to Facebook.
 @param permissions Array of publish permissions to use.

 @returns The task that will have a `result` set to `@YES` if operation succeeds.
 */
+ (BFTask *)linkUserInBackground:(PFUser *)user withPublishPermissions:(NSArray *)permissions;

/*!
 @abstract *Asynchronously* links Facebook with publish permissions to an existing <PFUser>.

 @discussion This method delegates to the Facebook SDK to authenticate
 the user, and then automatically links the account to the <PFUser>.
 It will also save any unsaved changes that were made to the `user`.

 @param user        User to link to Facebook.
 @param permissions Array of publish permissions to use.
 @param block       The block to execute when the linking completes.
 It should have the following signature: `^(BOOL succeeded, NSError *error)`.
 */
+ (void)linkUserInBackground:(PFUser *)user
      withPublishPermissions:(NSArray *)permissions
                       block:(PF_NULLABLE PFBooleanResultBlock)block;

/*!
 @abstract *Asynchronously* links Facebook Access Token to an existing <PFUser>.

 @discussion This method delegates to the Facebook SDK to authenticate
 the user, and then automatically links the account to the <PFUser>.
 It will also save any unsaved changes that were made to the `user`.

 @param user        User to link to Facebook.
 @param accessToken An instance of `FBSDKAccessToken` to use.

 @returns The task that will have a `result` set to `@YES` if operation succeeds.
 */
+ (BFTask *)linkUserInBackground:(PFUser *)user withAccessToken:(FBSDKAccessToken *)accessToken;

/*!
 @abstract *Asynchronously* links Facebook Access Token to an existing <PFUser>.

 @discussion This method delegates to the Facebook SDK to authenticate
 the user, and then automatically links the account to the <PFUser>.
 It will also save any unsaved changes that were made to the `user`.

 @param user        User to link to Facebook.
 @param accessToken An instance of `FBSDKAccessToken` to use.
 @param block       The block to execute when the linking completes.
 It should have the following signature: `^(BOOL succeeded, NSError *error)`.
 */
+ (void)linkUserInBackground:(PFUser *)user
             withAccessToken:(FBSDKAccessToken *)accessToken
                       block:(PF_NULLABLE PFBooleanResultBlock)block;

///--------------------------------------
/// @name Unlinking Users
///--------------------------------------

/*!
 @abstract Unlinks the <PFUser> from a Facebook account *asynchronously*.

 @param user User to unlink from Facebook.
 @returns The task, that encapsulates the work being done.
 */
+ (BFTask *)unlinkUserInBackground:(PFUser *)user;

/*!
 @abstract Unlinks the <PFUser> from a Facebook account *asynchronously*.

 @param user User to unlink from Facebook.
 @param block The block to execute.
 It should have the following argument signature: `^(BOOL succeeded, NSError *error)`.
 */
+ (void)unlinkUserInBackground:(PFUser *)user block:(PF_NULLABLE PFBooleanResultBlock)block;

///--------------------------------------
/// @name Getting Linked State
///--------------------------------------

/*!
 @abstract Whether the user has their account linked to Facebook.

 @param user User to check for a facebook link. The user must be logged in on this device.

 @returns `YES` if the user has their account linked to Facebook, otherwise `NO`.
 */
+ (BOOL)isLinkedWithUser:(PFUser *)user;

@end

PF_ASSUME_NONNULL_END
