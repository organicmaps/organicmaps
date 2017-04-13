//
//  MPHTMLInterstitialViewController.m
//  MoPub
//
//  Copyright (c) 2012 MoPub, Inc. All rights reserved.
//

#import "MPHTMLInterstitialViewController.h"
#import "MPWebView.h"
#import "MPAdDestinationDisplayAgent.h"
#import "MPInstanceProvider.h"

@interface MPHTMLInterstitialViewController ()

@property (nonatomic, strong) MPWebView *backingView;

@end

///////////////////////////////////////////////////////////////////////////////////////////////////

@implementation MPHTMLInterstitialViewController

@synthesize delegate = _delegate;
@synthesize backingViewAgent = _backingViewAgent;
@synthesize backingView = _backingView;

- (void)dealloc
{
    self.backingViewAgent.delegate = nil;

    self.backingView.delegate = nil;
}

- (void)viewDidLoad
{
    [super viewDidLoad];

    self.view.backgroundColor = [UIColor blackColor];
    self.backingViewAgent = [[MPInstanceProvider sharedProvider] buildMPAdWebViewAgentWithAdWebViewFrame:self.view.bounds
                                                                                                delegate:self];
}

#pragma mark - Public

- (void)loadConfiguration:(MPAdConfiguration *)configuration
{
    [self view];
    [self.backingViewAgent loadConfiguration:configuration];

    self.backingView = self.backingViewAgent.view;
    self.backingView.frame = self.view.bounds;
    self.backingView.autoresizingMask = UIViewAutoresizingFlexibleWidth |
    UIViewAutoresizingFlexibleHeight;
    [self.view addSubview:self.backingView];
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

    // XXX: In certain cases, UIWebView's content appears off-center due to rotation / auto-
    // resizing while off-screen. -forceRedraw corrects this issue, but there is always a brief
    // instant when the old content is visible. We mask this using a short fade animation.
    [self.backingViewAgent forceRedraw];

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

#pragma mark - Autorotation

- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation
{
    [super didRotateFromInterfaceOrientation:fromInterfaceOrientation];

    [self.backingViewAgent rotateToOrientation:self.interfaceOrientation];
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
