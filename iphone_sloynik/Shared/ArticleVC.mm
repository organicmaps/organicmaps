#import "ArticleVC.h"
#import "SearchVC.h"
#import <QuartzCore/QuartzCore.h>
#include "global.hpp"
#include "../../words/sloynik_engine.hpp"
#include "../../base/assert.hpp"
#include "../../base/logging.hpp"


@implementation ArticleVC

@synthesize searchVC;
@synthesize webView;
@synthesize navBar;
@synthesize navSearch;
@synthesize navArticle;
@synthesize swipeLeftGestureRecognizer;
@synthesize swipeRightGestureRecognizer;
@synthesize pinchGestureRecognizer;
@synthesize backForwardButtons;

- (void)dealloc
{
  self.searchVC = nil;
  self.webView = nil;
  self.navBar = nil;
  self.navSearch = nil;
  self.navArticle = nil;
  self.swipeLeftGestureRecognizer = nil;
  self.swipeRightGestureRecognizer = nil;
  self.pinchGestureRecognizer = nil;
  [super dealloc];
}

- (void)loadWebView
{
  self.webView = [[[UIWebView alloc] initWithFrame:m_webViewFrame] autorelease];
  self.webView.delegate = self;
  self.webView.autoresizingMask = UIViewAutoresizingFlexibleWidth;

  if (self.pinchGestureRecognizer)
    [self.webView addGestureRecognizer:self.pinchGestureRecognizer];
  if (self.swipeLeftGestureRecognizer)
    [self.webView addGestureRecognizer:self.swipeLeftGestureRecognizer];
  if (self.swipeRightGestureRecognizer)
    [self.webView addGestureRecognizer:self.swipeRightGestureRecognizer];
}

- (void)articleTransition:(unsigned int)newArticleId
                     type:(NSString *)type
                  subtype:(NSString *)subtype
{
  UIWebView * oldWebView = [[self.webView retain] autorelease];
  oldWebView.delegate = nil;
  [self loadWebView];

  [self setArticleById:newArticleId];

  [oldWebView removeFromSuperview];
  [self.view addSubview:self.webView];

  CATransition * animation = [CATransition animation];
  animation.duration = 0.2;
  animation.type = type;
  animation.subtype = subtype;
  animation.timingFunction =
  [CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseInEaseOut];
  [[self.view layer] addAnimation:animation forKey:@"SwitchToLeftArticleView"];
}

- (void)onSwipeLeft
{
  if (m_articleId + 1 < GetSloynikEngine()->WordCount())
    [self articleTransition:(m_articleId + 1)
                       type:kCATransitionReveal
                    subtype:kCATransitionFromRight];
}

- (void)onSwipeRight
{
  if (m_articleId > 0)
    [self articleTransition:(m_articleId-1)
                       type:kCATransitionMoveIn
                    subtype:kCATransitionFromLeft];
}

- (unsigned int)textSizeAdjustment
{
  if (m_fontScale == 0.0)
    m_fontScale = 1.0;
  return static_cast<int>(100 * m_fontScale);
}

- (void)onPinch:(UIPinchGestureRecognizer *)sender
{
  if (m_fontScale == 0.0)
    m_fontScale = 1.0;
  if (m_fontScaleOnPinchStart == 0.0 || sender.state == UIGestureRecognizerStateBegan)
    m_fontScaleOnPinchStart = m_fontScale;

  m_fontScale = min(4.0, max(0.25, m_fontScaleOnPinchStart * sender.scale));

  [webView stringByEvaluatingJavaScriptFromString:
   [NSString stringWithFormat:
    @"document.getElementsByTagName('body')[0]"
    ".style.webkitTextSizeAdjust= '%d%%'", [self textSizeAdjustment]]];
}

- (void)loadView
{
  int const toolbarH = 44;
  CGRect frame = [[UIScreen mainScreen] applicationFrame];
  CGRect navBarFrame = CGRectMake(0, 0, frame.size.width, toolbarH);
  m_webViewFrame = CGRectMake(0, toolbarH, frame.size.width, frame.size.height - toolbarH);

  self.navBar = [[[UINavigationBar alloc] initWithFrame:navBarFrame] autorelease];
  self.navBar.delegate = self;
  self.navBar.autoresizingMask = UIViewAutoresizingFlexibleWidth;
  self.navSearch  = [[UINavigationItem alloc] initWithTitle:@"Search"];
  self.navArticle = [[UINavigationItem alloc] initWithTitle:@""];
  [self.navBar pushNavigationItem:navSearch  animated:NO];
  [self.navBar pushNavigationItem:navArticle animated:NO];

  self.backForwardButtons = [[UISegmentedControl alloc] initWithItems:
                             [NSArray arrayWithObjects:
                              [UIImage imageNamed:@"back.png"],
                              [UIImage imageNamed:@"forw.png"],
                              nil]];
  self.backForwardButtons.segmentedControlStyle = UISegmentedControlStyleBar;
  self.backForwardButtons.momentary = YES;
  [self.backForwardButtons setEnabled:NO forSegmentAtIndex:0];
  [self.backForwardButtons setEnabled:NO forSegmentAtIndex:1];

  UIBarButtonItem * segmentBarItem = [[UIBarButtonItem alloc]
                                      initWithCustomView:self.backForwardButtons];
  self.navArticle.rightBarButtonItem = segmentBarItem;
  [segmentBarItem release];
  
  if ([NSClassFromString(@"UISwipeGestureRecognizer")
       instancesRespondToSelector:@selector(setDirection:)])
  {
    LOG(LINFO, ("Using", "UISwipeGestureRecognizer"));
    self.swipeLeftGestureRecognizer = [[[UISwipeGestureRecognizer alloc]
                                        initWithTarget:self action:@selector(onSwipeLeft)]
                                       autorelease];
    self.swipeRightGestureRecognizer = [[[UISwipeGestureRecognizer alloc]
                                         initWithTarget:self action:@selector(onSwipeRight)]
                                        autorelease];
    self.swipeLeftGestureRecognizer.direction = UISwipeGestureRecognizerDirectionLeft;
    self.swipeRightGestureRecognizer.direction = UISwipeGestureRecognizerDirectionRight;
    self.swipeLeftGestureRecognizer.delegate = self;
    self.swipeRightGestureRecognizer.delegate = self;
  }
  if (NSClassFromString(@"UIPinchGestureRecognizer"))
  {
    self.pinchGestureRecognizer = [[[UIPinchGestureRecognizer alloc]
                                    initWithTarget:self action:@selector(onPinch:)]
                                   autorelease];
    self.pinchGestureRecognizer.delegate = self;
  }
  
  [self loadWebView];

  UIView * mainView = [[[UIView alloc] initWithFrame:frame] autorelease];
  [mainView addSubview:self.navBar];
  [mainView addSubview:self.webView];
  self.view = mainView;
}

