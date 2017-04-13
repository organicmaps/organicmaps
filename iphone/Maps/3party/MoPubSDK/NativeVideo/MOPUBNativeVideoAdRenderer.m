//
//  MOPUBNativeVideoAdRenderer.m
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import "MOPUBNativeVideoAdRenderer.h"
#import "MPNativeAdRendererConfiguration.h"
#import "MPNativeAdRenderer.h"
#import "MPNativeAdRendering.h"
#import "MPNativeAdAdapter.h"
#import "MPNativeAdConstants.h"
#import "MPNativeAdError.h"
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
#import "MOPUBNativeVideoImpressionAgent.h"
#import "MOPUBPlayerViewController.h"
#import "MPNativeAdRenderingImageLoader.h"

static const CGFloat kAutoPlayTimerInterval = 0.25f;

@interface MOPUBNativeVideoAdRenderer () <MPNativeAdRenderer, MOPUBPlayerViewControllerDelegate, MOPUBFullscreenPlayerViewControllerDelegate, MPNativeAdRendererImageHandlerDelegate>

@property (nonatomic) UIView<MPNativeAdRendering> *adView;
@property (nonatomic) MOPUBNativeVideoAdAdapter<MPNativeAdAdapter> *adapter;
@property (nonatomic) BOOL adViewInViewHierarchy;
@property (nonatomic) Class renderingViewClass;
@property (nonatomic) MPNativeAdRendererImageHandler *rendererImageHandler;

@property (nonatomic, weak) MOPUBPlayerViewController *videoController;
@property (nonatomic) MPTimer *autoPlayTimer;
@property (nonatomic) MPVideoConfig *videoConfig;
@property (nonatomic) MPVASTTracking *vastTracking;
@property (nonatomic) MOPUBNativeVideoAdConfigValues *nativeVideoAdConfig;
@property (nonatomic) MOPUBNativeVideoImpressionAgent *trackingAgent;
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
        _renderingViewClass = settings.renderingViewClass;
        _viewSizeHandler = [settings.viewSizeHandler copy];
        _rendererImageHandler = [MPNativeAdRendererImageHandler new];
        _rendererImageHandler.delegate = self;
    }

    return self;
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
    if (!self.trackingImpressionFired && [self.trackingAgent shouldTrackImpressionWithCurrentPlaybackTime:playbackTime]) {
        self.trackingImpressionFired = YES;

        // Fire MoPub impression tracking
        [self.adapter handleVideoViewImpression];
        // Fire VAST Impression Tracking
        [self.vastTracking handleVideoEvent:MPVideoEventTypeImpression
                            videoTimeOffset:playbackTime];
    }
    [self.vastTracking handleVideoEvent:MPVideoEventTypeTimeUpdate videoTimeOffset:playbackTime];
}

- (void)ctaTapped:(MOPUBFullscreenPlayerViewController *)viewController
{
    // MoPub video CTA button clicked. Only the first click is tracked. The check is handled in MPNativeAd.
    [self.adapter handleVideoViewClick];
    [self.vastTracking handleVideoEvent:MPVideoEventTypeClick
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
    [self.vastTracking handleVideoEvent:MPVideoEventTypeClick
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
    MOPUBFullscreenPlayerViewController *vc = [[MOPUBFullscreenPlayerViewController alloc] initWithVideoPlayer:self.videoController dismissBlock:^(UIView *originalParentView) {
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
        self.nativeVideoAdConfig = [self.adapter.properties objectForKey:kNativeVideoAdConfigKey];

        if (!self.videoController || self.videoController.disposed) {
            createdNewVideoController = YES;
            self.videoController = [[MOPUBPlayerManager sharedInstance] playerViewControllerWithVideoConfig:self.videoConfig
                                                                                        nativeVideoAdConfig:self.nativeVideoAdConfig
                                                                                         logEventProperties:[self.adapter.properties valueForKey:kLogEventRequestPropertiesKey]];
            self.videoController.defaultActionURL = self.adapter.defaultActionURL;
            self.videoController.displayMode = MOPUBPlayerDisplayModeInline;
            self.videoController.delegate = self;
            self.videoController.view.frame = self.adView.nativeVideoView.bounds;
            [self.adView.nativeVideoView addSubview:self.videoController.view];
            [self.adView bringSubviewToFront:self.adView.nativeVideoView];

            if (!self.autoPlayTimer) {
                self.autoPlayTimer = [MPTimer timerWithTimeInterval:kAutoPlayTimerInterval target:self selector:@selector(tick:) repeats:YES];
                self.autoPlayTimer.runLoopMode = NSRunLoopCommonModes;
                [self.autoPlayTimer scheduleNow];
            }

            self.trackingAgent = [[MOPUBNativeVideoImpressionAgent alloc] initWithVideoView:self.videoController.playerView requiredVisibilityPercentage:self.nativeVideoAdConfig.impressionMinVisiblePercent/100.0f requiredPlaybackDuration:self.nativeVideoAdConfig.impressionVisible];
        }
        // Lazy load vast tracking. It must be created after we know the video controller has been initialized.
        // If we created a new video controller, we must ensure the vast tracking has the new view.
        if (!self.vastTracking) {
            self.vastTracking = [[MPVASTTracking alloc] initWithMPVideoConfig:self.videoConfig videoView:self.videoController.playerView];
        } else if (createdNewVideoController) {
            [self.vastTracking handleNewVideoView:self.videoController.playerView];
        }
        // Always set the videoControllers vast tracking object to be the current renderer's
        self.videoController.vastTracking = self.vastTracking;
    }
}

@end
