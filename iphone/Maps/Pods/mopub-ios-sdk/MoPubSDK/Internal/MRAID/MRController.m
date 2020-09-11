//
//  MRController.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MRController.h"
#import "MRBridge.h"
#import "MRCommand.h"
#import "MRProperty.h"
#import "MPAdDestinationDisplayAgent.h"
#import "MRExpandModalViewController.h"
#import "MPCoreInstanceProvider.h"
#import "MPClosableView.h"
#import "MPGlobal.h"
#import "MPLogging.h"
#import "MPTimer.h"
#import "NSHTTPURLResponse+MPAdditions.h"
#import "NSURL+MPAdditions.h"
#import "MPForceableOrientationProtocol.h"
#import "MPAPIEndPoints.h"
#import "MoPub.h"
#import "MPViewabilityTracker.h"
#import "MPHTTPNetworkSession.h"
#import "MPURLRequest.h"

static const NSTimeInterval kAdPropertyUpdateTimerInterval = 1.0;
static const NSTimeInterval kMRAIDResizeAnimationTimeInterval = 0.3;

static NSString *const kMRAIDCommandExpand = @"expand";
static NSString *const kMRAIDCommandResize = @"resize";

@interface MRController () <MRBridgeDelegate, MPClosableViewDelegate, MPAdDestinationDisplayAgentDelegate>

@property (nonatomic, strong) MRBridge *mraidBridge;
@property (nonatomic, strong) MRBridge *mraidBridgeTwoPart;
@property (nonatomic, strong) MPClosableView *mraidAdView;
@property (nonatomic, strong) MPClosableView *mraidAdViewTwoPart;
@property (nonatomic, strong) UIView *resizeBackgroundView;
@property (nonatomic, strong) MPTimer *adPropertyUpdateTimer;
@property (nonatomic, assign) MRAdViewPlacementType placementType;
@property (nonatomic, strong) MRExpandModalViewController *expandModalViewController;
@property (nonatomic, weak) MPMRAIDInterstitialViewController *interstitialViewController;
@property (nonatomic, assign) CGRect mraidDefaultAdFrame;
@property (nonatomic, assign) CGRect mraidDefaultAdFrameInKeyWindow;
@property (nonatomic, assign) CGSize currentAdSize;
@property (nonatomic, assign) NSUInteger modalViewCount;
@property (nonatomic, assign) BOOL isAppSuspended;
@property (nonatomic, assign) MRAdViewState currentState;
// Track the original super view for when we move the ad view to the key window for a 1-part expand.
@property (nonatomic, weak) UIView *originalSuperview;
@property (nonatomic, assign) BOOL isViewable;
@property (nonatomic, assign) BOOL isAnimatingAdSize;
@property (nonatomic, assign) BOOL isAdLoading;
// Whether or not an interstitial requires precaching.  Does not affect banners.
@property (nonatomic, assign) BOOL adRequiresPrecaching;
@property (nonatomic, assign) BOOL isAdVastVideoPlayer;
@property (nonatomic, assign) BOOL didConfigureOrientationNotificationObservers;

// Points to mraidAdView (one-part expand) or mraidAdViewTwoPart (two-part expand) while expanded.
@property (nonatomic, strong) MPClosableView *expansionContentView;

@property (nonatomic, strong) id<MPAdDestinationDisplayAgent> destinationDisplayAgent;

// Use UIInterfaceOrientationMaskALL to specify no forcing.
@property (nonatomic, assign) UIInterfaceOrientationMask forceOrientationMask;

@property (nonatomic, assign) UIInterfaceOrientation currentInterfaceOrientation;

@property (nonatomic, copy) void (^forceOrientationAfterAnimationBlock)(void);

@property (nonatomic, strong) MPViewabilityTracker *viewabilityTracker;
@property (nonatomic, strong) MPWebView *mraidWebView;

// Networking
@property (nonatomic, strong) NSURLSessionTask *task;

// Previously set values used to determine if an update needs to be sent
@property (nonatomic, assign) CGRect previousCurrentPosition;
@property (nonatomic, assign) CGRect previousDefaultPosition;
@property (nonatomic, assign) CGSize previousScreenSize;
@property (nonatomic, assign) CGSize previousMaxSize;

// Safe area insets
@property (nonatomic, assign) BOOL includeSafeAreaInsetsInCalculations;

// MRAID capability feature flags
@property (nonatomic, assign) BOOL allowCustomClose;

@end

@implementation MRController

- (instancetype)initWithAdViewFrame:(CGRect)adViewFrame
              supportedOrientations:(MPInterstitialOrientationType)orientationType
                    adPlacementType:(MRAdViewPlacementType)placementType
                           delegate:(id<MRControllerDelegate>)delegate
{
    if (self = [super init]) {
        _includeSafeAreaInsetsInCalculations = YES;
        _placementType = placementType;
        _currentState = MRAdViewStateDefault;
        _forceOrientationMask = MPInterstitialOrientationTypeToUIInterfaceOrientationMask(orientationType);
        _isAnimatingAdSize = NO;
        _didConfigureOrientationNotificationObservers = NO;
        _currentAdSize = CGSizeZero;
        _isAppSuspended = NO;

        _mraidDefaultAdFrame = adViewFrame;

        _adPropertyUpdateTimer = [MPTimer timerWithTimeInterval:kAdPropertyUpdateTimerInterval
                                                         target:self
                                                       selector:@selector(updateMRAIDProperties)
                                                        repeats:YES
                                                    runLoopMode:NSRunLoopCommonModes];

        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(viewEnteredBackground)
                                                     name:UIApplicationDidEnterBackgroundNotification
                                                   object:nil];

        //Setting the frame here is irrelevant - we update it whenever an ad resizes to match the
        //application frame.
        _resizeBackgroundView = [[UIView alloc] initWithFrame:adViewFrame];
        _resizeBackgroundView.backgroundColor = [UIColor clearColor];

        _destinationDisplayAgent = [MPAdDestinationDisplayAgent agentWithDelegate:self];
        _delegate = delegate;

        _previousCurrentPosition = CGRectNull;
        _previousDefaultPosition = CGRectNull;
        _previousScreenSize = CGSizeZero;
        _previousMaxSize = CGSizeZero;
        _allowCustomClose = NO;
    }

    return self;
}

