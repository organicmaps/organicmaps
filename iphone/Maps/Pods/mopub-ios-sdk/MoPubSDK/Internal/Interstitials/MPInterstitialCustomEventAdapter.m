//
//  MPInterstitialCustomEventAdapter.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPInterstitialCustomEventAdapter.h"

#import "MPAdConfiguration.h"
#import "MPAdTargeting.h"
#import "MPConstants.h"
#import "MPCoreInstanceProvider.h"
#import "MPError.h"
#import "MPHTMLInterstitialCustomEvent.h"
#import "MPLogging.h"
#import "MPInterstitialCustomEvent.h"
#import "MPInterstitialAdController.h"
#import "MPMRAIDInterstitialCustomEvent.h"
#import "MPVASTInterstitialCustomEvent.h"
#import "MPRealTimeTimer.h"

@interface MPInterstitialCustomEventAdapter ()

@property (nonatomic, strong) MPInterstitialCustomEvent *interstitialCustomEvent;
@property (nonatomic, strong) MPAdConfiguration *configuration;
@property (nonatomic, assign) BOOL hasTrackedImpression;
@property (nonatomic, assign) BOOL hasTrackedClick;
@property (nonatomic, strong) MPRealTimeTimer *expirationTimer;

@end

@implementation MPInterstitialCustomEventAdapter

- (void)dealloc
{
    if ([self.interstitialCustomEvent respondsToSelector:@selector(invalidate)]) {
        // Secret API to allow us to detach the custom event from (shared instance) routers synchronously
        // See the chartboost interstitial custom event for an example use case.
        [self.interstitialCustomEvent performSelector:@selector(invalidate)];
    }
    self.interstitialCustomEvent.delegate = nil;

    // make sure the custom event isn't released synchronously as objects owned by the custom event
    // may do additional work after a callback that results in dealloc being called
    [[MPCoreInstanceProvider sharedProvider] keepObjectAliveForCurrentRunLoopIteration:_interstitialCustomEvent];
}

- (void)getAdWithConfiguration:(MPAdConfiguration *)configuration targeting:(MPAdTargeting *)targeting
{
    MPLogInfo(@"Looking for custom event class named %@.", configuration.customEventClass);
    self.configuration = configuration;

    MPInterstitialCustomEvent *customEvent = [[configuration.customEventClass alloc] init];
    if (![customEvent isKindOfClass:[MPInterstitialCustomEvent class]]) {
        NSError * error = [NSError customEventClass:configuration.customEventClass doesNotInheritFrom:MPInterstitialCustomEvent.class];
        MPLogEvent([MPLogEvent error:error message:nil]);
        [self.delegate adapter:self didFailToLoadAdWithError:error];
        return;
    }
    customEvent.delegate = self;
    customEvent.localExtras = targeting.localExtras;
    self.interstitialCustomEvent = customEvent;

    [self.interstitialCustomEvent requestInterstitialWithCustomEventInfo:configuration.customEventClassData adMarkup:configuration.advancedBidPayload];
}

- (void)showInterstitialFromViewController:(UIViewController *)controller
{
    [self.interstitialCustomEvent showInterstitialFromRootViewController:controller];
}

#pragma mark - MPInterstitialCustomEventDelegate

- (NSString *)adUnitId
{
    return [self.delegate interstitialAdController].adUnitId;
}

- (CLLocation *)location
{
    return [self.delegate location];
}

- (id)interstitialDelegate
{
    return [self.delegate interstitialDelegate];
}

- (void)interstitialCustomEvent:(MPInterstitialCustomEvent *)customEvent
                      didLoadAd:(id)ad
{
    [self didStopLoading];
    [self.delegate adapterDidFinishLoadingAd:self];

    // Check for MoPub-specific custom events before setting the timer
    // Custom events for 3rd party SDK have their own timeout and expiration handling
    if ([customEvent isKindOfClass:[MPHTMLInterstitialCustomEvent class]]
        || [customEvent isKindOfClass:[MPMRAIDInterstitialCustomEvent class]]
        || [customEvent isKindOfClass:[MPVASTInterstitialCustomEvent class]]) {
        // Set up timer for expiration
        __weak __typeof__(self) weakSelf = self;
        self.expirationTimer = [[MPRealTimeTimer alloc] initWithInterval:[MPConstants adsExpirationInterval] block:^(MPRealTimeTimer *timer){
            __strong __typeof__(weakSelf) strongSelf = weakSelf;
            if (strongSelf && !strongSelf.hasTrackedImpression) {
                [strongSelf interstitialCustomEventDidExpire:strongSelf.interstitialCustomEvent];
            }
            [strongSelf.expirationTimer invalidate];
        }];
        [self.expirationTimer scheduleNow];
    }
}

- (void)interstitialCustomEvent:(MPInterstitialCustomEvent *)customEvent
       didFailToLoadAdWithError:(NSError *)error
{
    [self didStopLoading];
    [self.delegate adapter:self didFailToLoadAdWithError:error];
}

- (void)interstitialCustomEventWillAppear:(MPInterstitialCustomEvent *)customEvent
{
    [self.delegate interstitialWillAppearForAdapter:self];
}

- (void)interstitialCustomEventDidAppear:(MPInterstitialCustomEvent *)customEvent
{
    if ([self.interstitialCustomEvent enableAutomaticImpressionAndClickTracking] && !self.hasTrackedImpression) {
        [self trackImpression];
    }
    [self.delegate interstitialDidAppearForAdapter:self];
}

- (void)interstitialCustomEventWillDisappear:(MPInterstitialCustomEvent *)customEvent
{
    [self.delegate interstitialWillDisappearForAdapter:self];
}

- (void)interstitialCustomEventDidDisappear:(MPInterstitialCustomEvent *)customEvent
{
    [self.delegate interstitialDidDisappearForAdapter:self];
}

- (void)interstitialCustomEventDidExpire:(MPInterstitialCustomEvent *)customEvent
{
    [self.delegate interstitialDidExpireForAdapter:self];
}

- (void)interstitialCustomEventDidReceiveTapEvent:(MPInterstitialCustomEvent *)customEvent
{
    if ([self.interstitialCustomEvent enableAutomaticImpressionAndClickTracking] && !self.hasTrackedClick) {
        self.hasTrackedClick = YES;
        [self trackClick];
    }

    [self.delegate interstitialDidReceiveTapEventForAdapter:self];
}

- (void)interstitialCustomEventWillLeaveApplication:(MPInterstitialCustomEvent *)customEvent
{
    [self.delegate interstitialWillLeaveApplicationForAdapter:self];
}

- (void)trackImpression {
    [super trackImpression];
    self.hasTrackedImpression = YES;
    [self.expirationTimer invalidate];
}

@end
