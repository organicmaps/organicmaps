//
//  MOPUBNativeVideoAdRenderer.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MOPUBNativeVideoAdRenderer.h"
#import "MPBaseNativeAdRenderer+Internal.h"
#import "MPNativeAdRendererConfiguration.h"
#import "MPNativeAdRenderer.h"
#import "MPNativeAdRendering.h"
#import "MPNativeAdAdapter.h"
#import "MPNativeAdConstants.h"
#import "MPNativeAdError.h"
#import "MPNativeAdError+VAST.h"
#import "MPNativeAdRendererImageHandler.h"
#import "MPTimer.h"
#import "MPGlobal.h"
#import "MPLogging.h"
#import "MOPUBNativeVideoAdRendererSettings.h"
#import "MOPUBFullscreenPlayerViewController.h"
#import "MOPUBPlayerManager.h"
#import "MOPUBNativeVideoAdAdapter.h"
#import "MPVASTTracking.h"
#import "MPVideoConfig.h"
#import "MOPUBNativeVideoAdConfigValues.h"
#import "MOPUBPlayerViewController.h"
#import "MPNativeAdRenderingImageLoader.h"
#import "MPURLRequest.h"
#import "MPHTTPNetworkSession.h"
#import "MPMemoryCache.h"

static const CGFloat kAutoPlayTimerInterval = 0.25f;

@interface MOPUBNativeVideoAdRenderer () <MPNativeAdRenderer, MOPUBPlayerViewControllerDelegate, MOPUBFullscreenPlayerViewControllerDelegate, MPNativeAdRendererImageHandlerDelegate>

@property (nonatomic) MOPUBNativeVideoAdAdapter<MPNativeAdAdapter> *adapter;
@property (nonatomic) BOOL adViewInViewHierarchy;
@property (nonatomic) MPNativeAdRendererImageHandler *rendererImageHandler;

@property (nonatomic, weak) MOPUBPlayerViewController *videoController;
@property (nonatomic) MPTimer *autoPlayTimer;
@property (nonatomic) MPVideoConfig *videoConfig;
@property (nonatomic) MPVASTTracking *vastTracking;
@property (nonatomic) MOPUBNativeVideoAdConfigValues *nativeVideoAdConfig;
@property (nonatomic) BOOL trackingImpressionFired;

@end

@implementation MOPUBNativeVideoAdRenderer

+ (MPNativeAdRendererConfiguration *)rendererConfigurationWithRendererSettings:(id<MPNativeAdRendererSettings>)rendererSettings
{
    MPNativeAdRendererConfiguration *config = [[MPNativeAdRendererConfiguration alloc] init];
    config.rendererClass = [self class];
    config.rendererSettings = rendererSettings;
    config.supportedCustomEvents = @[@"MOPUBNativeVideoCustomEvent"];

    return config;
}

- (instancetype)initWithRendererSettings:(id<MPNativeAdRendererSettings>)rendererSettings
{
    if (self = [super init]) {
        MOPUBNativeVideoAdRendererSettings *settings = (MOPUBNativeVideoAdRendererSettings *)rendererSettings;
        self.renderingViewClass = settings.renderingViewClass;
        _viewSizeHandler = [settings.viewSizeHandler copy];
        _rendererImageHandler = [MPNativeAdRendererImageHandler new];
        _rendererImageHandler.delegate = self;
    }

    return self;
}

- (MPVASTTracking *)vastTracking {
    return self.videoController.vastTracking;
}

- (void)dealloc
{
    [_autoPlayTimer invalidate];
    _autoPlayTimer = nil;

    // free the video memory
    [[MOPUBPlayerManager sharedInstance] disposePlayerViewController];
}

