//
//  MPMRAIDInterstitialViewController.m
//  MoPub
//
//  Copyright (c) 2012 MoPub, Inc. All rights reserved.
//

#import "MPMRAIDInterstitialViewController.h"
#import "MPInstanceProvider.h"
#import "MPAdConfiguration.h"

@interface MPMRAIDInterstitialViewController ()

@property (nonatomic, retain) MRAdView *interstitialView;
@property (nonatomic, retain) MPAdConfiguration *configuration;
@property (nonatomic, assign) BOOL advertisementHasCustomCloseButton;

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@implementation MPMRAIDInterstitialViewController

@synthesize delegate = _delegate;
@synthesize interstitialView = _interstitialView;
@synthesize configuration = _configuration;
@synthesize advertisementHasCustomCloseButton = _advertisementHasCustomCloseButton;

- (id)initWithAdConfiguration:(MPAdConfiguration *)configuration
{
    self = [super init];
    if (self) {
        CGFloat width = MAX(configuration.preferredSize.width, 1);
        CGFloat height = MAX(configuration.preferredSize.height, 1);
        CGRect frame = CGRectMake(0, 0, width, height);
        self.interstitialView = [[MPInstanceProvider sharedProvider] buildMRAdViewWithFrame:frame
                                                                            allowsExpansion:NO
                                                                           closeButtonStyle:MRAdViewCloseButtonStyleAdControlled
                                                                              placementType:MRAdViewPlacementTypeInterstitial
                                                                                   delegate:self];

        self.interstitialView.adType = configuration.precacheRequired ? MRAdViewAdTypePreCached : MRAdViewAdTypeDefault;
        self.configuration = configuration;
        self.orientationType = [self.configuration orientationType];
        self.advertisementHasCustomCloseButton = NO;
    }
    return self;
}

- (void)dealloc
{
    self.interstitialView.delegate = nil;
    self.interstitialView = nil;
    self.configuration = nil;
    [super dealloc];
}

- (void)viewDidLoad
{
    [super viewDidLoad];

    self.interstitialView.frame = self.view.bounds;
    self.interstitialView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    [self.view addSubview:self.interstitialView];
}

#pragma mark - Public

- (void)startLoading
{
    [self.interstitialView loadCreativeWithHTMLString:[self.configuration adResponseHTMLString]
                                              baseURL:nil];
}

- (BOOL)shouldDisplayCloseButton
{
    return !self.advertisementHasCustomCloseButton;
}

- (void)willPresentInterstitial
{
    if ([self.delegate respondsToSelector:@selector(interstitialWillAppear:)]) {
        [self.delegate interstitialWillAppear:self];
    }
}

- (void)didPresentInterstitial
{
    if ([self.delegate respondsToSelector:@selector(interstitialDidAppear:)]) {
        [self.delegate interstitialDidAppear:self];
    }
}

- (void)willDismissInterstitial
{
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

#pragma mark - MRAdViewDelegate

- (CLLocation *)location
{
    return [self.delegate location];
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

- (void)adDidLoad:(MRAdView *)adView
{
    if ([self.delegate respondsToSelector:@selector(interstitialDidLoadAd:)]) {
        [self.delegate interstitialDidLoadAd:self];
    }
}

- (void)adDidFailToLoad:(MRAdView *)adView
{
    if ([self.delegate respondsToSelector:@selector(interstitialDidFailToLoadAd:)]) {
        [self.delegate interstitialDidFailToLoadAd:self];
    }
}

- (void)adWillClose:(MRAdView *)adView
{
    [self dismissInterstitialAnimated:YES];
}

- (void)adDidClose:(MRAdView *)adView
{
    // TODO:
}

- (void)ad:(MRAdView *)adView didRequestCustomCloseEnabled:(BOOL)enabled
{
    self.advertisementHasCustomCloseButton = enabled;
    [self layoutCloseButton];
}

- (void)appShouldSuspendForAd:(MRAdView *)adView
{

}

- (void)appShouldResumeFromAd:(MRAdView *)adView
{

}

@end
