#import <WebKit/WebKit.h>
#import "MWMViewController.h"
#import <CoreApi/MWMTypes.h>

NS_ASSUME_NONNULL_BEGIN

typedef void (^WebViewControllerWillLoadBlock)(BOOL, NSDictionary<NSString *, NSString *> * _Nullable);

@interface WebViewController : MWMViewController <WKNavigationDelegate>

@property (nonatomic) NSURL * _Nullable m_url;
@property (copy, nonatomic) NSString * _Nullable m_htmlText;
// Set to YES if external browser should be launched
@property (nonatomic) BOOL openInSafari;
@property (nonatomic, readonly) WKWebView * webView;

- (instancetype _Nullable)initWithUrl:(NSURL *)url title:( NSString * _Nullable)title;
- (instancetype _Nullable)initWithHtml:(NSString *)htmlText
                               baseUrl:(NSURL * _Nullable)url
                                 title:(NSString * _Nullable)title;
- (instancetype _Nullable)initWithAuthURL:(NSURL *)url
                            onSuccessAuth:(MWMStringBlock _Nullable)success
                                onFailure:(MWMVoidBlock _Nullable)failure;
- (void)willLoadUrl:(WebViewControllerWillLoadBlock)decisionHandler;
- (BOOL)shouldAddAccessToken;
- (void)forward;
- (void)back;
- (void)reloadFromOrigin;
- (NSString *)configuredHtmlWithText:(NSString *)htmlText;
- (void)performURLRequest;

@end

NS_ASSUME_NONNULL_END
