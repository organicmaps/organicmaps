//
//  MPVideoPlayerContainerView.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPLogging.h"
#import "MPVideoPlayerContainerView.h"
#import "MPVideoPlayerFullScreenVASTAdOverlay.h"
#import "MPVideoPlayerView.h"
#import "MPVideoPlayerViewOverlay.h"
#import "UIView+MPAdditions.h"

static const NSTimeInterval kAnimationTimeInterval = 0.5;

@interface MPVideoPlayerContainerView ()

@property (nonatomic, strong) MPVideoConfig *videoConfig;
@property (nonatomic, strong) MPVideoPlayerView *videoPlayerView;
@property (nonatomic, strong) MPVASTCompanionAdView *companionAdView;
@property (nonatomic, strong) NSArray<NSLayoutConstraint *> *companionAdViewEdgeConstraints; // see `updateCompanionAdViewEdgeConstraints`
@property (nonatomic, strong) UIVisualEffectView *blurEffectView; // only show if video ends with no companion ad
@property (nonatomic, strong) UIView<MPVideoPlayerViewOverlay> *overlay;
@property (nonatomic, assign) BOOL isVideoFinished; // default to NO

@end

@interface MPVideoPlayerContainerView (MPVideoPlayerViewDelegate) <MPVideoPlayerViewDelegate>
@end

@interface MPVideoPlayerContainerView (MPVideoPlayerFullScreenVASTAdOverlayDelegate) <MPVideoPlayerFullScreenVASTAdOverlayDelegate>
@end

@interface MPVideoPlayerContainerView (MPVASTCompanionAdViewDelegate) <MPVASTCompanionAdViewDelegate>
@end

@implementation MPVideoPlayerContainerView

#pragma mark - MPVideoPlayer

- (instancetype)initWithVideoURL:(NSURL *)videoURL videoConfig:(MPVideoConfig *)videoConfig  {
    if (self = [super init]) {
        _videoConfig = videoConfig;
        _videoPlayerView = [[MPVideoPlayerView alloc] initWithVideoURL:videoURL
                                                           videoConfig:videoConfig];
        _videoPlayerView.delegate = self;
        self.backgroundColor = UIColor.blackColor;

        [self addSubview:self.videoPlayerView];
        self.videoPlayerView.translatesAutoresizingMaskIntoConstraints = NO;
        [[self.videoPlayerView.mp_safeTopAnchor constraintEqualToAnchor:self.mp_safeTopAnchor] setActive:YES];
        [[self.videoPlayerView.mp_safeLeadingAnchor constraintEqualToAnchor:self.mp_safeLeadingAnchor] setActive:YES];
        [[self.videoPlayerView.mp_safeBottomAnchor constraintEqualToAnchor:self.mp_safeBottomAnchor] setActive:YES];
        [[self.videoPlayerView.mp_safeTrailingAnchor constraintEqualToAnchor:self.mp_safeTrailingAnchor] setActive:YES];
    }
    return self;
}

- (void)loadVideo {
    if (self.videoPlayerView.didLoadVideo) {
        return;
    }

    [self.videoPlayerView loadVideo];
    [self setUpOverlay];
}

- (void)play {
    if (self.videoPlayerView.hasStartedPlaying == NO) {
        [self preloadCompanionAd];
        [self.overlay handleVideoStartForSkipOffset:self.videoConfig.skipOffset
                                      videoDuration:self.videoPlayerView.videoDuration];
    }

    [self.videoPlayerView play];

    if ([self.overlay respondsToSelector:@selector(resumeTimer)]) {
        [self.overlay resumeTimer];
    }
}

- (void)pause {
    [self.videoPlayerView pause];

    if ([self.overlay respondsToSelector:@selector(pauseTimer)]) {
        [self.overlay pauseTimer];
    }
}

- (void)enableAppLifeCycleEventObservationForAutoPlayPause {
    [self.videoPlayerView enableAppLifeCycleEventObservationForAutoPlayPause];
}

- (void)disableAppLifeCycleEventObservationForAutoPlayPause {
    [self.videoPlayerView disableAppLifeCycleEventObservationForAutoPlayPause];
}

