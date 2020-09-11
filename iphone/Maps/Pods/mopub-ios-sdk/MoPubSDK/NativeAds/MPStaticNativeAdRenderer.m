//
//  MPStaticNativeAdRenderer.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPAdDestinationDisplayAgent.h"
#import "MPBaseNativeAdRenderer+Internal.h"
#import "MPHTTPNetworkSession.h"
#import "MPLogging.h"
#import "MPMemoryCache.h"
#import "MPNativeAdAdapter.h"
#import "MPNativeAdConstants.h"
#import "MPNativeAdError.h"
#import "MPNativeAdRenderer.h"
#import "MPNativeAdRendererConfiguration.h"
#import "MPNativeAdRendererConstants.h"
#import "MPNativeAdRendererImageHandler.h"
#import "MPNativeAdRenderingImageLoader.h"
#import "MPNativeCache.h"
#import "MPNativeView.h"
#import "MPStaticNativeAdRenderer.h"
#import "MPStaticNativeAdRendererSettings.h"
#import "MPURLRequest.h"

/**
 *  -1.0 is somewhat significant because this also happens to be what `UITableViewAutomaticDimension`
 *  is so it makes for easier migration to use `UITableViewAutomaticDimension` on iOS 8+ later but is not
 *  currently passed back in `-tableView:shouldIndentWhileEditingRowAtIndexPath:` directly so it can
 *  be any abitrary value.
 */
const CGFloat MPNativeViewDynamicDimension = -1.0;

@interface MPStaticNativeAdRenderer () <MPNativeAdRendererImageHandlerDelegate>

@property (nonatomic) id<MPNativeAdAdapter> adapter;
@property (nonatomic) BOOL adViewInViewHierarchy;
@property (nonatomic) MPNativeAdRendererImageHandler *rendererImageHandler;

@end

@implementation MPStaticNativeAdRenderer

+ (MPNativeAdRendererConfiguration *)rendererConfigurationWithRendererSettings:(id<MPNativeAdRendererSettings>)rendererSettings
{
    return [MPStaticNativeAdRenderer rendererConfigurationWithRendererSettings:rendererSettings additionalSupportedCustomEvents:@[]];
}

+ (MPNativeAdRendererConfiguration *)rendererConfigurationWithRendererSettings:(id<MPNativeAdRendererSettings>)rendererSettings
                                               additionalSupportedCustomEvents:(NSArray *)additionalSupportedCustomEvents
{
    MPNativeAdRendererConfiguration *config = [[MPNativeAdRendererConfiguration alloc] init];
    config.rendererClass = [self class];
    config.rendererSettings = rendererSettings;
    config.supportedCustomEvents = [@[@"MPMoPubNativeCustomEvent"] arrayByAddingObjectsFromArray:additionalSupportedCustomEvents];

    return config;
}

- (instancetype)initWithRendererSettings:(id<MPNativeAdRendererSettings>)rendererSettings
{
    if (self = [super init]) {
        MPStaticNativeAdRendererSettings *settings = (MPStaticNativeAdRendererSettings *)rendererSettings;
        self.renderingViewClass = settings.renderingViewClass;
        _viewSizeHandler = [settings.viewSizeHandler copy];
        _rendererImageHandler = [MPNativeAdRendererImageHandler new];
        _rendererImageHandler.delegate = self;
    }

    return self;
}

