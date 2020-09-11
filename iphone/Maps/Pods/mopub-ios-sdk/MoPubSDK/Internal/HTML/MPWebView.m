//
//  MPWebView.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPWebView.h"
#import "MPContentBlocker.h"
#import <WebKit/WebKit.h>

static BOOL const kMoPubAllowsInlineMediaPlaybackDefault = YES;
static BOOL const kMoPubRequiresUserActionForMediaPlaybackDefault = NO;

// Set defaults for this as its default differs between different iOS versions.
static BOOL const kMoPubAllowsLinkPreviewDefault = NO;

static NSString *const kMoPubJavaScriptDisableDialogScript = @"window.alert = function() { }; window.prompt = function() { }; window.confirm = function() { };";

static NSString *const kMoPubFrameKeyPathString = @"frame";

@interface MPWebView () <WKNavigationDelegate, WKUIDelegate, UIScrollViewDelegate>

@property (weak, nonatomic) WKWebView *wkWebView;

@property (strong, nonatomic) NSArray<NSLayoutConstraint *> *webViewLayoutConstraints;

@property (nonatomic, assign) BOOL hasMovedToWindow;

@end

@implementation MPWebView

- (instancetype)init {
    if (self = [super init]) {
        [self setUp];
    }

    return self;
}

- (instancetype)initWithCoder:(NSCoder *)aDecoder {
    if (self = [super initWithCoder:aDecoder]) {
        [self setUp];
    }

    return self;
}

- (instancetype)initWithFrame:(CGRect)frame {
    if (self = [super initWithFrame:frame]) {
        [self setUp];
    }

    return self;
}

- (void)setUp {
    WKUserContentController *contentController = [[WKUserContentController alloc] init];
    WKWebViewConfiguration *config = [[WKWebViewConfiguration alloc] init];
    config.allowsInlineMediaPlayback = kMoPubAllowsInlineMediaPlaybackDefault;
    config.requiresUserActionForMediaPlayback = kMoPubRequiresUserActionForMediaPlaybackDefault;
    config.userContentController = contentController;

    if (@available(iOS 11, *)) {
        [WKContentRuleListStore.defaultStore compileContentRuleListForIdentifier:@"ContentBlockingRules"
                                                          encodedContentRuleList:MPContentBlocker.blockedResourcesList
                                                               completionHandler:^(WKContentRuleList * rulesList, NSError * error) {
            if (error == nil) {
                [config.userContentController addContentRuleList:rulesList];
            }
        }];
    }

    WKWebView *wkWebView = [[WKWebView alloc] initWithFrame:self.bounds configuration:config];
    wkWebView.UIDelegate = self;
    wkWebView.navigationDelegate = self;
    self.wkWebView = wkWebView;

    // Put WKWebView onto the offscreen view so any loading will complete correctly; see comment below.
    [self retainWKWebViewOffscreen:wkWebView];

    wkWebView.backgroundColor = [UIColor clearColor];
    wkWebView.opaque = NO;
    wkWebView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;

    // set default scalesPageToFit
    self.scalesPageToFit = NO;

    // set default `shouldConformToSafeArea`
    self.shouldConformToSafeArea = NO;

    // configure like the old MPAdWebView
    self.backgroundColor = [UIColor clearColor];
    self.opaque = NO;

    // set default for allowsLinkPreview as they're different between iOS versions
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
        [self constrainView:self.wkWebView shouldUseSafeArea:self.shouldConformToSafeArea];
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
        if (@available(iOS 11, *)) {
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
        [self constrainView:self.wkWebView shouldUseSafeArea:shouldConformToSafeArea];
    }
}

