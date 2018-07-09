//
//  MPAdWebViewAgent.m
//  MoPub
//
//  Copyright (c) 2013 MoPub. All rights reserved.
//

#import "MPAdWebViewAgent.h"
#import "MPAdConfiguration.h"
#import "MPGlobal.h"
#import "MPLogging.h"
#import "MPAdDestinationDisplayAgent.h"
#import "NSURL+MPAdditions.h"
#import "UIWebView+MPAdditions.h"
#import "MPWebView.h"
#import "MPInstanceProvider.h"
#import "MPCoreInstanceProvider.h"
#import "MPUserInteractionGestureRecognizer.h"
#import "NSJSONSerialization+MPAdditions.h"
#import "NSURL+MPAdditions.h"
#import "MPInternalUtils.h"
#import "MPAPIEndPoints.h"
#import "MoPub.h"
#import "MPViewabilityTracker.h"
#import "NSString+MPAdditions.h"

#ifndef NSFoundationVersionNumber_iOS_6_1
#define NSFoundationVersionNumber_iOS_6_1 993.00
#endif

#define MPOffscreenWebViewNeedsRenderingWorkaround() (floor(NSFoundationVersionNumber) > NSFoundationVersionNumber_iOS_6_1)

@interface MPAdWebViewAgent () <UIGestureRecognizerDelegate>

@property (nonatomic, strong) MPAdConfiguration *configuration;
@property (nonatomic, strong) MPAdDestinationDisplayAgent *destinationDisplayAgent;
@property (nonatomic, assign) BOOL shouldHandleRequests;
@property (nonatomic, strong) id<MPAdAlertManagerProtocol> adAlertManager;
@property (nonatomic, assign) BOOL userInteractedWithWebView;
@property (nonatomic, strong) MPUserInteractionGestureRecognizer *userInteractionRecognizer;
@property (nonatomic, assign) CGRect frame;
@property (nonatomic, strong, readwrite) MPViewabilityTracker *viewabilityTracker;

- (void)performActionForMoPubSpecificURL:(NSURL *)URL;
- (BOOL)shouldIntercept:(NSURL *)URL navigationType:(UIWebViewNavigationType)navigationType;
- (void)interceptURL:(NSURL *)URL;

@end

@implementation MPAdWebViewAgent

@synthesize configuration = _configuration;
@synthesize delegate = _delegate;
@synthesize destinationDisplayAgent = _destinationDisplayAgent;
@synthesize shouldHandleRequests = _shouldHandleRequests;
@synthesize view = _view;
@synthesize adAlertManager = _adAlertManager;
@synthesize userInteractedWithWebView = _userInteractedWithWebView;
@synthesize userInteractionRecognizer = _userInteractionRecognizer;

- (id)initWithAdWebViewFrame:(CGRect)frame delegate:(id<MPAdWebViewAgentDelegate>)delegate;
{
    self = [super init];
    if (self) {
        _frame = frame;

        self.destinationDisplayAgent = [MPAdDestinationDisplayAgent agentWithDelegate:self];
        self.delegate = delegate;
        self.shouldHandleRequests = YES;
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

- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer *)otherGestureRecognizer;
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
    if (configuration.adType != MPAdTypeInterstitial) {
        if ([configuration hasPreferredSize]) {
            CGRect frame = self.view.frame;
            frame.size.width = configuration.preferredSize.width;
            frame.size.height = configuration.preferredSize.height;
            self.view.frame = frame;
        }
    }

    [self.view mp_setScrollable:configuration.scrollable];
    [self.view disableJavaScriptDialogs];

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

- (BOOL)webView:(MPWebView *)webView shouldStartLoadWithRequest:(NSURLRequest *)request
 navigationType:(UIWebViewNavigationType)navigationType
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
    [self.view disableJavaScriptDialogs];
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
            MPLogWarn(@"MPAdWebView - unsupported MoPub URL: %@", [URL absoluteString]);
            break;
    }
}

#pragma mark - URL Interception
- (BOOL)shouldIntercept:(NSURL *)URL navigationType:(UIWebViewNavigationType)navigationType
{
    if ([URL mp_hasTelephoneScheme] || [URL mp_hasTelephonePromptScheme]) {
        return YES;
    } else if (navigationType == UIWebViewNavigationTypeLinkClicked) {
        return YES;
    } else if (navigationType == UIWebViewNavigationTypeOther && self.userInteractedWithWebView) {
        return YES;
    } else {
        return NO;
    }
}

- (void)interceptURL:(NSURL *)URL
{
    NSURL *redirectedURL = URL;
    if (self.configuration.clickTrackingURL) {
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
    return (self.configuration.adType == MPAdTypeInterstitial);
}

- (void)initAdAlertManager
{
    self.adAlertManager.adConfiguration = self.configuration;
    self.adAlertManager.adUnitId = [self.delegate adUnitId];
    self.adAlertManager.targetAdView = self.view;
    self.adAlertManager.location = [self.delegate location];
    [self.adAlertManager beginMonitoringAlerts];
}

- (void)rotateToOrientation:(UIInterfaceOrientation)orientation
{
    [self forceRedraw];
}

- (void)forceRedraw
{
    UIInterfaceOrientation orientation = [[UIApplication sharedApplication] statusBarOrientation];
    int angle = -1;
    switch (orientation) {
        case UIInterfaceOrientationPortrait: angle = 0; break;
        case UIInterfaceOrientationLandscapeLeft: angle = 90; break;
        case UIInterfaceOrientationLandscapeRight: angle = -90; break;
        case UIInterfaceOrientationPortraitUpsideDown: angle = 180; break;
        default: break;
    }

    if (angle == -1) return;

    // UIWebView doesn't seem to fire the 'orientationchange' event upon rotation, so we do it here.
    NSString *orientationEventScript = [NSString stringWithFormat:
                                        @"window.__defineGetter__('orientation',function(){return %d;});"
                                        @"(function(){ var evt = document.createEvent('Events');"
                                        @"evt.initEvent('orientationchange',true,true);window.dispatchEvent(evt);})();",
                                        angle];
    [self.view stringByEvaluatingJavaScriptFromString:orientationEventScript];

    // XXX: In iOS 7, off-screen UIWebViews will fail to render certain image creatives.
    // Specifically, creatives that only contain an <img> tag whose src attribute uses a 302
    // redirect will not be rendered at all. One workaround is to temporarily change the web view's
    // internal contentInset property; this seems to force the web view to re-draw.
    if (MPOffscreenWebViewNeedsRenderingWorkaround()) {
        if ([self.view respondsToSelector:@selector(scrollView)]) {
            UIScrollView *scrollView = self.view.scrollView;
            UIEdgeInsets originalInsets = scrollView.contentInset;
            UIEdgeInsets newInsets = UIEdgeInsetsMake(originalInsets.top + 1,
                                                      originalInsets.left,
                                                      originalInsets.bottom,
                                                      originalInsets.right);
            scrollView.contentInset = newInsets;
            scrollView.contentInset = originalInsets;
        }
    }
}

@end
