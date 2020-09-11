//
//  MPBaseBannerAdapter.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPBaseBannerAdapter.h"
#import "MPConstants.h"

#import "MPAdConfiguration.h"
#import "MPLogging.h"
#import "MPCoreInstanceProvider.h"
#import "MPAnalyticsTracker.h"
#import "MPTimer.h"
#import "MPError.h"

@interface MPBaseBannerAdapter ()

@property (nonatomic, strong) MPAdConfiguration *configuration;
@property (nonatomic, strong) MPTimer *timeoutTimer;

- (void)startTimeoutTimer;

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@implementation MPBaseBannerAdapter

- (instancetype)initWithDelegate:(id<MPBannerAdapterDelegate>)delegate
{
    if (self = [super init]) {
        self.delegate = delegate;
    }
    return self;
}

- (void)dealloc
{
    [self unregisterDelegate];
    [self.timeoutTimer invalidate];
}

- (void)unregisterDelegate
{
    self.delegate = nil;
}

#pragma mark - Requesting Ads

- (void)getAdWithConfiguration:(MPAdConfiguration *)configuration targeting:(MPAdTargeting *)targeting containerSize:(CGSize)size
{
    // To be implemented by subclasses.
    [self doesNotRecognizeSelector:_cmd];
}

- (void)_getAdWithConfiguration:(MPAdConfiguration *)configuration targeting:(MPAdTargeting *)targeting containerSize:(CGSize)size
{
    self.configuration = configuration;

    [self startTimeoutTimer];
    [self getAdWithConfiguration:configuration targeting:targeting containerSize:size];
}

- (void)didStopLoading
{
    [self.timeoutTimer invalidate];
}

- (void)didDisplayAd
{
    [self trackImpression];
}

- (void)startTimeoutTimer
{
    NSTimeInterval timeInterval = (self.configuration && self.configuration.adTimeoutInterval >= 0) ?
    self.configuration.adTimeoutInterval : BANNER_TIMEOUT_INTERVAL;

    if (timeInterval > 0) {
        self.timeoutTimer = [MPTimer timerWithTimeInterval:timeInterval
                                                    target:self
                                                  selector:@selector(timeout)
                                                   repeats:NO];
        [self.timeoutTimer scheduleNow];
    }
}

- (void)timeout
{
    NSError * error = [NSError errorWithCode:MOPUBErrorAdRequestTimedOut
                           localizedDescription:@"Banner ad request timed out"];
    [self.delegate adapter:self didFailToLoadAdWithError:error];
}

#pragma mark - Rotation

- (void)rotateToOrientation:(UIInterfaceOrientation)newOrientation
{
    // Do nothing by default. Subclasses can override.
}

#pragma mark - Metrics

- (void)trackImpression
{
    [[MPAnalyticsTracker sharedTracker] trackImpressionForConfiguration:self.configuration];
}

- (void)trackClick
{
    [[MPAnalyticsTracker sharedTracker] trackClickForConfiguration:self.configuration];
}

@end
