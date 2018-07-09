//
//  MPWebView.m
//  MoPubSDK
//
//  Copyright © 2016 MoPub. All rights reserved.
//

#import "MPWebView.h"

#import <WebKit/WebKit.h>

static BOOL const kMoPubAllowsInlineMediaPlaybackDefault = YES;
static BOOL const kMoPubRequiresUserActionForMediaPlaybackDefault = NO;

// Set defaults for this as its default differs between UIWebView and WKWebView
static BOOL const kMoPubAllowsLinkPreviewDefault = NO;

static NSString *const kMoPubJavaScriptDisableDialogScript = @"window.alert = function() { }; window.prompt = function() { }; window.confirm = function() { };";

static NSString *const kMoPubFrameKeyPathString = @"frame";

static BOOL gForceWKWebView = NO;

@interface MPWebView () <UIWebViewDelegate, WKNavigationDelegate, WKUIDelegate, UIScrollViewDelegate>

@property (weak, nonatomic) WKWebView *wkWebView;
@property (weak, nonatomic) UIWebView *uiWebView;

@property (strong, nonatomic) NSArray<NSLayoutConstraint *> *wkWebViewLayoutConstraints;

@property (nonatomic, assign) BOOL hasMovedToWindow;

@end

@implementation MPWebView

- (instancetype)init {
    if (self = [super init]) {
        [self setUpStepsForceUIWebView:NO];
    }

    return self;
}

- (instancetype)initWithCoder:(NSCoder *)aDecoder {
    if (self = [super initWithCoder:aDecoder]) {
        [self setUpStepsForceUIWebView:NO];
    }

    return self;
}

- (instancetype)initWithFrame:(CGRect)frame {
    if (self = [super initWithFrame:frame]) {
        [self setUpStepsForceUIWebView:NO];
    }

    return self;
}

- (instancetype)initWithFrame:(CGRect)frame forceUIWebView:(BOOL)forceUIWebView {
    if (self = [super initWithFrame:frame]) {
        [self setUpStepsForceUIWebView:forceUIWebView];
    }

    return self;
}

- (void)setUpStepsForceUIWebView:(BOOL)forceUIWebView {
    // set up web view
    UIView *webView;

    if ((gForceWKWebView || !forceUIWebView) && [WKWebView class]) {
        WKUserContentController *contentController = [[WKUserContentController alloc] init];
        WKWebViewConfiguration *config = [[WKWebViewConfiguration alloc] init];
        config.allowsInlineMediaPlayback = kMoPubAllowsInlineMediaPlaybackDefault;
        if (@available(iOS 9.0, *)) {
            config.requiresUserActionForMediaPlayback = kMoPubRequiresUserActionForMediaPlaybackDefault;
        } else {
            config.mediaPlaybackRequiresUserAction = kMoPubRequiresUserActionForMediaPlaybackDefault;
        }
        config.userContentController = contentController;

        WKWebView *wkWebView = [[WKWebView alloc] initWithFrame:self.bounds configuration:config];

        wkWebView.UIDelegate = self;
        wkWebView.navigationDelegate = self;

        webView = wkWebView;

        self.wkWebView = wkWebView;

        // Put WKWebView onto the offscreen view so any loading will complete correctly; see comment below.
        [self retainWKWebViewOffscreen:wkWebView];
    } else {
        UIWebView *uiWebView = [[UIWebView alloc] initWithFrame:self.bounds];

        uiWebView.allowsInlineMediaPlayback = kMoPubAllowsInlineMediaPlaybackDefault;
        uiWebView.mediaPlaybackRequiresUserAction = kMoPubRequiresUserActionForMediaPlaybackDefault;

        uiWebView.delegate = self;

        webView = uiWebView;

        self.uiWebView = uiWebView;

        [self addSubview:webView];
    }

    webView.backgroundColor = [UIColor clearColor];
    webView.opaque = NO;

    webView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;

    // set default scalesPageToFit
    self.scalesPageToFit = NO;

    // set default `shouldConformToSafeArea`
    self.shouldConformToSafeArea = NO;

    // configure like the old MPAdWebView
    self.backgroundColor = [UIColor clearColor];
    self.opaque = NO;

    // set default for allowsLinkPreview as they're different between WKWebView and UIWebView
    self.allowsLinkPreview = kMoPubAllowsLinkPreviewDefault;

    // set up KVO to adjust the frame of the WKWebView to avoid white screens
    if (self.wkWebView) {
        [self addObserver:self
               forKeyPath:kMoPubFrameKeyPathString
                  options:NSKeyValueObservingOptionOld
                  context:NULL];
    }
}

