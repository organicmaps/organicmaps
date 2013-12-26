//
//  MRAdView.m
//  MoPub
//
//  Created by Andrew He on 12/20/11.
//  Copyright (c) 2011 MoPub, Inc. All rights reserved.
//

#import "MRAdView.h"
#import "UIWebView+MPAdditions.h"
#import "MPGlobal.h"
#import "MPLogging.h"
#import "MRAdViewDisplayController.h"
#import "MRCommand.h"
#import "MRProperty.h"
#import "MPInstanceProvider.h"
#import "MRCalendarManager.h"
#import "MRJavaScriptEventEmitter.h"
#import "UIViewController+MPAdditions.h"
#import "MRBundleManager.h"

static NSString *const kExpandableCloseButtonImageName = @"MPCloseButtonX.png";
static NSString *const kMraidURLScheme = @"mraid";
static NSString *const kMoPubURLScheme = @"mopub";
static NSString *const kMoPubPrecacheCompleteHost = @"precacheComplete";

@interface MRAdView () <UIGestureRecognizerDelegate>

@property (nonatomic, retain) NSMutableData *data;
@property (nonatomic, retain) MPAdDestinationDisplayAgent *destinationDisplayAgent;
@property (nonatomic, retain) MRCalendarManager *calendarManager;
@property (nonatomic, retain) MRPictureManager *pictureManager;
@property (nonatomic, retain) MRVideoPlayerManager *videoPlayerManager;
@property (nonatomic, retain) MRJavaScriptEventEmitter *jsEventEmitter;
@property (nonatomic, retain) id<MPAdAlertManagerProtocol> adAlertManager;
@property (nonatomic, assign) BOOL userTappedWebView;
@property (nonatomic, retain) UITapGestureRecognizer *tapRecognizer;

- (void)loadRequest:(NSURLRequest *)request;
- (void)loadHTMLString:(NSString *)string baseURL:(NSURL *)baseURL;

- (NSMutableString *)HTMLWithJavaScriptBridge:(NSString *)HTML;
- (BOOL)HTMLStringIsMRAIDFragment:(NSString *)string;
- (NSMutableString *)fullHTMLFromMRAIDFragment:(NSString *)fragment;
- (NSString *)MRAIDScriptPath;

- (void)layoutCloseButton;
- (void)initializeJavascriptState;

// Delegate callback methods wrapped with -respondsToSelector: checks.
- (void)adDidLoad;
- (void)adDidFailToLoad;
- (void)adWillClose;
- (void)adDidClose;
- (void)adDidRequestCustomCloseEnabled:(BOOL)enabled;
- (void)adWillExpandToFrame:(CGRect)frame;
- (void)adDidExpandToFrame:(CGRect)frame;
- (void)adWillPresentModalView;
- (void)adDidDismissModalView;
- (void)appShouldSuspend;
- (void)appShouldResume;
- (void)adViewableDidChange:(BOOL)viewable;

@end

@implementation MRAdView

@synthesize delegate = _delegate;
@synthesize usesCustomCloseButton = _usesCustomCloseButton;
@synthesize expanded = _expanded;
@synthesize data = _data;
@synthesize displayController = _displayController;
@synthesize destinationDisplayAgent = _destinationDisplayAgent;
@synthesize calendarManager = _calendarManager;
@synthesize pictureManager = _pictureManager;
@synthesize videoPlayerManager = _videoPlayerManager;
@synthesize jsEventEmitter = _jsEventEmitter;
@synthesize adAlertManager = _adAlertManager;
@synthesize adType = _adType;