- (void)updateConstraints {
    [super updateConstraints];

    // No companion ad available; do nothing.
    MPVASTCompanionAd *ad = self.companionAdView.ad;
    if (ad == nil) {
        return;
    }

    // If the container view size cannot fit the ad size, or if the ad is web content, then activate
    // the edge constraints of the companion ad view so that it becomes small enough to be shown without
    // being cropped.
    // The dimension constraints have lower priority thus the edge constraints are effective first with higher priority.
    BOOL isContainerSmallerThanCompanionAdSize = self.bounds.size.width < ad.width || self.bounds.size.height < ad.height;
    if (isContainerSmallerThanCompanionAdSize || self.companionAdView.isWebContent) {
        [NSLayoutConstraint activateConstraints:self.companionAdViewEdgeConstraints];
    }
    else {
        [NSLayoutConstraint deactivateConstraints:self.companionAdViewEdgeConstraints];
    }
}

#pragma mark - Overlay

/**
 A helper for setting up @c overlay. Call this during init only.
 */
- (void)setUpOverlay {
    if (self.overlay != nil) {
        MPLogDebug(@"video player overlay has been set up");
        return;
    }

    MPVideoPlayerViewOverlayConfig *config
    = [[MPVideoPlayerViewOverlayConfig alloc]
       initWithCallToActionButtonTitle:self.videoConfig.callToActionButtonTitle
       isRewarded:self.videoConfig.isRewarded
       isClickthroughAllowed:self.videoConfig.clickThroughURL.absoluteString.length > 0
       hasCompanionAd:self.videoConfig.hasCompanionAd
       enableEarlyClickthroughForNonRewardedVideo:self.videoConfig.enableEarlyClickthroughForNonRewardedVideo];
    MPVideoPlayerFullScreenVASTAdOverlay *overlay = [[MPVideoPlayerFullScreenVASTAdOverlay alloc] initWithConfig:config];
    overlay.delegate = self;
    self.overlay = overlay;

    [self addSubview:overlay];
    overlay.translatesAutoresizingMaskIntoConstraints = NO;
    [[overlay.mp_safeTopAnchor constraintEqualToAnchor:self.mp_safeTopAnchor] setActive:YES];
    [[overlay.mp_safeLeadingAnchor constraintEqualToAnchor:self.mp_safeLeadingAnchor] setActive:YES];
    [[overlay.mp_safeBottomAnchor constraintEqualToAnchor:self.mp_safeBottomAnchor] setActive:YES];
    [[overlay.mp_safeTrailingAnchor constraintEqualToAnchor:self.mp_safeTrailingAnchor] setActive:YES];
}

#pragma mark - Companion Ad

- (void)preloadCompanionAd {
    MPVASTCompanionAd *ad = [self.videoConfig companionAdForContainerSize:self.bounds.size];
    if (ad == nil) {
        return;
    }

    if (self.companionAdView != nil) {
        return; // only show one for once
    }

    self.companionAdView = [[MPVASTCompanionAdView alloc] initWithCompanionAd:ad];
    self.companionAdView.delegate = self;
    self.companionAdView.clipsToBounds = YES;
    [self insertSubview:self.companionAdView belowSubview:self.overlay];
    self.companionAdView.translatesAutoresizingMaskIntoConstraints = NO;

    // All companion ad types may pin to the edges of the container.
    self.companionAdViewEdgeConstraints = @[
        [self.companionAdView.mp_safeTopAnchor constraintEqualToAnchor:self.mp_safeTopAnchor],
        [self.companionAdView.mp_safeLeadingAnchor constraintEqualToAnchor:self.mp_safeLeadingAnchor],
        [self.companionAdView.mp_safeBottomAnchor constraintEqualToAnchor:self.mp_safeBottomAnchor],
        [self.companionAdView.mp_safeTrailingAnchor constraintEqualToAnchor:self.mp_safeTrailingAnchor]
    ];

    // Non-web content companion ads should retain their aspect ratio scaling.
    if (!self.companionAdView.isWebContent) {
        NSLayoutConstraint *widthContraint = [self.companionAdView.mp_safeWidthAnchor constraintLessThanOrEqualToConstant:ad.width];
        NSLayoutConstraint *aspectRatioConstraint = [self.companionAdView.mp_safeHeightAnchor constraintEqualToAnchor:self.companionAdView.mp_safeWidthAnchor multiplier:ad.height/ad.width];
        // "High" priority is 750, less than the default "Required" 1000. The edge constraints have the
        // higher priority, so that the companion ad view can be resize to fit into smaller container.
        widthContraint.priority = UILayoutPriorityDefaultHigh;
        aspectRatioConstraint.priority = UILayoutPriorityDefaultHigh;
        [NSLayoutConstraint activateConstraints:@[
            [self.companionAdView.mp_safeCenterXAnchor constraintEqualToAnchor:self.mp_safeCenterXAnchor],
            [self.companionAdView.mp_safeCenterYAnchor constraintEqualToAnchor:self.mp_safeCenterYAnchor],
            widthContraint,
            aspectRatioConstraint
        ]];
    }

    self.companionAdView.alpha = 0; // hidden by default, only show after loaded and video finishes
    [self.companionAdView loadCompanionAd]; // delegate will handle load status updates
}

