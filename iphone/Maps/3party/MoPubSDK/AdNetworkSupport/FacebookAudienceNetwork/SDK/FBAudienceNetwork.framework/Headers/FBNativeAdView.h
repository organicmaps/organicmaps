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
#import <FBAudienceNetwork/FBNativeAd.h>

NS_ASSUME_NONNULL_BEGIN

@class FBNativeAdViewAttributes;

/**
 Determines the type of native ad template. Different views are created
 for different values of FBNativeAdViewType
 */
typedef NS_ENUM(NSInteger, FBNativeAdViewType) {
    /// Fixed height view, 100 points (banner equivalent)
    FBNativeAdViewTypeGenericHeight100 = 1,
    /// Fixed height view, 120 points (banner equivalent)
    FBNativeAdViewTypeGenericHeight120,
    /// Fixed height view, 300 points
    FBNativeAdViewTypeGenericHeight300,
    /// Fixed height view, 400 points
    FBNativeAdViewTypeGenericHeight400,
};

/**
  The FBNativeAdView creates prebuilt native ad template views and manages native ads.
 */
FB_CLASS_EXPORT
@interface FBNativeAdView : UIView

/**
  The type of the view, specifies which template to use
 */
@property (nonatomic, assign, readonly) FBNativeAdViewType type;

/**
  This is a method to create a native ad template using the given placement id and type.
 - Parameter nativeAd: The native ad to use to create this view.
 - Parameter type: The type of this native ad template. For more information, consult FBNativeAdViewType.
 */
+ (instancetype)nativeAdViewWithNativeAd:(FBNativeAd *)nativeAd withType:(FBNativeAdViewType)type;

/**
  A view controller that is used to present modal content. If nil, the view searches for a view controller.
 */
@property (nonatomic, weak, nullable) UIViewController *rootViewController;

/**
  This is a method to create a native ad template using the given placement id and type.
 - Parameter nativeAd: The native ad to use to create this view.
 - Parameter type: The type of this native ad template. For more information, consult FBNativeAdViewType.
 - Parameter attributes: The attributes to render this native ad template with.
 */
+ (instancetype)nativeAdViewWithNativeAd:(FBNativeAd *)nativeAd withType:(FBNativeAdViewType)type withAttributes:(FBNativeAdViewAttributes *)attributes;

@end

/**
  Describes the look and feel of a native ad view.
 */
@interface FBNativeAdViewAttributes : NSObject <NSCopying>

/**
  This is a method to create native ad view attributes with a dictionary
 */
- (instancetype)initWithDictionary:(NSDictionary *) dict NS_DESIGNATED_INITIALIZER;

/**
  Background color of the native ad view.
 */
@property (nonatomic, copy, nullable) UIColor *backgroundColor;
/**
  Color of the title label.
 */
@property (nonatomic, copy, nullable) UIColor *titleColor;
/**
  Font of the title label.
 */
@property (nonatomic, copy, nullable) UIFont *titleFont;
/**
  Color of the description label.
 */
@property (nonatomic, copy, nullable) UIColor *descriptionColor;
/**
  Font of the description label.
 */
@property (nonatomic, copy, nullable) UIFont *descriptionFont;
/**
  Background color of the call to action button.
 */
@property (nonatomic, copy, nullable) UIColor *buttonColor;
/**
  Color of the call to action button's title label.
 */
@property (nonatomic, copy, nullable) UIColor *buttonTitleColor;
/**
  Font of the call to action button's title label.
 */
@property (nonatomic, copy, nullable) UIFont *buttonTitleFont;
/**
  Border color of the call to action button. If nil, no border is shown.
 */
@property (nonatomic, copy, nullable) UIColor *buttonBorderColor;
/**
  Enables or disables autoplay for some types of media. Defaults to YES.
 */
@property (nonatomic, assign, getter=isAutoplayEnabled) BOOL autoplayEnabled;

/**
  Returns default attributes for a given type.

 - Parameter type: The type for this layout.
 */
+ (instancetype)defaultAttributesForType:(FBNativeAdViewType)type;

@end

NS_ASSUME_NONNULL_END