- (void)viewDidUnload
{
  // From the documentation:
  // Before releasing an instance of UIWebView for which you have set a delegate, you must first
  // set the UIWebView delegate property to nil before disposing of the UIWebView instance.
  // This can be done, for example, in the dealloc method where you dispose of the UIWebView.
  self.webView.delegate = nil;

  self.webView = nil;
  self.navBar = nil;
  [super viewDidUnload];
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
  return YES;  // All orientations are supported.
}

- (BOOL)navigationBar:(UINavigationBar *)navigationBar shouldPopItem:(UINavigationItem *)item
{
  UIView * superView = self.view.superview;

  [searchVC viewWillAppear:YES];
  [self viewWillDisappear:YES];

  [self.view removeFromSuperview];
  [superView addSubview:self.searchVC.view];

  [self viewDidDisappear:YES];
  [searchVC viewDidAppear:YES];

  CATransition * animation = [CATransition animation];
  animation.duration = 0.2;
  animation.type = kCATransitionPush;
  animation.subtype = kCATransitionFromLeft;
  animation.timingFunction =
      [CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseInEaseOut];
  [[superView layer] addAnimation:animation forKey:@"SwitchToSearchView"];

  self.searchVC = nil;
  return NO;
}

- (void)setArticleData:(NSString *)htmlString name:(NSString *)name
{
  // Make sure, that WebView is created.
  [self view];  

  // Load data into WebView.
  [self.webView loadHTMLString:htmlString baseURL:[NSURL URLWithString:@"http://s/"]];

  // Adjust NavigationBar.
  self.navArticle.title = name;
}

- (void)setArticleById:(unsigned int)articleId
{
  if (articleId >= GetSloynikEngine()->WordCount())
  {
    LOG(LERROR, ("ArticleId out of range", articleId));
    return;
  }

  m_articleId = articleId;

  sl::SloynikEngine::WordInfo wordInfo;
  GetSloynikEngine()->GetWordInfo(articleId, wordInfo);

  LOG(LINFO, ("Loading", wordInfo.m_Word));
  sl::SloynikEngine::ArticleData data;
  GetSloynikEngine()->GetArticleData(articleId, data);

  [self setArticleData:[NSString stringWithFormat:
                        @"<html><body style='-webkit-text-size-adjust:%d%%'>%s</body></html>",
                        [self textSizeAdjustment], data.m_HTML.c_str()]
                  name:[NSString stringWithUTF8String:wordInfo.m_Word.c_str()]];
}

- (BOOL)webView:(UIWebView *)webView shouldStartLoadWithRequest:(NSURLRequest *)request
 navigationType:(UIWebViewNavigationType)navigationType
{
  string const url([[[request URL] path] UTF8String]);
  size_t const lastSlash = url.find_last_of('/');
  if (lastSlash == string::npos)
  {
    LOG(LWARNING, ("Strange URL", url));
    return YES;
  }
  string const articleName = url.substr(lastSlash + 1);
  if (articleName.size() == 0 || articleName.empty())
  {
    // Loading article from SearchVC.
    return YES;
  }
  sl::SloynikEngine::SearchResult searchResult;
  GetSloynikEngine()->Search(articleName, searchResult);
  ASSERT_LESS_OR_EQUAL(searchResult.m_FirstMatched, GetSloynikEngine()->WordCount(), ());
  if (searchResult.m_FirstMatched >= GetSloynikEngine()->WordCount())
  {
    LOG(LWARNING, ("Article not found", url));
    return NO;
  }

  sl::SloynikEngine::WordInfo wordInfo;
  GetSloynikEngine()->GetWordInfo(searchResult.m_FirstMatched, wordInfo);
  if (wordInfo.m_Word != articleName)
  {
    LOG(LWARNING, ("Article not found", url));
    return NO;
  }

  [self setArticleById:searchResult.m_FirstMatched];
  return NO;
}

- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer
shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer *)otherGestureRecognize
{
  return YES;
}


@end
