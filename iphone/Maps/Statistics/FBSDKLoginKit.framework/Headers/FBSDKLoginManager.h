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

#import <Accounts/Accounts.h>
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

@class FBSDKLoginManagerLoginResult;

/*!
 @abstract Describes the call back to the FBSDKLoginManager
 @param result the result of the authorization
 @param error the authorization error, if any.
 */
typedef void (^FBSDKLoginManagerRequestTokenHandler)(FBSDKLoginManagerLoginResult *result, NSError *error);


/*!
 @typedef FBSDKDefaultAudience enum

 @abstract
 Passed to open to indicate which default audience to use for sessions that post data to Facebook.

 @discussion
 Certain operations such as publishing a status or publishing a photo require an audience. When the user
 grants an application permission to perform a publish operation, a default audience is selected as the
 publication ceiling for the application. This enumerated value allows the application to select which
 audience to ask the user to grant publish permission for.
 */
typedef NS_ENUM(NSUInteger, FBSDKDefaultAudience)
{
  /*! Indicates that the user's friends are able to see posts made by the application */
  FBSDKDefaultAudienceFriends = 0,
  /*! Indicates that only the user is able to see posts made by the application */
  FBSDKDefaultAudienceOnlyMe,
  /*! Indicates that all Facebook users are able to see posts made by the application */
  FBSDKDefaultAudienceEveryone,
};

/*!
 @typedef FBSDKLoginBehavior enum

 @abstract
 Passed to the \c FBSDKLoginManager to indicate how Facebook Login should be attempted.

 @discussion
 Facebook Login authorizes the application to act on behalf of the user, using the user's
 Facebook account. Usually a Facebook Login will rely on an account maintained outside of
 the application, by the native Facebook application, the browser, or perhaps the device
 itself. This avoids the need for a user to enter their username and password directly, and
 provides the most secure and lowest friction way for a user to authorize the application to
 interact with Facebook.

 The \c FBSDKLoginBehavior enum specifies which log in method should be attempted. Most
 applications will use the default, which attempts a login through the Facebook app and falls
 back to the browser if needed.

 If log in cannot be completed using the specificed behavior, the completion handler will
 be invoked with an error in the \c FBSDKErrorDomain and a code of \c FBSDKLoginUnknownErrorCode.
 */
typedef NS_ENUM(NSUInteger, FBSDKLoginBehavior)
{
  /*!
   @abstract Attempts log in through the native Facebook app. If the Facebook app is
   not installed on the device, falls back to \c FBSDKLoginBehaviorBrowser. This is the
   default behavior.
   */
  FBSDKLoginBehaviorNative = 0,
  /*!
   @abstract Attempts log in through the Safari browser
   */
  FBSDKLoginBehaviorBrowser,
  /*!
   @abstract Attempts log in through the Facebook account currently signed in through
   the device Settings.
   @note If the account is not available to the app (either not configured by user or
   as determined by the SDK) this behavior falls back to \c FBSDKLoginBehaviorNative.
   */
  FBSDKLoginBehaviorSystemAccount,
  /*!
   @abstract Attemps log in through a modal \c UIWebView pop up

   @note This behavior is only available to certain types of apps. Please check the Facebook
   Platform Policy to verify your app meets the restrictions.
   */
  FBSDKLoginBehaviorWeb,
};

/*!
 @abstract `FBSDKLoginManager` provides methods for logging the user in and out.
 @discussion `FBSDKLoginManager` works directly with `[FBSDKAccessToken currentAccessToken]` and
  sets the "currentAccessToken" upon successful authorizations (or sets `nil` in case of `logOut`).

 You should check `[FBSDKAccessToken currentAccessToken]` before calling logIn* to see if there is
 a cached token available (typically in your viewDidLoad).

 If you are managing your own token instances outside of "currentAccessToken", you will need to set
 "currentAccessToken" before calling logIn* to authorize futher permissions on your tokens.
 */
@interface FBSDKLoginManager : NSObject

/*!
 @abstract the default audience.
 @discussion you should set this if you intend to ask for publish permissions.
 */
@property (assign, nonatomic) FBSDKDefaultAudience defaultAudience;

