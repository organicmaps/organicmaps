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
#import <Parse/PFObject.h>
#import <Parse/PFSubclassing.h>

NS_ASSUME_NONNULL_BEGIN

typedef void(^PFUserSessionUpgradeResultBlock)(NSError *__nullable error);
typedef void(^PFUserLogoutResultBlock)(NSError *__nullable error);

@class PFQuery PF_GENERIC(PFGenericObject : PFObject *);
@protocol PFUserAuthenticationDelegate;

/*!
 The `PFUser` class is a local representation of a user persisted to the Parse Data.
 This class is a subclass of a <PFObject>, and retains the same functionality of a <PFObject>,
 but also extends it with various user specific methods, like authentication, signing up, and validation uniqueness.

 Many APIs responsible for linking a `PFUser` with Facebook or Twitter have been deprecated in favor of dedicated
 utilities for each social network. See <PFFacebookUtils>, <PFTwitterUtils> and <PFAnonymousUtils> for more information.
 */

@interface PFUser : PFObject <PFSubclassing>

///--------------------------------------
/// @name Accessing the Current User
///--------------------------------------

/*!
 @abstract Gets the currently logged in user from disk and returns an instance of it.

 @returns Returns a `PFUser` that is the currently logged in user. If there is none, returns `nil`.
 */
+ (nullable instancetype)currentUser;

/*!
 @abstract The session token for the `PFUser`.

 @discussion This is set by the server upon successful authentication.
 */
@property (nullable, nonatomic, copy, readonly) NSString *sessionToken;

/*!
 @abstract Whether the `PFUser` was just created from a request.

 @discussion This is only set after a Facebook or Twitter login.
 */
@property (nonatomic, assign, readonly) BOOL isNew;

/*!
 @abstract Whether the user is an authenticated object for the device.

 @discussion An authenticated `PFUser` is one that is obtained via a <signUp> or <logIn> method.
 An authenticated object is required in order to save (with altered values) or delete it.
 */
@property (nonatomic, assign, readonly, getter=isAuthenticated) BOOL authenticated;

///--------------------------------------
/// @name Creating a New User
///--------------------------------------

/*!
 @abstract Creates a new `PFUser` object.

 @returns Returns a new `PFUser` object.
 */
+ (instancetype)user;

/*!
 @abstract Enables automatic creation of anonymous users.

 @discussion After calling this method, <currentUser> will always have a value.
 The user will only be created on the server once the user has been saved,
 or once an object with a relation to that user or an ACL that refers to the user has been saved.

 @warning <[PFObject saveEventually]> will not work on if an item being saved has a relation
 to an automatic user that has never been saved.
 */
+ (void)enableAutomaticUser;

/*!
 @abstract The username for the `PFUser`.
 */
@property (nullable, nonatomic, strong) NSString *username;

/**!
 @abstract The password for the `PFUser`.

 @discussion This will not be filled in from the server with the password.
 It is only meant to be set.
 */
@property (nullable, nonatomic, strong) NSString *password;

/*!
 @abstract The email for the `PFUser`.
 */
@property (nullable, nonatomic, strong) NSString *email;

/*!
 @abstract Signs up the user *synchronously*.

 @discussion This will also enforce that the username isn't already taken.

 @warning Make sure that password and username are set before calling this method.

 @returns Returns `YES` if the sign up was successful, otherwise `NO`.
 */
- (BOOL)signUp PF_SWIFT_UNAVAILABLE;

/*!
 @abstract Signs up the user *synchronously*.

 @discussion This will also enforce that the username isn't already taken.

 @warning Make sure that password and username are set before calling this method.

 @param error Error object to set on error.

 @returns Returns whether the sign up was successful.
 */
- (BOOL)signUp:(NSError **)error;

/*!
 @abstract Signs up the user *asynchronously*.

 @discussion This will also enforce that the username isn't already taken.

 @warning Make sure that password and username are set before calling this method.

 @returns The task, that encapsulates the work being done.
 */
- (BFTask PF_GENERIC(NSNumber *)*)signUpInBackground;

/*!
 @abstract Signs up the user *asynchronously*.

 @discussion This will also enforce that the username isn't already taken.

 @warning Make sure that password and username are set before calling this method.

 @param block The block to execute.
 It should have the following argument signature: `^(BOOL succeeded, NSError *error)`.
 */
- (void)signUpInBackgroundWithBlock:(nullable PFBooleanResultBlock)block;