// WKWebView won't load/execute javascript unless it's on the view hierarchy. Because the MoPub SDK uses a lot of
// javascript before adding the view to the hierarchy, let's stick the WKWebView into an offscreen-but-on-the-window
// view, and move it to self when self gets a window.
static UIView *gOffscreenView = nil;

- (void)retainWKWebViewOffscreen:(WKWebView *)webView {
    if (!gOffscreenView) {
        gOffscreenView = [self constructOffscreenView];
    }
    [gOffscreenView addSubview:webView];
}

- (void)cleanUpOffscreenView {
    if (gOffscreenView.subviews.count == 0) {
        [gOffscreenView removeFromSuperview];
        gOffscreenView = nil;
    }
}

- (UIView *)constructOffscreenView {
    UIView *view = [[UIView alloc] initWithFrame:CGRectZero];
    view.clipsToBounds = YES;

    UIWindow *appWindow = [[UIApplication sharedApplication] keyWindow];
    [appWindow addSubview:view];

    return view;
}

- (void)didMoveToWindow {
    // If using WKWebView, and if MPWebView is in the view hierarchy, and if the WKWebView is in the offscreen view currently,
    // move our WKWebView to self and deallocate OffscreenView if no other MPWebView is using it.
    if (self.wkWebView
        && !self.hasMovedToWindow
        && self.window != nil
        && [self.wkWebView.superview isEqual:gOffscreenView]) {
        self.wkWebView.frame = self.bounds;
        [self addSubview:self.wkWebView];
        [self constrainWebViewShouldUseSafeArea:self.shouldConformToSafeArea];
        self.hasMovedToWindow = YES;

        // Don't keep OffscreenView if we don't need it; it can always be re-allocated again later
        [self cleanUpOffscreenView];
    }
}

// Occasionally, we encounter an issue where, when MPWebView is initialized at a different frame size than when it's shown,
// the WKWebView shows as all white because it doesn't have a chance to get redrawn at the new size before getting shown.
// This makes sure WKWebView is always already rendered at the correct size when it gets moved to the window.
- (void)observeValueForKeyPath:(NSString *)keyPath
                      ofObject:(id)object
                        change:(NSDictionary *)change
                       context:(void *)context {
    // Only keep the wkWebView up-to-date if its superview is the offscreen view.
    // If it's attached to self, the autoresizing mask should come into play & this is just extra work.
    if ([keyPath isEqualToString:kMoPubFrameKeyPathString]
        && [self.wkWebView.superview isEqual:gOffscreenView]) {
        if (@available(iOS 11.0, *)) {
            // In iOS 11, WKWebView loads web view contents into the safe area only unless `viewport-fit=cover` is
            // included in the page's viewport tag. Also, as of iOS 11, it appears WKWebView does not redraw page
            // contents to match the safe area of a new position after being moved. As a result, making `wkWebView`'s
            // X/Y coordinates (0,0) can introduce an issue on iPhone X where banners do not load inside of
            // `wkWebView`'s bounds, even if the banner is moved into the safe area after loading.
            //
            // To skirt around these problems, always put `wkWebView` into the safe area when using iOS 11 or later.
            self.wkWebView.frame = CGRectMake(gOffscreenView.safeAreaInsets.left,
                                              gOffscreenView.safeAreaInsets.top,
                                              CGRectGetWidth(self.bounds),
                                              CGRectGetHeight(self.bounds));
        } else {
            self.wkWebView.frame = self.bounds;
        }
    }
}

