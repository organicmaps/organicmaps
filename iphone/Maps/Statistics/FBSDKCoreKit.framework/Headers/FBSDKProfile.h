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

#import "FBSDKMacros.h"
#import "FBSDKProfilePictureView.h"

/*!
 @abstract Notification indicating that the `currentProfile` has changed.
 @discussion the userInfo dictionary of the notification will contain keys
 `FBSDKProfileChangeOldKey` and
 `FBSDKProfileChangeNewKey`.
 */
FBSDK_EXTERN NSString *const FBSDKProfileDidChangeNotification;

/*  @abstract key in notification's userInfo object for getting the old profile.
 @discussion If there was no old profile, the key will not be present.
 */
FBSDK_EXTERN NSString *const FBSDKProfileChangeOldKey;

/*  @abstract key in notification's userInfo object for getting the new profile.
 @discussion If there is no new profile, the key will not be present.
 */
FBSDK_EXTERN NSString *const FBSDKProfileChangeNewKey;

/*!
 @abstract Represents an immutable Facebook profile
 @discussion This class provides a global "currentProfile" instance to more easily
 add social context to your application. When the profile changes, a notification is
 posted so that you can update relevant parts of your UI and is persisted to NSUserDefaults.

 Typically, you will want to call `enableUpdatesOnAccessTokenChange:YES` so that
 it automatically observes changes to the `[FBSDKAccessToken currentAccessToken]`.

 You can use this class to build your own `FBSDKProfilePictureView` or in place of typical requests to "/me".
 */
@interface FBSDKProfile : NSObject<NSCopying, NSSecureCoding>

/*!
 @abstract initializes a new instance.
 @param userID the user ID
 @param firstName the user's first name
 @param middleName the user's middle name
 @param lastName the user's last name
 @param name the user's complete name
 @param linkURL the link for this profile
 @param refreshDate the optional date this profile was fetched. Defaults to [NSDate date].
 */
- (instancetype)initWithUserID:(NSString *)userID
                     firstName:(NSString *)firstName
                    middleName:(NSString *)middleName
                      lastName:(NSString *)lastName
                          name:(NSString *)name
                       linkURL:(NSURL *)linkURL
                   refreshDate:(NSDate *)refreshDate NS_DESIGNATED_INITIALIZER;
/*!
 @abstract The user id
 */
@property (nonatomic, readonly) NSString *userID;
/*!
 @abstract The user's first name
 */
@property (nonatomic, readonly) NSString *firstName;
/*!
 @abstract The user's middle name
 */
@property (nonatomic, readonly) NSString *middleName;
/*!
 @abstract The user's last name
 */
@property (nonatomic, readonly) NSString *lastName;
/*!
 @abstract The user's complete name
 */
@property (nonatomic, readonly) NSString *name;
/*!
 @abstract A URL to the user's profile.
 @discussion Consider using Bolts and `FBSDKAppLinkResolver` to resolve this
 to an app link to link directly to the user's profile in the Facebook app.
 */
@property (nonatomic, readonly) NSURL *linkURL;

/*!
 @abstract The last time the profile data was fetched.
 */
@property (nonatomic, readonly) NSDate *refreshDate;

/*!
 @abstract Gets the current FBSDKProfile instance.
 */
+ (FBSDKProfile *)currentProfile;

/*!
 @abstract Sets the current instance and posts the appropriate notification if the profile parameter is different
 than the receiver.
 @param profile the profile to set
 @discussion This persists the profile to NSUserDefaults.
 */
+ (void)setCurrentProfile:(FBSDKProfile *)profile;

/*!
 @abstract Indicates if `currentProfile` will automatically observe `FBSDKAccessTokenDidChangeNotification` notifications
 @param enable YES is observing
 @discussion If observing, this class will issue a graph request for public profile data when the current token's userID
 differs from the current profile. You can observe `FBSDKProfileDidChangeNotification` for when the profile is updated.

 Note that if `[FBSDKAccessToken currentAccessToken]` is unset, the `currentProfile` instance remains. It's also possible
 for `currentProfile` to return nil until the data is fetched.
 */
+ (void)enableUpdatesOnAccessTokenChange:(BOOL)enable;

/*!
 @abstract A convenience method for returning a Graph API path for retrieving the user's profile image.
 @discussion You can pass this to a `FBSDKGraphRequest` instance to download the image.
 @param mode The picture mode
 @param size The height and width. This will be rounded to integer precision.
 */
- (NSString *)imagePathForPictureMode:(FBSDKProfilePictureMode)mode size:(CGSize)size;

/*!
 @abstract Returns YES if the profile is equivalent to the receiver.
 @param profile the profile to compare to.
 */
- (BOOL)isEqualToProfile:(FBSDKProfile *)profile;
@end