/**
 Note: Do nothing before the video finishes.
 */
- (void)showCompanionAd {
    if (self.isVideoFinished == NO) { // timing guard
        return;
    }

    if (self.companionAdView != nil
        && self.companionAdView.isLoaded
        && self.companionAdView.alpha == 0) {

        // Notify UI that contraints and layout need to be updated
        [self setNeedsUpdateConstraints];
        [self setNeedsLayout];

        [UIView animateWithDuration:kAnimationTimeInterval animations:^{
            self.companionAdView.alpha = 1;
            self.videoPlayerView.alpha = 0;
        } completion:^(BOOL finished) {
            [self.videoPlayerView removeFromSuperview];
            self.videoPlayerView = nil;
        }];

        [self.delegate videoPlayerContainerView:self didShowCompanionAdView:self.companionAdView];
    } else {
        [self makeVideoBlurry];
    }
}

/**
 Make the video blurry if there is no companion ad to show after the video finishes.
 */
- (void)makeVideoBlurry {
    if (self.blurEffectView != nil) {
        return; // only show one for once
    }

    self.blurEffectView = [UIVisualEffectView new];
    [self.videoPlayerView addSubview:self.blurEffectView];

    self.blurEffectView.translatesAutoresizingMaskIntoConstraints = NO;
    [[self.blurEffectView.mp_safeTopAnchor constraintEqualToAnchor:self.videoPlayerView.mp_safeTopAnchor] setActive:YES];
    [[self.blurEffectView.mp_safeLeadingAnchor constraintEqualToAnchor:self.videoPlayerView.mp_safeLeadingAnchor] setActive:YES];
    [[self.blurEffectView.mp_safeBottomAnchor constraintEqualToAnchor:self.videoPlayerView.mp_safeBottomAnchor] setActive:YES];
    [[self.blurEffectView.mp_safeTrailingAnchor constraintEqualToAnchor:self.videoPlayerView.mp_safeTrailingAnchor] setActive:YES];

    [UIView animateWithDuration:kAnimationTimeInterval animations:^{
        self.blurEffectView.effect = [UIBlurEffect effectWithStyle:UIBlurEffectStyleLight];
    }];
}

@end

#pragma mark - MPVideoPlayerViewDelegate

@implementation MPVideoPlayerContainerView (MPVideoPlayerViewDelegate)

- (void)videoPlayerViewDidLoadVideo:(MPVideoPlayerView *)videoPlayerView {
    [self.delegate videoPlayerContainerViewDidLoadVideo:self];
}

- (void)videoPlayerViewDidFailToLoadVideo:(MPVideoPlayerView *)videoPlayerView error:(NSError *)error {
    [self.delegate videoPlayerContainerViewDidFailToLoadVideo:self error:error];
}

- (void)videoPlayerViewDidStartVideo:(MPVideoPlayerView *)videoPlayerView duration:(NSTimeInterval)duration {
    [self.delegate videoPlayerContainerViewDidStartVideo:self duration:duration];
}

- (void)videoPlayerViewDidCompleteVideo:(MPVideoPlayerView *)videoPlayerView duration:(NSTimeInterval)duration {
    self.isVideoFinished = YES;
    [self showCompanionAd];
    [self.overlay handleVideoComplete];
    [self.delegate videoPlayerContainerViewDidCompleteVideo:self duration:duration];
}

