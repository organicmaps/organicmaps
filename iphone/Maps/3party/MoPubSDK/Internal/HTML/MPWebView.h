//
//  MPWebView.h
//  MoPubSDK
//
//  Copyright © 2016 MoPub. All rights reserved.
//

/***
 * MPWebView
 * This class is a wrapper class for WKWebView and UIWebView. Internally, it utilizes WKWebView when possible, and
 * falls back on UIWebView only when WKWebView isn't available (i.e., in iOS 7). MPWebView's interface is meant to
 * imitate UIWebView, and, in many cases, MPWebView can act as a drop-in replacement for UIWebView. MPWebView also
 * blocks all JavaScript text boxes from appearing.
 *
 * While `stringByEvaluatingJavaScriptFromString:` does exist for UIWebView compatibility reasons, it's highly
 * recommended that the caller uses `evaluateJavaScript:completionHandler:` whenever code can be reworked
 * to make use of completion blocks to keep the advantages of asynchronicity. It solely fires off the javascript
 * execution within WKWebView and does not wait or return.
 *
 * MPWebView currently does not support a few other features of UIWebView -- such as pagination -- as WKWebView also
 * does not contain support.
 ***/

#import <UIKit/UIKit.h>

@class MPWebView;

@protocol MPWebViewDelegate <NSObject>

@optional

- (BOOL)webView:(MPWebView *)webView
shouldStartLoadWithRequest:(NSURLRequest *)request
 navigationType:(UIWebViewNavigationType)navigationType;

- (void)webViewDidStartLoad:(MPWebView *)webView;

- (void)webViewDidFinishLoad:(MPWebView *)webView;

- (void)webView:(MPWebView *)webView
didFailLoadWithError:(NSError *)error;

@end

typedef void (^MPWebViewJavascriptEvaluationCompletionHandler)(id result, NSError *error);

@interface MPWebView : UIView

// If you -need- UIWebView for some reason, use this method to init and send `YES` to `forceUIWebView` to be sure
// you're using UIWebView regardless of OS. If any other `init` method is used, or if `NO` is used as the forceUIWebView
// parameter, WKWebView will be used when available.
- (instancetype)initWithFrame:(CGRect)frame forceUIWebView:(BOOL)forceUIWebView;

@property (weak, nonatomic) id<MPWebViewDelegate> delegate;

@property (nonatomic, readonly, getter=isLoading) BOOL loading;

// These methods and properties are non-functional below iOS 9. If you call or try to set them, they'll do nothing.
// For the properties, if you try to access them, you'll get `NO` 100% of the time. They are entirely hidden when
// compiling with iOS 8 SDK or below.
#if __IPHONE_OS_VERSION_MAX_ALLOWED >= 90000
- (void)loadData:(NSData *)data
        MIMEType:(NSString *)MIMEType
textEncodingName:(NSString *)encodingName
         baseURL:(NSURL *)baseURL;

@property (nonatomic) BOOL allowsLinkPreview;
@property (nonatomic, readonly) BOOL allowsPictureInPictureMediaPlayback;
#endif

- (void)loadHTMLString:(NSString *)string
               baseURL:(NSURL *)baseURL;

- (void)loadRequest:(NSURLRequest *)request;
- (void)stopLoading;
- (void)reload;

@property (nonatomic, readonly) BOOL canGoBack;
@property (nonatomic, readonly) BOOL canGoForward;
- (void)goBack;
- (void)goForward;

@property (nonatomic) BOOL scalesPageToFit;
@property (nonatomic, readonly) UIScrollView *scrollView;

- (void)evaluateJavaScript:(NSString *)javaScriptString
         completionHandler:(MPWebViewJavascriptEvaluationCompletionHandler)completionHandler;

// When using WKWebView, always returns @"" and solely fires the javascript execution without waiting on it.
// If you need a guaranteed return value from `stringByEvaluatingJavaScriptFromString:`, please use
// `evaluateJavaScript:completionHandler:` instead.
- (NSString *)stringByEvaluatingJavaScriptFromString:(NSString *)javaScriptString;

@property (nonatomic, readonly) BOOL allowsInlineMediaPlayback;
@property (nonatomic, readonly) BOOL mediaPlaybackRequiresUserAction;
@property (nonatomic, readonly) BOOL mediaPlaybackAllowsAirPlay;

// UIWebView+MPAdditions methods
- (void)mp_setScrollable:(BOOL)scrollable;
- (void)disableJavaScriptDialogs;

@end
