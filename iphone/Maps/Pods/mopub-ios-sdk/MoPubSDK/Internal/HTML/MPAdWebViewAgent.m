//
//  MPAdWebViewAgent.m
//
//  Copyright 2018-2019 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <WebKit/WebKit.h>
#import "MPAdWebViewAgent.h"
#import "MPAdConfiguration.h"
#import "MPGlobal.h"
#import "MPLogging.h"
#import "MPAdDestinationDisplayAgent.h"
#import "NSURL+MPAdditions.h"
#import "MPWebView.h"
#import "MPCoreInstanceProvider.h"
#import "MPUserInteractionGestureRecognizer.h"
#import "NSJSONSerialization+MPAdditions.h"
#import "NSURL+MPAdditions.h"
#import "MPAPIEndPoints.h"
#import "MoPub.h"
#import "MPViewabilityTracker.h"
#import "NSString+MPAdditions.h"

#ifndef NSFoundationVersionNumber_iOS_6_1
#define NSFoundationVersionNumber_iOS_6_1 993.00
#endif

@interface MPAdWebViewAgent () <UIGestureRecognizerDelegate>

@property (nonatomic, strong) MPAdConfiguration *configuration;
@property (nonatomic, strong) MPAdDestinationDisplayAgent *destinationDisplayAgent;
@property (nonatomic, assign) BOOL shouldHandleRequests;
@property (nonatomic, strong) id<MPAdAlertManagerProtocol> adAlertManager;
@property (nonatomic, assign) BOOL userInteractedWithWebView;
@property (nonatomic, strong) MPUserInteractionGestureRecognizer *userInteractionRecognizer;
@property (nonatomic, assign) CGRect frame;
@property (nonatomic, strong, readwrite) MPViewabilityTracker *viewabilityTracker;
@property (nonatomic, assign) BOOL didFireClickImpression;

- (void)performActionForMoPubSpecificURL:(NSURL *)URL;
- (BOOL)shouldIntercept:(NSURL *)URL navigationType:(WKNavigationType)navigationType;
- (void)interceptURL:(NSURL *)URL;

@end

@implementation MPAdWebViewAgent

- (id)initWithAdWebViewFrame:(CGRect)frame delegate:(id<MPAdWebViewAgentDelegate>)delegate
{
    self = [super init];
    if (self) {
        _frame = frame;

        self.destinationDisplayAgent = [MPAdDestinationDisplayAgent agentWithDelegate:self];
        self.delegate = delegate;
        self.shouldHandleRequests = YES;
        self.didFireClickImpression = NO;
        self.adAlertManager = [[MPCoreInstanceProvider sharedProvider] buildMPAdAlertManagerWithDelegate:self];

        self.userInteractionRecognizer = [[MPUserInteractionGestureRecognizer alloc] initWithTarget:self action:@selector(handleInteraction:)];
        self.userInteractionRecognizer.cancelsTouchesInView = NO;
        self.userInteractionRecognizer.delegate = self;
    }
    return self;
}

- (void)dealloc
{
    [self.viewabilityTracker stopTracking];
    self.userInteractionRecognizer.delegate = nil;
    [self.userInteractionRecognizer removeTarget:self action:nil];
    [self.destinationDisplayAgent cancel];
    [self.destinationDisplayAgent setDelegate:nil];
    self.view.delegate = nil;
}

- (void)handleInteraction:(UITapGestureRecognizer *)sender
{
    if (sender.state == UIGestureRecognizerStateEnded) {
        self.userInteractedWithWebView = YES;
    }
}

#pragma mark - <UIGestureRecognizerDelegate>

- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer *)otherGestureRecognizer
{
    return YES;
}

#pragma mark - <MPAdAlertManagerDelegate>

- (UIViewController *)viewControllerForPresentingMailVC
{
    return [self.delegate viewControllerForPresentingModalView];
}

- (void)adAlertManagerDidTriggerAlert:(MPAdAlertManager *)manager
{
    [self.adAlertManager processAdAlertOnce];
}

#pragma mark - Public

- (void)loadConfiguration:(MPAdConfiguration *)configuration
{
    self.configuration = configuration;

    // Initialize web view
    if (self.view != nil) {
        self.view.delegate = nil;
        [self.view removeFromSuperview];
        self.view = nil;
    }
    self.view = [[MPWebView alloc] initWithFrame:self.frame];
    self.view.shouldConformToSafeArea = [self isInterstitialAd];
    self.view.delegate = self;
    [self.view addGestureRecognizer:self.userInteractionRecognizer];

    // Ignore server configuration size for interstitials. At this point our web view
    // is sized correctly for the device's screen. Currently the server sends down values for a 3.5in
    // screen, and they do not size correctly on a 4in screen.
    if (configuration.isFullscreenAd == false) {
        if ([configuration hasPreferredSize]) {
            CGRect frame = self.view.frame;
            frame.size.width = configuration.preferredSize.width;
            frame.size.height = configuration.preferredSize.height;
            self.view.frame = frame;
        }
    }

    [self.view mp_setScrollable:NO];

    // Initialize viewability trackers before loading self.view
    [self init3rdPartyViewabilityTrackers];

    [self.view loadHTMLString:[configuration adResponseHTMLString]
                      baseURL:[NSURL URLWithString:[MPAPIEndpoints baseURL]]
     ];

    [self initAdAlertManager];
}