- (UIView *)retrieveViewWithAdapter:(id<MPNativeAdAdapter>)adapter error:(NSError **)error
{
    if (!adapter) {
        if (error) {
            *error = MPNativeAdNSErrorForRenderValueTypeError();
        }

        return nil;
    }

    self.adapter = adapter;

    if ([self.renderingViewClass respondsToSelector:@selector(nibForAd)]) {
        self.adView = (UIView<MPNativeAdRendering> *)[[[self.renderingViewClass nibForAd] instantiateWithOwner:nil options:nil] firstObject];
    } else {
        self.adView = [[self.renderingViewClass alloc] init];
    }

    self.adView.autoresizingMask = UIViewAutoresizingFlexibleHeight | UIViewAutoresizingFlexibleWidth;

    // We only load text here. We delay loading of images until the view is added to the view hierarchy
    // so we don't unnecessarily load images from the cache if the user is scrolling fast. So we will
    // just store the image URLs for now.
    if ([self.adView respondsToSelector:@selector(nativeMainTextLabel)]) {
        self.adView.nativeMainTextLabel.text = adapter.properties[kAdTextKey];
    }

    if ([self.adView respondsToSelector:@selector(nativeTitleTextLabel)]) {
        self.adView.nativeTitleTextLabel.text = adapter.properties[kAdTitleKey];
    }

    if ([self.adView respondsToSelector:@selector(nativeCallToActionTextLabel)]) {
        self.adView.nativeCallToActionTextLabel.text = adapter.properties[kAdCTATextKey];
    }

    [self renderSponsoredByTextWithAdapter:adapter];

    if ([self.adView respondsToSelector:@selector(nativePrivacyInformationIconImageView)]) {
        UIImage *privacyIconImage = adapter.properties[kAdPrivacyIconUIImageKey];
        NSString *privacyIconImageUrl = adapter.properties[kAdPrivacyIconImageUrlKey];
        // A cached privacy information icon image exists; it should be used.
        if (privacyIconImage != nil) {
            UIImageView *imageView = self.adView.nativePrivacyInformationIconImageView;
            imageView.hidden = NO;
            imageView.image = privacyIconImage;

            // Attach a gesture recognizer to handle loading the privacy icon URL.
            UITapGestureRecognizer *tapRecognizer = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(onPrivacyIconTapped)];
            imageView.userInteractionEnabled = YES;
            [imageView addGestureRecognizer:tapRecognizer];
        }
        // No cached privacy information icon image was cached, but there is a URL for the
        // icon. Go fetch the icon and populate the UIImageView when complete.
        else if (privacyIconImageUrl != nil) {
            NSURL *iconUrl = [NSURL URLWithString:privacyIconImageUrl];
            MPURLRequest *imageRequest = [MPURLRequest requestWithURL:iconUrl];

            __weak __typeof__(self) weakSelf = self;
            [MPHTTPNetworkSession startTaskWithHttpRequest:imageRequest responseHandler:^(NSData * _Nonnull data, NSHTTPURLResponse * _Nonnull response) {
                // Cache the successfully retrieved icon image
                [MPMemoryCache.sharedInstance setData:data forKey:privacyIconImageUrl];

                // Populate the image view
                __typeof__(self) strongSelf = weakSelf;
                UIImageView *imageView = strongSelf.adView.nativePrivacyInformationIconImageView;
                imageView.hidden = NO;
                imageView.image = [UIImage imageWithData:data];

                // Attach a gesture recognizer to handle loading the privacy icon URL.
                UITapGestureRecognizer *tapRecognizer = [[UITapGestureRecognizer alloc] initWithTarget:strongSelf action:@selector(onPrivacyIconTapped)];
                imageView.userInteractionEnabled = YES;
                [imageView addGestureRecognizer:tapRecognizer];
            } errorHandler:^(NSError * _Nonnull error) {
                MPLogInfo(@"Failed to retrieve privacy icon from %@", privacyIconImageUrl);
            }];
        }
        // The ad network may provide its own view for its privacy information icon.
        // We assume the ad handles the tap on the icon as well.
        else if ([adapter respondsToSelector:@selector(privacyInformationIconView)]) {
            UIView *privacyIconAdView = [adapter privacyInformationIconView];
            privacyIconAdView.frame = self.adView.nativePrivacyInformationIconImageView.bounds;
            privacyIconAdView.autoresizingMask = UIViewAutoresizingFlexibleHeight | UIViewAutoresizingFlexibleWidth;
            self.adView.nativePrivacyInformationIconImageView.userInteractionEnabled = YES;
            [self.adView.nativePrivacyInformationIconImageView addSubview:privacyIconAdView];
            self.adView.nativePrivacyInformationIconImageView.hidden = NO;
        }
        // No privacy icon
        else {
            self.adView.nativePrivacyInformationIconImageView.userInteractionEnabled = NO;
            self.adView.nativePrivacyInformationIconImageView.hidden = YES;
        }
    }

    if ([self hasIconView]) {
        UIView *iconView = [self.adapter iconMediaView];
        UIView *iconImageView = [self.adView nativeIconImageView];

        iconView.frame = iconImageView.bounds;
        iconView.autoresizingMask = UIViewAutoresizingFlexibleHeight | UIViewAutoresizingFlexibleWidth;
        iconImageView.userInteractionEnabled = YES;

        [iconImageView addSubview:iconView];
    }

    if ([self shouldLoadMediaView]) {
        UIView *mediaView = [self.adapter mainMediaView];
        UIView *mainImageView = [self.adView nativeMainImageView];

        mediaView.frame = mainImageView.bounds;
        mediaView.autoresizingMask = UIViewAutoresizingFlexibleHeight | UIViewAutoresizingFlexibleWidth;
        mainImageView.userInteractionEnabled = YES;

        [mainImageView addSubview:mediaView];
    }

    // See if the ad contains a star rating and notify the view if it does.
    if ([self.adView respondsToSelector:@selector(layoutStarRating:)]) {
        NSNumber *starRatingNum = adapter.properties[kAdStarRatingKey];

        if ([starRatingNum isKindOfClass:[NSNumber class]] && starRatingNum.floatValue >= kStarRatingMinValue && starRatingNum.floatValue <= kStarRatingMaxValue) {
            [self.adView layoutStarRating:starRatingNum];
        }
    }

    return self.adView;
}