- (void)dealloc {
    // Remove KVO observer
    if (self.wkWebView) {
        [self removeObserver:self forKeyPath:kMoPubFrameKeyPathString];
    }

    // Avoids EXC_BAD_INSTRUCTION crash
    self.wkWebView.scrollView.delegate = nil;

    // Be sure our WKWebView doesn't stay stuck to the static OffscreenView
    [self.wkWebView removeFromSuperview];
    // Deallocate OffscreenView if needed
    [self cleanUpOffscreenView];
}

- (void)setShouldConformToSafeArea:(BOOL)shouldConformToSafeArea {
    _shouldConformToSafeArea = shouldConformToSafeArea;

    if (self.hasMovedToWindow) {
        [self constrainWebViewShouldUseSafeArea:shouldConformToSafeArea];
    }
}

- (void)constrainWebViewShouldUseSafeArea:(BOOL)shouldUseSafeArea {
    if (@available(iOS 11.0, *)) {
        self.wkWebView.translatesAutoresizingMaskIntoConstraints = NO;

        if (self.wkWebViewLayoutConstraints) {
            [NSLayoutConstraint deactivateConstraints:self.wkWebViewLayoutConstraints];
        }

        if (shouldUseSafeArea) {
            self.wkWebViewLayoutConstraints = @[
                                                [self.wkWebView.topAnchor constraintEqualToAnchor:self.safeAreaLayoutGuide.topAnchor],
                                                [self.wkWebView.leadingAnchor constraintEqualToAnchor:self.safeAreaLayoutGuide.leadingAnchor],
                                                [self.wkWebView.trailingAnchor constraintEqualToAnchor:self.safeAreaLayoutGuide.trailingAnchor],
                                                [self.wkWebView.bottomAnchor constraintEqualToAnchor:self.safeAreaLayoutGuide.bottomAnchor],
                                                ];
        } else {
            self.wkWebViewLayoutConstraints = @[
                                                [self.wkWebView.topAnchor constraintEqualToAnchor:self.topAnchor],
                                                [self.wkWebView.leadingAnchor constraintEqualToAnchor:self.leadingAnchor],
                                                [self.wkWebView.trailingAnchor constraintEqualToAnchor:self.trailingAnchor],
                                                [self.wkWebView.bottomAnchor constraintEqualToAnchor:self.bottomAnchor],
                                                ];
        }

        [NSLayoutConstraint activateConstraints:self.wkWebViewLayoutConstraints];
    }
}

- (BOOL)isLoading {
    return self.uiWebView ? self.uiWebView.isLoading : self.wkWebView.isLoading;
}

- (void)loadData:(NSData *)data
        MIMEType:(NSString *)MIMEType
textEncodingName:(NSString *)encodingName
         baseURL:(NSURL *)baseURL {
    if (self.uiWebView) {
        [self.uiWebView loadData:data
                        MIMEType:MIMEType
                textEncodingName:encodingName
                         baseURL:baseURL];
    } else if (@available(iOS 9.0, *)) {
        [self.wkWebView loadData:data
                        MIMEType:MIMEType
           characterEncodingName:encodingName
                         baseURL:baseURL];
    }
}

+ (void)forceWKWebView:(BOOL)shouldForce
{
    gForceWKWebView = shouldForce;
}

+ (BOOL)isForceWKWebView
{
    return gForceWKWebView;
}

- (void)loadHTMLString:(NSString *)string
               baseURL:(NSURL *)baseURL {
    if (self.uiWebView) {
        [self.uiWebView loadHTMLString:string
                               baseURL:baseURL];
    } else {
        [self.wkWebView loadHTMLString:string
                               baseURL:baseURL];
    }
}

- (void)loadRequest:(NSURLRequest *)request {
    if (self.uiWebView) {
        [self.uiWebView loadRequest:request];
    } else {
        [self.wkWebView loadRequest:request];
    }
}

