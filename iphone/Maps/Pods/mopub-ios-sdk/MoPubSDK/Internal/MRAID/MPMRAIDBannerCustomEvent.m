//
//  MPMRAIDBannerCustomEvent.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPMRAIDBannerCustomEvent.h"
#import "MPLogging.h"
#import "MPAdConfiguration.h"
#import "MRController.h"
#import "MPError.h"
#import "MPWebView.h"
#import "MPViewabilityTracker.h"

@interface MPMRAIDBannerCustomEvent () <MRControllerDelegate>

@property (nonatomic, strong) MRController *mraidController;

@end

@implementation MPMRAIDBannerCustomEvent

// Explicitly `@synthesize` here to fix a "-Wobjc-property-synthesis" warning because super class `delegate` is
// `id<MPBannerCustomEventDelegate>` and this `delegate` is `id<MPPrivateInterstitialCustomEventDelegate>`
@synthesize delegate;

- (void)requestAdWithSize:(CGSize)size customEventInfo:(NSDictionary *)info adMarkup:(NSString *)adMarkup
{
    MPAdConfiguration *configuration = self.delegate.configuration;

    MPLogAdEvent([MPLogEvent adLoadAttemptForAdapter:NSStringFromClass(configuration.customEventClass) dspCreativeId:configuration.dspCreativeId dspName:nil], self.adUnitId);

    CGRect adViewFrame = CGRectZero;
    if ([configuration hasPreferredSize]) {
        adViewFrame = CGRectMake(0, 0, configuration.preferredSize.width,
                                 configuration.preferredSize.height);
    }

    self.mraidController = [[MRController alloc] initWithAdViewFrame:adViewFrame
                                               supportedOrientations:configuration.orientationType
                                                     adPlacementType:MRAdViewPlacementTypeInline
                                                            delegate:self];
    [self.mraidController loadAdWithConfiguration:configuration];
}

#pragma mark - MRControllerDelegate

- (NSString *)adUnitId
{
    return [self.delegate adUnitId];
}

- (UIViewController *)viewControllerForPresentingModalView
{
    return [self.delegate viewControllerForPresentingModalView];
}

- (void)adDidLoad:(UIView *)adView
{
    MPLogAdEvent([MPLogEvent adLoadSuccessForAdapter:NSStringFromClass(self.class)], self.adUnitId);
    [self.delegate bannerCustomEvent:self didLoadAd:adView];
}

- (void)adDidFailToLoad:(UIView *)adView
{
    NSString * message = [NSString stringWithFormat:@"Failed to load creative:\n%@", self.delegate.configuration.adResponseHTMLString];
    NSError * error = [NSError errorWithCode:MOPUBErrorAdapterFailedToLoadAd localizedDescription:message];

    MPLogAdEvent([MPLogEvent adLoadFailedForAdapter:NSStringFromClass(self.class) error:error], self.adUnitId);
    [self.delegate bannerCustomEvent:self didFailToLoadAdWithError:error];
}

- (void)adDidReceiveClickthrough:(NSURL *)url
{
    [self.delegate trackClick];
}

- (void)closeButtonPressed
{
    //don't care
}

- (void)appShouldSuspendForAd:(UIView *)adView
{
    [self.delegate bannerCustomEventWillBeginAction:self];
}

- (void)appShouldResumeFromAd:(UIView *)adView
{
    [self.delegate bannerCustomEventDidFinishAction:self];
}

- (void)trackImpressionsIncludedInMarkup
{
    [self.mraidController triggerWebviewDidAppear];
}

- (void)startViewabilityTracker
{
    [self.mraidController startViewabilityTracking];
}

- (void)adWillExpand:(UIView *)adView
{
    if ([self.delegate respondsToSelector:@selector(bannerCustomEventWillExpandAd:)]) {
        [self.delegate bannerCustomEventWillExpandAd:self];
    }
}

- (void)adDidCollapse:(UIView *)adView
{
    if ([self.delegate respondsToSelector:@selector(bannerCustomEventDidCollapseAd:)]) {
        [self.delegate bannerCustomEventDidCollapseAd:self];
    }
}

@end
