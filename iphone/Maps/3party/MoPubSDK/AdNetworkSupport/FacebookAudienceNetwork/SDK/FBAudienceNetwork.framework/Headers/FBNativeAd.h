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

@protocol FBNativeAdDelegate;
@class FBAdImage;

/**
 Determines what parts of a native ad's content are cached through FBMediaView
 */
typedef NS_OPTIONS(NSInteger, FBNativeAdsCachePolicy) {
    /// No ad content is cached
    FBNativeAdsCachePolicyNone = 1 << 0,
    /// Icon is cached
    FBNativeAdsCachePolicyIcon = 1 << 1,
    /// Cover image is cached
    FBNativeAdsCachePolicyCoverImage = 1 << 2,
    /// Video is cached
    FBNativeAdsCachePolicyVideo = 1 << 3,
    /// AdChoices icon is cached
    FBNativeAdsCachePolicyAdChoices = 1 << 4,
    /// All content is cached
    FBNativeAdsCachePolicyAll = FBNativeAdsCachePolicyCoverImage | FBNativeAdsCachePolicyIcon | FBNativeAdsCachePolicyVideo | FBNativeAdsCachePolicyAdChoices,
};

/**
 The FBNativeAd represents ad metadata to allow you to construct custom ad views.
  See the AdUnitsSample in the sample apps section of the Audience Network framework.
 */
FB_CLASS_EXPORT FB_SUBCLASSING_RESTRICTED
@interface FBNativeAd : NSObject

/**
  Typed access to the id of the ad placement.
 */
@property (nonatomic, copy, readonly) NSString *placementID;
/**
  Typed access to the ad star rating. See `FBAdStarRating` for details.
 */
@property (nonatomic, assign, readonly) struct FBAdStarRating starRating FB_DEPRECATED;
/**
  Typed access to the ad title.
 */
@property (nonatomic, copy, readonly, nullable) NSString *title;
/**
  Typed access to the ad subtitle.
 */
@property (nonatomic, copy, readonly, nullable) NSString *subtitle;
/**
  Typed access to the ad social context, for example "Over half a million users".
 */
@property (nonatomic, copy, readonly, nullable) NSString *socialContext;
/**
  Typed access to the call to action phrase of the ad, for example "Install Now".
 */
@property (nonatomic, copy, readonly, nullable) NSString *callToAction;
/**
  Typed access to the ad icon. See `FBAdImage` for details.
 */
@property (nonatomic, strong, readonly, nullable) FBAdImage *icon;
/**
  Typed access to the ad cover image creative. See `FBAdImage` for details.
 */
@property (nonatomic, strong, readonly, nullable) FBAdImage *coverImage;
/**
  Typed access to the body raw untruncated text, usually a longer description of the ad. Note, this method always returns untruncated text, as opposed to -(NSString *) body.
 */
@property (nonatomic, copy, readonly, nullable) NSString *rawBody;
/**
  Typed access to the body text, usually a longer description of the ad.
 */
@property (nonatomic, copy, readonly, nullable) NSString *body;
/**
 Typed access to the AdChoices icon. See `FBAdImage` for details. See `FBAdChoicesView` for an included implementation.
 */
@property (nonatomic, strong, readonly, nullable) FBAdImage *adChoicesIcon;
/**
 Typed access to the AdChoices URL. Navigate to this link when the icon is tapped. See `FBAdChoicesView` for an included implementation.
 */
@property (nonatomic, copy, readonly, nullable) NSURL *adChoicesLinkURL;
/**
 Typed access to the AdChoices text, usually a localized version of "AdChoices". See `FBAdChoicesView` for an included implementation.
 */
@property (nonatomic, copy, readonly, nullable) NSString *adChoicesText;

/**
  Set the native ad caching policy. This controls which media (images, video, etc) from the native ad are cached before the native ad calls nativeAdLoaded on its delegate. The default is to not block on caching. Ensure that media is loaded through FBMediaView or through [FBAdImage loadImageAsyncWithBlock:] to take full advantage of caching.
 */
@property (nonatomic, assign) FBNativeAdsCachePolicy mediaCachePolicy;
/**
  the delegate
 */
@property (nonatomic, weak, nullable) id<FBNativeAdDelegate> delegate;

/**
  This is a method to initialize a FBNativeAd object matching the given placement id.

 - Parameter placementID: The id of the ad placement. You can create your placement id from Facebook developers page.
 */
- (instancetype)initWithPlacementID:(NSString *)placementID NS_DESIGNATED_INITIALIZER;

/**
  This is a method to associate a FBNativeAd with the UIView you will use to display the native ads.

 - Parameter view: The UIView you created to render all the native ads data elements.
 - Parameter viewController: The UIViewController that will be used to present SKStoreProductViewController
 (iTunes Store product information) or the in-app browser. If nil is passed, the top view controller currently shown will be used.


 The whole area of the UIView will be clickable.
 */
- (void)registerViewForInteraction:(UIView *)view
                withViewController:(nullable UIViewController *)viewController;

/**
  This is a method to associate FBNativeAd with the UIView you will use to display the native ads
  and set clickable areas.

 - Parameter view: The UIView you created to render all the native ads data elements.
 - Parameter viewController: The UIViewController that will be used to present SKStoreProductViewController
 (iTunes Store product information). If nil is passed, the top view controller currently shown will be used.
 - Parameter clickableViews: An array of UIView you created to render the native ads data element, e.g.
 CallToAction button, Icon image, which you want to specify as clickable.
 */
