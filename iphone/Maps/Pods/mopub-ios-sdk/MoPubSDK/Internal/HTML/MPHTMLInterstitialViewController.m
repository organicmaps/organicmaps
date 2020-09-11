//
//  MPHTMLInterstitialViewController.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPHTMLInterstitialViewController.h"
#import "MPWebView.h"
#import "MPAdDestinationDisplayAgent.h"
#import "MPViewabilityTracker.h"

@interface MPHTMLInterstitialViewController ()

@property (nonatomic, strong) MPWebView *backingView;

@end

///////////////////////////////////////////////////////////////////////////////////////////////////

@implementation MPHTMLInterstitialViewController

- (void)dealloc
{
    self.backingViewAgent.delegate = nil;

    self.backingView.delegate = nil;
}

- (void)viewDidLoad
{
    [super viewDidLoad];

    self.view.backgroundColor = [UIColor blackColor];
    self.backingViewAgent = [[MPAdWebViewAgent alloc] initWithAdWebViewFrame:self.view.bounds delegate:self];
}

#pragma mark - Public

- (void)loadConfiguration:(MPAdConfiguration *)configuration
{
    [self view];
    [self.backingViewAgent loadConfiguration:configuration];

    self.backingView = self.backingViewAgent.view;
    [self.view addSubview:self.backingView];
    self.backingView.frame = self.view.bounds;
    self.backingView.autoresizingMask = UIViewAutoresizingFlexibleWidth |
    UIViewAutoresizingFlexibleHeight;
    if (@available(iOS 11, *)) {
        self.backingView.translatesAutoresizingMaskIntoConstraints = NO;
        [NSLayoutConstraint activateConstraints:@[
                                                  [self.backingView.topAnchor constraintEqualToAnchor:self.view.topAnchor],
                                                  [self.backingView.leadingAnchor constraintEqualToAnchor:self.view.leadingAnchor],
                                                  [self.backingView.trailingAnchor constraintEqualToAnchor:self.view.trailingAnchor],
                                                  [self.backingView.bottomAnchor constraintEqualToAnchor:self.view.bottomAnchor],
                                                  ]];
    }

    [self.backingViewAgent.viewabilityTracker registerFriendlyObstructionView:self.closeButton];
}

- (void)willPresentInterstitial
{
    self.backingView.alpha = 0.0;
    [self.delegate interstitialWillAppear:self];
}

- (void)didPresentInterstitial
{
    [self.backingViewAgent enableRequestHandling];
    [self.backingViewAgent invokeJavaScriptForEvent:MPAdWebViewEventAdDidAppear];

    [UIView beginAnimations:nil context:nil];
    [UIView setAnimationDuration:0.3];
    self.backingView.alpha = 1.0;
    [UIView commitAnimations];

    [self.delegate interstitialDidAppear:self];
}

- (void)willDismissInterstitial
{
    [self.backingViewAgent disableRequestHandling];
    [self.delegate interstitialWillDisappear:self];
}

- (void)didDismissInterstitial
{
    [self.backingViewAgent invokeJavaScriptForEvent:MPAdWebViewEventAdDidDisappear];
    [self.delegate interstitialDidDisappear:self];
}

#pragma mark - MPAdWebViewAgentDelegate

- (UIViewController *)viewControllerForPresentingModalView
{
    return self;
}

- (void)adDidFinishLoadingAd:(MPWebView *)ad
{
    [self.delegate interstitialDidLoadAd:self];
}

- (void)adDidFailToLoadAd:(MPWebView *)ad
{
    [self.delegate interstitialDidFailToLoadAd:self];
}

- (void)adActionWillBegin:(MPWebView *)ad
{
    [self.delegate interstitialDidReceiveTapEvent:self];
}

- (void)adActionWillLeaveApplication:(MPWebView *)ad
{
    [self.delegate interstitialWillLeaveApplication:self];
    [self dismissInterstitialAnimated:NO];
}

- (void)adActionDidFinish:(MPWebView *)ad
{
    //NOOP: the landing page is going away, but not the interstitial.
}

- (void)adDidClose:(MPWebView *)ad
{
    //NOOP: the ad is going away, but not the interstitial.
}

@end
