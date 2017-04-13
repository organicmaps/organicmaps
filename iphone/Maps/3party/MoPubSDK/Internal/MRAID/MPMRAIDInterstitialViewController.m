//
//  MPMRAIDInterstitialViewController.m
//  MoPub
//
//  Copyright (c) 2012 MoPub, Inc. All rights reserved.
//

#import "MPMRAIDInterstitialViewController.h"
#import "MPInstanceProvider.h"
#import "MPAdConfiguration.h"
#import "MRController.h"

@interface MPMRAIDInterstitialViewController () <MRControllerDelegate>

@property (nonatomic, strong) MPAdConfiguration *configuration;
@property (nonatomic, strong) MRController *mraidController;
@property (nonatomic, strong) UIView *interstitialView;
@property (nonatomic, assign) UIInterfaceOrientationMask supportedOrientationMask;

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@implementation MPMRAIDInterstitialViewController


- (id)initWithAdConfiguration:(MPAdConfiguration *)configuration
{
    self = [super init];
    if (self) {
        CGFloat width = MAX(configuration.preferredSize.width, 1);
        CGFloat height = MAX(configuration.preferredSize.height, 1);
        CGRect frame = CGRectMake(0, 0, width, height);
        self.mraidController = [[MPInstanceProvider sharedProvider] buildInterstitialMRControllerWithFrame:frame delegate:self];

        self.configuration = configuration;
        self.orientationType = [self.configuration orientationType];
    }
    return self;
}

#pragma mark - Public

- (void)startLoading
{
    [self.mraidController loadAdWithConfiguration:self.configuration];
}

- (void)willPresentInterstitial
{
    [self.mraidController disableRequestHandling];
    if ([self.delegate respondsToSelector:@selector(interstitialWillAppear:)]) {
        [self.delegate interstitialWillAppear:self];
    }
}

- (void)didPresentInterstitial
{
    // This ensures that we handle didPresentInterstitial at the end of the run loop, and prevents a bug
    // where code is run before UIKit thinks the presentViewController animation is complete, even though
    // this is is called from the completion block for said animation.
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 0), dispatch_get_main_queue(), ^{
        [self.mraidController handleMRAIDInterstitialDidPresentWithViewController:self];
        if ([self.delegate respondsToSelector:@selector(interstitialDidAppear:)]) {
            [self.delegate interstitialDidAppear:self];
        }
    });
}

- (void)willDismissInterstitial
{
    [self.mraidController disableRequestHandling];
    if ([self.delegate respondsToSelector:@selector(interstitialWillDisappear:)]) {
        [self.delegate interstitialWillDisappear:self];
    }
}

- (void)didDismissInterstitial
{
    if ([self.delegate respondsToSelector:@selector(interstitialDidDisappear:)]) {
        [self.delegate interstitialDidDisappear:self];
    }
}

#pragma mark - MRControllerDelegate

- (CLLocation *)location
{
    if ([self.delegate respondsToSelector:@selector(location)]) {
        return [self.delegate location];
    }
    return nil;
}

- (NSString *)adUnitId
{
    return [self.delegate adUnitId];
}

- (MPAdConfiguration *)adConfiguration
{
    return self.configuration;
}

- (UIViewController *)viewControllerForPresentingModalView
{
    return self;
}

- (void)adDidLoad:(UIView *)adView
{
    [self.interstitialView removeFromSuperview];

    self.interstitialView = adView;
    self.interstitialView.frame = self.view.bounds;
    self.interstitialView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    [self.view addSubview:self.interstitialView];

    if ([self.delegate respondsToSelector:@selector(interstitialDidLoadAd:)]) {
        [self.delegate interstitialDidLoadAd:self];
    }
}

- (void)adDidFailToLoad:(UIView *)adView
{
    if ([self.delegate respondsToSelector:@selector(interstitialDidFailToLoadAd:)]) {
        [self.delegate interstitialDidFailToLoadAd:self];
    }
}

- (void)adWillClose:(UIView *)adView
{
    [self dismissInterstitialAnimated:YES];
}

- (void)adDidClose:(UIView *)adView
{
    // TODO:
}

- (void)appShouldSuspendForAd:(UIView *)adView
{
    [self.delegate interstitialDidReceiveTapEvent:self];
}

- (void)appShouldResumeFromAd:(UIView *)adView
{

}

- (void)setSupportedOrientationMask:(UIInterfaceOrientationMask)supportedOrientationMask
{
    _supportedOrientationMask = supportedOrientationMask;

    // This should be called whenever the return value of -shouldAutorotateToInterfaceOrientation changes. Since the return
    // value is based on _supportedOrientationMask, we do that here. Prevents possible rotation bugs.
    [UIViewController attemptRotationToDeviceOrientation];
}

- (void)rewardedVideoEnded
{
    if ([self.delegate respondsToSelector:@selector(interstitialRewardedVideoEnded)]) {
        [self.delegate interstitialRewardedVideoEnded];
    }
}

#pragma mark - Orientation Handling

// supportedInterfaceOrientations and shouldAutorotate are for ios 6, 7, and 8.
#if __IPHONE_OS_VERSION_MAX_ALLOWED >= MP_IOS_9_0
- (UIInterfaceOrientationMask)supportedInterfaceOrientations
#else
- (NSUInteger)supportedInterfaceOrientations
#endif
{
    return ([[UIApplication sharedApplication] mp_supportsOrientationMask:self.supportedOrientationMask]) ? self.supportedOrientationMask : UIInterfaceOrientationMaskAll;
}

- (BOOL)shouldAutorotate
{
    return YES;
}

// shouldAutorotateToInterfaceOrientation is for ios 5.
- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    return [[UIApplication sharedApplication] mp_doesOrientation:interfaceOrientation matchOrientationMask:self.supportedOrientationMask];
}

@end
