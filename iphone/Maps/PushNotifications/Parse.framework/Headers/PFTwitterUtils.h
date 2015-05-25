//
//  PFTwitterUtils.h
//
//  Copyright 2011-present Parse Inc. All rights reserved.
//

#import <Foundation/Foundation.h>

#import <Parse/PFConstants.h>
#import <Parse/PFUser.h>
#import <Parse/PF_Twitter.h>

PF_ASSUME_NONNULL_BEGIN

@class BFTask;

/*!
 The `PFTwitterUtils` class provides utility functions for working with Twitter in a Parse application.

 This class is currently for iOS only.
 */
@interface PFTwitterUtils : NSObject

///--------------------------------------
/// @name Interacting With Twitter
///--------------------------------------

/*!
 @abstract Gets the instance of the <PF_Twitter> object that Parse uses.

 @returns An instance of <PF_Twitter> object.
 */
+ (PF_NULLABLE PF_Twitter *)twitter;

/*!
 @abstract Initializes the Twitter singleton.

 @warning You must invoke this in order to use the Twitter functionality in Parse.

 @param consumerKey Your Twitter application's consumer key.
 @param consumerSecret Your Twitter application's consumer secret.
 */
+ (void)initializeWithConsumerKey:(NSString *)consumerKey
                   consumerSecret:(NSString *)consumerSecret;

/*!
 @abstract Whether the user has their account linked to Twitter.

 @param user User to check for a Twitter link. The user must be logged in on this device.

 @returns `YES` if the user has their account linked to Twitter, otherwise `NO`.
 */
+ (BOOL)isLinkedWithUser:(PF_NULLABLE PFUser *)user;

///--------------------------------------
/// @name Logging In & Creating Twitter-Linked Users
///--------------------------------------

/*!
 @abstract *Asynchronously* logs in a user using Twitter.

 @discussion This method delegates to Twitter to authenticate the user,
 and then automatically logs in (or creates, in the case where it is a new user) a <PFUser>.

 @returns The task, that encapsulates the work being done.
 */
+ (BFTask *)logInInBackground;

/*!
 @abstract *Asynchronously* logs in a user using Twitter.

 @discussion This method delegates to Twitter to authenticate the user,
 and then automatically logs in (or creates, in the case where it is a new user) <PFUser>.

 @param block The block to execute.
 It should have the following argument signature: `^(PFUser *user, NSError *error)`.
 */
+ (void)logInWithBlock:(PF_NULLABLE PFUserResultBlock)block;

/*
 @abstract *Asynchronously* Logs in a user using Twitter.

 @discussion This method delegates to Twitter to authenticate the user,
 and then automatically logs in (or creates, in the case where it is a new user) a <PFUser>.

 @param target Target object for the selector
 @param selector The selector that will be called when the asynchrounous request is complete.
 It should have the following signature: `(void)callbackWithUser:(PFUser *)user error:(NSError **)error`.
 */
+ (void)logInWithTarget:(PF_NULLABLE_S id)target selector:(PF_NULLABLE_S SEL)selector;

/*!
 @abstract *Asynchronously* logs in a user using Twitter.

 @discussion Allows you to handle user login to Twitter, then provide authentication
 data to log in (or create, in the case where it is a new user) the <PFUser>.

 @param twitterId The id of the Twitter user being linked.
 @param screenName The screen name of the Twitter user being linked.
 @param authToken The auth token for the user's session.
 @param authTokenSecret The auth token secret for the user's session.

 @returns The task, that encapsulates the work being done.
 */
+ (BFTask *)logInWithTwitterIdInBackground:(NSString *)twitterId
                                screenName:(NSString *)screenName
                                 authToken:(NSString *)authToken
                           authTokenSecret:(NSString *)authTokenSecret;

