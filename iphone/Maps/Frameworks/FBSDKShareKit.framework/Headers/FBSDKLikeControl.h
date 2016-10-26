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

#import <FBSDKShareKit/FBSDKLikeObjectType.h>
#import <FBSDKShareKit/FBSDKLiking.h>

/*!
 @typedef NS_ENUM (NSUInteger, FBSDKLikeControlAuxiliaryPosition)

 @abstract Specifies the position of the auxiliary view relative to the like button.
 */
typedef NS_ENUM(NSUInteger, FBSDKLikeControlAuxiliaryPosition)
{
  /*! The auxiliary view is inline with the like button. */
  FBSDKLikeControlAuxiliaryPositionInline,
  /*! The auxiliary view is above the like button. */
  FBSDKLikeControlAuxiliaryPositionTop,
  /*! The auxiliary view is below the like button. */
  FBSDKLikeControlAuxiliaryPositionBottom,
};

/*!
 @abstract Converts an FBSDKLikeControlAuxiliaryPosition to an NSString.
 */
FBSDK_EXTERN NSString *NSStringFromFBSDKLikeControlAuxiliaryPosition(FBSDKLikeControlAuxiliaryPosition auxiliaryPosition);

/*!
 @typedef NS_ENUM(NSUInteger, FBSDKLikeControlHorizontalAlignment)

 @abstract Specifies the horizontal alignment for FBSDKLikeControlStyleStandard with
 FBSDKLikeControlAuxiliaryPositionTop or FBSDKLikeControlAuxiliaryPositionBottom.
 */
typedef NS_ENUM(NSUInteger, FBSDKLikeControlHorizontalAlignment)
{
  /*! The subviews are left aligned. */
  FBSDKLikeControlHorizontalAlignmentLeft,
  /*! The subviews are center aligned. */
  FBSDKLikeControlHorizontalAlignmentCenter,
  /*! The subviews are right aligned. */
  FBSDKLikeControlHorizontalAlignmentRight,
};

/*!
 @abstract Converts an FBSDKLikeControlHorizontalAlignment to an NSString.
 */
FBSDK_EXTERN NSString *NSStringFromFBSDKLikeControlHorizontalAlignment(FBSDKLikeControlHorizontalAlignment horizontalAlignment);

/*!
 @typedef NS_ENUM (NSUInteger, FBSDKLikeControlStyle)

 @abstract Specifies the style of a like control.
 */
typedef NS_ENUM(NSUInteger, FBSDKLikeControlStyle)
{
  /*! Displays the button and the social sentence. */
  FBSDKLikeControlStyleStandard = 0,
  /*! Displays the button and a box that contains the like count. */
  FBSDKLikeControlStyleBoxCount,
};

/*!
 @abstract Converts an FBSDKLikeControlStyle to an NSString.
 */
FBSDK_EXTERN NSString *NSStringFromFBSDKLikeControlStyle(FBSDKLikeControlStyle style);

/*!
 @class FBSDKLikeControl

 @abstract UI control to like an object in the Facebook graph.

 @discussion Taps on the like button within this control will invoke an API call to the Facebook app through a
 fast-app-switch that allows the user to like the object.  Upon return to the calling app, the view will update
 with the new state and send actions for the UIControlEventValueChanged event.
 */
@interface FBSDKLikeControl : UIControl <FBSDKLiking>

/*!
 @abstract The foreground color to use for the content of the receiver.
 */
@property (nonatomic, strong) UIColor *foregroundColor;

/*!
 @abstract The position for the auxiliary view for the receiver.

 @see FBSDKLikeControlAuxiliaryPosition
 */
@property (nonatomic, assign) FBSDKLikeControlAuxiliaryPosition likeControlAuxiliaryPosition;

/*!
 @abstract The text alignment of the social sentence.

 @discussion This value is only valid for FBSDKLikeControlStyleStandard with
 FBSDKLikeControlAuxiliaryPositionTop|Bottom.
 */
@property (nonatomic, assign) FBSDKLikeControlHorizontalAlignment likeControlHorizontalAlignment;

/*!
 @abstract The style to use for the receiver.

 @see FBSDKLikeControlStyle
 */
@property (nonatomic, assign) FBSDKLikeControlStyle likeControlStyle;

/*!
 @abstract The preferred maximum width (in points) for autolayout.

 @discussion This property affects the size of the receiver when layout constraints are applied to it. During layout,
 if the text extends beyond the width specified by this property, the additional text is flowed to one or more new
 lines, thereby increasing the height of the receiver.
 */
@property (nonatomic, assign) CGFloat preferredMaxLayoutWidth;

/*!
 @abstract If YES, a sound is played when the receiver is toggled.

 @default YES
 */
@property (nonatomic, assign, getter = isSoundEnabled) BOOL soundEnabled;

@end
