//
//  MPMRAIDInterstitialCustomEvent.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPMRAIDInterstitialCustomEvent.h"
#import "MPMRAIDInterstitialViewController.h"
#import "MPAdConfiguration.h"
#import "MPError.h"
#import "MPLogging.h"

@interface MPMRAIDInterstitialCustomEvent ()

@property (nonatomic, strong) MPMRAIDInterstitialViewController *interstitial;

@end

@interface MPMRAIDInterstitialCustomEvent (MPInterstitialViewControllerDelegate) <MPInterstitialViewControllerDelegate>
@end

@implementation MPMRAIDInterstitialCustomEvent

// Explicitly `@synthesize` here to fix a "-Wobjc-property-synthesis" warning because super class `delegate` is
// `id<MPInterstitialCustomEventDelegate>` and this `delegate` is `id<MPPrivateInterstitialCustomEventDelegate>`
@synthesize delegate;

- (NSString *)adUnitId
{
    return [self.delegate adUnitId];
}

- (void)requestInterstitialWithCustomEventInfo:(NSDictionary *)info adMarkup:(NSString *)adMarkup
{
    MPAdConfiguration * configuration = self.delegate.configuration;
    MPLogAdEvent([MPLogEvent adLoadAttemptForAdapter:NSStringFromClass(self.class) dspCreativeId:configuration.dspCreativeId dspName:nil], self.adUnitId);

    self.interstitial = [[MPMRAIDInterstitialViewController alloc] initWithAdConfiguration:configuration];
    self.interstitial.delegate = self;

    // The MRAID ad view will handle the close button so we don't need the MPInterstitialViewController's close button.
    [self.interstitial setCloseButtonStyle:MPInterstitialCloseButtonStyleAlwaysHidden];
    [self.interstitial startLoading];
}

- (void)showInterstitialFromRootViewController:(UIViewController *)controller
{
    MPLogAdEvent([MPLogEvent adShowAttemptForAdapter:NSStringFromClass(self.class)], self.adUnitId);
    [self.interstitial presentInterstitialFromViewController:controller complete:^(NSError * error) {
        if (error != nil) {
            MPLogAdEvent([MPLogEvent adShowFailedForAdapter:NSStringFromClass(self.class) error:error], self.adUnitId);
        }
        else {
            MPLogAdEvent([MPLogEvent adShowSuccessForAdapter:NSStringFromClass(self.class)], self.adUnitId);
        }
    }];
}

@end

#pragma mark - MPInterstitialViewControllerDelegate

@implementation MPMRAIDInterstitialCustomEvent (MPInterstitialViewControllerDelegate)

- (void)interstitialDidLoadAd:(id<MPInterstitialViewController>)interstitial
{
    MPLogAdEvent([MPLogEvent adLoadSuccessForAdapter:NSStringFromClass(self.class)], self.adUnitId);
    [self.delegate interstitialCustomEvent:self didLoadAd:self.interstitial];
}

- (void)interstitialDidFailToLoadAd:(id<MPInterstitialViewController>)interstitial
{
    NSString * message = [NSString stringWithFormat:@"Failed to load creative:\n%@", self.delegate.configuration.adResponseHTMLString];
    NSError * error = [NSError errorWithCode:MOPUBErrorAdapterFailedToLoadAd localizedDescription:message];

    MPLogAdEvent([MPLogEvent adLoadFailedForAdapter:NSStringFromClass(self.class) error:error], self.adUnitId);
    [self.delegate interstitialCustomEvent:self didFailToLoadAdWithError:error];
}

- (void)interstitialWillAppear:(id<MPInterstitialViewController>)interstitial
{
    [self.delegate interstitialCustomEventWillAppear:self];
}

- (void)interstitialDidAppear:(id<MPInterstitialViewController>)interstitial
{
    [self.delegate interstitialCustomEventDidAppear:self];
}

- (void)interstitialWillDisappear:(id<MPInterstitialViewController>)interstitial
{
    [self.delegate interstitialCustomEventWillDisappear:self];
}

- (void)interstitialDidDisappear:(id<MPInterstitialViewController>)interstitial
{
    [self.delegate interstitialCustomEventDidDisappear:self];

    // Deallocate the interstitial as we don't need it anymore. If we don't deallocate the interstitial after dismissal,
    // then the html in the webview will continue to run which could lead to bugs such as continuing to play the sound of an inline
    // video since the app may hold onto the interstitial ad controller. Moreover, we keep an array of controllers around as well.
    self.interstitial = nil;
}

- (void)interstitialDidReceiveTapEvent:(id<MPInterstitialViewController>)interstitial
{
    [self.delegate interstitialCustomEventDidReceiveTapEvent:self];
}

- (void)interstitialWillLeaveApplication:(id<MPInterstitialViewController>)interstitial
{
    [self.delegate interstitialCustomEventWillLeaveApplication:self];
}

@end
