#import "ArticleVC.h"
#import <QuartzCore/QuartzCore.h>
#include "global.hpp"
#include "../../words/sloynik_engine.hpp"
#include "../../base/assert.hpp"
#include "../../base/logging.hpp"
#include "../../std/algorithm.hpp"


@implementation ArticleVC

@synthesize webView;
@synthesize navBar;
@synthesize navSearch;
@synthesize navArticle;
@synthesize pinchGestureRecognizer;
@synthesize articleFormat;
@synthesize previousView;

+ (NSURL *)baseURL
{
  return [NSURL URLWithString:@"http://m.wikitravel.org/en/"];
}

- (void)dealloc
{
  [webView release];
  [navBar release];
  [navSearch release];
  [navArticle release];
  [pinchGestureRecognizer release];
  [articleFormat release];
  [super dealloc];
}

- (id)initWithPreviousView:(UIView *)prevView
{
  if ((self = [super initWithNibName:nil bundle:nil]))
  {
    self.articleFormat = @"<html><body style='-webkit-text-size-adjust:%d%%'>%@</body></html>";
    self.previousView = prevView;
  }
  return self;
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

  self.navBar = [[[UINavigationBar alloc] initWithFrame:navBarFrame] autorelease];
  self.navBar.delegate = self;
  self.navBar.autoresizingMask = UIViewAutoresizingFlexibleWidth;
  self.navSearch  = [[[UINavigationItem alloc] initWithTitle:@"Search"] autorelease];
  self.navArticle = [[[UINavigationItem alloc] initWithTitle:@""] autorelease];
  [self.navBar pushNavigationItem:navSearch  animated:NO];
  [self.navBar pushNavigationItem:navArticle animated:NO];

  if (NSClassFromString(@"UIPinchGestureRecognizer"))
  {
    self.pinchGestureRecognizer = [[[UIPinchGestureRecognizer alloc]
                                    initWithTarget:self action:@selector(onPinch:)]
                                   autorelease];
    self.pinchGestureRecognizer.delegate = self;
  }

  CGRect webViewFrame = CGRectMake(0, toolbarH, frame.size.width, frame.size.height - toolbarH);
  self.webView = [[[UIWebView alloc] initWithFrame:webViewFrame] autorelease];
  self.webView.delegate = self;
  self.webView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;

  if (self.pinchGestureRecognizer)
    [self.webView addGestureRecognizer:self.pinchGestureRecognizer];

  UIView * mainView = [[[UIView alloc] initWithFrame:frame] autorelease];
  mainView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
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
  self.navSearch = nil;
  self.navArticle = nil;
  self.pinchGestureRecognizer = nil;
  [super viewDidUnload];
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
  return YES;  // All orientations are supported.
}

- (BOOL)navigationBar:(UINavigationBar *)navigationBar shouldPopItem:(UINavigationItem *)item
{
  // Clear webView.
  [self.webView loadHTMLString:@"" baseURL:[ArticleVC baseURL]];

  UIView * superView = self.view.superview;

  CATransition * animation = [CATransition animation];
  animation.duration = 0.2;
  animation.type = kCATransitionPush;
  NSString * direction = nil;
  switch (self.interfaceOrientation)
  {
    case UIInterfaceOrientationPortrait: direction = kCATransitionFromLeft; break;
    case UIInterfaceOrientationPortraitUpsideDown: direction = kCATransitionFromRight; break;
    case UIInterfaceOrientationLandscapeLeft: direction = kCATransitionFromTop; break;
    case UIInterfaceOrientationLandscapeRight: direction = kCATransitionFromBottom; break;
  }
  animation.subtype = direction;
  animation.timingFunction = [CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseInEaseOut];
  [[superView layer] addAnimation:animation forKey:@"SwitchToSearchView"];

  [UIView transitionFromView:self.view
                      toView:self.previousView
                    duration:0
                     options:UIViewAnimationOptionTransitionNone
                  completion:nil];

  return NO;
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

  // Make sure, that WebView is created.
  [self view];

  // Adjust NavigationBar.
  self.navArticle.title = [NSString stringWithUTF8String:wordInfo.m_Word.c_str()];

  [self.webView
   loadHTMLString:[NSString stringWithFormat:
                   self.articleFormat,
                   [self textSizeAdjustment],
                   [NSString stringWithUTF8String:data.m_HTML.c_str()]]
   baseURL:[[ArticleVC baseURL] URLByAppendingPathComponent:@"URLbySetArticleById"]];
}

- (BOOL)webView:(UIWebView *)webView shouldStartLoadWithRequest:(NSURLRequest *)request
 navigationType:(UIWebViewNavigationType)navigationType
{
  NSURL * nsURL = [request URL];
  char const * fullURLDescription = [[nsURL description] UTF8String];
  LOG(LINFO, ("Processing URL:", fullURLDescription));

  if ([[nsURL scheme] caseInsensitiveCompare:@"http"] != NSOrderedSame)
    return YES;

  if ([[nsURL host] rangeOfString:@"m.wikitravel.org"
                           options:(NSCaseInsensitiveSearch | NSBackwardsSearch | NSAnchoredSearch)
        ].location == NSNotFound)
    return YES;

  char const * urlChars = [[nsURL path] UTF8String];
  if (!urlChars)
  {
    LOG(LWARNING, ("Strange URL: path is empty."));
    return YES;
  }
  string const url = urlChars;
  if (url.size() <= 4 || url.substr(0, 4) != "/en/")
  {
    LOG(LWARNING, ("Strange URL:", url));
    return YES;
  }

  string articleName = url.substr(4);
  if (articleName == "URLbySetArticleById")
  {
    // URL set by setArticleById[].
    return YES;
  }

  replace(articleName.begin(), articleName.end(), '_', ' ');
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