/*!
 @abstract Logs in a user using Twitter.

 @discussion Allows you to handle user login to Twitter, then provide authentication data
 to log in (or create, in the case where it is a new user) the <PFUser>.

 @param twitterId The id of the Twitter user being linked
 @param screenName The screen name of the Twitter user being linked
 @param authToken The auth token for the user's session
 @param authTokenSecret The auth token secret for the user's session
 @param block The block to execute.
 It should have the following argument signature: `^(PFUser *user, NSError *error)`.
 */
+ (void)logInWithTwitterId:(NSString *)twitterId
                screenName:(NSString *)screenName
                 authToken:(NSString *)authToken
           authTokenSecret:(NSString *)authTokenSecret
                     block:(PF_NULLABLE PFUserResultBlock)block;

/*
 @abstract Logs in a user using Twitter.

 @discussion Allows you to handle user login to Twitter, then provide authentication data
 to log in (or create, in the case where it is a new user) the <PFUser>.

 @param twitterId The id of the Twitter user being linked.
 @param screenName The screen name of the Twitter user being linked.
 @param authToken The auth token for the user's session.
 @param authTokenSecret The auth token secret for the user's session.
 @param target Target object for the selector.
 @param selector The selector that will be called when the asynchronous request is complete.
 It should have the following signature: `(void)callbackWithUser:(PFUser *)user error:(NSError *)error`.
 */
+ (void)logInWithTwitterId:(NSString *)twitterId
                screenName:(NSString *)screenName
                 authToken:(NSString *)authToken
           authTokenSecret:(NSString *)authTokenSecret
                    target:(PF_NULLABLE_S id)target
                  selector:(PF_NULLABLE_S SEL)selector;

///--------------------------------------
/// @name Linking Users with Twitter
///--------------------------------------

/*!
 @abstract *Asynchronously* links Twitter to an existing PFUser.

 @discussion This method delegates to Twitter to authenticate the user,
 and then automatically links the account to the <PFUser>.

 @param user User to link to Twitter.

 @deprecated Please use `[PFTwitterUtils linkUserInBackground:]` instead.
 */
+ (void)linkUser:(PFUser *)user PARSE_DEPRECATED("Please use +linkUserInBackground: instead.");

/*!
 @abstract *Asynchronously* links Twitter to an existing <PFUser>.

 @discussion This method delegates to Twitter to authenticate the user,
 and then automatically links the account to the <PFUser>.

 @param user User to link to Twitter.

 @returns The task, that encapsulates the work being done.
 */
+ (BFTask *)linkUserInBackground:(PFUser *)user;

/*!
 @abstract *Asynchronously* links Twitter to an existing <PFUser>.

 @discussion This method delegates to Twitter to authenticate the user,
 and then automatically links the account to the <PFUser>.

 @param user User to link to Twitter.
 @param block The block to execute.
 It should have the following argument signature: `^(BOOL success, NSError *error)`.
 */
+ (void)linkUser:(PFUser *)user block:(PF_NULLABLE PFBooleanResultBlock)block;

/*
 @abstract *Asynchronously* links Twitter to an existing <PFUser>.

 @discussion This method delegates to Twitter to authenticate the user,
 and then automatically links the account to the <PFUser>.

 @param user User to link to Twitter.
 @param target Target object for the selector
 @param selector The selector that will be called when the asynchrounous request is complete.
 It should have the following signature: `(void)callbackWithResult:(NSNumber *)result error:(NSError *)error`.
 */
+ (void)linkUser:(PFUser *)user
          target:(PF_NULLABLE_S id)target
        selector:(PF_NULLABLE_S SEL)selector;

/*!
 @abstract *Asynchronously* links Twitter to an existing PFUser asynchronously.

 @discussion Allows you to handle user login to Twitter,
 then provide authentication data to link the account to the <PFUser>.

 @param user User to link to Twitter.
 @param twitterId The id of the Twitter user being linked.
 @param screenName The screen name of the Twitter user being linked.
 @param authToken The auth token for the user's session.
 @param authTokenSecret The auth token secret for the user's session.
 @returns The task, that encapsulates the work being done.
 */