/*!
 @abstract Signs up the user *asynchronously*.

 @discussion This will also enforce that the username isn't already taken.

 @warning Make sure that password and username are set before calling this method.

 @param target Target object for the selector.
 @param selector The selector that will be called when the asynchrounous request is complete.
 It should have the following signature: `(void)callbackWithResult:(NSNumber *)result error:(NSError *)error`.
 `error` will be `nil` on success and set if there was an error.
 `[result boolValue]` will tell you whether the call succeeded or not.
 */
- (void)signUpInBackgroundWithTarget:(nullable id)target selector:(nullable SEL)selector;

///--------------------------------------
/// @name Logging In
///--------------------------------------

/*!
 @abstract Makes a *synchronous* request to login a user with specified credentials.

 @discussion Returns an instance of the successfully logged in `PFUser`.
 This also caches the user locally so that calls to <currentUser> will use the latest logged in user.

 @param username The username of the user.
 @param password The password of the user.

 @returns Returns an instance of the `PFUser` on success.
 If login failed for either wrong password or wrong username, returns `nil`.
 */
+ (nullable instancetype)logInWithUsername:(NSString *)username
                                  password:(NSString *)password PF_SWIFT_UNAVAILABLE;

/*!
 @abstract Makes a *synchronous* request to login a user with specified credentials.

 @discussion Returns an instance of the successfully logged in `PFUser`.
 This also caches the user locally so that calls to <currentUser> will use the latest logged in user.

 @param username The username of the user.
 @param password The password of the user.
 @param error The error object to set on error.

 @returns Returns an instance of the `PFUser` on success.
 If login failed for either wrong password or wrong username, returns `nil`.
 */
+ (nullable instancetype)logInWithUsername:(NSString *)username
                                  password:(NSString *)password
                                     error:(NSError **)error;

/*!
 @abstract Makes an *asynchronous* request to login a user with specified credentials.

 @discussion Returns an instance of the successfully logged in `PFUser`.
 This also caches the user locally so that calls to <currentUser> will use the latest logged in user.

 @param username The username of the user.
 @param password The password of the user.

 @returns The task, that encapsulates the work being done.
 */
+ (BFTask PF_GENERIC(__kindof PFUser *)*)logInWithUsernameInBackground:(NSString *)username
                                                              password:(NSString *)password;

/*!
 @abstract Makes an *asynchronous* request to login a user with specified credentials.

 @discussion Returns an instance of the successfully logged in `PFUser`.
 This also caches the user locally so that calls to <currentUser> will use the latest logged in user.

 @param username The username of the user.
 @param password The password of the user.
 @param target Target object for the selector.
 @param selector The selector that will be called when the asynchrounous request is complete.
 It should have the following signature: `(void)callbackWithUser:(PFUser *)user error:(NSError *)error`.
 */
+ (void)logInWithUsernameInBackground:(NSString *)username
                             password:(NSString *)password
                               target:(nullable id)target
                             selector:(nullable SEL)selector;

/*!
 @abstract Makes an *asynchronous* request to log in a user with specified credentials.

 @discussion Returns an instance of the successfully logged in `PFUser`.
 This also caches the user locally so that calls to <currentUser> will use the latest logged in user.

 @param username The username of the user.
 @param password The password of the user.
 @param block The block to execute.
 It should have the following argument signature: `^(PFUser *user, NSError *error)`.
 */
+ (void)logInWithUsernameInBackground:(NSString *)username
                             password:(NSString *)password
                                block:(nullable PFUserResultBlock)block;

///--------------------------------------
/// @name Becoming a User
///--------------------------------------

/*!
 @abstract Makes a *synchronous* request to become a user with the given session token.

 @discussion Returns an instance of the successfully logged in `PFUser`.
 This also caches the user locally so that calls to <currentUser> will use the latest logged in user.

 @param sessionToken The session token for the user.

 @returns Returns an instance of the `PFUser` on success.
 If becoming a user fails due to incorrect token, it returns `nil`.
 */
+ (nullable instancetype)become:(NSString *)sessionToken PF_SWIFT_UNAVAILABLE;

/*!
 @abstract Makes a *synchronous* request to become a user with the given session token.

 @discussion Returns an instance of the successfully logged in `PFUser`.
 This will also cache the user locally so that calls to <currentUser> will use the latest logged in user.

 @param sessionToken The session token for the user.
 @param error The error object to set on error.

 @returns Returns an instance of the `PFUser` on success.
 If becoming a user fails due to incorrect token, it returns `nil`.
 */