- (void)dealloc
{
    [self.viewabilityTracker stopTracking];

    // Transfer delegation to the expand modal view controller in the event the modal is still being presented so it can dismiss itself.
    _expansionContentView.delegate = _expandModalViewController;

    [_adPropertyUpdateTimer invalidate];
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

#pragma mark - Public

- (void)loadAdWithConfiguration:(MPAdConfiguration *)configuration
{
    if (self.isAdLoading) {
        return;
    }

    self.isAdLoading = YES;
    self.adRequiresPrecaching = configuration.precacheRequired;
    self.isAdVastVideoPlayer = configuration.isVastVideoPlayer;

    // MoVideo is always allowed to use custom close since it utilizes JS and MRAID
    // to render the locked countdown experience and then show the close button.
    self.allowCustomClose = configuration.mraidAllowCustomClose || configuration.isMoVideo;

    [self commonSetupBeforeMRAIDBridgeLoadAd];

    // This load is guaranteed to never be called for a two-part expand so we know we need to load the HTML into the default web view.
    NSString *HTML = [configuration adResponseHTMLString];
    [self.mraidBridge loadHTMLString:HTML
                             baseURL:[NSURL URLWithString:[MPAPIEndpoints baseURL]]
     ];
}

- (void)loadVASTCompanionAd:(NSString *)companionAdHTML
{
    if (self.isAdLoading) {
        return;
    }

    self.isAdLoading = YES;
    self.adRequiresPrecaching = NO;
    self.isAdVastVideoPlayer = NO; // VAST companion ad cannot be a VAST video

    [self commonSetupBeforeMRAIDBridgeLoadAd];

    // This load is guaranteed to never be called for a two-part expand so we know we need to load the HTML into the default web view.
    [self.mraidBridge loadHTMLString:companionAdHTML
                             baseURL:[NSURL URLWithString:[MPAPIEndpoints baseURL]]
     ];
}

- (void)loadVASTCompanionAdUrl:(NSURL *)companionAdUrl
{
    if (self.isAdLoading) {
        return;
    }

    self.isAdLoading = YES;
    self.adRequiresPrecaching = NO;
    self.isAdVastVideoPlayer = NO; // VAST companion ad cannot be a VAST video

    [self commonSetupBeforeMRAIDBridgeLoadAd];

    // This load is guaranteed to never be called for a two-part expand so we know we need to load the HTML into the default web view.
    [self.mraidBridge loadHTMLUrl:companionAdUrl];
}

- (void)handleMRAIDInterstitialWillPresentWithViewController:(MPMRAIDInterstitialViewController *)viewController
{
    self.interstitialViewController = viewController;
    [self updateOrientation];
    [self willBeginAnimatingAdSize];
}

- (void)handleMRAIDInterstitialDidPresentWithViewController:(MPMRAIDInterstitialViewController *)viewController
{
    self.interstitialViewController = viewController;
    [self didEndAnimatingAdSize];
    [self updateMRAIDProperties];
    [self updateOrientation];

    // Start viewability tracking here for interstitals. For banners, this is handled by the custom event upon impression.
    if ([self isInterstitialAd]) {
        [self.viewabilityTracker startTracking];
    }
}

- (void)enableRequestHandling
{
    self.mraidBridge.shouldHandleRequests = YES;
    self.mraidBridgeTwoPart.shouldHandleRequests = YES;
    // If orientation has been forced while requests are disabled (during animation), we need to execute that command through the block forceOrientationAfterAnimationBlock() after the presentation completes.
    if (self.forceOrientationAfterAnimationBlock) {
        self.forceOrientationAfterAnimationBlock();
        self.forceOrientationAfterAnimationBlock = nil;
    }
}

- (void)disableRequestHandling
{
    self.mraidBridge.shouldHandleRequests = NO;
    self.mraidBridgeTwoPart.shouldHandleRequests = NO;
    [self.destinationDisplayAgent cancel];
}

- (void)startViewabilityTracking
{
    [self.viewabilityTracker startTracking];
}

- (void)disableClickthroughWebBrowser
{
    self.destinationDisplayAgent = nil;
}

- (void)triggerWebviewDidAppear
{
    [self.mraidWebView stringByEvaluatingJavaScriptFromString:@"webviewDidAppear();"];
}

#pragma mark - Loading Two Part Expand

- (void)loadTwoPartCreativeFromURL:(NSURL *)url
{
    self.isAdLoading = YES;

    MPURLRequest * request = [MPURLRequest requestWithURL:url];

    __weak __typeof__(self) weakSelf = self;
    self.task = [MPHTTPNetworkSession startTaskWithHttpRequest:request responseHandler:^(NSData * _Nonnull data, NSHTTPURLResponse * _Nonnull response) {
        __typeof__(self) strongSelf = weakSelf;

        NSURL *currentRequestUrl = strongSelf.task.currentRequest.URL;
        [strongSelf connectionDidFinishLoadingData:data withResponse:response fromRequestUrl:currentRequestUrl];
    } errorHandler:^(NSError * _Nonnull error) {
        __typeof__(self) strongSelf = weakSelf;
        [strongSelf didFailWithError:error];
    }];
}

- (void)didFailWithError:(NSError *)error
{
    self.isAdLoading = NO;
    // No matter what, show the close button on the expanded view.
    self.expansionContentView.closeButtonType = MPClosableViewCloseButtonTypeTappableWithImage;
    [self.mraidBridge fireErrorEventForAction:kMRAIDCommandExpand withMessage:@"Could not load URL."];
}

- (void)connectionDidFinishLoadingData:(NSData *)data withResponse:(NSHTTPURLResponse *)response fromRequestUrl:(NSURL *)requestUrl
{
    // Extract the response encoding type.
    NSDictionary *headers = [response allHeaderFields];
    NSString *contentType = [headers objectForKey:kMoPubHTTPHeaderContentType];
    NSStringEncoding responseEncoding = [response stringEncodingFromContentType:contentType];

    NSString *str = [[NSString alloc] initWithData:data encoding:responseEncoding];
    [self.mraidBridgeTwoPart loadHTMLString:str baseURL:requestUrl];
}

#pragma mark - Private

- (void)init3rdPartyViewabilityTrackers
{
    self.viewabilityTracker = [[MPViewabilityTracker alloc]
                               initWithWebView:self.mraidWebView
                               isVideo:self.isAdVastVideoPlayer
                               startTrackingImmediately:NO];
    [self.viewabilityTracker registerFriendlyObstructionView:self.mraidAdView.closeButton];
}

- (BOOL)isInterstitialAd
{
    return (self.placementType == MRAdViewPlacementTypeInterstitial);
}

- (MPClosableView *)adViewForBridge:(MRBridge *)bridge
{
    if (bridge == self.mraidBridgeTwoPart) {
        return self.mraidAdViewTwoPart;
    }

    return self.mraidAdView;
}

- (MRBridge *)bridgeForAdView:(MPClosableView *)view
{
    if (view == self.mraidAdViewTwoPart) {
        return self.mraidBridgeTwoPart;
    }

    return self.mraidBridge;
}

- (MPClosableView *)activeView
{
    if (self.currentState == MRAdViewStateExpanded) {
        return self.expansionContentView;
    }

    return self.mraidAdView;
}

- (MRBridge *)bridgeForActiveAdView
{
    MRBridge *bridge = [self bridgeForAdView:[self activeView]];
    return bridge;
}

- (MPWebView *)buildMRAIDWebViewWithFrame:(CGRect)frame
{
    MPWebView *webView = [[MPWebView alloc] initWithFrame:frame];
    webView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    webView.backgroundColor = [UIColor clearColor];
    webView.clipsToBounds = YES;
    webView.opaque = NO;
    [webView mp_setScrollable:NO];

    return webView;
}

/**
 Call this before calling `[self.mraidBridge loadHTMLString]`.
 */
- (void)commonSetupBeforeMRAIDBridgeLoadAd {
    self.mraidWebView = [self buildMRAIDWebViewWithFrame:self.mraidDefaultAdFrame];
    self.mraidWebView.shouldConformToSafeArea = [self isInterstitialAd];

    self.mraidBridge = [[MRBridge alloc] initWithWebView:self.mraidWebView delegate:self];
    self.mraidAdView = [[MPClosableView alloc] initWithFrame:self.mraidDefaultAdFrame
                                                 contentView:self.mraidWebView
                                                    delegate:self];
    if (self.placementType == MRAdViewPlacementTypeInterstitial) {
        self.mraidAdView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    }

    // Initially turn off the close button for default banner MRAID ads while defaulting to turning it on for interstitials.
    if (self.placementType == MRAdViewPlacementTypeInline) {
        self.mraidAdView.closeButtonType = MPClosableViewCloseButtonTypeNone;
    } else if (self.placementType == MRAdViewPlacementTypeInterstitial) {
        self.mraidAdView.closeButtonType = MPClosableViewCloseButtonTypeTappableWithImage;
    }

    [self init3rdPartyViewabilityTrackers];
}

#pragma mark - Orientation Notifications

- (void)orientationDidChange:(NSNotification *)notification
{
    // We listen for device notification changes because at that point our ad's frame is in
    // the correct state; however, MRAID updates should only happen when the interface changes, so the update logic is only executed if the interface orientation has changed.

    //MPInterfaceOrientation is guaranteed to be the new orientation at this point.
    UIInterfaceOrientation newInterfaceOrientation = MPInterfaceOrientation();
    if (newInterfaceOrientation != self.currentInterfaceOrientation) {
        // Update all properties and fire a size change event.
        [self updateMRAIDProperties];

        //According to MRAID Specs, a resized ad should close when there's an orientation change
        //due to unpredictability of the new layout.
        if (self.currentState == MRAdViewStateResized) {
            [self close];
        }

        self.currentInterfaceOrientation = newInterfaceOrientation;
    }
}

#pragma mark - Executing Javascript

- (void)configureMraidEnvironmentToShowAdForBridge:(MRBridge *)bridge
{
    // Set up some initial properties so mraid can operate.
    MPLogDebug(@"Injecting initial JavaScript state.");
    NSArray *startingMraidProperties = @[[MRHostSDKVersionProperty defaultProperty],
                                         [MRPlacementTypeProperty propertyWithType:self.placementType],
                                         [MRSupportsProperty defaultProperty],
                                         [MRStateProperty propertyWithState:self.currentState]
                                         ];

    [bridge fireChangeEventsForProperties:startingMraidProperties];

    [self updateMRAIDProperties];

    [bridge fireReadyEvent];
}

- (void)fireChangeEventToBothBridgesForProperty:(MRProperty *)property
{
    [self.mraidBridge fireChangeEventForProperty:property];
    [self.mraidBridgeTwoPart fireChangeEventForProperty:property];
}

#pragma mark - Resize Helpers

/**
 If the provided frame is not fully within the application safe area, to try to adjust it's origin so
 that the provided frame can fit into the application safe area if possible.

 Note: Only the origin is adjusted. If the size doesn't fit, then the original frame is returned.

 @param frame The frame to adjust.
 @param applicationSafeArea The frame of application safe area.
 @return The adjusted frame.
 */
+ (CGRect)adjustedFrameForFrame:(CGRect)frame toFitIntoApplicationSafeArea:(CGRect)applicationSafeArea
{
    if (CGRectContainsRect(applicationSafeArea, frame)) {
        return frame;
    } else if (CGRectGetWidth(frame) <= CGRectGetWidth(applicationSafeArea)
               && CGRectGetHeight(frame) <= CGRectGetHeight(applicationSafeArea)) {
        // given the size is fitting, we only need to move the frame by changing its origin
        if (CGRectGetMinX(frame) < CGRectGetMinX(applicationSafeArea)) {
            frame.origin.x = CGRectGetMinX(applicationSafeArea);
        } else if (CGRectGetMaxX(applicationSafeArea) < CGRectGetMaxX(frame)) {
            frame.origin.x = CGRectGetMaxX(applicationSafeArea) - CGRectGetWidth(frame);
        }

        if (CGRectGetMinY(frame) < CGRectGetMinY(applicationSafeArea)) {
            frame.origin.y = CGRectGetMinY(applicationSafeArea);
        } else if (CGRectGetMaxY(applicationSafeArea) < CGRectGetMaxY(frame)) {
            frame.origin.y = CGRectGetMaxY(applicationSafeArea) - CGRectGetHeight(frame);
        }
    }

    return frame;
}

/**
 Check whether the provided @c frame is valid for a resized ad.
 @param frame The ad frame to check
 @param applicationSafeArea The safe area of this application
 @param allowOffscreen Per MRAID spec https://www.iab.com/wp-content/uploads/2015/08/IAB_MRAID_v2_FINAL.pdf,
 page 35, @c is for "whether or not it should allow the resized creative to be drawn fully/partially
 offscreen".
 @return @c YES if the provided @c frame is valid for a resized ad, and @c NO otherwise.
 */
+ (BOOL)isValidResizeFrame:(CGRect)frame
     inApplicationSafeArea:(CGRect)applicationSafeArea
            allowOffscreen:(BOOL)allowOffscreen
{
    if (CGRectGetWidth(frame) < kCloseRegionSize.width || CGRectGetHeight(frame) < kCloseRegionSize.height) {
        /*
         Per MRAID spec https://www.iab.com/wp-content/uploads/2015/08/IAB_MRAID_v2_FINAL.pdf, page 34,
         "a resized ad must be at least 50x50 pixels, to ensure there is room on the resized creative
         for the close event region."
         */
        return false;
    } else {
        if (allowOffscreen) {
            return YES; // any frame with a valid size is valid, even off screen
        } else {
            return CGRectContainsRect(applicationSafeArea, frame);
        }
    }
}

/**
 Check whether the frame of Close button is valid.
 @param closeButtonLocation The Close button location.
 @param adFrame The ad frame that contains the Close button.
 @param applicationSafeArea The safe area of this application.
 @return @c YES if the frame of the Close button is valid, and @c NO otherwise.
 */
+ (BOOL)isValidCloseButtonPlacement:(MPClosableViewCloseButtonLocation)closeButtonLocation
                          inAdFrame:(CGRect)adFrame
              inApplicationSafeArea:(CGRect)applicationSafeArea {
    // Need to convert the corrdinate system of the Close button frame from "in the ad" to "in the window".
    CGRect closeButtonFrameInAd = MPClosableViewCustomCloseButtonFrame(adFrame.size, closeButtonLocation);
    CGRect closeButtonFrameInWindow = CGRectOffset(closeButtonFrameInAd, CGRectGetMinX(adFrame), CGRectGetMinY(adFrame));
    return CGRectContainsRect(applicationSafeArea, closeButtonFrameInWindow);
}

- (MPClosableViewCloseButtonLocation)adCloseButtonLocationFromString:(NSString *)closeButtonLocationString
{
    if ([closeButtonLocationString isEqualToString:@"top-left"]) {
        return MPClosableViewCloseButtonLocationTopLeft;
    } else if ([closeButtonLocationString isEqualToString:@"top-center"]) {
        return MPClosableViewCloseButtonLocationTopCenter;
    } else if ([closeButtonLocationString isEqualToString:@"bottom-left"]) {
        return MPClosableViewCloseButtonLocationBottomLeft;
    } else if ([closeButtonLocationString isEqualToString:@"bottom-center"]) {
        return MPClosableViewCloseButtonLocationBottomCenter;
    } else if ([closeButtonLocationString isEqualToString:@"bottom-right"]) {
        return MPClosableViewCloseButtonLocationBottomRight;
    } else if ([closeButtonLocationString isEqualToString:@"center"]) {
        return MPClosableViewCloseButtonLocationCenter;
    } else {
        return MPClosableViewCloseButtonLocationTopRight;
    }
}

- (void)animateViewFromDefaultStateToResizedState:(MPClosableView *)view withFrame:(CGRect)newFrame
{
    [self willBeginAnimatingAdSize];

    [UIView animateWithDuration:kMRAIDResizeAnimationTimeInterval animations:^{
        self.mraidAdView.frame = newFrame;
    } completion:^(BOOL finished) {
        [self changeStateTo:MRAdViewStateResized];
        [self didEndAnimatingAdSize];
    }];
}

#pragma mark - Expand Helpers

- (void)presentExpandModalViewControllerWithView:(MPClosableView *)view animated:(BOOL)animated
{
    [self presentExpandModalViewControllerWithView:view animated:animated completion:nil];
}

- (void)presentExpandModalViewControllerWithView:(MPClosableView *)view animated:(BOOL)animated completion:(void (^)(void))completionBlock
{
    [self willBeginAnimatingAdSize];

    self.expandModalViewController = [[MRExpandModalViewController alloc] initWithOrientationMask:self.forceOrientationMask];
    [self.expandModalViewController.view addSubview:view];
    view.frame = self.expandModalViewController.view.bounds;
    view.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    self.expandModalViewController.modalTransitionStyle = UIModalTransitionStyleCrossDissolve;

    [[self.delegate viewControllerForPresentingModalView] presentViewController:self.expandModalViewController
                                                                       animated:animated
                                                                     completion:^{
                                                                         self.currentInterfaceOrientation = MPInterfaceOrientation();
                                                                         [self didEndAnimatingAdSize];

                                                                         if (completionBlock) {
                                                                             completionBlock();
                                                                         }
                                                                     }];
}

- (void)willBeginAnimatingAdSize
{
    self.isAnimatingAdSize = YES;
    [self disableRequestHandling];
}

- (void)didEndAnimatingAdSize
{
    self.isAnimatingAdSize = NO;
    [self enableRequestHandling];
}

#pragma mark - Close Helpers

- (void)close
{
    switch (self.currentState) {
        case MRAdViewStateDefault:
            [self closeFromDefaultState];
            break;
        case MRAdViewStateExpanded:
            [self closeFromExpandedState];
            break;
        case MRAdViewStateResized:
            [self closeFromResizedState];
            break;
        case MRAdViewStateHidden:
            break;
        default:
            break;
    }
}

- (void)closeFromDefaultState
{
    [self adWillClose];

    self.mraidAdView.hidden = YES;
    [self changeStateTo:MRAdViewStateHidden];

    [self adDidClose];
}

- (void)closeFromExpandedState
{
    self.mraidAdView.closeButtonType = MPClosableViewCloseButtonTypeNone;

    // Immediately re-parent the ad so it will show up as the expand modal goes away rather than after.
    [self.originalSuperview addSubview:self.mraidAdView];
    self.mraidAdView.frame = self.mraidDefaultAdFrame;
    if (self.placementType != MRAdViewPlacementTypeInterstitial) {
        self.mraidAdView.autoresizingMask = UIViewAutoresizingNone;
    }

    // Track isAnimatingAdSize because we have a timer that will update the mraid ad properties. We don't want to examine our views when
    // they're in a transitional state.
    [self willBeginAnimatingAdSize];

    __weak __typeof__(self) weakSelf = self;
    [self.expandModalViewController dismissViewControllerAnimated:YES completion:^{
        __typeof__(self) strongSelf = weakSelf;


        [strongSelf didEndAnimatingAdSize];
        [strongSelf adDidDismissModalView];

        // Get rid of the bridge and view if we are closing from two-part expand.
        if (strongSelf.mraidAdViewTwoPart) {
            strongSelf.mraidAdViewTwoPart = nil;
            strongSelf.mraidBridgeTwoPart = nil;
        }

        strongSelf.expansionContentView = nil;
        strongSelf.expandModalViewController = nil;

        // Waiting this long to change the state results in some awkward animation. The full screen ad will briefly appear in the banner's
        // frame after the modal dismisses. However, this is a much safer time to change the state and results in less side effects.
        [strongSelf changeStateTo:MRAdViewStateDefault];

        // Notify listeners that the expanded ad was collapsed.
        if ([strongSelf.delegate respondsToSelector:@selector(adDidCollapse:)]) {
            [strongSelf.delegate adDidCollapse:strongSelf.mraidAdView];
        }
    }];
}

- (void)closeFromResizedState
{
    self.mraidAdView.closeButtonType = MPClosableViewCloseButtonTypeNone;

    [self willBeginAnimatingAdSize];

    __weak __typeof__(self) weakSelf = self;
    [UIView animateWithDuration:kMRAIDResizeAnimationTimeInterval animations:^{
        __typeof__(self) strongSelf = weakSelf;

        strongSelf.mraidAdView.frame = strongSelf.mraidDefaultAdFrameInKeyWindow;
    } completion:^(BOOL finished) {
        __typeof__(self) strongSelf = weakSelf;

        [strongSelf.resizeBackgroundView removeFromSuperview];
        [strongSelf.originalSuperview addSubview:strongSelf.mraidAdView];
        strongSelf.mraidAdView.frame = strongSelf.mraidDefaultAdFrame;
        [strongSelf changeStateTo:MRAdViewStateDefault];
        [strongSelf didEndAnimatingAdSize];
        [strongSelf adDidDismissModalView];

        // Notify listeners that the expanded ad was collapsed.
        if ([strongSelf.delegate respondsToSelector:@selector(adDidCollapse:)]) {
            [strongSelf.delegate adDidCollapse:strongSelf.mraidAdView];
        }
    }];
}

#pragma mark - <MRBridgeDelegate>

- (BOOL)isLoadingAd
{
    return self.isAdLoading;
}

- (BOOL)hasUserInteractedWithWebViewForBridge:(MRBridge *)bridge
{
    // inline videos seem to delay tap gesture recognition so that we get the click through
    // request in the webview delegate BEFORE we get the gesture recognizer triggered callback. For now
    // excuse all MRAID interstitials from the user interaction requirement. We do the same for expanded ads.
    // We can go ahead and return true for either bridge (default, expanded) if the current state is expanded since
    // the fact we're in expanded means that the first webview has been tapped. Then we just don't require the check
    // for the expanded bridge.
    if (self.placementType == MRAdViewPlacementTypeInterstitial || self.currentState == MRAdViewStateExpanded) {
        return YES;
    }

    MPClosableView *adView = [self adViewForBridge:bridge];
    return adView.wasTapped;
}

- (UIViewController *)viewControllerForPresentingModalView
{
    UIViewController *delegateVC = [self.delegate viewControllerForPresentingModalView];

    // Use the expand modal view controller as the presenting modal if it's being presented.
    if (self.expandModalViewController.presentingViewController != nil) {
        return self.expandModalViewController;
    }

    return delegateVC;
}

- (void)nativeCommandWillPresentModalView
{
    [self adWillPresentModalViewByExpanding:NO];
}

- (void)nativeCommandDidDismissModalView
{
    [self adDidDismissModalView];
}

- (void)bridge:(MRBridge *)bridge didFinishLoadingWebView:(MPWebView *)webView
{
    // Loading an iframe can cause this method to execute and could potentially cause us to initialize the javascript for a two-part expand
    // and fire the ready event more than once. The isAdLoading flags helps us prevent that from happening.
    if (self.isAdLoading) {

        self.isAdLoading = NO;

        if (!self.adRequiresPrecaching) {
            // Only tell the delegate that the ad loaded when the view is the default ad view and not a two-part ad view.
            if (bridge == self.mraidBridge) {
                // We do not start our timer for a banner load yet.  We wait until the ad is in the view hierarchy.
                // We are notified by the view when it is potentially added to the hierarchy in
                // -closableView:didMoveToWindow:.
                [self adDidLoad];
            } else if (bridge == self.mraidBridgeTwoPart) {
                // If the default ad was already viewable, we need to simply tell the two part it is viewable. Otherwise, if the default
                // ad wasn't viewable, we need to update the state across both webviews and the controller.
                if (self.isViewable) {
                    [self.mraidBridgeTwoPart fireChangeEventForProperty:[MRViewableProperty propertyWithViewable:YES]];
                } else {
                    [self updateViewabilityWithBool:YES];
                }

                // We initialize javascript and fire the ready event for the two part ad view once it loads
                // since it'll already be in the view hierarchy.
                [self configureMraidEnvironmentToShowAdForBridge:bridge];
            }
        }
    }
}

- (void)bridge:(MRBridge *)bridge didFailLoadingWebView:(MPWebView *)webView error:(NSError *)error
{
    self.isAdLoading = NO;

    if (bridge == self.mraidBridge) {
        // We need to report that the ad failed to load when the default ad fails to load.
        [self adDidFailToLoad];
    } else if (bridge == self.mraidBridgeTwoPart) {
        // Always show the close button when the two-part expand fails.
        self.expansionContentView.closeButtonType = MPClosableViewCloseButtonTypeTappableWithImage;

        // For two-part expands, we don't want to tell the delegate anything went wrong since the ad did successfully load.
        // We will fire an error to the javascript though.
        [self.mraidBridge fireErrorEventForAction:kMRAIDCommandExpand withMessage:@"Could not load URL."];
    }
}

- (void)bridge:(MRBridge *)bridge performActionForMoPubSpecificURL:(NSURL *)url
{
    MPLogDebug(@"MRController - loading MoPub URL: %@", url);
    MPMoPubHostCommand command = [url mp_mopubHostCommand];
    if (command == MPMoPubHostCommandPrecacheComplete && self.adRequiresPrecaching) {
        [self adDidLoad];
    } else if (command == MPMoPubHostCommandFailLoad) {
        [self adDidFailToLoad];
    } else if (command == MPMoPubHostCommandRewardedVideoEnded) {
        [self rewardedVideoEnded];
    } else {
        MPLogInfo(@"MRController - unsupported MoPub URL: %@", [url absoluteString]);
    }
}

- (void)handleNativeCommandCloseWithBridge:(MRBridge *)bridge
{
    [self close];
}

- (void)bridge:(MRBridge *)bridge handleDisplayForDestinationURL:(NSURL *)URL
{
    if ([self hasUserInteractedWithWebViewForBridge:bridge]) {
        [self.destinationDisplayAgent displayDestinationForURL:URL];

        if ([self.delegate respondsToSelector:@selector(adDidReceiveClickthrough:)]) {
            [self.delegate adDidReceiveClickthrough:URL];
        }
    }
}

- (void)bridge:(MRBridge *)bridge handleNativeCommandUseCustomClose:(BOOL)useCustomClose
{
    // `useCustomClose()` has not been allowed for this ad.
    if (!self.allowCustomClose) {
        MPLogInfo(@"MRAID command `useCustomClose()` is not allowed.");
        return;
    }

    // Calling useCustomClose() for banners won't take effect until expand() is called so we don't need to take
    // any action here as useCustomClose will be given to us when expand is called. Interstitials can have their
    // close buttons changed at any time though.
    if (self.placementType != MRAdViewPlacementTypeInterstitial) {
        return;
    }

    [self configureCloseButtonForView:self.mraidAdView forUseCustomClose:useCustomClose];
}

- (void)configureCloseButtonForView:(MPClosableView *)view forUseCustomClose:(BOOL)useCustomClose
{
    if (useCustomClose) {
        // When using custom close, we must leave a tappable region on the screen and just hide the image
        // unless the ad is a vast video ad. For vast video, we expect that the creative will have a tappable
        // close region.
        if (self.isAdVastVideoPlayer) {
            view.closeButtonType = MPClosableViewCloseButtonTypeNone;
        } else {
            view.closeButtonType = MPClosableViewCloseButtonTypeTappableWithoutImage;
        }
    } else {
        // When not using custom close, show our own image with a tappable region.
        view.closeButtonType = MPClosableViewCloseButtonTypeTappableWithImage;
    }
}

- (void)bridge:(MRBridge *)bridge handleNativeCommandSetOrientationPropertiesWithForceOrientationMask:(UIInterfaceOrientationMask)forceOrientationMask
{
    // If the ad is trying to force an orientation that the app doesn't support, we shouldn't try to force the orientation.
    if (![[UIApplication sharedApplication] mp_supportsOrientationMask:forceOrientationMask]) {
        return;
    }

    BOOL inExpandedState = self.currentState == MRAdViewStateExpanded;

    // If we aren't expanded or showing an interstitial ad, save the force orientation in case the
    // ad is expanded, but do not process further.
    if (!inExpandedState && self.placementType != MRAdViewPlacementTypeInterstitial) {
        self.forceOrientationMask = forceOrientationMask;
        return;
    }

    // If request handling is paused, we want to queue up this method to be called again when they are re-enabled.
    if (!bridge.shouldHandleRequests) {
        __weak __typeof__(self) weakSelf = self;
        self.forceOrientationAfterAnimationBlock = ^void() {
            __typeof__(self) strongSelf = weakSelf;
            [strongSelf bridge:bridge handleNativeCommandSetOrientationPropertiesWithForceOrientationMask:forceOrientationMask];
        };
        return;
    }

    // By this point, we've committed to forcing the orientation so we don't need a forceOrientationAfterAnimationBlock.
    self.forceOrientationAfterAnimationBlock = nil;
    self.forceOrientationMask = forceOrientationMask;

    BOOL inSameOrientation = [[UIApplication sharedApplication] mp_doesOrientation:MPInterfaceOrientation() matchOrientationMask:forceOrientationMask];
    UIViewController <MPForceableOrientationProtocol> *fullScreenAdViewController = inExpandedState ? self.expandModalViewController : self.interstitialViewController;

    // If we're currently in the force orientation, we don't need to do any rotation.  However, we still need to make sure
    // that the view controller knows to use the forced orientation when the user rotates the device.
    if (inSameOrientation) {
        fullScreenAdViewController.supportedOrientationMask = forceOrientationMask;
    } else {
        // Block our timer from updating properties while we force orientation on the view controller.
        [self willBeginAnimatingAdSize];

        UIViewController *presentingViewController = fullScreenAdViewController.presentingViewController;
        __weak __typeof__(self) weakSelf = self;
        [fullScreenAdViewController dismissViewControllerAnimated:NO completion:^{
            __typeof__(self) strongSelf = weakSelf;

            if (inExpandedState) {
                [strongSelf didEndAnimatingAdSize];

                // If expanded, we don't need to change the state of the ad once the modal is present here as the ad is technically
                // always in the expanded state throughout the process of dismissing and presenting.
                [strongSelf presentExpandModalViewControllerWithView:strongSelf.expansionContentView animated:NO completion:^{
                    [strongSelf updateMRAIDProperties];
                }];
            } else {
                fullScreenAdViewController.supportedOrientationMask = forceOrientationMask;
                [presentingViewController presentViewController:fullScreenAdViewController animated:NO completion:^{
                    [strongSelf didEndAnimatingAdSize];
                    strongSelf.currentInterfaceOrientation = MPInterfaceOrientation();
                    [strongSelf updateMRAIDProperties];
                }];
            }
        }];
    }
}

- (void)bridge:(MRBridge *)bridge handleNativeCommandExpandWithURL:(NSURL *)url useCustomClose:(BOOL)useCustomClose
{
    if (self.placementType != MRAdViewPlacementTypeInline) {
        [bridge fireErrorEventForAction:kMRAIDCommandExpand withMessage:@"Cannot expand from interstitial ads."];
        return;
    }

    // Save the state of the default ad view if it's in default state. If it's resized, the controller has already
    // been informed of a modal being presented on resize, and the expand basically takes its place. Additionally,
    // self.mraidDefaultAdFrame has already been set from resize, and the mraidAdView's frame is not the correct default.
    if (self.currentState != MRAdViewStateResized) {
        self.mraidDefaultAdFrame = self.mraidAdView.frame;
        [self adWillPresentModalViewByExpanding:YES];
    } else {
        [self.resizeBackgroundView removeFromSuperview];
    }

    // We change the state after the modal is fully presented which results in an undesirable animation where the banner will briefly appear in the modal which then
    // will instantly change to the full screen ad.  However, it is far safer to update the state like this and has less side effects.
    if (url) {
        // It doesn't matter what frame we use for the two-part expand. We'll overwrite it with a new frame when presenting the modal.
        CGRect twoPartFrame = self.mraidAdView.frame;

        MPWebView *twoPartWebView = [self buildMRAIDWebViewWithFrame:twoPartFrame];
        self.mraidBridgeTwoPart = [[MRBridge alloc] initWithWebView:twoPartWebView delegate:self];
        self.mraidAdViewTwoPart = [[MPClosableView alloc] initWithFrame:twoPartFrame
                                                            contentView:twoPartWebView
                                                               delegate:self];
        self.isAdLoading = YES;

        self.expansionContentView = self.mraidAdViewTwoPart;

        // To avoid race conditions, we start loading the two part creative after the ad has fully expanded.
        [self presentExpandModalViewControllerWithView:self.expansionContentView animated:YES completion:^{
            [self loadTwoPartCreativeFromURL:url];
            [self changeStateTo:MRAdViewStateExpanded];
        }];
    } else {
        self.expansionContentView = self.mraidAdView;
        //If the ad is resized, the original superview has already been set.
        if (self.currentState != MRAdViewStateResized) {
            self.originalSuperview = self.mraidAdView.superview;
        }
        [self presentExpandModalViewControllerWithView:self.expansionContentView animated:YES completion:^{
            [self changeStateTo:MRAdViewStateExpanded];
        }];
    }

    [self configureCloseButtonForView:self.expansionContentView forUseCustomClose:(useCustomClose && self.allowCustomClose)];
}

- (void)bridge:(MRBridge *)bridge handleNativeCommandResizeWithParameters:(NSDictionary *)parameters
{
    NSArray *parameterKeys = [parameters allKeys];
    if (self.currentState == MRAdViewStateExpanded) {
        [bridge fireErrorEventForAction:kMRAIDCommandResize withMessage:@"Cannot resize from and expanded state."];
        return;
    } else if (self.placementType != MRAdViewPlacementTypeInline) {
        [bridge fireErrorEventForAction:kMRAIDCommandResize withMessage:@"Cannot resize from interstitial ads."];
        return;
    } else if (![parameterKeys containsObject:@"width"] || ![parameterKeys containsObject:@"height"] || ![parameterKeys containsObject:@"offsetX"] || ![parameterKeys containsObject:@"offsetY"]) {
        [bridge fireErrorEventForAction:kMRAIDCommandResize withMessage:@"Cannot resize when missing required parameter(s)."];
        return;
    }

    CGFloat width = [[parameters objectForKey:@"width"] floatValue];
    CGFloat height = [[parameters objectForKey:@"height"] floatValue];
    CGFloat offsetX = [[parameters objectForKey:@"offsetX"] floatValue];
    CGFloat offsetY = [[parameters objectForKey:@"offsetY"] floatValue];
    BOOL allowOffscreen = [parameters objectForKey:@"allowOffscreen"] ? [[parameters objectForKey:@"allowOffscreen"] boolValue] : YES;
    NSString *customClosePositionString = [[parameters objectForKey:@"customClosePosition"] length] ? [parameters objectForKey:@"customClosePosition"] : @"top-right";

    //save default frame of the ad view
    if (self.currentState == MRAdViewStateDefault) {
        self.mraidDefaultAdFrameInKeyWindow = [self.mraidAdView.superview convertRect:self.mraidAdView.frame toView:MPKeyWindow().rootViewController.view];
    }

    MPClosableViewCloseButtonLocation closeButtonLocation = [self adCloseButtonLocationFromString:customClosePositionString];
    CGRect applicationSafeArea = MPApplicationFrame(self.includeSafeAreaInsetsInCalculations);
    CGRect newFrame = CGRectMake(CGRectGetMinX(applicationSafeArea) + offsetX,
                                 CGRectGetMinY(applicationSafeArea) + offsetY,
                                 width,
                                 height);
    if (!allowOffscreen) { // if `allowOffscreen` is YES, the frame doesn't need to be adjusted
        newFrame = [[self class] adjustedFrameForFrame:newFrame toFitIntoApplicationSafeArea:applicationSafeArea];
    }

    if (![[self class] isValidResizeFrame:newFrame
                    inApplicationSafeArea:applicationSafeArea
                           allowOffscreen:allowOffscreen]) {
        [self.mraidBridge fireErrorEventForAction:kMRAIDCommandResize withMessage:@"Could not display desired frame in compliance with MRAID 2.0 specifications."];
    } else if (![[self class] isValidCloseButtonPlacement:closeButtonLocation
                                                inAdFrame:newFrame
                                    inApplicationSafeArea:applicationSafeArea]) {
        [self.mraidBridge fireErrorEventForAction:kMRAIDCommandResize withMessage:@"Custom close event region locates in invalid area."];
    } else {
        // Update the close button
        self.mraidAdView.closeButtonType = MPClosableViewCloseButtonTypeTappableWithoutImage;
        self.mraidAdView.closeButtonLocation = closeButtonLocation;

        // If current state is default, save our current frame as the default frame, set originalSuperview, setup resizeBackgroundView,
        // move mraidAdView to rootViewController's view, and call adWillPresentModalView
        if (self.currentState == MRAdViewStateDefault) {
            self.mraidDefaultAdFrame = self.mraidAdView.frame;
            self.originalSuperview = self.mraidAdView.superview;

            self.mraidAdView.frame = self.mraidDefaultAdFrameInKeyWindow;
            self.resizeBackgroundView.frame = MPApplicationFrame(self.includeSafeAreaInsetsInCalculations);

            [MPKeyWindow().rootViewController.view addSubview:self.resizeBackgroundView];
            [MPKeyWindow().rootViewController.view addSubview:self.mraidAdView];

            [self adWillPresentModalViewByExpanding:YES];
        }

        [self animateViewFromDefaultStateToResizedState:self.mraidAdView withFrame:newFrame];
    }
}

#pragma mark - <MPClosableViewDelegate>

- (void)closeButtonPressed:(MPClosableView *)view
{
    [self close];
}

- (void)closableView:(MPClosableView *)closableView didMoveToWindow:(UIWindow *)window
{
    // Fire the ready event and initialize properties if the view has a window.
    MRBridge *bridge = [self bridgeForAdView:closableView];

    if (!self.didConfigureOrientationNotificationObservers && bridge == self.mraidBridge) {
        // The window may be nil if it was removed from a window or added to a view that isn't attached to a window so make sure it actually has a window.
        if (window != nil) {
            // Just in case this code is executed twice, ensures that self is only added as
            // an observer once.
            [[NSNotificationCenter defaultCenter] removeObserver:self name:UIDeviceOrientationDidChangeNotification object:nil];

            //Keep track of the orientation before we start observing changes.
            self.currentInterfaceOrientation = MPInterfaceOrientation();

            // Placing orientation notification observing here ensures that the controller only
            // observes changes after it's been added to the view hierarchy. Subscribing to
            // orientation changes so we can notify the javascript about the new screen size.
            [[NSNotificationCenter defaultCenter] addObserver:self
                                                     selector:@selector(orientationDidChange:)
                                                         name:UIDeviceOrientationDidChangeNotification
                                                       object:nil];

            [self.adPropertyUpdateTimer scheduleNow];
            self.didConfigureOrientationNotificationObservers = YES;
        }
    }
}

#pragma mark - <MPAdDestinationDisplayAgentDelegate>

- (void)displayAgentWillPresentModal
{
    [self adWillPresentModalViewByExpanding:NO];
}

- (void)displayAgentDidDismissModal
{
    [self adDidDismissModalView];
}

- (void)displayAgentWillLeaveApplication
{
    // Do nothing.
}

#pragma mark - Property Updating

- (void)updateMRAIDProperties
{
    // Don't need to update mraid properties while animating as they'll be set correctly when the animations start/finish and it
    // requires a bit of extra state logic to handle. We also don't want to check if the ad is visible during animation because
    // the view is transitioning to a parent view that may or may not be on screen at any given time.
    if (!self.isAnimatingAdSize) {
        [self updateCurrentPosition];
        [self updateDefaultPosition];
        [self updateScreenSize];
        [self updateMaxSize];
        [self updateEventSizeChange];

        // Updating the Viewable state should be last because the creative may have a
        // viewable event handler that relies upon the current position and sizes to be
        // properly set.
        [self checkViewability];
    }
}

- (CGRect)activeAdFrameInScreenSpace
{
    CGRect visibleFrame = CGRectZero;

    // Full screen ads, including inline ads that are in an expanded state.
    // The active area should be the full application frame residing in the
    // safe area.
    BOOL isExpandedInLineAd = (self.placementType == MRAdViewPlacementTypeInline &&
                               self.currentState == MRAdViewStateExpanded);
    if (self.placementType == MRAdViewPlacementTypeInterstitial || isExpandedInLineAd) {
        visibleFrame = MPApplicationFrame(self.includeSafeAreaInsetsInCalculations);
    }
    // Inline ads that are not in an expanded state.
    else if (self.placementType == MRAdViewPlacementTypeInline) {
        UIWindow *keyWindow = MPKeyWindow();
        visibleFrame = [self.mraidAdView.superview convertRect:self.mraidAdView.frame toView:keyWindow.rootViewController.view];
    }

    return visibleFrame;
}

- (CGRect)defaultAdFrameInScreenSpace
{
    CGRect defaultFrame = CGRectZero;

    if (self.placementType == MRAdViewPlacementTypeInline) {
        UIWindow *keyWindow = MPKeyWindow();
        if (self.expansionContentView == self.mraidAdViewTwoPart) {
            defaultFrame = [self.mraidAdView.superview convertRect:self.mraidAdView.frame toView:keyWindow.rootViewController.view];
        } else {
            defaultFrame = [self.originalSuperview convertRect:self.mraidDefaultAdFrame toView:keyWindow.rootViewController.view];
        }
    } else if (self.placementType == MRAdViewPlacementTypeInterstitial) {
        defaultFrame = self.mraidAdView.frame;
    }

    return defaultFrame;
}

- (void)updateCurrentPosition
{
    CGRect frame = [self activeAdFrameInScreenSpace];

    @synchronized (self) {
        // No need to update since nothing has changed.
        if (CGRectEqualToRect(frame, self.previousCurrentPosition)) {
            return;
        }

        // Update previous value
        self.previousCurrentPosition = frame;

        // Only fire to the active ad view.
        MRBridge *activeBridge = [self bridgeForActiveAdView];
        [activeBridge fireSetCurrentPositionWithPositionRect:frame];

        MPLogDebug(@"Current Position: %@", NSStringFromCGRect(frame));
    }
}

- (void)updateDefaultPosition
{
    CGRect defaultFrame = [self defaultAdFrameInScreenSpace];

    @synchronized (self) {
        // No need to update since nothing has changed.
        if (CGRectEqualToRect(defaultFrame, self.previousDefaultPosition)) {
            return;
        }

        // Update previous value
        self.previousDefaultPosition = defaultFrame;

        // Not necessary to fire to both ad views, but it's better that the two-part expand knows the default position than not.
        [self.mraidBridge fireSetDefaultPositionWithPositionRect:defaultFrame];
        [self.mraidBridgeTwoPart fireSetDefaultPositionWithPositionRect:defaultFrame];

        MPLogDebug(@"Default Position: %@", NSStringFromCGRect(defaultFrame));
    }
}

- (void)updateScreenSize
{
    // Fire an event for screen size changing. This includes the area of the status bar in its calculation.
    CGSize screenSize = MPScreenBounds().size;

    @synchronized (self) {
        // No need to update since nothing has changed.
        if (CGSizeEqualToSize(screenSize, self.previousScreenSize)) {
            return;
        }

        // Update previous value
        self.previousScreenSize = screenSize;

        // Fire to both ad views as it pertains to both views.
        [self.mraidBridge fireSetScreenSize:screenSize];
        [self.mraidBridgeTwoPart fireSetScreenSize:screenSize];

        MPLogDebug(@"Screen Size: %@", NSStringFromCGSize(screenSize));
    }
}

- (void)updateMaxSize
{
    // Similar to updateScreenSize except this doesn't include the area of the status bar in its calculation.
    CGSize maxSize = MPApplicationFrame(self.includeSafeAreaInsetsInCalculations).size;

    @synchronized (self) {
        // No need to update since nothing has changed.
        if (CGSizeEqualToSize(maxSize, self.previousMaxSize)) {
            return;
        }

        // Update previous value
        self.previousMaxSize = maxSize;

        // Fire to both ad views as it pertains to both views.
        [self.mraidBridge fireSetMaxSize:maxSize];
        [self.mraidBridgeTwoPart fireSetMaxSize:maxSize];

        MPLogDebug(@"Max Size: %@", NSStringFromCGSize(maxSize));
    }
}

- (void)updateOrientation
{
    self.expandModalViewController.supportedOrientationMask = self.forceOrientationMask;
    self.interstitialViewController.supportedOrientationMask = self.forceOrientationMask;

    MPLogDebug(@"Orientation: %ud", (unsigned int)self.forceOrientationMask);
}

#pragma mark - MRAID events

- (void)updateEventSizeChange
{
    CGSize adSize = [self activeAdFrameInScreenSpace].size;

    // Firing the size change event will broadcast the event to the ad. The ad may subscribe to this event and
    // perform some action when it receives the event. As a result, we don't want to have the ad do any work
    // when the size hasn't changed. So we make sure we don't fire the size change event unless the size has
    // actually changed. We don't place similar guards around updating properties that don't broadcast events
    // since the ad won't be notified when we update the properties. Thus, the ad can't do any unnecessary work
    // when we update other properties.
    if (!CGSizeEqualToSize(adSize, self.currentAdSize)) {
        MRBridge *activeBridge = [self bridgeForActiveAdView];
        self.currentAdSize = adSize;

        MPLogDebug(@"Ad Size (Size Event): %@", NSStringFromCGSize(self.currentAdSize));
        [activeBridge fireSizeChangeEvent:adSize];
    }
}

- (void)changeStateTo:(MRAdViewState)state
{
    self.currentState = state;

    // Update the mraid properties so they're ready before the state change happens.
    [self updateMRAIDProperties];
    [self fireChangeEventToBothBridgesForProperty:[MRStateProperty propertyWithState:self.currentState]];
}

#pragma mark - Viewability Helpers

- (void)checkViewability
{
    BOOL viewable = MPViewIsVisible([self activeView]) &&
        ([[UIApplication sharedApplication] applicationState] == UIApplicationStateActive);
    [self updateViewabilityWithBool:viewable];
}

- (void)viewEnteredBackground
{
    [self updateViewabilityWithBool:NO];
}

- (void)updateViewabilityWithBool:(BOOL)currentViewability
{
    if (self.isViewable != currentViewability)
    {
        MPLogDebug(@"Viewable changed to: %@", currentViewability ? @"YES" : @"NO");
        self.isViewable = currentViewability;

        // Both views in two-part expand need to report if they're viewable or not depending on the active one.
        [self fireChangeEventToBothBridgesForProperty:[MRViewableProperty propertyWithViewable:self.isViewable]];
    }
}

#pragma mark - Delegation Wrappers

- (void)adDidLoad
{
    // Configure environment and fire ready event when ad is finished loading.
    [self configureMraidEnvironmentToShowAdForBridge:self.mraidBridge];

    if ([self.delegate respondsToSelector:@selector(adDidLoad:)]) {
        [self.delegate adDidLoad:self.mraidAdView];
    }
}

- (void)adDidFailToLoad
{
    if ([self.delegate respondsToSelector:@selector(adDidFailToLoad:)]) {
        [self.delegate adDidFailToLoad:self.mraidAdView];
    }
}

- (void)adWillClose
{
    if ([self.delegate respondsToSelector:@selector(adWillClose:)]) {
        [self.delegate adWillClose:self.mraidAdView];
    }
}

- (void)adDidClose
{
    if ([self.delegate respondsToSelector:@selector(adDidClose:)]) {
        [self.delegate adDidClose:self.mraidAdView];
    }
}

- (void)rewardedVideoEnded
{
    if ([self.delegate respondsToSelector:@selector(rewardedVideoEnded)]) {
        [self.delegate rewardedVideoEnded];
    }
}

- (void)adWillPresentModalViewByExpanding:(BOOL)wasExpended
{
    self.modalViewCount++;
    if (self.modalViewCount >= 1 && !wasExpended) {
        [self appShouldSuspend];
    }

    // Notify listeners that the ad is expanding or resizing to present
    // a modal view.
    if (wasExpended && [self.delegate respondsToSelector:@selector(adWillExpand:)]) {
        [self.delegate adWillExpand:self.mraidAdView];
    }
}

- (void)adDidDismissModalView
{
    self.modalViewCount--;
    if (self.modalViewCount == 0) {
        [self appShouldResume];
    }
}

- (void)appShouldSuspend
{
    // App is already suspended; do nothing.
    if (self.isAppSuspended) {
        return;
    }

    self.isAppSuspended = YES;
    if ([self.delegate respondsToSelector:@selector(appShouldSuspendForAd:)]) {
        [self.delegate appShouldSuspendForAd:self.mraidAdView];
    }
}

- (void)appShouldResume
{
    // App is not suspended; do nothing.
    if (!self.isAppSuspended) {
        return;
    }

    self.isAppSuspended = NO;
    if ([self.delegate respondsToSelector:@selector(appShouldResumeFromAd:)]) {
        [self.delegate appShouldResumeFromAd:self.mraidAdView];
    }
}

@end