+ (BFTask *)linkUserInBackground:(PFUser *)user
                       twitterId:(NSString *)twitterId
                      screenName:(NSString *)screenName
                       authToken:(NSString *)authToken
                 authTokenSecret:(NSString *)authTokenSecret;

/*!
 @abstract *Asynchronously* links Twitter to an existing <PFUser>.

 @discussionAllows you to handle user login to Twitter,
 then provide authentication data to link the account to the <PFUser>.

 @param user User to link to Twitter.
 @param twitterId The id of the Twitter user being linked.
 @param screenName The screen name of the Twitter user being linked.
 @param authToken The auth token for the user's session.
 @param authTokenSecret The auth token secret for the user's session.
 @param block The block to execute.
 It should have the following argument signature: `^(BOOL success, NSError *error)`.
 */
+ (void)linkUser:(PFUser *)user
       twitterId:(NSString *)twitterId
      screenName:(NSString *)screenName
       authToken:(NSString *)authToken
 authTokenSecret:(NSString *)authTokenSecret
           block:(PF_NULLABLE PFBooleanResultBlock)block;

/*
 @abstract Links Twitter to an existing <PFUser>.

 @discussion This method allows you to handle user login to Twitter,
 then provide authentication data to link the account to the <PFUser>.

 @param user User to link to Twitter.
 @param twitterId The id of the Twitter user being linked.
 @param screenName The screen name of the Twitter user being linked.
 @param authToken The auth token for the user's session.
 @param authTokenSecret The auth token secret for the user's session.
 @param target Target object for the selector.
 @param selector The selector that will be called when the asynchronous request is complete.
 It should have the following signature: `(void)callbackWithResult:(NSNumber *)result error:(NSError *)error`.
 */
+ (void)linkUser:(PFUser *)user
       twitterId:(NSString *)twitterId
      screenName:(NSString *)screenName
       authToken:(NSString *)authToken
 authTokenSecret:(NSString *)authTokenSecret
          target:(PF_NULLABLE_S id)target
        selector:(PF_NULLABLE_S SEL)selector;

///--------------------------------------
/// @name Unlinking Users from Twitter
///--------------------------------------

/*!
 @abstract *Synchronously* unlinks the <PFUser> from a Twitter account.

 @param user User to unlink from Twitter.

 @returns Returns true if the unlink was successful.
 */
+ (BOOL)unlinkUser:(PFUser *)user;

/*!
 @abstract *Synchronously* unlinks the PFUser from a Twitter account.

 @param user User to unlink from Twitter.
 @param error Error object to set on error.

 @returns Returns `YES` if the unlink was successful, otherwise `NO`.
 */
+ (BOOL)unlinkUser:(PFUser *)user error:(NSError **)error;

/*!
 @abstract Makes an *asynchronous* request to unlink a user from a Twitter account.

 @param user User to unlink from Twitter.

 @returns The task, that encapsulates the work being done.
 */
+ (BFTask *)unlinkUserInBackground:(PFUser *)user;

/*!
 @abstract Makes an *asynchronous* request to unlink a user from a Twitter account.

 @param user User to unlink from Twitter.
 @param block The block to execute.
 It should have the following argument signature: `^(BOOL succeeded, NSError *error)`.
 */
+ (void)unlinkUserInBackground:(PFUser *)user
                         block:(PF_NULLABLE PFBooleanResultBlock)block;

/*
 @abstract Makes an *asynchronous* request to unlink a user from a Twitter account.

 @param user User to unlink from Twitter
 @param target Target object for the selector
 @param selector The selector that will be called when the asynchrounous request is complete.
 */
+ (void)unlinkUserInBackground:(PFUser *)user
                        target:(PF_NULLABLE_S id)target
                      selector:(PF_NULLABLE_S SEL)selector;

@end

PF_ASSUME_NONNULL_END
