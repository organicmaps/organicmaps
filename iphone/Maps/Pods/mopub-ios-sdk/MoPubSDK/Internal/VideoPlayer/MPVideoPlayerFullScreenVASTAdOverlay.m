//
//  MPVideoPlayerFullScreenVASTAdOverlay.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPCountdownTimerView.h"
#import "MPGlobal.h"
#import "MPLogging.h"
#import "MPTimer.h"
#import "MPVASTConstant.h"
#import "MPVideoPlayer.h"
#import "MPVideoPlayerFullScreenVASTAdOverlay.h"
#import "UIButton+MPAdditions.h"
#import "UIView+MPAdditions.h"

static CGFloat const kRectangleButtonPadding = 16;
static CGFloat const kRoundButtonPadding = 8;

@interface MPVideoPlayerFullScreenVASTAdOverlay ()

@property (nonatomic, strong) MPVideoPlayerViewOverlayConfig *config;
@property (nonatomic, assign) BOOL allowPassthroughForTouches; // default to NO until video is finished with companion shown
@property (nonatomic, strong) MPTimer *clickThroughEnablingTimer;
@property (nonatomic, strong) NSNotificationCenter *notificationCenter;

@property (nonatomic, strong) UIButton *callToActionButton; // located at the bottom-right corner
@property (nonatomic, strong) UIButton *closeButton; // located at the top-right corner
@property (nonatomic, strong) UIButton *skipButton; // located at the top-right corner
@property (nonatomic, strong) MPVASTIndustryIconView *iconView; // located at the top-left corner
@property (nonatomic, strong) NSLayoutConstraint *iconViewWidthConstraint;
@property (nonatomic, strong) NSLayoutConstraint *iconViewHeightConstraint;
@property (nonatomic, strong) MPCountdownTimerView *timerView; // located at the top-right corner

@end

@interface MPVideoPlayerFullScreenVASTAdOverlay (MPVASTIndustryIconViewDelegate) <MPVASTIndustryIconViewDelegate>
@end

@implementation MPVideoPlayerFullScreenVASTAdOverlay

- (instancetype)initWithConfig:(MPVideoPlayerViewOverlayConfig *)config {
    self = [super initWithFrame:CGRectZero];
    if (self) {
        _config = config;
        _allowPassthroughForTouches = NO;
        _notificationCenter = [NSNotificationCenter defaultCenter];
    }
    return self;
}

- (void)dealloc {
    [self.notificationCenter removeObserver:self];
    [self.timerView stopAndSignalCompletion:NO];
    [self.clickThroughEnablingTimer invalidate];
}

- (BOOL)pointInside:(CGPoint)point withEvent:(UIEvent *)event {
    /*
     When the video is playing, this overlay intercepts all touch events. After the video is
     finished, we might need to pass through the touch events to the companion ad underneath,
     unless the touch events happen upon the overlay subviews, such as the Close button.
     */
    if (self.allowPassthroughForTouches) {
        for (UIView *subview in self.subviews) {
            if (subview.isHidden == NO
                && subview.alpha > 0
                && subview.userInteractionEnabled
                && [subview pointInside:[self convertPoint:point toView:subview] withEvent:event]) {
                return YES; // let the subview handle the event
            }
        }
        return NO; // no subview can handle it, pass through
    } else {
        return YES; // let this overlay handle the event (with a tap gesture recognizer)
    }
}

- (void)handleVideoStartForSkipOffset:(MPVASTDurationOffset *)skipOffset
                        videoDuration:(NSTimeInterval)videoDuration {
    if (videoDuration <= 0) {
        MPLogError(@"Video duration [%.2f] is not positive" ,videoDuration);
        return;
    }

    // See the definition of "skippable" at https://developers.mopub.com/dsps/ad-formats/video/
    NSTimeInterval requestedSkipOffset = [skipOffset timeIntervalForVideoWithDuration:videoDuration];
    NSTimeInterval actualSkipOffset;
    if (videoDuration <= kVASTMinimumDurationOfSkippableVideo) { // non-skippable video: honor valid `requestedSkipOffset` override
        if (0 <= requestedSkipOffset && requestedSkipOffset < videoDuration) {
            actualSkipOffset = requestedSkipOffset;
        } else { // don't show Skip button
            actualSkipOffset = videoDuration;
        }
    } else { // skippable video `(kVASTMinimumDurationOfSkippableVideo < videoDuration)`
        if (0 <= requestedSkipOffset) {
            actualSkipOffset = MIN(requestedSkipOffset, kVASTMinimumDurationOfSkippableVideo);
        } else { // use default time offset if requested skip offset is not valid
            actualSkipOffset = kVASTVideoOffsetToShowSkipButtonForSkippableVideo;
        }
    }

    [self showTimerViewForSkipOffset:(skipOffset == nil ? videoDuration : actualSkipOffset)
                       videoDuration:videoDuration];

    if (self.config.isRewarded) {
        [self setUpClickthroughForOffset:actualSkipOffset videoDuration:videoDuration];
    } else { // non-rewarded video
        if (self.config.enableEarlyClickthroughForNonRewardedVideo) {
            [self enableClickthrough];
        } else {
            [self setUpClickthroughForOffset:actualSkipOffset videoDuration:videoDuration];
        }
    }

    [self.notificationCenter addObserver:self
                                selector:@selector(pauseTimer)
                                    name:UIApplicationDidEnterBackgroundNotification
                                  object:nil];
    [self.notificationCenter addObserver:self
                                selector:@selector(resumeTimer)
                                    name:UIApplicationWillEnterForegroundNotification
                                  object:nil];
}