- (void)constrainView:(UIView *)view shouldUseSafeArea:(BOOL)shouldUseSafeArea {
    if (@available(iOS 11, *)) {
        view.translatesAutoresizingMaskIntoConstraints = NO;

        if (self.webViewLayoutConstraints) {
            [NSLayoutConstraint deactivateConstraints:self.webViewLayoutConstraints];
        }

        if (shouldUseSafeArea) {
            self.webViewLayoutConstraints = @[
                [view.topAnchor constraintEqualToAnchor:self.safeAreaLayoutGuide.topAnchor],
                [view.leadingAnchor constraintEqualToAnchor:self.safeAreaLayoutGuide.leadingAnchor],
                [view.trailingAnchor constraintEqualToAnchor:self.safeAreaLayoutGuide.trailingAnchor],
                [view.bottomAnchor constraintEqualToAnchor:self.safeAreaLayoutGuide.bottomAnchor],
            ];
        } else {
            self.webViewLayoutConstraints = @[
                [view.topAnchor constraintEqualToAnchor:self.topAnchor],
                [view.leadingAnchor constraintEqualToAnchor:self.leadingAnchor],
                [view.trailingAnchor constraintEqualToAnchor:self.trailingAnchor],
                [view.bottomAnchor constraintEqualToAnchor:self.bottomAnchor],
            ];
        }

        [NSLayoutConstraint activateConstraints:self.webViewLayoutConstraints];
    }
}

- (BOOL)isLoading {
    return self.wkWebView.isLoading;
}

- (void)loadData:(NSData *)data
        MIMEType:(NSString *)MIMEType
textEncodingName:(NSString *)encodingName
         baseURL:(NSURL *)baseURL {
    [self.wkWebView loadData:data
                    MIMEType:MIMEType
       characterEncodingName:encodingName
                     baseURL:baseURL];
}

- (void)loadHTMLString:(NSString *)string baseURL:(NSURL *)baseURL {
    [self.wkWebView loadHTMLString:string baseURL:baseURL];
}

- (void)loadRequest:(NSURLRequest *)request {
    [self.wkWebView loadRequest:request];
}

- (void)stopLoading {
    [self.wkWebView stopLoading];
}

- (void)reload {
    [self.wkWebView reload];
}

- (BOOL)canGoBack {
    return self.wkWebView.canGoBack;
}

- (BOOL)canGoForward {
    return self.wkWebView.canGoForward;
}

- (void)goBack {
    [self.wkWebView goBack];
}

- (void)goForward {
    [self.wkWebView goForward];
}

- (void)setAllowsLinkPreview:(BOOL)allowsLinkPreview {
    self.wkWebView.allowsLinkPreview = allowsLinkPreview;
}

- (BOOL)allowsLinkPreview {
     return self.wkWebView.allowsLinkPreview;
}

- (void)setScalesPageToFit:(BOOL)scalesPageToFit {
    if (scalesPageToFit) {
        self.wkWebView.scrollView.delegate = nil;

        [self.wkWebView.configuration.userContentController removeAllUserScripts];
    } else {
        // Make sure the scroll view can't scroll (prevent double tap to zoom)
        self.wkWebView.scrollView.delegate = self;
    }
}

- (BOOL)scalesPageToFit {
    return self.scrollView.delegate == nil;
}

- (UIScrollView *)scrollView {
    return self.wkWebView.scrollView;
}

- (void)evaluateJavaScript:(NSString *)javaScriptString
         completionHandler:(MPWebViewJavascriptEvaluationCompletionHandler)completionHandler {
    [self.wkWebView evaluateJavaScript:javaScriptString completionHandler:completionHandler];
}

- (NSString *)stringByEvaluatingJavaScriptFromString:(NSString *)javaScriptString {
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

- (BOOL)allowsInlineMediaPlayback {
    return self.wkWebView.configuration.allowsInlineMediaPlayback;
}

- (BOOL)mediaPlaybackRequiresUserAction {
    return self.wkWebView.configuration.requiresUserActionForMediaPlayback;
}

- (BOOL)mediaPlaybackAllowsAirPlay {
    return self.wkWebView.configuration.allowsAirPlayForMediaPlayback;
}

- (BOOL)allowsPictureInPictureMediaPlayback {
    return self.wkWebView.configuration.allowsPictureInPictureMediaPlayback;
}

#pragma mark - UIScrollView related

/// Find all subviews that are UIScrollViews or subclasses and set their scrolling and bounce.
- (void)mp_setScrollable:(BOOL)scrollable {
    UIScrollView *scrollView = self.scrollView;
    scrollView.scrollEnabled = scrollable;
    scrollView.bounces = scrollable;
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
        policy = [self.delegate webView:self
             shouldStartLoadWithRequest:navigationAction.request
                         navigationType:navigationAction.navigationType] ? WKNavigationActionPolicyAllow : WKNavigationActionPolicyCancel;
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
completionHandler:(void (^)(void))completionHandler {
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
