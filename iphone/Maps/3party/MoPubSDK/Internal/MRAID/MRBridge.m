//
//  MRBridge.m
//  MoPubSDK
//
//  Copyright (c) 2014 MoPub. All rights reserved.
//

#import "MRBridge.h"
#import "MPConstants.h"
#import "MPLogging.h"
#import "NSURL+MPAdditions.h"
#import "MPGlobal.h"
#import "MRBundleManager.h"
#import "MPInstanceProvider.h"
#import "UIWebView+MPAdditions.h"
#import "MRError.h"
#import "MRProperty.h"
#import "MRNativeCommandHandler.h"

static NSString * const kMraidURLScheme = @"mraid";

@interface MRBridge () <MPWebViewDelegate, MRNativeCommandHandlerDelegate>

@property (nonatomic, strong) MPWebView *webView;
@property (nonatomic, strong) MRNativeCommandHandler *nativeCommandHandler;

@end

@implementation MRBridge

- (instancetype)initWithWebView:(MPWebView *)webView
{
    if (self = [super init]) {
        _webView = webView;
        _webView.delegate = self;
        _nativeCommandHandler = [[MPInstanceProvider sharedProvider] buildMRNativeCommandHandlerWithDelegate:self];
    }

    return self;
}

- (void)dealloc
{
    _webView.delegate = nil;
}

- (void)loadHTMLString:(NSString *)HTML baseURL:(NSURL *)baseURL
{
    // Bail out if we can't locate mraid.js.
    if (![self MRAIDScriptPath]) {
        NSError *error = [NSError errorWithDomain:MoPubMRAIDAdsSDKDomain code:MRErrorMRAIDJSNotFound userInfo:nil];
        [self.delegate bridge:self didFailLoadingWebView:self.webView error:error];
        return;
    }

    if (HTML) {
        dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
            // Execute the javascript in the web view directly.
            NSString *mraidString = [NSString stringWithContentsOfFile:[self MRAIDScriptPath] encoding:NSUTF8StringEncoding error:nil];

            // Once done loading from the file, execute the javascript and load the html into the web view.
            dispatch_async(dispatch_get_main_queue(), ^{
                [self.webView evaluateJavaScript:mraidString completionHandler:^(id result, NSError *error){
                    [self.webView disableJavaScriptDialogs];
                    [self.webView loadHTMLString:HTML baseURL:baseURL];
                }];
            });
        });
    }
}

- (void)fireReadyEvent
{
    [self executeJavascript:@"window.mraidbridge.fireReadyEvent();"];
}

- (void)fireChangeEventForProperty:(MRProperty *)property
{
    NSString *JSON = [NSString stringWithFormat:@"{%@}", property];
    [self executeJavascript:@"window.mraidbridge.fireChangeEvent(%@);", JSON];
    MPLogTrace(@"JSON: %@", JSON);
}

- (void)fireChangeEventsForProperties:(NSArray *)properties
{
    NSString *JSON = [NSString stringWithFormat:@"{%@}", [properties componentsJoinedByString:@", "]];
    [self executeJavascript:@"window.mraidbridge.fireChangeEvent(%@);", JSON];
    MPLogTrace(@"JSON: %@", JSON);
}

- (void)fireErrorEventForAction:(NSString *)action withMessage:(NSString *)message
{
    [self executeJavascript:@"window.mraidbridge.fireErrorEvent('%@', '%@');", message, action];
}

- (void)fireSizeChangeEvent:(CGSize)size
{
    [self executeJavascript:@"window.mraidbridge.notifySizeChangeEvent(%.1f, %.1f);", size.width, size.height];
}

- (void)fireSetScreenSize:(CGSize)size
{
    [self executeJavascript:@"window.mraidbridge.setScreenSize(%.1f, %.1f);", size.width, size.height];
}

- (void)fireSetPlacementType:(NSString *)placementType
{
    [self executeJavascript:@"window.mraidbridge.setPlacementType('%@');", placementType];
}

- (void)fireSetCurrentPositionWithPositionRect:(CGRect)positionRect
{
    [self executeJavascript:@"window.mraidbridge.setCurrentPosition(%.1f, %.1f, %.1f, %.1f);", positionRect.origin.x, positionRect.origin.y,
     positionRect.size.width, positionRect.size.height];
}

- (void)fireSetDefaultPositionWithPositionRect:(CGRect)positionRect
{
    [self executeJavascript:@"window.mraidbridge.setDefaultPosition(%.1f, %.1f, %.1f, %.1f);", positionRect.origin.x, positionRect.origin.y,
     positionRect.size.width, positionRect.size.height];
}

- (void)fireSetMaxSize:(CGSize)maxSize
{
    [self executeJavascript:@"window.mraidbridge.setMaxSize(%.1f, %.1f);", maxSize.width, maxSize.height];
}

#pragma mark - <MPWebViewDelegate>