- (void)handleVideoComplete {
    [self showCloseButton];

    if (self.config.hasCompanionAd) {
        self.allowPassthroughForTouches = YES;

        // companion ad and CTA button are mutually exclusive
        [self.callToActionButton removeFromSuperview];
        self.callToActionButton = nil;

        // companion ad and industry icon are mutually exclusive
        [self.iconView removeFromSuperview];
        self.iconView = nil;
    }
}

#pragma mark - App Life Cycle And Timer

- (void)pauseTimer {
    [self.clickThroughEnablingTimer pause];
    [self.timerView pause];
}

- (void)resumeTimer {
    [self.clickThroughEnablingTimer resume];
    [self.timerView resume];
}

#pragma mark - Timer View

/**
 Show the timer view for a given skip offset. If the skip offset is less the video duration, the
 Skip button is shown after the timer reaches 0. If the skip offset is no less than the video duration,
 the Close button is shown after the timer reaches 0.
 */
- (void)showTimerViewForSkipOffset:(NSTimeInterval)skipOffset videoDuration:(NSTimeInterval)videoDuration {
    if (self.timerView) {
        return;
    }

    __weak __typeof__(self) weakSelf = self;
    MPCountdownTimerView *timerView = [[MPCountdownTimerView alloc] initWithDuration:skipOffset timerCompletion:^(BOOL hasElapsed) {
        if (skipOffset < videoDuration) {
            [weakSelf showSkipButton];
        } else {
            [weakSelf showCloseButton];
        }
        [weakSelf.timerView removeFromSuperview];
    }];

    self.timerView = timerView;

    [self addSubview:timerView];
    timerView.translatesAutoresizingMaskIntoConstraints = NO;
    [[timerView.topAnchor constraintEqualToAnchor:self.mp_safeTopAnchor constant:kRoundButtonPadding] setActive:YES];
    [[timerView.trailingAnchor constraintEqualToAnchor:self.mp_safeTrailingAnchor constant:-kRoundButtonPadding] setActive:YES];

    [timerView start];
}

#pragma mark - Click-through (Call To Action / Learn More button)

- (void)setUpClickthroughForOffset:(NSTimeInterval)skipOffset videoDuration:(NSTimeInterval)videoDuration {
    if (self.config.isClickthroughAllowed == NO) {
        return;
    }

    // See click-through timing definition at https://developers.mopub.com/dsps/ad-formats/video/
    self.clickThroughEnablingTimer = [MPTimer timerWithTimeInterval:MIN(skipOffset, videoDuration)
                                                             target:self
                                                           selector:@selector(enableClickthrough)
                                                            repeats:NO];
    [self.clickThroughEnablingTimer scheduleNow];
}

