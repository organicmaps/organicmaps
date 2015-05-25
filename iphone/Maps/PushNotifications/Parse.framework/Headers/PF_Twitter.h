//
//  PF_Twitter.h
//
//  Copyright 2011-present Parse Inc. All rights reserved.
//

#import <Foundation/Foundation.h>

#import <Parse/PFNullability.h>

PF_ASSUME_NONNULL_BEGIN

@class BFTask;

/*!
 The `PF_Twitter` class is a simple interface for interacting with the Twitter REST API,
 automating sign-in and OAuth signing of requests against the API.
 */
@interface PF_Twitter : NSObject

/*!
 @abstract Consumer key of the application that is used to authorize with Twitter.
 */
@property (PF_NULLABLE_PROPERTY nonatomic, copy) NSString *consumerKey;

/*!
 @abstract Consumer secret of the application that is used to authorize with Twitter.
 */
@property (PF_NULLABLE_PROPERTY nonatomic, copy) NSString *consumerSecret;

/*!
 @abstract Auth token for the current user.
 */
@property (PF_NULLABLE_PROPERTY nonatomic, copy) NSString *authToken;

/*!
 @abstract Auth token secret for the current user.
 */
@property (PF_NULLABLE_PROPERTY nonatomic, copy) NSString *authTokenSecret;

/*!
 @abstract Twitter user id of the currently signed in user.
 */
@property (PF_NULLABLE_PROPERTY nonatomic, copy) NSString *userId;

/*!
 @abstract Twitter screen name of the currently signed in user.
 */
@property (PF_NULLABLE_PROPERTY nonatomic, copy) NSString *screenName;

/*!
 @abstract Displays an auth dialog and populates the authToken, authTokenSecret, userId, and screenName properties
 if the Twitter user grants permission to the application.

 @returns The task, that encapsulates the work being done.
 */
- (BFTask *)authorizeInBackground;

/*!
 @abstract Displays an auth dialog and populates the authToken, authTokenSecret, userId, and screenName properties
 if the Twitter user grants permission to the application.

 @param success Invoked upon successful authorization.
 @param failure Invoked upon an error occurring in the authorization process.
 @param cancel Invoked when the user cancels authorization.
 */
- (void)authorizeWithSuccess:(PF_NULLABLE void (^)(void))success
                     failure:(PF_NULLABLE void (^)(NSError *PF_NULLABLE_S error))failure
                      cancel:(PF_NULLABLE void (^)(void))cancel;

/*!
 @abstract Adds a 3-legged OAuth signature to an `NSMutableURLRequest` based
 upon the properties set for the Twitter object.

 @discussion Use this function to sign requests being made to the Twitter API.

 @param request Request to sign.
 */
- (void)signRequest:(PF_NULLABLE NSMutableURLRequest *)request;

@end

PF_ASSUME_NONNULL_END