+ (nullable instancetype)become:(NSString *)sessionToken error:(NSError **)error;

/*!
 @abstract Makes an *asynchronous* request to become a user with the given session token.

 @discussion Returns an instance of the successfully logged in `PFUser`.
 This also caches the user locally so that calls to <currentUser> will use the latest logged in user.

 @param sessionToken The session token for the user.

 @returns The task, that encapsulates the work being done.
 */
+ (BFTask PF_GENERIC(__kindof PFUser *)*)becomeInBackground:(NSString *)sessionToken;

/*!
 @abstract Makes an *asynchronous* request to become a user with the given session token.

 @discussion Returns an instance of the successfully logged in `PFUser`. This also caches the user locally
 so that calls to <currentUser> will use the latest logged in user.

 @param sessionToken The session token for the user.
 @param block The block to execute.
 The block should have the following argument signature: `^(PFUser *user, NSError *error)`.
 */
+ (void)becomeInBackground:(NSString *)sessionToken block:(nullable PFUserResultBlock)block;

/*!
 @abstract Makes an *asynchronous* request to become a user with the given session token.

 @discussion Returns an instance of the successfully logged in `PFUser`. This also caches the user locally
 so that calls to <currentUser> will use the latest logged in user.

 @param sessionToken The session token for the user.
 @param target Target object for the selector.
 @param selector The selector that will be called when the asynchrounous request is complete.
 It should have the following signature: `(void)callbackWithUser:(PFUser *)user error:(NSError *)error`.
 */
+ (void)becomeInBackground:(NSString *)sessionToken
                    target:(__nullable id)target
                  selector:(__nullable SEL)selector;

///--------------------------------------
/// @name Revocable Session
///--------------------------------------

/*!
 @abstract Enables revocable sessions and migrates the currentUser session token to use revocable session if needed.

 @discussion This method is required if you want to use <PFSession> APIs
 and your application's 'Require Revocable Session' setting is turned off on `http://parse.com` app settings.
 After returned `BFTask` completes - <PFSession> class and APIs will be available for use.

 @returns An instance of `BFTask` that is completed when revocable
 sessions are enabled and currentUser token is migrated.
 */
+ (BFTask *)enableRevocableSessionInBackground;

/*!
 @abstract Enables revocable sessions and upgrades the currentUser session token to use revocable session if needed.

 @discussion This method is required if you want to use <PFSession> APIs
 and legacy sessions are enabled in your application settings on `http://parse.com/`.
 After returned `BFTask` completes - <PFSession> class and APIs will be available for use.

 @param block Block that will be called when revocable sessions are enabled and currentUser token is migrated.
 */
+ (void)enableRevocableSessionInBackgroundWithBlock:(nullable PFUserSessionUpgradeResultBlock)block;

///--------------------------------------
/// @name Logging Out
///--------------------------------------

/*!
 @abstract *Synchronously* logs out the currently logged in user on disk.
 */
+ (void)logOut;

/*!
 @abstract *Asynchronously* logs out the currently logged in user.

 @discussion This will also remove the session from disk, log out of linked services
 and all future calls to <currentUser> will return `nil`. This is preferrable to using <logOut>,
 unless your code is already running from a background thread.

 @returns An instance of `BFTask`, that is resolved with `nil` result when logging out completes.
 */
+ (BFTask *)logOutInBackground;

/*!
 @abstract *Asynchronously* logs out the currently logged in user.

 @discussion This will also remove the session from disk, log out of linked services
 and all future calls to <currentUser> will return `nil`. This is preferrable to using <logOut>,
 unless your code is already running from a background thread.

 @param block A block that will be called when logging out completes or fails.
 */
+ (void)logOutInBackgroundWithBlock:(nullable PFUserLogoutResultBlock)block;

///--------------------------------------
/// @name Requesting a Password Reset
///--------------------------------------

/*!
 @abstract *Synchronously* Send a password reset request for a specified email.

 @discussion If a user account exists with that email, an email will be sent to that address
 with instructions on how to reset their password.

 @param email Email of the account to send a reset password request.

 @returns Returns `YES` if the reset email request is successful. `NO` - if no account was found for the email address.
 */
+ (BOOL)requestPasswordResetForEmail:(NSString *)email PF_SWIFT_UNAVAILABLE;