- (void)stopLoading {
    if (self.uiWebView) {
        [self.uiWebView stopLoading];
    } else {
        [self.wkWebView stopLoading];
    }
}

- (void)reload {
    if (self.uiWebView) {
        [self.uiWebView reload];
    } else {
        [self.wkWebView reload];
    }
}

- (BOOL)canGoBack {
    return self.uiWebView ? self.uiWebView.canGoBack : self.wkWebView.canGoBack;
}

- (BOOL)canGoForward {
    return self.uiWebView ? self.uiWebView.canGoForward : self.wkWebView.canGoForward;
}

- (void)goBack {
    if (self.uiWebView) {
        [self.uiWebView goBack];
    } else {
        [self.wkWebView goBack];
    }
}

- (void)goForward {
    if (self.uiWebView) {
        [self.uiWebView goForward];
    } else {
        [self.wkWebView goForward];
    }
}

- (void)setAllowsLinkPreview:(BOOL)allowsLinkPreview {
    if (@available(iOS 9.0, *)) {
        if (self.uiWebView) {
            self.uiWebView.allowsLinkPreview = allowsLinkPreview;
        } else {
            self.wkWebView.allowsLinkPreview = allowsLinkPreview;
        }
    }
}

- (BOOL)allowsLinkPreview {
    if (@available(iOS 9.0, *)) {
        if (self.uiWebView) {
            return self.uiWebView.allowsLinkPreview;
        } else {
            return self.wkWebView.allowsLinkPreview;
        }
    }

    return NO;
}

- (void)setScalesPageToFit:(BOOL)scalesPageToFit {
    if (self.uiWebView) {
        self.uiWebView.scalesPageToFit = scalesPageToFit;
    } else {
        if (scalesPageToFit) {
            self.wkWebView.scrollView.delegate = nil;

            [self.wkWebView.configuration.userContentController removeAllUserScripts];
        } else {
            // Make sure the scroll view can't scroll (prevent double tap to zoom)
            self.wkWebView.scrollView.delegate = self;
        }
    }
}

- (BOOL)scalesPageToFit {
    return self.uiWebView ? self.uiWebView.scalesPageToFit : self.scrollView.delegate == nil;
}

- (UIScrollView *)scrollView {
    return self.uiWebView ? self.uiWebView.scrollView : self.wkWebView.scrollView;
}

- (void)evaluateJavaScript:(NSString *)javaScriptString
         completionHandler:(MPWebViewJavascriptEvaluationCompletionHandler)completionHandler {
    if (self.uiWebView) {
        NSString *resultString = [self.uiWebView stringByEvaluatingJavaScriptFromString:javaScriptString];

        if (completionHandler) {
            completionHandler(resultString, nil);
        }
    } else {
        [self.wkWebView evaluateJavaScript:javaScriptString
                         completionHandler:completionHandler];
    }
}

- (NSString *)stringByEvaluatingJavaScriptFromString:(NSString *)javaScriptString {
    if (self.uiWebView) {
        return [self.uiWebView stringByEvaluatingJavaScriptFromString:javaScriptString];
    } else {
        // There is no way to reliably wait for `evaluateJavaScript:completionHandler:` to finish without risk of
        // deadlocking the main thread. This method is called on the main thread and the completion block is also
        // called on the main thread.
        // Instead of waiting, just fire and return an empty string.

        // Methods attempted:
        // libdispatch dispatch groups
        // http://stackoverflow.com/questions/17920169/how-to-wait-for-method-that-has-completion-block-all-on-main-thread

        [self.wkWebView evaluateJavaScript:javaScriptString completionHandler:nil];
        return @"";
    }
}

- (BOOL)allowsInlineMediaPlayback {
    return self.uiWebView ? self.uiWebView.allowsInlineMediaPlayback : self.wkWebView.configuration.allowsInlineMediaPlayback;
}

