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

NS_ASSUME_NONNULL_BEGIN

/**
 Provides utility functions for working with Anonymously logged-in users.
 Anonymous users have some unique characteristics:

 - Anonymous users don't need a user name or password.
 - Once logged out, an anonymous user cannot be recovered.
 - When the current user is anonymous, the following methods can be used to switch
 to a different user or convert the anonymous user into a regular one:
 - signUp converts an anonymous user to a standard user with the given username and password.
 Data associated with the anonymous user is retained.
 - logIn switches users without converting the anonymous user.
 Data associated with the anonymous user will be lost.
 - Service logIn (e.g. Facebook, Twitter) will attempt to convert
 the anonymous user into a standard user by linking it to the service.
 If a user already exists that is linked to the service, it will instead switch to the existing user.
 - Service linking (e.g. Facebook, Twitter) will convert the anonymous user
 into a standard user by linking it to the service.
 */
@interface PFAnonymousUtils : NSObject

///--------------------------------------
/// @name Creating an Anonymous User
///--------------------------------------

/**
 Creates an anonymous user asynchronously and sets as a result to `BFTask`.

 @return The task, that encapsulates the work being done.
 */
+ (BFTask PF_GENERIC(PFUser *)*)logInInBackground;

/**
 Creates an anonymous user.

 @param block The block to execute when anonymous user creation is complete.
 It should have the following argument signature: `^(PFUser *user, NSError *error)`.
 */
+ (void)logInWithBlock:(nullable PFUserResultBlock)block;

/*
 Creates an anonymous user.

 @param target Target object for the selector.
 @param selector The selector that will be called when the asynchronous request is complete.
 It should have the following signature: `(void)callbackWithUser:(PFUser *)user error:(NSError *)error`.
 */
+ (void)logInWithTarget:(nullable id)target selector:(nullable SEL)selector;

///--------------------------------------
/// @name Determining Whether a User is Anonymous
///--------------------------------------

/**
 Whether the `PFUser` object is logged in anonymously.

 @param user `PFUser` object to check for anonymity. The user must be logged in on this device.

 @return `YES` if the user is anonymous. `NO` if the user is not the current user or is not anonymous.
 */
+ (BOOL)isLinkedWithUser:(nullable PFUser *)user;

@end

NS_ASSUME_NONNULL_END
