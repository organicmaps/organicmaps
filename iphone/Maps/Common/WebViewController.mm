#import "WebViewController.h"

@implementation WebViewController

- (id)initWithUrl:(NSURL *)url andTitleOrNil:(NSString *)title
{
  self = [super initWithNibName:nil bundle:nil];
  if (self)
  {
    _m_url = url;
    if (title)
      self.navigationItem.title = title;
  }
  return self;
}

- (id)initWithHtml:(NSString *)htmlText baseUrl:(NSURL *)url andTitleOrNil:(NSString *)title
{
  self = [super initWithNibName:nil bundle:nil];
  if (self)
  {
    auto html = [htmlText stringByReplacingOccurrencesOfString:@"<body>"
                                                    withString:@"<body><font face=\"helvetica\">"];
    html = [html stringByReplacingOccurrencesOfString:@"</body>" withString:@"</font></body>"];
    _m_htmlText = html;
    _m_url = url;
    if (title)
      self.navigationItem.title = title;
  }
  return self;
}

- (void)viewDidLoad
{
  [super viewDidLoad];
  UIView * view = self.view;
  view.backgroundColor = UIColor.whiteColor;

  UIWebView * webView = [[UIWebView alloc] initWithFrame:{}];
  [view addSubview:webView];

  webView.translatesAutoresizingMaskIntoConstraints = NO;
  webView.autoresizesSubviews = YES;
  NSLayoutYAxisAnchor * topAnchor = view.topAnchor;
  NSLayoutYAxisAnchor * bottomAnchor = view.bottomAnchor;
  NSLayoutXAxisAnchor * leadingAnchor = view.leadingAnchor;
  NSLayoutXAxisAnchor * trailingAnchor = view.trailingAnchor;
  if (@available(iOS 11.0, *))
  {
    UILayoutGuide * safeAreaLayoutGuide = view.safeAreaLayoutGuide;
    topAnchor = safeAreaLayoutGuide.topAnchor;
    bottomAnchor = safeAreaLayoutGuide.bottomAnchor;
    leadingAnchor = safeAreaLayoutGuide.leadingAnchor;
    trailingAnchor = safeAreaLayoutGuide.trailingAnchor;
  }

  [webView.topAnchor constraintEqualToAnchor:topAnchor].active = YES;
  [webView.bottomAnchor constraintEqualToAnchor:bottomAnchor].active = YES;
  [webView.leadingAnchor constraintEqualToAnchor:leadingAnchor].active = YES;
  [webView.trailingAnchor constraintEqualToAnchor:trailingAnchor].active = YES;

  webView.backgroundColor = UIColor.whiteColor;
  webView.delegate = self;

  if (self.m_htmlText)
    [webView loadHTMLString:self.m_htmlText baseURL:self.m_url];
  else
    [webView loadRequest:[NSURLRequest requestWithURL:self.m_url]];
}

- (BOOL)webView:(UIWebView *)inWeb shouldStartLoadWithRequest:(NSURLRequest *)inRequest navigationType:(UIWebViewNavigationType)inType
{
  if (self.openInSafari && inType == UIWebViewNavigationTypeLinkClicked
      && ![inRequest.URL.scheme isEqualToString:@"applewebdata"]) // do not try to open local links in Safari
  {
    NSURL * url = [inRequest URL];
    [UIApplication.sharedApplication openURL:url];
    return NO;
  }

  return YES;
}

@end