- (BOOL)webView:(MPWebView *)webView shouldStartLoadWithRequest:(NSURLRequest *)request navigationType:(UIWebViewNavigationType)navigationType
{
    NSURL *url = [request URL];
    NSMutableString *urlString = [NSMutableString stringWithString:[url absoluteString]];
    NSString *scheme = url.scheme;

    if ([scheme isEqualToString:kMraidURLScheme]) {
        // Some native commands such as useCustomClose should be allowed to run even if we're not handling requests.
        // The command handler will make sure we don't execute native commands that aren't allowed to execute while we're not handling requests.
        MPLogDebug(@"Trying to process command: %@", urlString);
        NSString *command = url.host;
        NSDictionary *properties = MPDictionaryFromQueryString(url.query);
        [self.nativeCommandHandler handleNativeCommand:command withProperties:properties];
        return NO;
    } else if ([url mp_isMoPubScheme]) {
        [self.delegate bridge:self performActionForMoPubSpecificURL:url];
        return NO;
    } else if ([scheme isEqualToString:@"ios-log"]) {
        [urlString replaceOccurrencesOfString:@"%20"
                                   withString:@" "
                                      options:NSLiteralSearch
                                        range:NSMakeRange(0, [urlString length])];
        MPLogDebug(@"Web console: %@", urlString);
        return NO;
    }

    if (!self.shouldHandleRequests) {
        return NO;
    }

    if ([url mp_hasTelephoneScheme] || [url mp_hasTelephonePromptScheme]) {
        [self.delegate bridge:self handleDisplayForDestinationURL:url];
        return NO;
    }

    BOOL isLoading = [self.delegate isLoadingAd];
    BOOL userInteractedWithWebView = [self.delegate hasUserInteractedWithWebViewForBridge:self];
    BOOL safeToAutoloadLink = navigationType == UIWebViewNavigationTypeLinkClicked || userInteractedWithWebView || [url mp_isSafeForLoadingWithoutUserAction];

    if (!isLoading && (navigationType == UIWebViewNavigationTypeOther || navigationType == UIWebViewNavigationTypeLinkClicked)) {
        BOOL iframe = ![request.URL isEqual:request.mainDocumentURL];

        // If we load a URL from an iFrame that did not originate from a click or
        // is a deep link, handle normally and return safeToAutoloadLink.
        if (iframe && !((navigationType == UIWebViewNavigationTypeLinkClicked) && ([scheme isEqualToString:@"https"] || [scheme isEqualToString:@"http"]))) {
            return safeToAutoloadLink;
        }

        // Otherwise, open the URL in a new web view.
        [self.delegate bridge:self handleDisplayForDestinationURL:url];
        return NO;
    }

    return safeToAutoloadLink;
}

- (void)webViewDidStartLoad:(MPWebView *)webView
{
    [webView disableJavaScriptDialogs];
}

- (void)webViewDidFinishLoad:(MPWebView *)webView
{
    [self.delegate bridge:self didFinishLoadingWebView:webView];
}

- (void)webView:(MPWebView *)webView didFailLoadWithError:(NSError *)error
{
    if (error.code == NSURLErrorCancelled) {
        return;
    }

    [self.delegate bridge:self didFailLoadingWebView:webView error:error];
}

#pragma mark - Private

- (NSString *)MRAIDScriptPath
{
    MRBundleManager *bundleManager = [[MPInstanceProvider sharedProvider] buildMRBundleManager];
    return [bundleManager mraidPath];
}

- (void)executeJavascript:(NSString *)javascript, ...
{
    va_list args;
    va_start(args, javascript);
    [self executeJavascript:javascript withVarArgs:args];
    va_end(args);
}

- (void)fireNativeCommandCompleteEvent:(NSString *)command
{
    [self executeJavascript:@"window.mraidbridge.nativeCallComplete('%@');", command];
}

- (void)executeJavascript:(NSString *)javascript withVarArgs:(va_list)args
{
    NSString *js = [[NSString alloc] initWithFormat:javascript arguments:args];
    [self.webView stringByEvaluatingJavaScriptFromString:js];
}

#pragma mark - MRNativeCommandHandlerDelegate

- (void)handleMRAIDUseCustomClose:(BOOL)useCustomClose
{
    [self.delegate bridge:self handleNativeCommandUseCustomClose:useCustomClose];
}

- (void)handleMRAIDSetOrientationPropertiesWithForceOrientationMask:(UIInterfaceOrientationMask)forceOrientationMask
{
    [self.delegate bridge:self handleNativeCommandSetOrientationPropertiesWithForceOrientationMask:forceOrientationMask];
}

- (void)handleMRAIDOpenCallForURL:(NSURL *)URL
{
    [self.delegate bridge:self handleDisplayForDestinationURL:URL];
}

- (void)handleMRAIDExpandWithParameters:(NSDictionary *)params
{
    id urlValue = [params objectForKey:@"url"];
    [self.delegate bridge:self handleNativeCommandExpandWithURL:(urlValue == [NSNull null]) ? nil : urlValue
           useCustomClose:[[params objectForKey:@"useCustomClose"] boolValue]];
}

- (void)handleMRAIDResizeWithParameters:(NSDictionary *)params
{
    [self.delegate bridge:self handleNativeCommandResizeWithParameters:params];
}

- (void)handleMRAIDClose
{
    [self.delegate handleNativeCommandCloseWithBridge:self];
}

- (void)nativeCommandWillPresentModalView
{
    [self.delegate nativeCommandWillPresentModalView];
}

- (void)nativeCommandDidDismissModalView
{
    [self.delegate nativeCommandDidDismissModalView];
}

- (void)nativeCommandCompleted:(NSString *)command
{
    [self fireNativeCommandCompleteEvent:command];
}

- (void)nativeCommandFailed:(NSString *)command withMessage:(NSString *)message
{
    [self fireErrorEventForAction:command withMessage:message];
}

- (UIViewController *)viewControllerForPresentingModalView
{
    return [self.delegate viewControllerForPresentingModalView];
}

- (MRAdViewPlacementType)adViewPlacementType
{
    return [self.delegate placementType];
}

- (BOOL)userInteractedWithWebView
{
    return [self.delegate hasUserInteractedWithWebViewForBridge:self];
}

- (BOOL)handlingWebviewRequests
{
    return self.shouldHandleRequests;
}

@end