- (BOOL)mediaPlaybackRequiresUserAction {
    if (self.uiWebView) {
        return self.uiWebView.mediaPlaybackRequiresUserAction;
    } else {
        if (@available(iOS 9.0, *)) {
            return self.wkWebView.configuration.requiresUserActionForMediaPlayback;
        } else if (![self.wkWebView.configuration respondsToSelector:@selector(requiresUserActionForMediaPlayback)]) {
            return self.wkWebView.configuration.mediaPlaybackRequiresUserAction;
        } else {
            return NO;
        }
    }
}

- (BOOL)mediaPlaybackAllowsAirPlay {
    if (self.uiWebView) {
        return self.uiWebView.mediaPlaybackAllowsAirPlay;
    } else {
        if (@available(iOS 9.0, *)) {
            return self.wkWebView.configuration.allowsAirPlayForMediaPlayback;
        } else if (![self.wkWebView.configuration respondsToSelector:@selector(allowsAirPlayForMediaPlayback)]) {
            return self.wkWebView.configuration.mediaPlaybackAllowsAirPlay;
        } else {
            return NO;
        }
    }
}

- (BOOL)allowsPictureInPictureMediaPlayback {
    if (@available(iOS 9.0, *)) {
        if (self.uiWebView) {
            return self.uiWebView.allowsPictureInPictureMediaPlayback;
        } else {
            return self.wkWebView.configuration.allowsPictureInPictureMediaPlayback;
        }
    }

    return NO;
}

#pragma mark - UIWebViewDelegate

- (BOOL)webView:(UIWebView *)webView
shouldStartLoadWithRequest:(NSURLRequest *)request
 navigationType:(UIWebViewNavigationType)navigationType {
    if ([self.delegate respondsToSelector:@selector(webView:shouldStartLoadWithRequest:navigationType:)]) {
        return [self.delegate webView:self
           shouldStartLoadWithRequest:request
                       navigationType:navigationType];
    }

    return YES;
}

- (void)webViewDidStartLoad:(UIWebView *)webView {
    if ([self.delegate respondsToSelector:@selector(webViewDidStartLoad:)]) {
        [self.delegate webViewDidStartLoad:self];
    }
}

- (void)webViewDidFinishLoad:(UIWebView *)webView {
    if ([self.delegate respondsToSelector:@selector(webViewDidFinishLoad:)]) {
        [self.delegate webViewDidFinishLoad:self];
    }
}

- (void)webView:(UIWebView *)webView didFailLoadWithError:(NSError *)error {
    if ([self.delegate respondsToSelector:@selector(webView:didFailLoadWithError:)]) {
        [self.delegate webView:self didFailLoadWithError:error];
    }
}

#pragma mark - UIWebView+MPAdditions methods

/// Find all subviews that are UIScrollViews or subclasses and set their scrolling and bounce.
- (void)mp_setScrollable:(BOOL)scrollable {
    UIScrollView *scrollView = self.scrollView;
    scrollView.scrollEnabled = scrollable;
    scrollView.bounces = scrollable;
}

/// Redefine alert, prompt, and confirm to do nothing
- (void)disableJavaScriptDialogs
{
    if (self.uiWebView) { // Only redefine on UIWebView, as the WKNavigationDelegate for WKWebView takes care of this.
        [self stringByEvaluatingJavaScriptFromString:kMoPubJavaScriptDisableDialogScript];
    }
}

#pragma mark - WKNavigationDelegate

- (void)webView:(WKWebView *)webView didStartProvisionalNavigation:(WKNavigation *)navigation {
    if ([self.delegate respondsToSelector:@selector(webViewDidStartLoad:)]) {
        [self.delegate webViewDidStartLoad:self];
    }
}

- (void)webView:(WKWebView *)webView didFinishNavigation:(WKNavigation *)navigation {
    if ([self.delegate respondsToSelector:@selector(webViewDidFinishLoad:)]) {
        [self.delegate webViewDidFinishLoad:self];
    }
}

- (void)webView:(WKWebView *)webView
didFailNavigation:(WKNavigation *)navigation
      withError:(NSError *)error {
    if ([self.delegate respondsToSelector:@selector(webView:didFailLoadWithError:)]) {
        [self.delegate webView:self didFailLoadWithError:error];
    }
}

