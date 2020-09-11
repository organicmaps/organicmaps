//
//  MPAdImpressionTimer.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPAdImpressionTimer.h"
#import "MPTimer.h"
#import "MPGlobal.h"
#import "MPLogging.h"

@interface MPAdImpressionTimer ()

@property (nonatomic) UIView *adView;
@property (nonatomic) MPTimer *viewVisibilityTimer;
@property (nonatomic) NSTimeInterval firstVisibilityTimestamp;
@property (nonatomic) CGFloat pixelsRequiredForViewVisibility;
@property (nonatomic) CGFloat percentageRequiredForViewVisibility;
@property (nonatomic) NSTimeInterval requiredSecondsForImpression;

@end

@implementation MPAdImpressionTimer

static const CGFloat kImpressionTimerInterval = 0.1;
static const NSTimeInterval kFirstVisibilityTimestampNone = -1;
static const CGFloat kDefaultPixelCountWhenUsingPercentage = CGFLOAT_MIN;

- (instancetype)initWithRequiredSecondsForImpression:(NSTimeInterval)requiredSecondsForImpression requiredViewVisibilityPixels:(CGFloat)visibilityPixels
{
    if (self = [super init]) {
        _viewVisibilityTimer = [MPTimer timerWithTimeInterval:kImpressionTimerInterval
                                                       target:self
                                                     selector:@selector(tick:)
                                                      repeats:YES
                                                  runLoopMode:NSRunLoopCommonModes];
        _requiredSecondsForImpression = requiredSecondsForImpression;
        _pixelsRequiredForViewVisibility = visibilityPixels;
        _firstVisibilityTimestamp = kFirstVisibilityTimestampNone;
    }

    return self;
}

- (instancetype)initWithRequiredSecondsForImpression:(NSTimeInterval)requiredSecondsForImpression requiredViewVisibilityPercentage:(CGFloat)visibilityPercentage
{
    if (self = [super init]) {
        // Set `pixelsRequiredForViewVisibility` to a default invalid value so that we know to use the percent directly instead.
        _pixelsRequiredForViewVisibility = kDefaultPixelCountWhenUsingPercentage;

        _viewVisibilityTimer = [MPTimer timerWithTimeInterval:kImpressionTimerInterval
                                                       target:self
                                                     selector:@selector(tick:)
                                                      repeats:YES
                                                  runLoopMode:NSRunLoopCommonModes];
        _requiredSecondsForImpression = requiredSecondsForImpression;
        _percentageRequiredForViewVisibility = visibilityPercentage;
        _firstVisibilityTimestamp = kFirstVisibilityTimestampNone;
    }

    return self;
}

- (void)dealloc
{
    [_viewVisibilityTimer invalidate];
    _delegate = nil;
    _viewVisibilityTimer = nil;
}

- (void)startTrackingView:(UIView *)view
{
    if (!view) {
        MPLogInfo(@"Cannot track empty view");
        return;
    }

    if (self.viewVisibilityTimer.isCountdownActive) {
        MPLogInfo(@"viewVisibilityTimer is already started.");
        return;
    }

    [self.viewVisibilityTimer scheduleNow];
    self.adView = view;
}

#pragma mark - MPTimer

- (void)tick:(MPTimer *)timer
{
    CGFloat adViewArea = CGRectGetWidth(self.adView.bounds) * CGRectGetHeight(self.adView.bounds);
    if (adViewArea == 0) {
        MPLogInfo(@"ad view area cannot be 0");
        return;
    }

    // If using pixels, recalculate the percent of the view that the pixels take up in case the view has resized
    // since the previous tick. If using percent, this is irrelevant.
    if (self.pixelsRequiredForViewVisibility != kDefaultPixelCountWhenUsingPercentage) {
        // The following comment explains the meaning of Ads visible.
        // Assume adView frame: (0, 0, 10, 10), self.requiredViewVisibilityPixels = 1.
        // We get adViewArea = 10 * 10 = 100. percentVisible = (1 * 1) / 100 = 0.01
        // In this case, adView is considerated to be visible when the ratio of adView's intersection area with it's parent window is >= 0.01.
        self.percentageRequiredForViewVisibility = (self.pixelsRequiredForViewVisibility * self.pixelsRequiredForViewVisibility) / adViewArea;
    }

    // Calculate visibility based on percent
    BOOL visible = MPViewIsVisible(self.adView) && MPViewIntersectsParentWindowWithPercent(self.adView, self.percentageRequiredForViewVisibility) && ([[UIApplication sharedApplication] applicationState] == UIApplicationStateActive);

    if (visible) {
        NSTimeInterval now = [[NSDate date] timeIntervalSinceReferenceDate];

        if (self.firstVisibilityTimestamp == kFirstVisibilityTimestampNone) {
            self.firstVisibilityTimestamp = now;
        } else if (now - self.firstVisibilityTimestamp >= self.requiredSecondsForImpression) {
            // Invalidate the timer and tell the delegate it should track an impression.
            self.firstVisibilityTimestamp = kFirstVisibilityTimestampNone;
            [self.viewVisibilityTimer invalidate];
            self.viewVisibilityTimer = nil;

            [self.delegate adViewWillLogImpression:self.adView];
        }
    } else {
        self.firstVisibilityTimestamp = kFirstVisibilityTimestampNone;
    }
}

@end