- (id)initWithFrame:(CGRect)frame allowsExpansion:(BOOL)expansion
   closeButtonStyle:(MRAdViewCloseButtonStyle)style placementType:(MRAdViewPlacementType)type
{
    self = [super initWithFrame:frame];
    if (self) {
        self.backgroundColor = [UIColor clearColor];
        self.opaque = NO;

        _webView = [[[MPInstanceProvider sharedProvider] buildUIWebViewWithFrame:frame] retain];
        _webView.autoresizingMask = UIViewAutoresizingFlexibleWidth |
                UIViewAutoresizingFlexibleHeight;
        _webView.backgroundColor = [UIColor clearColor];
        _webView.clipsToBounds = YES;
        _webView.delegate = self;
        _webView.opaque = NO;
        [_webView mp_setScrollable:NO];

        if ([_webView respondsToSelector:@selector(setAllowsInlineMediaPlayback:)]) {
            [_webView setAllowsInlineMediaPlayback:YES];
        }

        if ([_webView respondsToSelector:@selector(setMediaPlaybackRequiresUserAction:)]) {
            [_webView setMediaPlaybackRequiresUserAction:NO];
        }

        [self addSubview:_webView];

        _closeButton = [[UIButton buttonWithType:UIButtonTypeCustom] retain];
        _closeButton.frame = CGRectMake(0, 0, 50, 50);
        UIImage *image = [UIImage imageNamed:kExpandableCloseButtonImageName];
        [_closeButton setImage:image forState:UIControlStateNormal];

        _allowsExpansion = expansion;
        _closeButtonStyle = style;
        _placementType = type;

        _displayController = [[MRAdViewDisplayController alloc] initWithAdView:self
                                                               allowsExpansion:expansion
                                                              closeButtonStyle:style
                                                               jsEventEmitter:[[MPInstanceProvider sharedProvider] buildMRJavaScriptEventEmitterWithWebView:_webView]];

        [_closeButton addTarget:_displayController action:@selector(closeButtonPressed) forControlEvents:UIControlEventTouchUpInside];

        _destinationDisplayAgent = [[[MPInstanceProvider sharedProvider]
                                    buildMPAdDestinationDisplayAgentWithDelegate:self] retain];
        _calendarManager = [[[MPInstanceProvider sharedProvider]
                             buildMRCalendarManagerWithDelegate:self] retain];
        _pictureManager = [[[MPInstanceProvider sharedProvider]
                             buildMRPictureManagerWithDelegate:self] retain];
        _videoPlayerManager = [[[MPInstanceProvider sharedProvider]
                                buildMRVideoPlayerManagerWithDelegate:self] retain];
        _jsEventEmitter = [[[MPInstanceProvider sharedProvider]
                             buildMRJavaScriptEventEmitterWithWebView:_webView] retain];

        self.adAlertManager = [[MPInstanceProvider sharedProvider] buildMPAdAlertManagerWithDelegate:self];

        self.adType = MRAdViewAdTypeDefault;

        self.tapRecognizer = [[[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(handleTap:)] autorelease];
        [self addGestureRecognizer:self.tapRecognizer];
        self.tapRecognizer.delegate = self;

        // XXX jren: inline videos seem to delay tap gesture recognition so that we get the click through
        // request in the webview delegate BEFORE we get the gesture recognizer triggered callback. For now
        // excuse all MRAID interstitials from the user interaction requirement.
        if (_placementType == MRAdViewPlacementTypeInterstitial) {
            self.userTappedWebView = YES;
        }
    }
    return self;
}

- (void)dealloc
{
    _webView.delegate = nil;
    [_webView release];
    [_closeButton release];
    [_data release];
    [_displayController release];
    [_destinationDisplayAgent setDelegate:nil];
    [_destinationDisplayAgent release];
    [_calendarManager setDelegate:nil];
    [_calendarManager release];
    [_pictureManager setDelegate:nil];
    [_pictureManager release];
    [_videoPlayerManager setDelegate:nil];
    [_videoPlayerManager release];
    [_jsEventEmitter release];
    self.adAlertManager.targetAdView = nil;
    self.adAlertManager.delegate = nil;
    self.adAlertManager = nil;
    self.tapRecognizer.delegate = nil;
    [self.tapRecognizer removeTarget:self action:nil];
    self.tapRecognizer = nil;
    [super dealloc];
}

- (void)handleTap:(UITapGestureRecognizer *)sender
{
    if(sender.state == UIGestureRecognizerStateEnded)
    {
        self.userTappedWebView = YES;
    }
}

#pragma mark - <UIGestureRecognizerDelegate>

- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer *)otherGestureRecognizer;
{
    return YES;
}

- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer shouldReceiveTouch:(UITouch *)touch
{
    if([touch.view isKindOfClass:[UIButton class]])
    {
        // we touched a button
        return NO; // ignore the touch
    }
    return YES; // handle the touch
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

- (void)setDelegate:(id<MRAdViewDelegate>)delegate
{
    [_closeButton removeTarget:delegate
                        action:NULL
              forControlEvents:UIControlEventTouchUpInside];

    _delegate = delegate;

    [_closeButton addTarget:_delegate
                     action:@selector(closeButtonPressed)
           forControlEvents:UIControlEventTouchUpInside];
}

- (void)setExpanded:(BOOL)expanded
{
    _expanded = expanded;
    [self layoutCloseButton];
}

- (void)setUsesCustomCloseButton:(BOOL)shouldUseCustomCloseButton
{
    _usesCustomCloseButton = shouldUseCustomCloseButton;
    [self layoutCloseButton];
}

- (BOOL)isViewable
{
    return MPViewIsVisible(self);
}

- (void)loadCreativeFromURL:(NSURL *)url
{
    [_displayController revertViewToDefaultState];
    _isLoading = YES;
    [self loadRequest:[NSURLRequest requestWithURL:url]];
}

- (void)loadCreativeWithHTMLString:(NSString *)html baseURL:(NSURL *)url
{
    [_displayController revertViewToDefaultState];
    _isLoading = YES;
    [self loadHTMLString:html baseURL:url];
}

- (void)rotateToOrientation:(UIInterfaceOrientation)newOrientation
{
    [_displayController rotateToOrientation:newOrientation];
}

- (NSString *)placementType
{
    switch (_placementType) {
        case MRAdViewPlacementTypeInline:
            return @"inline";
        case MRAdViewPlacementTypeInterstitial:
            return @"interstitial";
        default:
            return @"unknown";
    }
}

- (BOOL)safeHandleDisplayDestinationForURL:(NSURL *)URL
{
    BOOL handled = NO;

    if (self.userTappedWebView) {
        handled = YES;
        [self.destinationDisplayAgent displayDestinationForURL:URL];
    }

    return handled;
}

- (void)handleMRAIDOpenCallForURL:(NSURL *)URL
{
    [self safeHandleDisplayDestinationForURL:URL];
}

#pragma mark - Private

- (void)initAdAlertManager
{
    self.adAlertManager.adConfiguration = [self.delegate adConfiguration];
    self.adAlertManager.adUnitId = [self.delegate adUnitId];
    self.adAlertManager.targetAdView = self;
    self.adAlertManager.location = [self.delegate location];
    [self.adAlertManager beginMonitoringAlerts];
}

- (void)loadRequest:(NSURLRequest *)request
{
    [self initAdAlertManager];

    NSURLConnection *connection = [NSURLConnection connectionWithRequest:request delegate:self];
    if (connection) {
        self.data = [NSMutableData data];
    }
}

- (void)loadHTMLString:(NSString *)string baseURL:(NSURL *)baseURL
{
    [self initAdAlertManager];

    // Bail out if we can't locate mraid.js.
    if (![self MRAIDScriptPath]) {
        [self adDidFailToLoad];
        return;
    }

    NSString *HTML = [self HTMLWithJavaScriptBridge:string];
    if (HTML) {
        [_webView disableJavaScriptDialogs];
        [_webView loadHTMLString:HTML baseURL:baseURL];
    }
}

- (NSMutableString *)HTMLWithJavaScriptBridge:(NSString *)HTML
{
    NSMutableString *resultHTML = [[HTML mutableCopy] autorelease];

    if ([self HTMLStringIsMRAIDFragment:HTML]) {
        MPLogDebug(@"Fragment detected: converting to full payload.");
        resultHTML = [self fullHTMLFromMRAIDFragment:resultHTML];
    }

    NSURL *MRAIDScriptURL = [NSURL fileURLWithPath:[self MRAIDScriptPath]];

    NSRange headTagRange = [resultHTML rangeOfString:@"<head>"];
    NSString *MRAIDScriptTag = [NSString stringWithFormat:@"<script src='%@'></script>",
                                [MRAIDScriptURL absoluteString]];
    [resultHTML insertString:MRAIDScriptTag atIndex:headTagRange.location + headTagRange.length];

    return resultHTML;
}

- (BOOL)HTMLStringIsMRAIDFragment:(NSString *)string
{
    return ([string rangeOfString:@"<html>"].location == NSNotFound ||
            [string rangeOfString:@"<head>"].location == NSNotFound);
}

- (NSMutableString *)fullHTMLFromMRAIDFragment:(NSString *)fragment
{
    NSMutableString *result = [fragment mutableCopy];

    NSString *prepend = @"<html><head>"
        @"<meta name='viewport' content='user-scalable=no; initial-scale=1.0'/>"
        @"</head>"
        @"<body style='margin:0;padding:0;overflow:hidden;background:transparent;'>";
    [result insertString:prepend atIndex:0];
    [result appendString:@"</body></html>"];

    return [result autorelease];
}

- (NSString *)MRAIDScriptPath
{
    MRBundleManager *bundleManager = [[MPInstanceProvider sharedProvider] buildMRBundleManager];
    return [bundleManager mraidPath];
}

- (void)layoutCloseButton
{
    if (!_usesCustomCloseButton) {
        CGRect frame = _closeButton.frame;
        frame.origin.x = CGRectGetWidth(CGRectApplyAffineTransform(self.frame, self.transform)) -
                _closeButton.frame.size.width;
        _closeButton.frame = frame;
        _closeButton.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin;
        [self addSubview:_closeButton];
        [self bringSubviewToFront:_closeButton];
    } else {
        [_closeButton removeFromSuperview];
    }
}

- (void)initializeJavascriptState
{
    MPLogDebug(@"Injecting initial JavaScript state.");
    [_displayController initializeJavascriptStateWithViewProperties:@[
            [MRPlacementTypeProperty propertyWithType:_placementType],
            [MRSupportsProperty defaultProperty]]];
}

- (BOOL)handleUserInteractionRequiredMRAIDCommand:(NSString *)command parameters:(NSDictionary *)parameters
{
    BOOL handled = NO;

    if (self.userTappedWebView) {
        if ([command isEqualToString:@"createCalendarEvent"]) {
            handled = YES;
            [self.calendarManager createCalendarEventWithParameters:parameters];
        } else if ([command isEqualToString:@"storePicture"]) {
            handled = YES;
            [self.pictureManager storePicture:parameters];
        }
    }

    // XXX jren: localize hack here, totally refactor mraid command handling later. Allow playVideo to auto-execute for interstitials
    if ([command isEqualToString:@"playVideo"] && (self.userTappedWebView || _placementType == MRAdViewPlacementTypeInterstitial)) {
        handled = YES;
        [self.videoPlayerManager playVideo:parameters];
    }

    return handled;
}

- (void)handleCommandWithURL:(NSURL *)URL
{
    NSString *command = URL.host;
    NSDictionary *parameters = MPDictionaryFromQueryString(URL.query);
    BOOL success = YES;

    // XXX jren: there should only be one command registry
    success = [self handleUserInteractionRequiredMRAIDCommand:command parameters:parameters];
    if (!success) {
        MRCommand *cmd = [MRCommand commandForString:command];
        if (cmd == nil) {
            success = NO;
        } else if ([self shouldExecuteMRCommand:cmd]) {
            cmd.parameters = parameters;
            cmd.view = self;
            success = [cmd execute];
        }
    }

    [self.jsEventEmitter fireNativeCommandCompleteEvent:command];

    if (!success) {
        MPLogDebug(@"Unknown command: %@", command);
        [self.jsEventEmitter fireErrorEventForAction:command withMessage:@"Specified command is not implemented."];
    }
}

- (BOOL)shouldExecuteMRCommand:(MRCommand *)cmd
{
    // some MRAID commands may not require user interaction
    return ![cmd requiresUserInteraction] || self.userTappedWebView;
}

- (void)performActionForMoPubSpecificURL:(NSURL *)url
{
    MPLogDebug(@"MRAdView - loading MoPub URL: %@", url);
    NSString *host = [url host];
    if ([host isEqualToString:kMoPubPrecacheCompleteHost] && self.adType == MRAdViewAdTypePreCached) {
        [self adDidLoad];
    } else {
        MPLogWarn(@"MRAdView - unsupported MoPub URL: %@", [url absoluteString]);
    }
}

#pragma mark - NSURLConnectionDelegate

- (void)connection:(NSURLConnection *)connection didReceiveResponse:(NSURLResponse *)response
{
    [self.data setLength:0];
}

- (void)connection:(NSURLConnection *)connection didReceiveData:(NSData *)data
{
    [self.data appendData:data];
}

- (void)connection:(NSURLConnection *)connection didFailWithError:(NSError *)error
{
    [self adDidFailToLoad];
}

- (void)connectionDidFinishLoading:(NSURLConnection *)connection
{
    NSString *str = [[NSString alloc] initWithData:self.data encoding:NSUTF8StringEncoding];
    [self loadHTMLString:str baseURL:nil];
    [str release];
}

#pragma mark - UIWebViewDelegate

- (BOOL)webView:(UIWebView *)webView shouldStartLoadWithRequest:(NSURLRequest *)request
 navigationType:(UIWebViewNavigationType)navigationType
{
    NSURL *url = [request URL];
    NSMutableString *urlString = [NSMutableString stringWithString:[url absoluteString]];
    NSString *scheme = url.scheme;

    if ([scheme isEqualToString:kMraidURLScheme]) {
        MPLogDebug(@"Trying to process command: %@", urlString);
        [self handleCommandWithURL:url];
        return NO;
    } else if ([scheme isEqualToString:kMoPubURLScheme]) {
        [self performActionForMoPubSpecificURL:url];
        return NO;
    } else if ([scheme isEqualToString:@"ios-log"]) {
        [urlString replaceOccurrencesOfString:@"%20"
                                   withString:@" "
                                      options:NSLiteralSearch
                                        range:NSMakeRange(0, [urlString length])];
        MPLogDebug(@"Web console: %@", urlString);
        return NO;
    }

    if (!_isLoading && (navigationType == UIWebViewNavigationTypeOther ||
            navigationType == UIWebViewNavigationTypeLinkClicked)) {
        BOOL iframe = ![request.URL isEqual:request.mainDocumentURL];
        if (iframe) return YES;

        [self safeHandleDisplayDestinationForURL:url];
        return NO;
    }

    return YES;
}

- (void)webViewDidStartLoad:(UIWebView *)webView
{
    [_webView disableJavaScriptDialogs];
}

- (void)webViewDidFinishLoad:(UIWebView *)webView
{
    if (_isLoading) {
        _isLoading = NO;
        [self initializeJavascriptState];

        switch (self.adType) {
            case MRAdViewAdTypeDefault:
                [self adDidLoad];
                break;
            case MRAdViewAdTypePreCached:
                // wait for the ad to tell us it's done precaching before we notify the publisher
                break;
            default:
                break;
        }
    }
}

- (void)webView:(UIWebView *)webView didFailLoadWithError:(NSError *)error
{
    if (error.code == NSURLErrorCancelled) return;
    _isLoading = NO;
    [self adDidFailToLoad];
}

#pragma mark - <MPAdDestinationDisplayAgentDelegate>

- (UIViewController *)viewControllerForPresentingModalView
{
    return [self.delegate viewControllerForPresentingModalView];
}

- (void)displayAgentWillPresentModal
{
    [self adWillPresentModalView];
}

- (void)displayAgentDidDismissModal
{
    [self adDidDismissModalView];
}

- (void)displayAgentWillLeaveApplication
{
    // Do nothing.
}

#pragma mark - <MRCalendarManagerDelegate>

- (void)calendarManager:(MRCalendarManager *)manager
        didFailToCreateCalendarEventWithErrorMessage:(NSString *)message
{
    [self.jsEventEmitter fireErrorEventForAction:@"createCalendarEvent"
                                      withMessage:message];
}

- (void)calendarManagerWillPresentCalendarEditor:(MRCalendarManager *)manager
{
    [self adWillPresentModalView];
}

- (void)calendarManagerDidDismissCalendarEditor:(MRCalendarManager *)manager
{
    [self adDidDismissModalView];
}

- (UIViewController *)viewControllerForPresentingCalendarEditor
{
    return [self viewControllerForPresentingModalView];
}

#pragma mark - <MRPictureManagerDelegate>

- (void)pictureManager:(MRPictureManager *)manager didFailToStorePictureWithErrorMessage:(NSString *)message
{
    [self.jsEventEmitter fireErrorEventForAction:@"storePicture"
                                     withMessage:message];
}

#pragma mark - <MRVideoPlayerManagerDelegate>

- (void)videoPlayerManager:(MRVideoPlayerManager *)manager didFailToPlayVideoWithErrorMessage:(NSString *)message
{
    [self.jsEventEmitter fireErrorEventForAction:@"playVideo"
                                     withMessage:message];
}

- (void)videoPlayerManagerWillPresentVideo:(MRVideoPlayerManager *)manager
{
    [self adWillPresentModalView];
}

- (void)videoPlayerManagerDidDismissVideo:(MRVideoPlayerManager *)manager
{
    [self adDidDismissModalView];
}

- (UIViewController *)viewControllerForPresentingVideoPlayer
{
    return [self viewControllerForPresentingModalView];
}

#pragma mark - Delegation Wrappers

- (void)adDidLoad
{
    if ([self.delegate respondsToSelector:@selector(adDidLoad:)]) {
        [self.delegate adDidLoad:self];
    }
}

- (void)adDidFailToLoad
{
    if ([self.delegate respondsToSelector:@selector(adDidFailToLoad:)]) {
        [self.delegate adDidFailToLoad:self];
    }
}

- (void)adWillClose
{
    if ([self.delegate respondsToSelector:@selector(adWillClose:)]) {
        [self.delegate adWillClose:self];
    }
}

- (void)adDidClose
{
    if ([self.delegate respondsToSelector:@selector(adDidClose:)]) {
        [self.delegate adDidClose:self];
    }
}

- (void)adWillExpandToFrame:(CGRect)frame
{
    if ([self.delegate respondsToSelector:@selector(willExpandAd:toFrame:)]) {
        [self.delegate willExpandAd:self toFrame:frame];
    }
}

- (void)adDidExpandToFrame:(CGRect)frame
{
    if ([self.delegate respondsToSelector:@selector(didExpandAd:toFrame:)]) {
        [self.delegate didExpandAd:self toFrame:frame];
    }
}

- (void)adDidRequestCustomCloseEnabled:(BOOL)enabled
{
    if ([self.delegate respondsToSelector:@selector(ad:didRequestCustomCloseEnabled:)]) {
        [self.delegate ad:self didRequestCustomCloseEnabled:enabled];
    }
}

- (void)adWillPresentModalView
{
    [_displayController additionalModalViewWillPresent];

    _modalViewCount++;
    if (_modalViewCount == 1) [self appShouldSuspend];
}

- (void)adDidDismissModalView
{
    [_displayController additionalModalViewDidDismiss];

    _modalViewCount--;
    if (_modalViewCount == 0) [self appShouldResume];
}

- (void)appShouldSuspend
{
    if ([self.delegate respondsToSelector:@selector(appShouldSuspendForAd:)]) {
        [self.delegate appShouldSuspendForAd:self];
    }
}

- (void)appShouldResume
{
    if ([self.delegate respondsToSelector:@selector(appShouldResumeFromAd:)]) {
        [self.delegate appShouldResumeFromAd:self];
    }
}

- (void)adViewableDidChange:(BOOL)viewable
{
    [self.jsEventEmitter fireChangeEventForProperty:[MRViewableProperty propertyWithViewable:viewable]];
}

@end