- (void)invokeJavaScriptForEvent:(MPAdWebViewEvent)event
{
    switch (event) {
        case MPAdWebViewEventAdDidAppear:
            // For banner, viewability tracker is handled right after adView is initialized (not here).
            // For interstitial (handled here), we start tracking viewability if it's not started during adView initialization.
            if (![self shouldStartViewabilityDuringInitialization]) {
                [self startViewabilityTracker];
            }

            [self.view stringByEvaluatingJavaScriptFromString:@"webviewDidAppear();"];
            break;
        case MPAdWebViewEventAdDidDisappear:
            [self.view stringByEvaluatingJavaScriptFromString:@"webviewDidClose();"];
            break;
        default:
            break;
    }
}

- (void)startViewabilityTracker
{
    [self.viewabilityTracker startTracking];
}

- (void)disableRequestHandling
{
    self.shouldHandleRequests = NO;
    [self.destinationDisplayAgent cancel];
}

- (void)enableRequestHandling
{
    self.shouldHandleRequests = YES;
}

#pragma mark - <MPAdDestinationDisplayAgentDelegate>

- (UIViewController *)viewControllerForPresentingModalView
{
    return [self.delegate viewControllerForPresentingModalView];
}

- (void)displayAgentWillPresentModal
{
    [self.delegate adActionWillBegin:self.view];
}

- (void)displayAgentWillLeaveApplication
{
    [self.delegate adActionWillLeaveApplication:self.view];
}

- (void)displayAgentDidDismissModal
{
    [self.delegate adActionDidFinish:self.view];
}

- (MPAdConfiguration *)adConfiguration
{
    return self.configuration;
}

#pragma mark - <MPWebViewDelegate>

- (BOOL)webView:(MPWebView *)webView
shouldStartLoadWithRequest:(NSURLRequest *)request
 navigationType:(WKNavigationType)navigationType
{
    if (!self.shouldHandleRequests) {
        return NO;
    }

    NSURL *URL = [request URL];
    if ([URL mp_isMoPubScheme]) {
        [self performActionForMoPubSpecificURL:URL];
        return NO;
    } else if ([self shouldIntercept:URL navigationType:navigationType]) {

        // Disable intercept without user interaction
        if (!self.userInteractedWithWebView) {
            MPLogInfo(@"Redirect without user interaction detected");
            return NO;
        }

        [self interceptURL:URL];
        return NO;
    } else {
        // don't handle any deep links without user interaction
        return self.userInteractedWithWebView || [URL mp_isSafeForLoadingWithoutUserAction];
    }
}

- (void)webViewDidStartLoad:(MPWebView *)webView
{
    // no op
}

#pragma mark - MoPub-specific URL handlers
- (void)performActionForMoPubSpecificURL:(NSURL *)URL
{
    MPLogDebug(@"MPAdWebView - loading MoPub URL: %@", URL);
    MPMoPubHostCommand command = [URL mp_mopubHostCommand];
    switch (command) {
        case MPMoPubHostCommandClose:
            [self.delegate adDidClose:self.view];
            break;
        case MPMoPubHostCommandFinishLoad:
            [self.delegate adDidFinishLoadingAd:self.view];
            break;
        case MPMoPubHostCommandFailLoad:
            [self.delegate adDidFailToLoadAd:self.view];
            break;
        default:
            MPLogInfo(@"MPAdWebView - unsupported MoPub URL: %@", [URL absoluteString]);
            break;
    }
}

#pragma mark - URL Interception
- (BOOL)shouldIntercept:(NSURL *)URL navigationType:(WKNavigationType)navigationType
{
    if (navigationType == WKNavigationTypeLinkActivated) {
        return YES;
    } else if (navigationType == WKNavigationTypeOther && self.userInteractedWithWebView) {
        return YES;
    } else {
        return NO;
    }
}

- (void)interceptURL:(NSURL *)URL
{
    NSURL *redirectedURL = URL;
    if (self.configuration.clickTrackingURL && !self.didFireClickImpression) {
        self.didFireClickImpression = YES; // fire click impression only once

        NSString *path = [NSString stringWithFormat:@"%@&r=%@",
                          self.configuration.clickTrackingURL.absoluteString,
                          [[URL absoluteString] mp_URLEncodedString]];
        redirectedURL = [NSURL URLWithString:path];
    }

    [self.destinationDisplayAgent displayDestinationForURL:redirectedURL];
}

#pragma mark - Utility

- (void)init3rdPartyViewabilityTrackers
{
    self.viewabilityTracker = [[MPViewabilityTracker alloc] initWithAdView:self.view isVideo:self.configuration.isVastVideoPlayer startTrackingImmediately:[self shouldStartViewabilityDuringInitialization]];
}

- (BOOL)shouldStartViewabilityDuringInitialization
{
    // If viewabile impression tracking experiment is enabled, we defer viewability trackers until
    // ad view is at least x pixels on screen for y seconds, where x and y are configurable values defined in server.
    if (self.adConfiguration.visibleImpressionTrackingEnabled) {
        return NO;
    }

    return ![self isInterstitialAd];
}

- (BOOL)isInterstitialAd
{
    return self.configuration.isFullscreenAd;
}

- (void)initAdAlertManager
{
    self.adAlertManager.adConfiguration = self.configuration;
    self.adAlertManager.adUnitId = [self.delegate adUnitId];
    self.adAlertManager.targetAdView = self.view;
    self.adAlertManager.location = [self.delegate location];
    [self.adAlertManager beginMonitoringAlerts];
}

@end
