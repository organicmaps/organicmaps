//
//  MPStaticNativeAdImpressionTimer.m
//  MoPubSDK
//
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import "MPStaticNativeAdImpressionTimer.h"

#import "MPTimer.h"
#import "MPGlobal.h"
#import "MPNativeAdConstants.h"

static const CGFloat kImpressionTimerInterval = 0.25;
static const NSTimeInterval kFirstVisibilityTimestampNone = -1;

@interface MPStaticNativeAdImpressionTimer ()

@property (nonatomic) UIView *adView;
@property (nonatomic) MPTimer *viewVisibilityTimer;
@property (nonatomic, assign) NSTimeInterval firstVisibilityTimestamp;
@property (nonatomic, assign) CGFloat requiredViewVisibilityPercentage;
@property (nonatomic, readonly) NSTimeInterval requiredSecondsForImpression;

@end

@implementation MPStaticNativeAdImpressionTimer

- (instancetype)initWithRequiredSecondsForImpression:(NSTimeInterval)requiredSecondsForImpression requiredViewVisibilityPercentage:(CGFloat)visibilityPercentage
{
    if (self = [super init]) {
        _viewVisibilityTimer = [MPTimer timerWithTimeInterval:kImpressionTimerInterval target:self selector:@selector(tick:) repeats:YES];
        _viewVisibilityTimer.runLoopMode = NSRunLoopCommonModes;
        _requiredSecondsForImpression = requiredSecondsForImpression;
        _requiredViewVisibilityPercentage = visibilityPercentage;
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
    self.adView = view;

    if (!self.viewVisibilityTimer.isScheduled) {
        [self.viewVisibilityTimer scheduleNow];
    }
}

- (void)tick:(NSTimer *)timer
{
    BOOL visible = MPViewIsVisible(self.adView) && MPViewIntersectsParentWindowWithPercent(self.adView, self.requiredViewVisibilityPercentage) && ([[UIApplication sharedApplication] applicationState] == UIApplicationStateActive);

    if (visible) {
        NSTimeInterval now = [[NSDate date] timeIntervalSinceReferenceDate];

        if (self.firstVisibilityTimestamp == kFirstVisibilityTimestampNone) {
            self.firstVisibilityTimestamp = now;
        } else if (now - self.firstVisibilityTimestamp >= self.requiredSecondsForImpression) {
            // Invalidate the timer and tell the delegate it should track an impression.
            self.firstVisibilityTimestamp = kFirstVisibilityTimestampNone;
            [self.viewVisibilityTimer invalidate];
            self.viewVisibilityTimer = nil;

            [self.delegate trackImpression];
        }
    } else {
        self.firstVisibilityTimestamp = kFirstVisibilityTimestampNone;
    }
}

@end