- (void)videoPlayerView:(MPVideoPlayerView *)videoPlayerView
videoDidReachProgressTime:(NSTimeInterval)videoProgress
               duration:(NSTimeInterval)duration {
    [self.delegate videoPlayerContainerView:self
                  videoDidReachProgressTime:videoProgress
                                   duration:duration];
}

- (void)videoPlayerView:(MPVideoPlayerView *)videoPlayerView
        didTriggerEvent:(MPVideoPlayerEvent)event
          videoProgress:(NSTimeInterval)videoProgress {
    [self.delegate videoPlayerContainerView:self
                            didTriggerEvent:event
                              videoProgress:videoProgress];
}

- (void)videoPlayerView:(MPVideoPlayerView *)videoPlayerView
       showIndustryIcon:(MPVASTIndustryIcon *)icon {
    [self.overlay showIndustryIcon:icon];
}

- (void)videoPlayerViewHideIndustryIcon:(MPVideoPlayerView *)videoPlayerView {
    [self.overlay hideIndustryIcon];
}

@end

#pragma mark - MPVideoPlayerFullScreenVASTAdOverlayDelegate

@implementation MPVideoPlayerContainerView (MPVideoPlayerFullScreenVASTAdOverlayDelegate)

#pragma mark - MPVideoPlayerViewOverlayDelegate

- (void)videoPlayerViewOverlay:(id<MPVideoPlayerViewOverlay>)overlay
               didTriggerEvent:(MPVideoPlayerEvent)event {
    [self.delegate videoPlayerContainerView:self
                            didTriggerEvent:event
                              videoProgress:self.videoPlayerView.videoProgress];
}

#pragma mark - MPVASTIndustryIconViewDelegate

- (void)industryIconView:(MPVASTIndustryIconView *)iconView
         didTriggerEvent:(MPVASTResourceViewEvent)event {
    switch (event) {
        case MPVASTResourceViewEvent_ClickThrough: {
            [self.delegate videoPlayerContainerView:self
                           didClickIndustryIconView:iconView
                          overridingClickThroughURL:nil];
            break;
        }
        case MPVASTResourceViewEvent_DidLoadView: {
            [self.delegate videoPlayerContainerView:self didShowIndustryIconView:iconView];
            break;
        }
        case MPVASTResourceViewEvent_FailedToLoadView: {
            MPLogError(@"Failed to load industry icon view: %@", iconView.icon);
            break;
        }
    }
}

- (void)industryIconView:(MPVASTIndustryIconView *)iconView
didTriggerOverridingClickThrough:(NSURL *)url {
    [self.delegate videoPlayerContainerView:self
                   didClickIndustryIconView:iconView
                  overridingClickThroughURL:url];
}

@end

#pragma mark - MPVASTCompanionAdViewDelegate

@implementation MPVideoPlayerContainerView (MPVASTCompanionAdViewDelegate)

- (UIViewController *)viewControllerForPresentingModalMRAIDExpandedView {
    return self.delegate.viewControllerForPresentingModalMRAIDExpandedView;
}

- (void)companionAdView:(MPVASTCompanionAdView *)companionAdView
        didTriggerEvent:(MPVASTResourceViewEvent)event {
    switch (event) {
        case MPVASTResourceViewEvent_ClickThrough: {
            [self.delegate videoPlayerContainerView:self
                            didClickCompanionAdView:companionAdView
                          overridingClickThroughURL:nil];
            break;
        }
        case MPVASTResourceViewEvent_DidLoadView: {
            if (self.isVideoFinished) {
                [self showCompanionAd];
            }
            break;
        }
        case MPVASTResourceViewEvent_FailedToLoadView: {
            MPLogError(@"Failed to load companion ad view: %@", companionAdView.ad);
            [self.companionAdView removeFromSuperview];
            self.companionAdView = nil;
            [self.delegate videoPlayerContainerView:self didFailToLoadCompanionAdView:companionAdView];
            break;
        }
    }
}

- (void)companionAdView:(MPVASTCompanionAdView *)companionAdView
didTriggerOverridingClickThrough:(NSURL *)url {
    [self.delegate videoPlayerContainerView:self
                    didClickCompanionAdView:companionAdView
                  overridingClickThroughURL:url];
}

- (void)companionAdViewRequestDismiss:(MPVASTCompanionAdView *)companionAdView {
    [self.delegate videoPlayerContainerView:self
              companionAdViewRequestDismiss:companionAdView];
}

@end
