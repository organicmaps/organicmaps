// Copyright 2004-present Facebook. All Rights Reserved.
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

#import <FBAudienceNetwork/FBAdDefines.h>

NS_ASSUME_NONNULL_BEGIN

@class FBAdImage;
@class FBNativeAd;
@class FBNativeAdViewAttributes;

/**
  FBAdChoicesView offers a simple way to display a sponsored or AdChoices icon.
 */
FB_CLASS_EXPORT FB_SUBCLASSING_RESTRICTED
@interface FBAdChoicesView : UIView

/**
  Access to the text label contained in this view.
 */
@property (nonatomic, weak, readonly, nullable) UILabel *label;

/**
  Determines whether the background mask is shown, or a transparent mask is used.
 */
@property (nonatomic, assign, getter=isBackgroundShown) BOOL backgroundShown;

/**
  Determines whether the view can be expanded upon being tapped, or defaults to fullsize. Defaults to NO.
 */
@property (nonatomic, assign, readonly, getter=isExpandable) BOOL expandable;

/*
  The native ad that provides AdChoices info, such as the image url, and click url. Setting this updates the nativeAd.
 */
@property (nonatomic, weak, readwrite, nullable) FBNativeAd *nativeAd;

/*
  Affects background mask rendering. Setting this property updates the rendering.
 */
@property (nonatomic, assign, readwrite) UIRectCorner corner;

/*
  The view controller to present the ad choices info from. If nil, the top view controller is used.
 */
@property (nonatomic, weak, readwrite, null_resettable) UIViewController *viewController;

/**
  Initialize this view with a given native ad. Configuration is pulled from the native ad.

 - Parameter nativeAd: The native ad to initialize with.
 */
- (instancetype)initWithNativeAd:(FBNativeAd *)nativeAd;

/**
  Initialize this view with a given native ad. Configuration is pulled from the native ad.

 - Parameter nativeAd: The native ad to initialize with.
 - Parameter expandable: Controls whether view defaults to expanded or not, see property documentation
 */
- (instancetype)initWithNativeAd:(FBNativeAd *)nativeAd
                      expandable:(BOOL)expandable;

/**
  Initialize this view with explicit parameters.

 - Parameter viewController: View controller to present the AdChoices webview from.
 - Parameter adChoicesIcon: Native ad AdChoices icon.
 - Parameter adChoicesLinkURL: Native ad AdChoices link URL.
 - Parameter attributes: Attributes to configure look and feel.
 */
- (instancetype)initWithViewController:(nullable UIViewController *)viewController
                         adChoicesIcon:(nullable FBAdImage *)adChoicesIcon
                      adChoicesLinkURL:(nullable NSURL *)adChoicesLinkURL
                            attributes:(nullable FBNativeAdViewAttributes *)attributes;

/**
  Initialize this view with explicit parameters.

 - Parameter viewController: View controller to present the AdChoices webview from.
 - Parameter adChoicesIcon: Native ad AdChoices icon.
 - Parameter adChoicesLinkURL: Native ad AdChoices link URL.
 - Parameter attributes: Attributes to configure look and feel.
 - Parameter expandable: Controls whether view defaults to expanded or not, see property documentation
 */
- (instancetype)initWithViewController:(nullable UIViewController *)viewController
                         adChoicesIcon:(nullable FBAdImage *)adChoicesIcon
                      adChoicesLinkURL:(nullable NSURL *)adChoicesLinkURL
                            attributes:(nullable FBNativeAdViewAttributes *)attributes
                            expandable:(BOOL)expandable;

/**
  Initialize this view with explicit parameters.

 - Parameter viewController: View controller to present the AdChoices webview from.
 - Parameter adChoicesIcon: Native ad AdChoices icon.
 - Parameter adChoicesLinkURL: Native ad AdChoices link URL.
 - Parameter adChoicesText: Native ad AdChoices label.
 - Parameter attributes: Attributes to configure look and feel.
 - Parameter expandable: Controls whether view defaults to expanded or not, see property documentation
 */
- (instancetype)initWithViewController:(nullable UIViewController *)viewController
                         adChoicesIcon:(nullable FBAdImage *)adChoicesIcon
                      adChoicesLinkURL:(nullable NSURL *)adChoicesLinkURL
                         adChoicesText:(nullable NSString*)adChoicesText
                            attributes:(nullable FBNativeAdViewAttributes *)attributes
                            expandable:(BOOL)expandable NS_DESIGNATED_INITIALIZER;
/**
  Using the superview, this updates the frame of this view, positioning the icon in the top right corner by default.
 */
- (void)updateFrameFromSuperview;

/**
  Using the superview, this updates the frame of this view, positioning the icon in the corner specified. UIRectCornerAllCorners not supported.

 - Parameter corner: The corner to display this view from.
 */
- (void)updateFrameFromSuperview:(UIRectCorner)corner;

@end

NS_ASSUME_NONNULL_END