/*!
 @abstract *Synchronously* send a password reset request for a specified email and sets an error object.

 @discussion If a user account exists with that email, an email will be sent to that address
 with instructions on how to reset their password.

 @param email Email of the account to send a reset password request.
 @param error Error object to set on error.
 @returns Returns `YES` if the reset email request is successful. `NO` - if no account was found for the email address.
 */
+ (BOOL)requestPasswordResetForEmail:(NSString *)email error:(NSError **)error;

/*!
 @abstract Send a password reset request asynchronously for a specified email and sets an
 error object. If a user account exists with that email, an email will be sent to
 that address with instructions on how to reset their password.
 @param email Email of the account to send a reset password request.
 @returns The task, that encapsulates the work being done.
 */
+ (BFTask PF_GENERIC(NSNumber *)*)requestPasswordResetForEmailInBackground:(NSString *)email;

/*!
 @abstract Send a password reset request *asynchronously* for a specified email.

 @discussion If a user account exists with that email, an email will be sent to that address
 with instructions on how to reset their password.

 @param email Email of the account to send a reset password request.
 @param block The block to execute.
 It should have the following argument signature: `^(BOOL succeeded, NSError *error)`.
 */
+ (void)requestPasswordResetForEmailInBackground:(NSString *)email
                                           block:(nullable PFBooleanResultBlock)block;

/*!
 @abstract Send a password reset request *asynchronously* for a specified email and sets an error object.

 @discussion If a user account exists with that email, an email will be sent to that address
 with instructions on how to reset their password.

 @param email Email of the account to send a reset password request.
 @param target Target object for the selector.
 @param selector The selector that will be called when the asynchronous request is complete.
 It should have the following signature: `(void)callbackWithResult:(NSNumber *)result error:(NSError *)error`.
 `error` will be `nil` on success and set if there was an error.
 `[result boolValue]` will tell you whether the call succeeded or not.
 */
+ (void)requestPasswordResetForEmailInBackground:(NSString *)email
                                          target:(__nullable id)target
                                        selector:(__nullable SEL)selector;

///--------------------------------------
/// @name Third-party Authentication
///--------------------------------------

/*!
 @abstract Registers a third party authentication delegate.

 @note This method shouldn't be invoked directly unless developing a third party authentication library.
 @see PFUserAuthenticationDelegate

 @param delegate The third party authenticaiton delegate to be registered.
 @param authType The name of the type of third party authentication source.
 */
+ (void)registerAuthenticationDelegate:(id<PFUserAuthenticationDelegate>)delegate forAuthType:(NSString *)authType;

/*!
 @abstract Logs in a user with third party authentication credentials.

 @note This method shouldn't be invoked directly unless developing a third party authentication library.
 @see PFUserAuthenticationDelegate

 @param authType The name of the type of third party authentication source.
 @param authData The user credentials of the third party authentication source.

 @returns A `BFTask` that is resolved to `PFUser` when logging in completes.
 */
+ (BFTask PF_GENERIC(PFUser *)*)logInWithAuthTypeInBackground:(NSString *)authType
                                                     authData:(NSDictionary PF_GENERIC(NSString *, NSString *)*)authData;

/*!
 @abstract Links this user to a third party authentication library.

 @note This method shouldn't be invoked directly unless developing a third party authentication library.
 @see PFUserAuthenticationDelegate

 @param authType The name of the type of third party authentication source.
 @param authData The user credentials of the third party authentication source.

 @returns A `BFTask` that is resolved to `@YES` if linking succeeds.
 */
- (BFTask PF_GENERIC(NSNumber *)*)linkWithAuthTypeInBackground:(NSString *)authType
                                                      authData:(NSDictionary PF_GENERIC(NSString *, NSString *)*)authData;

/*!
 @abstract Unlinks this user from a third party authentication library.

 @note This method shouldn't be invoked directly unless developing a third party authentication library.
 @see PFUserAuthenticationDelegate

 @param authType The name of the type of third party authentication source.

 @returns A `BFTask` that is resolved to `@YES` if unlinking succeeds.
 */
- (BFTask PF_GENERIC(NSNumber *)*)unlinkWithAuthTypeInBackground:(NSString *)authType;

/*!
 @abstract Indicates whether this user is linked with a third party authentication library of a specific type.

 @note This method shouldn't be invoked directly unless developing a third party authentication library.
 @see PFUserAuthenticationDelegate

 @param authType The name of the type of third party authentication source.

 @returns `YES` if the user is linked with a provider, otherwise `NO`.
 */
- (BOOL)isLinkedWithAuthType:(NSString *)authType;

@end

NS_ASSUME_NONNULL_END