- (void)registerViewForInteraction:(UIView *)view
                withViewController:(nullable UIViewController *)viewController
                withClickableViews:(FB_NSArrayOf(UIView *)*)clickableViews;

/**
  This is a method to disconnect a FBNativeAd with the UIView you used to display the native ads.
 */
- (void)unregisterView;

/**
  Begins loading the FBNativeAd content.

  You can implement `nativeAdDidLoad:` and `nativeAd:didFailWithError:` methods
  of `FBNativeAdDelegate` if you would like to be notified as loading succeeds or fails.
 */
- (void)loadAd;

/**
  Begins loading the FBNativeAd content from a bid payload attained through a server side bid.

 - Parameter bidPayload: The payload of the ad bid. You can get your bid payload from Facebook bidder endpoint.
 */
- (void)loadAdWithBidPayload:(NSString *)bidPayload;

/**
  Call isAdValid to check whether native ad is valid & internal consistent prior rendering using its properties. If
  rendering is done as part of the loadAd callback, it is guarantee to be consistent
 */
@property (nonatomic, getter=isAdValid, readonly) BOOL adValid;

@property (nonatomic, copy, readonly, nullable, getter=getAdNetwork) NSString *adNetwork;

@end

/**
  The methods declared by the FBNativeAdDelegate protocol allow the adopting delegate to respond to messages
 from the FBNativeAd class and thus respond to operations such as whether the native ad has been loaded.
 */
@protocol FBNativeAdDelegate <NSObject>

@optional

/**
  Sent when an FBNativeAd has been successfully loaded.

 - Parameter nativeAd: An FBNativeAd object sending the message.
 */
- (void)nativeAdDidLoad:(FBNativeAd *)nativeAd;

/**
  Sent immediately before the impression of an FBNativeAd object will be logged.

 - Parameter nativeAd: An FBNativeAd object sending the message.
 */
- (void)nativeAdWillLogImpression:(FBNativeAd *)nativeAd;

/**
  Sent when an FBNativeAd is failed to load.

 - Parameter nativeAd: An FBNativeAd object sending the message.
 - Parameter error: An error object containing details of the error.
 */
- (void)nativeAd:(FBNativeAd *)nativeAd didFailWithError:(NSError *)error;

/**
  Sent after an ad has been clicked by the person.

 - Parameter nativeAd: An FBNativeAd object sending the message.
 */
- (void)nativeAdDidClick:(FBNativeAd *)nativeAd;

/**
  When an ad is clicked, the modal view will be presented. And when the user finishes the
 interaction with the modal view and dismiss it, this message will be sent, returning control
 to the application.

 - Parameter nativeAd: An FBNativeAd object sending the message.
 */
- (void)nativeAdDidFinishHandlingClick:(FBNativeAd *)nativeAd;

@end

/**
  Represents the Facebook ad star rating, which contains the rating value and rating scale.
 */
FB_EXPORT struct FBAdStarRating {
    /// The value of the star rating, X in X/5
    CGFloat value;
    /// The total possible star rating, Y in 4/Y
    NSInteger scale;
} FBAdStarRating;

/**
  Represents an image creative.
 */
FB_CLASS_EXPORT
@interface FBAdImage : NSObject

/**
  Typed access to the image url.
 */
@property (nonatomic, copy, readonly) NSURL *url;
/**
  Typed access to the image width.
 */
@property (nonatomic, assign, readonly) NSInteger width;
/**
  Typed access to the image height.
 */
@property (nonatomic, assign, readonly) NSInteger height;

/**
  This is a method to initialize an FBAdImage.

 - Parameter url: the image url.
 - Parameter width: the image width.
 - Parameter height: the image height.
 */
- (instancetype)initWithURL:(NSURL *)url
                      width:(NSInteger)width
                     height:(NSInteger)height NS_DESIGNATED_INITIALIZER;

/**
  Loads an image from self.url over the network, or returns the cached image immediately.

 - Parameter block: Block to handle the loaded image.
 */
- (void)loadImageAsyncWithBlock:(nullable void (^)(UIImage * __nullable image))block;

@end

/**
  Helper view that draws a star rating based off a native ad.
 */
FB_CLASS_EXPORT FB_DEPRECATED
@interface FBAdStarRatingView : UIView

/**
  The current rating from an FBNativeAd. When set, updates the view.
 */
@property (nonatomic, assign) struct FBAdStarRating rating FB_DEPRECATED;

/**
  The color drawn for filled-in stars. Defaults to yellow.
 */
@property (strong, nonatomic) UIColor *primaryColor FB_DEPRECATED;

/**
  The color drawn for empty stars. Defaults to gray.
 */
@property (strong, nonatomic) UIColor *secondaryColor FB_DEPRECATED;

/**
  Initializes a star rating view with a given frame and star rating.

 - Parameter frame: Frame of this view.
 - Parameter starRating: Star rating from a native ad.
 */
- (instancetype)initWithFrame:(CGRect)frame withStarRating:(struct FBAdStarRating)starRating NS_DESIGNATED_INITIALIZER FB_DEPRECATED;

@end

NS_ASSUME_NONNULL_END
