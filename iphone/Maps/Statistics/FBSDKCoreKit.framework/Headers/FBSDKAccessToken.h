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

#import <Foundation/Foundation.h>

#import <FBSDKCoreKit/FBSDKCopying.h>
#import <FBSDKCoreKit/FBSDKGraphRequestConnection.h>
#import <FBSDKCoreKit/FBSDKMacros.h>

/*!
 @abstract Notification indicating that the `currentAccessToken` has changed.
 @discussion the userInfo dictionary of the notification will contain keys
 `FBSDKAccessTokenChangeOldKey` and
 `FBSDKAccessTokenChangeNewKey`.
 */
FBSDK_EXTERN NSString *const FBSDKAccessTokenDidChangeNotification;

/*!
 @abstract A key in the notification's userInfo that will be set
  if and only if the user ID changed between the old and new tokens.
 @discussion Token refreshes can occur automatically with the SDK
  which do not change the user. If you're only interested in user
  changes (such as logging out), you should check for the existence
  of this key. The value is a NSNumber with a boolValue.

  On a fresh start of the app where the SDK reads in the cached value
  of an access token, this key will also exist since the access token
  is moving from a null state (no user) to a non-null state (user).
 */
FBSDK_EXTERN NSString *const FBSDKAccessTokenDidChangeUserID;

/*
 @abstract key in notification's userInfo object for getting the old token.
 @discussion If there was no old token, the key will not be present.
 */
FBSDK_EXTERN NSString *const FBSDKAccessTokenChangeOldKey;

/*
 @abstract key in notification's userInfo object for getting the new token.
 @discussion If there is no new token, the key will not be present.
 */
FBSDK_EXTERN NSString *const FBSDKAccessTokenChangeNewKey;


/*!
 @class FBSDKAccessToken
 @abstract Represents an immutable access token for using Facebook services.
 */
@interface FBSDKAccessToken : NSObject<FBSDKCopying, NSSecureCoding>

/*!
 @abstract Returns the app ID.
 */
@property (readonly, copy, nonatomic) NSString *appID;

/*!
 @abstract Returns the known declined permissions.
 */
@property (readonly, copy, nonatomic) NSSet *declinedPermissions;

/*!
 @abstract Returns the expiration date.
 */
@property (readonly, copy, nonatomic) NSDate *expirationDate;

/*!
 @abstract Returns the known granted permissions.
 */
@property (readonly, copy, nonatomic) NSSet *permissions;

/*!
 @abstract Returns the date the token was last refreshed.
*/
@property (readonly, copy, nonatomic) NSDate *refreshDate;

/*!
 @abstract Returns the opaque token string.
 */
@property (readonly, copy, nonatomic) NSString *tokenString;

/*!
 @abstract Returns the user ID.
 */
@property (readonly, copy, nonatomic) NSString *userID;

/*!
 @abstract Initializes a new instance.
 @param tokenString the opaque token string.
 @param permissions the granted permissions. Note this is converted to NSSet and is only
 an NSArray for the convenience of literal syntax.
 @param declinedPermissions the declined permissions. Note this is converted to NSSet and is only
 an NSArray for the convenience of literal syntax.
 @param appID the app ID.
 @param userID the user ID.
 @param expirationDate the optional expiration date (defaults to distantFuture).
 @param refreshDate the optional date the token was last refreshed (defaults to today).
 @discussion This initializer should only be used for advanced apps that
 manage tokens explicitly. Typical login flows only need to use `FBSDKLoginManager`
 along with `+currentAccessToken`.
 */
- (instancetype)initWithTokenString:(NSString *)tokenString
                        permissions:(NSArray *)permissions
                declinedPermissions:(NSArray *)declinedPermissions
                              appID:(NSString *)appID
                             userID:(NSString *)userID
                     expirationDate:(NSDate *)expirationDate
                        refreshDate:(NSDate *)refreshDate
NS_DESIGNATED_INITIALIZER;

/*!
 @abstract Convenience getter to determine if a permission has been granted
 @param permission  The permission to check.
 */
- (BOOL)hasGranted:(NSString *)permission;

/*!
 @abstract Compares the receiver to another FBSDKAccessToken
 @param token The other token
 @return YES if the receiver's values are equal to the other token's values; otherwise NO
 */
- (BOOL)isEqualToAccessToken:(FBSDKAccessToken *)token;

/*!
 @abstract Returns the "global" access token that represents the currently logged in user.
 @discussion The `currentAccessToken` is a convenient representation of the token of the
 current user and is used by other SDK components (like `FBSDKLoginManager`).
 */
+ (FBSDKAccessToken *)currentAccessToken;

/*!
 @abstract Sets the "global" access token that represents the currently logged in user.
 @param token The access token to set.
 @discussion This will broadcast a notification and save the token to the app keychain.
 */
+ (void)setCurrentAccessToken:(FBSDKAccessToken *)token;

/*!
 @abstract Refresh the current access token's permission state and extend the token's expiration date,
  if possible.
 @param completionHandler an optional callback handler that can surface any errors related to permission refreshing.
 @discussion On a successful refresh, the currentAccessToken will be updated so you typically only need to
  observe the `FBSDKAccessTokenDidChangeNotification` notification.

 If a token is already expired, it cannot be refreshed.
 */
+ (void)refreshCurrentAccessToken:(FBSDKGraphRequestHandler)completionHandler;

@end