/*!
 @abstract the login behavior
 @discussion you should only set this if you want an explicit login flow; otherwise, the SDK
  will automatically determine the best flow available.
 */
@property (assign, nonatomic) FBSDKLoginBehavior loginBehavior;

/*!
 @deprecated use logInWithReadPermissions:fromViewController:handler: instead
 */
- (void)logInWithReadPermissions:(NSArray *)permissions handler:(FBSDKLoginManagerRequestTokenHandler)handler
__attribute__ ((deprecated("use logInWithReadPermissions:fromViewController:handler: instead")));

/*!
 @deprecated use logInWithPublishPermissions:fromViewController:handler: instead
 */
- (void)logInWithPublishPermissions:(NSArray *)permissions handler:(FBSDKLoginManagerRequestTokenHandler)handler
__attribute__ ((deprecated("use logInWithPublishPermissions:fromViewController:handler: instead")));

/*!
 @abstract Logs the user in or authorizes additional permissions.
 @param permissions the optional array of permissions. Note this is converted to NSSet and is only
  an NSArray for the convenience of literal syntax.
 @param fromViewController the view controller to present from. If nil, the topmost view controller will be
  automatically determined as best as possible.
 @param handler the callback.
 @discussion Use this method when asking for read permissions. You should only ask for permissions when they
  are needed and explain the value to the user. You can inspect the result.declinedPermissions to also
  provide more information to the user if they decline permissions.

 If `[FBSDKAccessToken currentAccessToken]` is not nil, it will be treated as a reauthorization for that user
  and will pass the "rerequest" flag to the login dialog.

 This method will present UI the user. You typically should check if `[FBSDKAccessToken currentAccessToken]`
 already contains the permissions you need before asking to reduce unnecessary app switching. For example,
 you could make that check at viewDidLoad.
 */
- (void)logInWithReadPermissions:(NSArray *)permissions
              fromViewController:(UIViewController *)fromViewController
                         handler:(FBSDKLoginManagerRequestTokenHandler)handler;

/*!
 @abstract Logs the user in or authorizes additional permissions.
 @param permissions the optional array of permissions. Note this is converted to NSSet and is only
 an NSArray for the convenience of literal syntax.
 @param fromViewController the view controller to present from. If nil, the topmost view controller will be
 automatically determined as best as possible.
 @param handler the callback.
 @discussion Use this method when asking for publish permissions. You should only ask for permissions when they
 are needed and explain the value to the user. You can inspect the result.declinedPermissions to also
 provide more information to the user if they decline permissions.

 If `[FBSDKAccessToken currentAccessToken]` is not nil, it will be treated as a reauthorization for that user
 and will pass the "rerequest" flag to the login dialog.

 This method will present UI the user. You typically should check if `[FBSDKAccessToken currentAccessToken]`
 already contains the permissions you need before asking to reduce unnecessary app switching. For example,
 you could make that check at viewDidLoad.
 */
- (void)logInWithPublishPermissions:(NSArray *)permissions
                 fromViewController:(UIViewController *)fromViewController
                            handler:(FBSDKLoginManagerRequestTokenHandler)handler;

/*!
 @abstract Logs the user out
 @discussion This calls [FBSDKAccessToken setCurrentAccessToken:nil] and [FBSDKProfile setCurrentProfile:nil].
 */
- (void)logOut;

/*!
 @method

 @abstract Issues an asychronous renewCredentialsForAccount call to the device's Facebook account store.

 @param handler The completion handler to call when the renewal is completed. This can be invoked on an arbitrary thread.

 @discussion This can be used to explicitly renew account credentials and is provided as a convenience wrapper around
 `[ACAccountStore renewCredentialsForAccount:completion]`. Note the method will not issue the renewal call if the the
 Facebook account has not been set on the device, or if access had not been granted to the account (though the handler
 wil receive an error).

 If the `[FBSDKAccessToken currentAccessToken]` was from the account store, a succesful renewal will also set
 a new "currentAccessToken".
 */
+ (void)renewSystemCredentials:(void (^)(ACAccountCredentialRenewResult result, NSError *error))handler;

@end
