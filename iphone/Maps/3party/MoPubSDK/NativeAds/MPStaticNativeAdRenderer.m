//
//  MPStaticNativeAdRenderer.m
//  MoPubSDK
//
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import "MPAdDestinationDisplayAgent.h"
#import "MPLogging.h"
#import "MPNativeAdAdapter.h"
#import "MPNativeAdConstants.h"
#import "MPNativeAdError.h"
#import "MPNativeAdRenderer.h"
#import "MPNativeAdRendererConfiguration.h"
#import "MPNativeAdRendererConstants.h"
#import "MPNativeAdRendererImageHandler.h"
#import "MPNativeAdRendering.h"
#import "MPNativeAdRenderingImageLoader.h"
#import "MPNativeCache.h"
#import "MPNativeView.h"
#import "MPStaticNativeAdRenderer.h"
#import "MPStaticNativeAdRendererSettings.h"

/**
 *  -1.0 is somewhat significant because this also happens to be what `UITableViewAutomaticDimension`
 *  is so it makes for easier migration to use `UITableViewAutomaticDimension` on iOS 8+ later but is not
 *  currently passed back in `-tableView:shouldIndentWhileEditingRowAtIndexPath:` directly so it can
 *  be any abitrary value.
 */
const CGFloat MPNativeViewDynamicDimension = -1.0;

@interface MPStaticNativeAdRenderer () <MPNativeAdRendererImageHandlerDelegate>

@property (nonatomic) UIView<MPNativeAdRendering> *adView;
@property (nonatomic) id<MPNativeAdAdapter> adapter;
@property (nonatomic) BOOL adViewInViewHierarchy;
@property (nonatomic) Class renderingViewClass;
@property (nonatomic) MPNativeAdRendererImageHandler *rendererImageHandler;

@end

@implementation MPStaticNativeAdRenderer

+ (MPNativeAdRendererConfiguration *)rendererConfigurationWithRendererSettings:(id<MPNativeAdRendererSettings>)rendererSettings
{
    MPNativeAdRendererConfiguration *config = [[MPNativeAdRendererConfiguration alloc] init];
    config.rendererClass = [self class];
    config.rendererSettings = rendererSettings;
    config.supportedCustomEvents = @[@"MPMoPubNativeCustomEvent", @"FacebookNativeCustomEvent", @"MillennialNativeCustomEvent"];

    return config;
}

- (instancetype)initWithRendererSettings:(id<MPNativeAdRendererSettings>)rendererSettings
{
    if (self = [super init]) {
        MPStaticNativeAdRendererSettings *settings = (MPStaticNativeAdRendererSettings *)rendererSettings;
        _renderingViewClass = settings.renderingViewClass;
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
        self.adView.nativeMainTextLabel.text = [adapter.properties objectForKey:kAdTextKey];
    }

    if ([self.adView respondsToSelector:@selector(nativeTitleTextLabel)]) {
        self.adView.nativeTitleTextLabel.text = [adapter.properties objectForKey:kAdTitleKey];
    }

    if ([self.adView respondsToSelector:@selector(nativeCallToActionTextLabel)] && self.adView.nativeCallToActionTextLabel) {
        self.adView.nativeCallToActionTextLabel.text = [adapter.properties objectForKey:kAdCTATextKey];
    }

    if ([self.adView respondsToSelector:@selector(nativePrivacyInformationIconImageView)]) {
        // MoPub ads pass the privacy information icon key through the properties dictionary.
        NSString *daaIconImageLoc = [adapter.properties objectForKey:kAdDAAIconImageKey];
        if (daaIconImageLoc) {
            UIImageView *imageView = self.adView.nativePrivacyInformationIconImageView;
            imageView.hidden = NO;

            UIImage *daaIconImage = [UIImage imageNamed:daaIconImageLoc];
            imageView.image = daaIconImage;

            // Attach a gesture recognizer to handle loading the daa icon URL.
            UITapGestureRecognizer *tapRecognizer = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(DAAIconTapped)];
            imageView.userInteractionEnabled = YES;
            [imageView addGestureRecognizer:tapRecognizer];
        } else if ([adapter respondsToSelector:@selector(privacyInformationIconView)]) {
            // The ad network may provide its own view for its privacy information icon. We assume the ad handles the tap on the icon as well.
            UIView *privacyIconAdView = [adapter privacyInformationIconView];
            privacyIconAdView.frame = self.adView.nativePrivacyInformationIconImageView.bounds;
            privacyIconAdView.autoresizingMask = UIViewAutoresizingFlexibleHeight | UIViewAutoresizingFlexibleWidth;
            self.adView.nativePrivacyInformationIconImageView.userInteractionEnabled = YES;
            [self.adView.nativePrivacyInformationIconImageView addSubview:privacyIconAdView];
            self.adView.nativePrivacyInformationIconImageView.hidden = NO;
        } else {
            self.adView.nativePrivacyInformationIconImageView.userInteractionEnabled = NO;
            self.adView.nativePrivacyInformationIconImageView.hidden = YES;
        }
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
        NSNumber *starRatingNum = [adapter.properties objectForKey:kAdStarRatingKey];

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

- (void)DAAIconTapped
{
    if ([self.adapter respondsToSelector:@selector(displayContentForDAAIconTap)]) {
        [self.adapter displayContentForDAAIconTap];
    }
}

- (void)adViewWillMoveToSuperview:(UIView *)superview
{
    self.adViewInViewHierarchy = (superview != nil);

    if (superview) {
        // We'll start asychronously loading the native ad images now.
        if ([self.adapter.properties objectForKey:kAdIconImageKey] && [self.adView respondsToSelector:@selector(nativeIconImageView)]) {
            [self.rendererImageHandler loadImageForURL:[NSURL URLWithString:[self.adapter.properties objectForKey:kAdIconImageKey]] intoImageView:self.adView.nativeIconImageView];
        }

        // Only handle the loading of the main image if the adapter doesn't already have a view for it.
        if (!([self.adapter respondsToSelector:@selector(mainMediaView)] && [self.adapter mainMediaView])) {
            if ([self.adapter.properties objectForKey:kAdMainImageKey] && [self.adView respondsToSelector:@selector(nativeMainImageView)]) {
                [self.rendererImageHandler loadImageForURL:[NSURL URLWithString:[self.adapter.properties objectForKey:kAdMainImageKey]] intoImageView:self.adView.nativeMainImageView];
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
