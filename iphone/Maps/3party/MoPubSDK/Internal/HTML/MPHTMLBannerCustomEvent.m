//
//  MPHTMLBannerCustomEvent.m
//  MoPub
//
//  Copyright (c) 2013 MoPub. All rights reserved.
//

#import "MPHTMLBannerCustomEvent.h"
#import "MPWebView.h"
#import "MPLogging.h"
#import "MPAdConfiguration.h"
#import "MPInstanceProvider.h"
#import "MPAnalyticsTracker.h"

@interface MPHTMLBannerCustomEvent ()

@property (nonatomic, strong) MPAdWebViewAgent *bannerAgent;

@end

@implementation MPHTMLBannerCustomEvent

@synthesize bannerAgent = _bannerAgent;

- (BOOL)enableAutomaticImpressionAndClickTracking
{
    return NO;
}

- (void)requestAdWithSize:(CGSize)size customEventInfo:(NSDictionary *)info
{
    MPLogInfo(@"Loading MoPub HTML banner");
    MPLogTrace(@"Loading banner with HTML source: %@", [[self.delegate configuration] adResponseHTMLString]);

    CGRect adWebViewFrame = CGRectMake(0, 0, size.width, size.height);
    self.bannerAgent = [[MPInstanceProvider sharedProvider] buildMPAdWebViewAgentWithAdWebViewFrame:adWebViewFrame
                                                                                           delegate:self];
    [self.bannerAgent loadConfiguration:[self.delegate configuration]];
}

- (void)dealloc
{
    self.bannerAgent.delegate = nil;
}

- (void)rotateToOrientation:(UIInterfaceOrientation)newOrientation
{
    [self.bannerAgent rotateToOrientation:newOrientation];
}

#pragma mark - MPAdWebViewAgentDelegate

- (CLLocation *)location
{
    return [self.delegate location];
}

- (NSString *)adUnitId
{
    return [self.delegate adUnitId];
}

- (UIViewController *)viewControllerForPresentingModalView
{
    return [self.delegate viewControllerForPresentingModalView];
}

- (void)adDidFinishLoadingAd:(MPWebView *)ad
{
    MPLogInfo(@"MoPub HTML banner did load");
    [self.delegate bannerCustomEvent:self didLoadAd:ad];
}

- (void)adDidFailToLoadAd:(MPWebView *)ad
{
    MPLogInfo(@"MoPub HTML banner did fail");
    [self.delegate bannerCustomEvent:self didFailToLoadAdWithError:nil];
}

- (void)adDidClose:(MPWebView *)ad
{
    //don't care
}

- (void)adActionWillBegin:(MPWebView *)ad
{
    MPLogInfo(@"MoPub HTML banner will begin action");
    [self.delegate bannerCustomEventWillBeginAction:self];
}

- (void)adActionDidFinish:(MPWebView *)ad
{
    MPLogInfo(@"MoPub HTML banner did finish action");
    [self.delegate bannerCustomEventDidFinishAction:self];
}

- (void)adActionWillLeaveApplication:(MPWebView *)ad
{
    MPLogInfo(@"MoPub HTML banner will leave application");
    [self.delegate bannerCustomEventWillLeaveApplication:self];
}

- (void)trackMPXAndThirdPartyImpressions
{
    [self.bannerAgent invokeJavaScriptForEvent:MPAdWebViewEventAdDidAppear];
}

- (void)startViewabilityTracker
{
    [self.bannerAgent startViewabilityTracker];
}

@end
