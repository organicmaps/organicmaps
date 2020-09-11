//
//  MPBaseInterstitialAdapter.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPBaseInterstitialAdapter.h"
#import "MPAdConfiguration.h"
#import "MPGlobal.h"
#import "MPAnalyticsTracker.h"
#import "MPCoreInstanceProvider.h"
#import "MPError.h"
#import "MPTimer.h"
#import "MPConstants.h"

@interface MPBaseInterstitialAdapter ()

@property (nonatomic, strong) MPAdConfiguration *configuration;
@property (nonatomic, strong) MPTimer *timeoutTimer;

- (void)startTimeoutTimer;

@end

@implementation MPBaseInterstitialAdapter

- (id)initWithDelegate:(id<MPInterstitialAdapterDelegate>)delegate
{
    self = [super init];
    if (self) {
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

- (void)getAdWithConfiguration:(MPAdConfiguration *)configuration targeting:(MPAdTargeting *)targeting
{
    // To be implemented by subclasses.
    [self doesNotRecognizeSelector:_cmd];
}

- (void)_getAdWithConfiguration:(MPAdConfiguration *)configuration targeting:(MPAdTargeting *)targeting
{
    self.configuration = configuration;

    [self startTimeoutTimer];
    [self getAdWithConfiguration:configuration targeting:targeting];
}

- (void)startTimeoutTimer
{
    NSTimeInterval timeInterval = (self.configuration && self.configuration.adTimeoutInterval >= 0) ?
            self.configuration.adTimeoutInterval : INTERSTITIAL_TIMEOUT_INTERVAL;

    if (timeInterval > 0) {
        self.timeoutTimer = [MPTimer timerWithTimeInterval:timeInterval
                                                    target:self
                                                  selector:@selector(timeout)
                                                   repeats:NO];
        [self.timeoutTimer scheduleNow];
    }
}

- (void)didStopLoading
{
    [self.timeoutTimer invalidate];
}

- (void)timeout
{
    NSError * error = [NSError errorWithCode:MOPUBErrorAdRequestTimedOut localizedDescription:@"Interstitial ad request timed out"];
    [self.delegate adapter:self didFailToLoadAdWithError:error];
    self.delegate = nil;
}

#pragma mark - Presentation

- (void)showInterstitialFromViewController:(UIViewController *)controller
{
    [self doesNotRecognizeSelector:_cmd];
}

#pragma mark - Metrics

- (void)trackImpression
{
    [[MPAnalyticsTracker sharedTracker] trackImpressionForConfiguration:self.configuration];
    [self.delegate interstitialDidReceiveImpressionEventForAdapter:self];
}

- (void)trackClick
{
    [[MPAnalyticsTracker sharedTracker] trackClickForConfiguration:self.configuration];
}

@end