- (BOOL)shouldLoadMediaView
{
    return [self.adapter respondsToSelector:@selector(mainMediaView)]
        && [self.adapter mainMediaView]
        && [self.adView respondsToSelector:@selector(nativeMainImageView)];
}

- (BOOL)hasIconView
{
    return [self.adapter respondsToSelector:@selector(iconMediaView)]
        && [self.adapter iconMediaView]
        && [self.adView respondsToSelector:@selector(nativeIconImageView)];
}

- (void)onPrivacyIconTapped
{
    if ([self.adapter respondsToSelector:@selector(displayContentForDAAIconTap)]) {
        [self.adapter displayContentForDAAIconTap];
    }
}

- (void)adViewWillMoveToSuperview:(UIView *)superview
{
    self.adViewInViewHierarchy = (superview != nil);

    if (superview) {
        // Only handle the loading of the icon image if the adapter doesn't already have a view for it.
        if (![self hasIconView] && self.adapter.properties[kAdIconImageKey] && [self.adView respondsToSelector:@selector(nativeIconImageView)]) {
            [self.rendererImageHandler loadImageForURL:[NSURL URLWithString:self.adapter.properties[kAdIconImageKey]] intoImageView:self.adView.nativeIconImageView];
        }

        // Only handle the loading of the main image if the adapter doesn't already have a view for it.
        if (!([self.adapter respondsToSelector:@selector(mainMediaView)] && [self.adapter mainMediaView])) {
            if (self.adapter.properties[kAdMainImageKey] && [self.adView respondsToSelector:@selector(nativeMainImageView)]) {
                [self.rendererImageHandler loadImageForURL:[NSURL URLWithString:self.adapter.properties[kAdMainImageKey]] intoImageView:self.adView.nativeMainImageView];
            }
        }

        // Layout custom assets here as the custom assets may contain images that need to be loaded.
        if ([self.adView respondsToSelector:@selector(layoutCustomAssetsWithProperties:imageLoader:)]) {
            // Create a simplified image loader for the ad view to use.
            MPNativeAdRenderingImageLoader *imageLoader = [[MPNativeAdRenderingImageLoader alloc] initWithImageHandler:self.rendererImageHandler];
            [self.adView layoutCustomAssetsWithProperties:self.adapter.properties imageLoader:imageLoader];
        }
    }
}

#pragma mark - MPNativeAdRendererImageHandlerDelegate

- (BOOL)nativeAdViewInViewHierarchy
{
    return self.adViewInViewHierarchy;
}

@end
