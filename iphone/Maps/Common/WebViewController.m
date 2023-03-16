#import "WebViewController.h"
#import <CoreApi/MWMFrameworkHelper.h>
#import "SwiftBridge.h"

@interface WebViewController ()

@property(copy, nonatomic) MWMVoidBlock onFailure;
@property(copy, nonatomic) MWMStringBlock onSuccess;
@property(nonatomic) BOOL authorized;
@property(nonatomic) WKWebView *webView;
@property(nonatomic) BOOL shouldResendHeaders;

@end

@implementation WebViewController

- (id)initWithUrl:(NSURL *)url title:(NSString *)title {
  self = [super initWithNibName:nil bundle:nil];
  if (self) {
    _m_url = url;
    if (title)
      self.navigationItem.title = title;
  }
  return self;
}

- (id)initWithHtml:(NSString *)htmlText baseUrl:(NSURL *)url title:(NSString *)title {
  self = [super initWithNibName:nil bundle:nil];
  if (self) {
    _m_htmlText = [self configuredHtmlWithText:htmlText];
    _m_url = url;
    if (title)
      self.navigationItem.title = title;
  }
  return self;
}

- (NSString *)configuredHtmlWithText:(NSString *)htmlText {
  NSString *html = [htmlText stringByReplacingOccurrencesOfString:@"<body>"
                                                       withString:@"<body><font face=\"helvetica\" size=\"14pt\">"];
  html = [html stringByReplacingOccurrencesOfString:@"</body>" withString:@"</font></body>"];
  return html;
}

- (instancetype)initWithAuthURL:(NSURL *)url onSuccessAuth:(MWMStringBlock)success onFailure:(MWMVoidBlock)failure {
  self = [super initWithNibName:nil bundle:nil];
  if (self) {
    _m_url = url;
    _onFailure = failure;
    _onSuccess = success;
  }
  return self;
}

- (void)viewDidLoad {
  [super viewDidLoad];
  UIView *view = self.view;
  view.styleName = @"Background";

  self.webView = [[WKWebView alloc] initWithFrame:CGRectZero];
  self.webView.backgroundColor = UIColor.clearColor;
  self.webView.opaque = NO;
  self.webView.navigationDelegate = self;
  [view addSubview:self.webView];

  self.webView.translatesAutoresizingMaskIntoConstraints = NO;
  self.webView.autoresizesSubviews = YES;
  NSLayoutYAxisAnchor *topAnchor = view.topAnchor;
  NSLayoutYAxisAnchor *bottomAnchor = view.bottomAnchor;
  NSLayoutXAxisAnchor *leadingAnchor = view.leadingAnchor;
  NSLayoutXAxisAnchor *trailingAnchor = view.trailingAnchor;
  UILayoutGuide *safeAreaLayoutGuide = view.safeAreaLayoutGuide;
  topAnchor = safeAreaLayoutGuide.topAnchor;
  bottomAnchor = safeAreaLayoutGuide.bottomAnchor;
  leadingAnchor = safeAreaLayoutGuide.leadingAnchor;
  trailingAnchor = safeAreaLayoutGuide.trailingAnchor;

  [self.webView.topAnchor constraintEqualToAnchor:topAnchor].active = YES;
  [self.webView.bottomAnchor constraintEqualToAnchor:bottomAnchor].active = YES;
  [self.webView.leadingAnchor constraintEqualToAnchor:leadingAnchor].active = YES;
  [self.webView.trailingAnchor constraintEqualToAnchor:trailingAnchor].active = YES;

  self.webView.allowsLinkPreview = NO;

  [self performURLRequest];
}

- (void)performURLRequest {
  __weak __typeof(self) ws = self;
  [self willLoadUrl:^(BOOL load, NSDictionary<NSString *, NSString *> *headers) {
    __typeof(self) self = ws;
    if (load) {
      if (self.m_htmlText) {
        [self.webView loadHTMLString:self.m_htmlText baseURL:self.m_url];
      } else {
        NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:self.m_url];
        for (NSString *header in headers.allKeys) {
          [request setValue:headers[header] forHTTPHeaderField:header];
        }

//        if (self.shouldAddAccessToken) {
//          NSString *authHeader = [NSString stringWithFormat:@"Bearer %@", [MWMFrameworkHelper userAccessToken]];
//          [request setValue:authHeader forHTTPHeaderField:@"Authorization"];
//        }
        if ([UIColor isNightMode]) {
          [request setValue:@"dark" forHTTPHeaderField:@"x-mapsme-theme"];
        }
        [self.webView loadRequest:request];
      }
    }
  }];
}

- (void)willLoadUrl:(WebViewControllerWillLoadBlock)decisionHandler {
  decisionHandler(YES, nil);
}

- (BOOL)shouldAddAccessToken {
  return NO;
}

- (void)viewDidDisappear:(BOOL)animated {
  [super viewDidDisappear:animated];
  if (self.isMovingFromParentViewController && !self.authorized && self.onFailure)
    self.onFailure();
}

- (void)pop {
  [self.navigationController popViewControllerAnimated:YES];
}

- (void)webView:(WKWebView *)webView
  decidePolicyForNavigationAction:(WKNavigationAction *)navigationAction
                  decisionHandler:(void (^)(WKNavigationActionPolicy))decisionHandler {
  NSURL *url = navigationAction.request.URL;
  // about:blank#localAnchor, e.g. in copyright.html.
  if ([url.scheme isEqualToString:@"about"]) {
    decisionHandler(WKNavigationActionPolicyAllow);
    return;
  }

  if ([url.host isEqualToString:@"localhost"]) {
    NSString *query = url.query;
    NSArray<NSString *> *components = [query componentsSeparatedByString:@"="];
    if (components.count != 2) {
      NSAssert(false, @"Incorrect query:", query);
      [self pop];
      decisionHandler(WKNavigationActionPolicyCancel);
      return;
    }

    self.authorized = YES;
    [self pop];
    self.onSuccess(components[1]);
    decisionHandler(WKNavigationActionPolicyCancel);
    return;
  }

  if (self.openInSafari && navigationAction.navigationType == WKNavigationTypeLinkActivated &&
      ![url.scheme isEqualToString:@"applewebdata"])  // do not try to open local links in Safari
  {
    [UIApplication.sharedApplication openURL:url options:@{} completionHandler:nil];
    decisionHandler(WKNavigationActionPolicyCancel);
    return;
  }

  if (!self.shouldResendHeaders) {
    decisionHandler(WKNavigationActionPolicyAllow);
  } else {
    _m_url = url;
    self.shouldResendHeaders = NO;
    decisionHandler(WKNavigationActionPolicyCancel);
    [self performURLRequest];
  }
}

- (void)forward {
  [self.webView goForward];
}

- (void)back {
  [self.webView goBack];
}

- (void)reloadFromOrigin {
  self.shouldResendHeaders = YES;
  [self.webView reloadFromOrigin];
}

#if DEBUG
- (void)webView:(WKWebView *)webView
  didReceiveAuthenticationChallenge:(NSURLAuthenticationChallenge *)challenge
                  completionHandler:(void (^)(NSURLSessionAuthChallengeDisposition disposition,
                                              NSURLCredential *_Nullable credential))completionHandler {
  NSURLCredential *credential = [[NSURLCredential alloc] initWithTrust:[challenge protectionSpace].serverTrust];
  completionHandler(NSURLSessionAuthChallengeUseCredential, credential);
}
#endif

@end
