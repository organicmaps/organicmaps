//
//  MPHTMLBannerCustomEvent.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPHTMLBannerCustomEvent.h"
#import "MPWebView.h"
#import "MPError.h"
#import "MPLogging.h"
#import "MPAdConfiguration.h"
#import "MPAnalyticsTracker.h"

@interface MPHTMLBannerCustomEvent ()

@property (nonatomic, strong) MPAdWebViewAgent *bannerAgent;

@end

@implementation MPHTMLBannerCustomEvent

// Explicitly `@synthesize` here to fix a "-Wobjc-property-synthesis" warning because super class `delegate` is
// `id<MPBannerCustomEventDelegate>` and this `delegate` is `id<MPPrivateInterstitialCustomEventDelegate>`
@synthesize delegate;

- (BOOL)enableAutomaticImpressionAndClickTracking
{
    return YES;
}

- (void)requestAdWithSize:(CGSize)size customEventInfo:(NSDictionary *)info adMarkup:(NSString *)adMarkup
{
    MPAdConfiguration * configuration = self.delegate.configuration;

    MPLogAdEvent([MPLogEvent adLoadAttemptForAdapter:NSStringFromClass(configuration.customEventClass) dspCreativeId:configuration.dspCreativeId dspName:nil], self.adUnitId);

    CGRect adWebViewFrame = CGRectMake(0, 0, size.width, size.height);
    self.bannerAgent = [[MPAdWebViewAgent alloc] initWithAdWebViewFrame:adWebViewFrame delegate:self];
    [self.bannerAgent loadConfiguration:configuration];
}

- (void)dealloc
{
    self.bannerAgent.delegate = nil;
}

#pragma mark - MPAdWebViewAgentDelegate

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
    MPLogAdEvent([MPLogEvent adLoadSuccessForAdapter:NSStringFromClass(self.class)], self.adUnitId);
    [self.delegate bannerCustomEvent:self didLoadAd:ad];
}

- (void)adDidFailToLoadAd:(MPWebView *)ad
{
    NSString * message = [NSString stringWithFormat:@"Failed to load creative:\n%@", self.delegate.configuration.adResponseHTMLString];
    NSError * error = [NSError errorWithCode:MOPUBErrorAdapterFailedToLoadAd localizedDescription:message];

    MPLogAdEvent([MPLogEvent adLoadFailedForAdapter:NSStringFromClass(self.class) error:error], self.adUnitId);
    [self.delegate bannerCustomEvent:self didFailToLoadAdWithError:error];
}

- (void)adDidClose:(MPWebView *)ad
{
    //don't care
}

- (void)adActionWillBegin:(MPWebView *)ad
{
    [self.delegate bannerCustomEventWillBeginAction:self];
}

- (void)adActionDidFinish:(MPWebView *)ad
{
    [self.delegate bannerCustomEventDidFinishAction:self];
}

- (void)adActionWillLeaveApplication:(MPWebView *)ad
{
    [self.delegate bannerCustomEventWillLeaveApplication:self];
}

- (void)trackImpressionsIncludedInMarkup
{
    [self.bannerAgent invokeJavaScriptForEvent:MPAdWebViewEventAdDidAppear];
}

- (void)startViewabilityTracker
{
    [self.bannerAgent startViewabilityTracker];
}

@end