- (void)webView:(WKWebView *)webView
didFailProvisionalNavigation:(WKNavigation *)navigation
      withError:(NSError *)error {
    if ([self.delegate respondsToSelector:@selector(webView:didFailLoadWithError:)]) {
        [self.delegate webView:self didFailLoadWithError:error];
    }
}

- (void)webView:(WKWebView *)webView
decidePolicyForNavigationAction:(WKNavigationAction *)navigationAction
decisionHandler:(void (^)(WKNavigationActionPolicy))decisionHandler {
    WKNavigationActionPolicy policy = WKNavigationActionPolicyAllow;

    if ([self.delegate respondsToSelector:@selector(webView:shouldStartLoadWithRequest:navigationType:)]) {
        NSURLRequest *request = navigationAction.request;
        UIWebViewNavigationType navType;
        switch (navigationAction.navigationType) {
            case WKNavigationTypeLinkActivated:
                navType = UIWebViewNavigationTypeLinkClicked;
                break;
            case WKNavigationTypeFormSubmitted:
                navType = UIWebViewNavigationTypeFormSubmitted;
                break;
            case WKNavigationTypeBackForward:
                navType = UIWebViewNavigationTypeBackForward;
                break;
            case WKNavigationTypeReload:
                navType = UIWebViewNavigationTypeReload;
                break;
            case WKNavigationTypeFormResubmitted:
                navType = UIWebViewNavigationTypeFormResubmitted;
                break;
            default:
                navType = UIWebViewNavigationTypeOther;
                break;
        }

        policy = [self.delegate webView:self
             shouldStartLoadWithRequest:request
                         navigationType:navType] ? WKNavigationActionPolicyAllow : WKNavigationActionPolicyCancel;
    }

    decisionHandler(policy);
}

- (WKWebView *)webView:(WKWebView *)webView
createWebViewWithConfiguration:(WKWebViewConfiguration *)configuration
forNavigationAction:(WKNavigationAction *)navigationAction
windowFeatures:(WKWindowFeatures *)windowFeatures {
    // Open any links to new windows in the current WKWebView rather than create a new one
    if (!navigationAction.targetFrame.isMainFrame) {
        [webView loadRequest:navigationAction.request];
    }

    return nil;
}

#pragma mark - UIScrollViewDelegate

// Avoid double tap to zoom in WKWebView
// Delegate is only set when scalesPagesToFit is set to NO
- (UIView *)viewForZoomingInScrollView:(UIScrollView *)scrollView {
    return nil;
}

#pragma mark - WKUIDelegate

// WKUIDelegate method implementations makes it so that, if a WKWebView is being used, javascript dialog boxes can
// never show. They're programatically dismissed with the "Cancel" option (if there is any such option) before showing
// a view.

- (void)webView:(WKWebView *)webView
runJavaScriptAlertPanelWithMessage:(NSString *)message
initiatedByFrame:(WKFrameInfo *)frame
#if __IPHONE_OS_VERSION_MAX_ALLOWED < MP_IOS_9_0 // This pre-processor code is to be sure we can compile under both iOS 8 and 9 SDKs
completionHandler:(void (^)())completionHandler {
#else
completionHandler:(void (^)(void))completionHandler {
#endif
    completionHandler();
}

- (void)webView:(WKWebView *)webView
runJavaScriptConfirmPanelWithMessage:(NSString *)message
initiatedByFrame:(WKFrameInfo *)frame
completionHandler:(void (^)(BOOL))completionHandler {
    completionHandler(NO);
}

- (void)webView:(WKWebView *)webView
runJavaScriptTextInputPanelWithPrompt:(NSString *)prompt
    defaultText:(NSString *)defaultText
initiatedByFrame:(WKFrameInfo *)frame
completionHandler:(void (^)(NSString *result))completionHandler {
    completionHandler(nil);
}

@end