- (UIView *)retrieveViewWithAdapter:(MOPUBNativeVideoAdAdapter<MPNativeAdAdapter> *)adapter error:(NSError **)error
{
    if (!adapter) {
        if (error) {
            *error = MPNativeAdNSErrorForRenderValueTypeError();
            [self.vastTracking handleVASTError:VASTErrorCodeFromNativeAdErrorCode((*error).code) videoTimeOffset:0];
        }

        return nil;
    }

    self.adapter = adapter;

    [self initAdView];
    [self setupVideoView];

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

    [self renderSponsoredByTextWithAdapter:adapter];

    if ([self.adView respondsToSelector:@selector(nativePrivacyInformationIconImageView)]) {
        UIImage *privacyIconImage = [adapter.properties objectForKey:kAdPrivacyIconUIImageKey];
        NSString *privacyIconImageUrl = [adapter.properties objectForKey:kAdPrivacyIconImageUrlKey];
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
                [self.vastTracking handleVASTError:MPVASTErrorUndefined videoTimeOffset:0];
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
        mediaView.userInteractionEnabled = YES;
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
        if (![self hasIconView] && [self.adapter.properties objectForKey:kAdIconImageKey] && [self.adView respondsToSelector:@selector(nativeIconImageView)]) {
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

- (void)tick:(MPTimer *)timer
{
    if (self.videoController) {
        BOOL playVisible = MPViewIntersectsParentWindowWithPercent(self.videoController.playerView, self.nativeVideoAdConfig.playVisiblePercent/100.0f);
        if (playVisible) {
            // start new
            if ([self.videoController shouldStartNewPlayer]) {
                [self.videoController loadAndPlayVideo];
            }

            // resume play
            if ([self.videoController shouldResumePlayer]) {
                [self.videoController resume];
            }
        }

        // pause video
        BOOL pauseVisible = !MPViewIntersectsParentWindowWithPercent(self.videoController.playerView, self.nativeVideoAdConfig.pauseVisiblePercent/100.0f);
        if (pauseVisible) {
            if ([self.videoController shouldPausePlayer]) {
                [self.videoController pause];
            }
        }
    }
}

#pragma mark - MOPUBPlayerViewControllerDelegate

- (void)willEnterFullscreen:(MOPUBPlayerViewController *)viewController
{
    [self enterFullscreen:[[self.adapter delegate] viewControllerForPresentingModalView]];
}

- (void)playerDidProgressToTime:(NSTimeInterval)playbackTime
{
    [self.adapter handleVideoHasProgressedToTime:playbackTime];

    // Only the first impression is tracked.
    if (!self.trackingImpressionFired) {
        self.trackingImpressionFired = YES;

        // Fire MoPub impression tracking
        [self.adapter handleVideoViewImpression];
        // Fire VAST Impression Tracking
        [self.vastTracking handleVideoEvent:MPVideoEventImpression
                            videoTimeOffset:playbackTime];
    }

    NSTimeInterval videoDuration = CMTimeGetSeconds(self.videoController.playerItem.duration);
    [self.vastTracking handleVideoProgressEvent:playbackTime videoDuration:videoDuration];
}

- (void)ctaTapped:(MOPUBFullscreenPlayerViewController *)viewController
{
    // MoPub video CTA button clicked. Only the first click is tracked. The check is handled in MPNativeAd.
    [self.adapter handleVideoViewClick];
    [self.vastTracking handleVideoEvent:MPVideoEventClick
                        videoTimeOffset:self.videoController.avPlayer.currentPlaybackTime];
}

- (void)fullscreenPlayerWillLeaveApplication:(MOPUBFullscreenPlayerViewController *)viewController
{
    if ([self.adapter.delegate respondsToSelector:@selector(nativeAdWillLeaveApplicationFromAdapter:)]) {
        [self.adapter.delegate nativeAdWillLeaveApplicationFromAdapter:self.adapter];
    }
}

// being called from MPNativeAd
- (void)nativeAdTapped
{
    [self.vastTracking handleVideoEvent:MPVideoEventClick
                        videoTimeOffset:self.videoController.avPlayer.currentPlaybackTime];
}

#pragma mark - MPNativeAdRendererImageHandlerDelegate

- (BOOL)nativeAdViewInViewHierarchy
{
    return self.adViewInViewHierarchy;
}

#pragma mark - Internal
- (void)enterFullscreen:(UIViewController *)fromViewController
{
    MOPUBFullscreenPlayerViewController *vc = [[MOPUBFullscreenPlayerViewController alloc] initWithVideoPlayer:self.videoController nativeAdProperties:self.adapter.properties dismissBlock:^(UIView *originalParentView) {
        self.videoController.view.frame = originalParentView.bounds;
        self.videoController.delegate = self;
        [self.videoController willExitFullscreen];
        if ([self.adapter.delegate respondsToSelector:@selector(nativeAdDidDismissModalForAdapter:)]) {
            [self.adapter.delegate nativeAdDidDismissModalForAdapter:self.adapter];
        }
        [originalParentView addSubview:self.videoController.playerView];
    }];
    vc.delegate = self;
    if ([self.adapter.delegate respondsToSelector:@selector(nativeAdWillPresentModalForAdapter:)]) {
        [self.adapter.delegate nativeAdWillPresentModalForAdapter:self.adapter];
    }
    [fromViewController presentViewController:vc animated:NO completion:nil];
}

- (void)initAdView
{
    if (!self.videoController) {
        if ([self.renderingViewClass respondsToSelector:@selector(nibForAd)]) {
            self.adView = (UIView<MPNativeAdRendering> *)[[[self.renderingViewClass nibForAd] instantiateWithOwner:nil options:nil] firstObject];
        } else {
            self.adView = [[self.renderingViewClass alloc] init];
        }
    }
}

- (void)setupVideoView
{
    // If a video controller is nil or it's already been disposed, create/recreate the videoController
    if ([self.adView respondsToSelector:(@selector(nativeVideoView))]) {
        BOOL createdNewVideoController = NO;
        self.videoConfig = [self.adapter.properties objectForKey:kVideoConfigKey];
        self.nativeVideoAdConfig = [self.adapter.properties objectForKey:kNativeAdConfigKey];

        if (!self.videoController || self.videoController.disposed) {
            createdNewVideoController = YES;
            self.videoController = [[MOPUBPlayerManager sharedInstance] playerViewControllerWithVideoConfig:self.videoConfig
                                                                                        nativeVideoAdConfig:self.nativeVideoAdConfig];
            self.videoController.defaultActionURL = self.adapter.defaultActionURL;
            self.videoController.displayMode = MOPUBPlayerDisplayModeInline;
            self.videoController.delegate = self;
            self.videoController.view.frame = self.adView.nativeVideoView.bounds;
            [self.adView.nativeVideoView addSubview:self.videoController.view];
            [self.adView bringSubviewToFront:self.adView.nativeVideoView];

            if (!self.autoPlayTimer) {
                self.autoPlayTimer = [MPTimer timerWithTimeInterval:kAutoPlayTimerInterval
                                                             target:self
                                                           selector:@selector(tick:)
                                                            repeats:YES
                                                        runLoopMode:NSRunLoopCommonModes];
                [self.autoPlayTimer scheduleNow];
            }
        }
    }
}

@end
