#import "WebViewController.h"

@implementation WebViewController

@synthesize m_url;

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

- (void)dealloc
{
  [super dealloc];
}

- (void)didReceiveMemoryWarning
{
  // Releases the view if it doesn't have a superview.
  [super didReceiveMemoryWarning];
  
  // Release any cached data, images, etc that aren't in use.
}

#pragma mark - View lifecycle

// Implement loadView to create a view hierarchy programmatically, without using a nib.
- (void)loadView
{
  CGRect frame = [[UIScreen mainScreen] applicationFrame];
  UIWebView * webView = [[UIWebView alloc] initWithFrame:frame];
//  webView.scalesPageToFit = YES;
  webView.autoresizesSubviews = YES;
  webView.autoresizingMask= (UIViewAutoresizingFlexibleHeight | UIViewAutoresizingFlexibleWidth);

  [webView loadRequest:[NSURLRequest requestWithURL:m_url]];
  
  self.view = webView;
  [webView release];
}

/*
 // Implement viewDidLoad to do additional setup after loading the view, typically from a nib.
 - (void)viewDidLoad
 {
 [super viewDidLoad];
 }
 */

- (void)viewDidUnload
{
  [super viewDidUnload];
  // Release any retained subviews of the main view.
  self.view = nil;
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
  // Return YES for supported orientations
  return YES;
}

@end