- (void)enableClickthrough {
    if (self.config.isClickthroughAllowed == NO) {
        return;
    }

    [self addGestureRecognizer:[[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(handleClickThrough)]];
    [self showCallToActionButton];
}

- (void)showCallToActionButton {
    // Per Format Unification Phase 2 item 1.2.1, for rewarded video, do not consider companion
    // ad for showing the CTA button - just show it after the skip threshold }

    if (self.config.isClickthroughAllowed == NO || self.config.callToActionButtonTitle.length == 0) {
        return;
    }

    if (self.callToActionButton) {
        [self.callToActionButton setHidden:NO];
        return;
    }

    UIButton *button = [UIButton buttonWithType:UIButtonTypeCustom];
    self.callToActionButton = button;
    button.accessibilityLabel = @"Call To Action Button";
    [button addTarget:self action:@selector(handleClickThrough) forControlEvents:UIControlEventTouchUpInside];
    [button applyMPVideoPlayerBorderedStyleWithTitle:self.config.callToActionButtonTitle];

    [self addSubview:button];
    button.translatesAutoresizingMaskIntoConstraints = NO;
    [[button.bottomAnchor constraintEqualToAnchor:self.mp_safeBottomAnchor constant:-kRectangleButtonPadding] setActive:YES];
    [[button.trailingAnchor constraintEqualToAnchor:self.mp_safeTrailingAnchor constant:-kRectangleButtonPadding] setActive:YES];
}

- (void)handleClickThrough {
    [self.delegate videoPlayerViewOverlay:self didTriggerEvent:MPVideoPlayerEvent_ClickThrough];
}

#pragma mark - Skip Button

- (void)showSkipButton {
    if (self.skipButton) {
        return;
    }

    UIButton *button = [UIButton buttonWithType:UIButtonTypeCustom];
    self.skipButton = button;
    button.accessibilityLabel = @"Skip Button";
    [button addTarget:self action:@selector(didHitSkipButton) forControlEvents:UIControlEventTouchUpInside];
    [button applyMPVideoPlayerBorderedStyleWithTitle:kVASTDefaultSkipButtonTitle];

    [self addSubview:button];
    button.translatesAutoresizingMaskIntoConstraints = NO;
    [[button.topAnchor constraintEqualToAnchor:self.mp_safeTopAnchor constant:kRectangleButtonPadding] setActive:YES];
    [[button.trailingAnchor constraintEqualToAnchor:self.mp_safeTrailingAnchor constant:-kRectangleButtonPadding] setActive:YES];
}

- (void)didHitSkipButton {
    [self.delegate videoPlayerViewOverlay:self didTriggerEvent:MPVideoPlayerEvent_Skip];
}

#pragma mark - Close Button

- (void)showCloseButton {
    if (self.closeButton) {
        return;
    }

    // timer view and Skip button should disappear when Close button is shown
    [self.timerView removeFromSuperview];
    [self.skipButton removeFromSuperview];

    UIButton *button = [UIButton buttonWithType:UIButtonTypeCustom];
    self.closeButton = button;
    button.accessibilityLabel = @"Close Button";
    [button addTarget:self action:@selector(didHitCloseButton) forControlEvents:UIControlEventTouchUpInside];
    [button setImage:[UIImage imageNamed:MPResourcePathForResource(@"MPCloseButtonX.png")] forState:UIControlStateNormal];

    [self addSubview:button];
    button.translatesAutoresizingMaskIntoConstraints = NO;
    [[button.topAnchor constraintEqualToAnchor:self.mp_safeTopAnchor constant:kRoundButtonPadding] setActive:YES];
    [[button.trailingAnchor constraintEqualToAnchor:self.mp_safeTrailingAnchor constant:-kRoundButtonPadding] setActive:YES];
}

- (void)didHitCloseButton {
    [self.delegate videoPlayerViewOverlay:self didTriggerEvent:MPVideoPlayerEvent_Close];
}

#pragma mark - Industry Icon

- (void)showIndustryIcon:(MPVASTIndustryIcon *)icon {
    if (self.iconView == nil) {
        self.iconView = [MPVASTIndustryIconView new];
        self.iconView.iconViewDelegate = self;
        self.iconViewWidthConstraint = [self.iconView.mp_safeWidthAnchor constraintEqualToConstant:icon.width];
        self.iconViewHeightConstraint = [self.iconView.mp_safeHeightAnchor constraintEqualToConstant:icon.height];

        [self addSubview:self.iconView];
        self.iconView.translatesAutoresizingMaskIntoConstraints = NO;
        [[self.iconView.mp_safeTopAnchor constraintEqualToAnchor:self.mp_safeTopAnchor] setActive:YES];
        [[self.iconView.mp_safeLeadingAnchor constraintEqualToAnchor:self.mp_safeLeadingAnchor] setActive:YES];
        [self.iconViewWidthConstraint setActive:YES];
        [self.iconViewHeightConstraint setActive:YES];
    } else {
        // if the icon view already exists, update the width and height
        self.iconViewWidthConstraint.constant = icon.width;
        self.iconViewHeightConstraint.constant = icon.height;
    }

    [self.iconView setHidden:YES]; // hidden by default, only show after loaded
    [self.iconView loadIcon:icon]; // delegate will handle load status updates
}

- (void)hideIndustryIcon {
    [self.iconView setHidden:YES];
}

@end

#pragma mark - MPVASTIndustryIconViewDelegate

@implementation MPVideoPlayerFullScreenVASTAdOverlay (MPVASTIndustryIconViewDelegate)

- (void)industryIconView:(MPVASTIndustryIconView *)iconView
         didTriggerEvent:(MPVASTResourceViewEvent)event {
    switch (event) {
        case MPVASTResourceViewEvent_ClickThrough: {
            break; // no op
        }
        case MPVASTResourceViewEvent_DidLoadView: {
            [self.iconView setHidden:NO];
            break;
        }
        case MPVASTResourceViewEvent_FailedToLoadView: {
            [self.iconView removeFromSuperview];
            self.iconView = nil;
            break;
        }
    }

    [self.delegate industryIconView:iconView didTriggerEvent:event];
}

- (void)industryIconView:(MPVASTIndustryIconView *)iconView
didTriggerOverridingClickThrough:(NSURL *)url {
    [self.delegate industryIconView:iconView didTriggerOverridingClickThrough:url];
}

@end
