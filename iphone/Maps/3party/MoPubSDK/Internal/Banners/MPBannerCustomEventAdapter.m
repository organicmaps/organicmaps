//
//  MPBannerCustomEventAdapter.m
//  MoPub
//
//  Copyright (c) 2012 MoPub, Inc. All rights reserved.
//

#import "MPBannerCustomEventAdapter.h"

#import "MPAdConfiguration.h"
#import "MPBannerCustomEvent.h"
#import "MPInstanceProvider.h"
#import "MPLogging.h"

@interface MPBannerCustomEventAdapter ()

@property (nonatomic, strong) MPBannerCustomEvent *bannerCustomEvent;
@property (nonatomic, strong) MPAdConfiguration *configuration;
@property (nonatomic, assign) BOOL hasTrackedImpression;
@property (nonatomic, assign) BOOL hasTrackedClick;

- (void)trackClickOnce;

@end

@implementation MPBannerCustomEventAdapter
@synthesize hasTrackedImpression = _hasTrackedImpression;
@synthesize hasTrackedClick = _hasTrackedClick;

- (void)unregisterDelegate
{
    if ([self.bannerCustomEvent respondsToSelector:@selector(invalidate)]) {
        // Secret API to allow us to detach the custom event from (shared instance) routers synchronously
        [self.bannerCustomEvent performSelector:@selector(invalidate)];
    }
    self.bannerCustomEvent.delegate = nil;

    // make sure the custom event isn't released synchronously as objects owned by the custom event
    // may do additional work after a callback that results in unregisterDelegate being called
    [[MPCoreInstanceProvider sharedProvider] keepObjectAliveForCurrentRunLoopIteration:_bannerCustomEvent];

    [super unregisterDelegate];
}

////////////////////////////////////////////////////////////////////////////////////////////////////

- (void)getAdWithConfiguration:(MPAdConfiguration *)configuration containerSize:(CGSize)size
{
    MPLogInfo(@"Looking for custom event class named %@.", configuration.customEventClass);
    self.configuration = configuration;

    self.bannerCustomEvent = [[MPInstanceProvider sharedProvider] buildBannerCustomEventFromCustomClass:configuration.customEventClass
                                                                                               delegate:self];
    if (self.bannerCustomEvent) {
        [self.bannerCustomEvent requestAdWithSize:size customEventInfo:configuration.customEventClassData];
    } else {
        [self.delegate adapter:self didFailToLoadAdWithError:nil];
    }
}

- (void)rotateToOrientation:(UIInterfaceOrientation)newOrientation
{
    [self.bannerCustomEvent rotateToOrientation:newOrientation];
}

- (void)didDisplayAd
{
    if ([self.bannerCustomEvent enableAutomaticImpressionAndClickTracking] && !self.hasTrackedImpression) {
        self.hasTrackedImpression = YES;
        [self trackImpression];
    }

    [self.bannerCustomEvent didDisplayAd];
}

////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma mark - MPPrivateBannerCustomEventDelegate

- (NSString *)adUnitId
{
    return [self.delegate banner].adUnitId;
}

- (UIViewController *)viewControllerForPresentingModalView
{
    return [self.delegate viewControllerForPresentingModalView];
}

- (id)bannerDelegate
{
    return [self.delegate bannerDelegate];
}

- (CLLocation *)location
{
    return [self.delegate location];
}

- (void)bannerCustomEvent:(MPBannerCustomEvent *)event didLoadAd:(UIView *)ad
{
    [self didStopLoading];
    if (ad) {
        [self.delegate adapter:self didFinishLoadingAd:ad];
    } else {
        [self.delegate adapter:self didFailToLoadAdWithError:nil];
    }
}

- (void)bannerCustomEvent:(MPBannerCustomEvent *)event didFailToLoadAdWithError:(NSError *)error
{
    [self didStopLoading];
    [self.delegate adapter:self didFailToLoadAdWithError:error];
}

- (void)bannerCustomEventWillBeginAction:(MPBannerCustomEvent *)event
{
    [self trackClickOnce];
    [self.delegate userActionWillBeginForAdapter:self];
}

- (void)bannerCustomEventDidFinishAction:(MPBannerCustomEvent *)event
{
    [self.delegate userActionDidFinishForAdapter:self];
}

- (void)bannerCustomEventWillLeaveApplication:(MPBannerCustomEvent *)event
{
    [self trackClickOnce];
    [self.delegate userWillLeaveApplicationFromAdapter:self];
}

- (void)trackClickOnce
{
    if ([self.bannerCustomEvent enableAutomaticImpressionAndClickTracking] && !self.hasTrackedClick) {
        self.hasTrackedClick = YES;
        [self trackClick];
    }
}

@end
