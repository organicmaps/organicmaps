#import "WebViewController.h"

@implementation WebViewController

@synthesize m_url;
@synthesize m_htmlText;

- (id) initWithUrl: (NSURL *)url andTitleOrNil:(NSString *)title
{
  self = [super initWithNibName:nil bundle:nil];
  if (self)
  {
    self.m_url = url;
    if (title)
      self.navigationItem.title = title;
  }
  return self;
}

- (id) initWithHtml: (NSString *)htmlText
            baseUrl:(NSURL *)url 
      andTitleOrNil:(NSString *)title
{
  self = [super initWithNibName:nil bundle:nil];
  if (self)
  {
    htmlText = [htmlText stringByReplacingOccurrencesOfString:@"<body>" withString:@"<body><font face=\"helvetica\">"];
    htmlText = [htmlText stringByReplacingOccurrencesOfString:@"</body>" withString:@"</font></body>"];
    self.m_htmlText = htmlText;
    self.m_url = url;
    if (title)
      self.navigationItem.title = title;
  }
  return self;
}

- (void)loadView
{
  CGRect frame = [[UIScreen mainScreen] applicationFrame];
  UIWebView * webView = [[UIWebView alloc] initWithFrame:frame];
//  webView.scalesPageToFit = YES;
  webView.autoresizesSubviews = YES;
  webView.autoresizingMask= (UIViewAutoresizingFlexibleHeight | UIViewAutoresizingFlexibleWidth);

  if (m_htmlText)
    [webView loadHTMLString:m_htmlText baseURL:m_url];
  else
    [webView loadRequest:[NSURLRequest requestWithURL:m_url]];

  self.view = webView;
  [webView release];
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
  return YES;
}

@end
