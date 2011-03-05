#import "SearchVC.h"
#import "ArticleVC.h"
#import "PerfCount.h"
#import <QuartzCore/QuartzCore.h>
#include "global.hpp"
#include "../../words/sloynik_engine.hpp"
#include "../../base/assert.hpp"
#include "../../std/string.hpp"

struct SloynikData
{
  sl::SloynikEngine::SearchResult m_SearchResult;
  
  SloynikData() : m_SearchResult()
  {
  }
  
  ~SloynikData()
  {
  }
};

@implementation SearchVC

@synthesize searchBar;
@synthesize resultsView;
@synthesize menuButton;
@synthesize articleVC;

- (void)dealloc
{
  delete m_pSloynikData;
  self.searchBar = nil;
  self.resultsView = nil;
  self.menuButton = nil;
  self.articleVC = nil;
  [super dealloc];
}

// Implement loadView to create a view hierarchy programmatically, without using a nib.
- (void)loadView
{
  int const buttonW = 40;
  int const toolbarH = 44;
  int const marginW = 5;
  CGRect frame = [[UIScreen mainScreen] applicationFrame];
  int const frameW = frame.size.width;
  int const frameH = frame.size.height;

  CGRect searchBarFrame = CGRectMake(0, 0, frameW - buttonW - 2 * marginW, toolbarH);
  CGRect navBarFrame = CGRectMake(frameW - buttonW - 2 * marginW, 0,
                                  buttonW + 2 * marginW, toolbarH);
  CGRect resultsFrame = CGRectMake(0, toolbarH, frameW, frameH - toolbarH);

  self.searchBar = [[[UISearchBar alloc] initWithFrame:searchBarFrame] autorelease];
  self.searchBar.delegate = self;
  self.searchBar.autoresizingMask = UIViewAutoresizingFlexibleWidth;
  [self.searchBar becomeFirstResponder];

  self.menuButton = [[UIBarButtonItem alloc] initWithImage:[UIImage imageNamed:@"menu-20.png"]
                                                     style:UIBarButtonItemStylePlain
                                                    target:self
                                                    action:@selector(menuButtonPressed)];

  UINavigationItem * navItem = [[[UINavigationItem alloc] initWithTitle:@""] autorelease];
  navItem.rightBarButtonItem = self.menuButton;

  UINavigationBar * navBar = [[[UINavigationBar alloc] initWithFrame:navBarFrame] autorelease];
  navBar.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin;
  [navBar pushNavigationItem:navItem animated:NO];

  self.resultsView = [[[UITableView alloc] initWithFrame:resultsFrame] autorelease];
  self.resultsView.rowHeight = 40;
  self.resultsView.autoresizingMask = (UIViewAutoresizingFlexibleWidth |
                                       UIViewAutoresizingFlexibleHeight);
  self.resultsView.dataSource = self;
  self.resultsView.delegate = self;
  self.resultsView.hidden = YES;

  UIView * mainView = [[[UIView alloc] initWithFrame:frame] autorelease];
  [mainView addSubview:self.searchBar];
  [mainView addSubview:navBar];
  [mainView addSubview:self.resultsView];
  self.view = mainView;
}

// Implement viewDidLoad to do additional setup after loading the view, typically from a nib.
- (void)viewDidLoad
{
  [super viewDidLoad];
  LogTimeCounter("StartTime", "SearchVC initializing.");
  m_pSloynikData = new SloynikData;
  GetSloynikEngine()->Search("", m_pSloynikData->m_SearchResult);
  LogTimeCounter("StartTime", "SearchVC initialized.");
}

// Override to allow orientations other than the default portrait orientation.
- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
  // Return YES for supported orientations
  return YES;
}

- (void)didReceiveMemoryWarning
{
  // Releases the view if it doesn't have a superview.
  [super didReceiveMemoryWarning];
  // Release any cached data, images, etc that aren't in use.
}

- (void)viewDidUnload
{
  [super viewDidUnload];
  // Release any retained subviews of the main view.
  // e.g. self.myOutlet = nil;
}

- (void)searchBar:(UISearchBar *)sender textDidChange:(NSString *)searchText
{
  ASSERT_EQUAL(self.searchBar, sender, ());
  if ([searchText length] != 0)
  {
    self.resultsView.hidden = NO;
    GetSloynikEngine()->Search([searchText UTF8String], m_pSloynikData->m_SearchResult);
    sl::SloynikEngine::WordId row = m_pSloynikData->m_SearchResult.m_FirstMatched;
    ASSERT_LESS_OR_EQUAL(row, GetSloynikEngine()->WordCount(), ());
    row = min(row, GetSloynikEngine()->WordCount() - 1);
    [self.resultsView scrollToRowAtIndexPath:[NSIndexPath indexPathForRow:row inSection:0]
                            atScrollPosition:UITableViewScrollPositionTop
                                    animated:NO];
    self.resultsView.contentOffset.y =
      self.resultsView.rowHeight * m_pSloynikData->m_SearchResult.m_FirstMatched;
  }
  else
    self.resultsView.hidden = YES;
}

- (void)scrollViewWillBeginDragging:(UIScrollView *)scrollView
{
  ASSERT_EQUAL(self.resultsView, scrollView, ());
  [self.searchBar resignFirstResponder];
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
  return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  ASSERT_EQUAL(self.resultsView, tableView, ());
  if (section != 0)
    return 0;
  int const rowsInView = self.resultsView.bounds.size.height / max(1.f, self.resultsView.rowHeight);
  return GetSloynikEngine()->WordCount() + max(1, rowsInView - 1);
}

- (UITableViewCell *)tableView:(UITableView *)tableView
         cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  UITableViewCell * cell = [tableView dequeueReusableCellWithIdentifier:@"MyTableViewCell"];
  if (!cell)
  {
    cell = [[[UITableViewCell alloc]
             initWithStyle:UITableViewCellStyleDefault reuseIdentifier:@"MyTableViewCell"]
            autorelease];
  }
  sl::SloynikEngine::WordId const wordId = indexPath.row;
  if (wordId < GetSloynikEngine()->WordCount())
  {
    sl::SloynikEngine::WordInfo wordInfo;
    GetSloynikEngine()->GetWordInfo(wordId, wordInfo);
    cell.textLabel.text = [NSString stringWithUTF8String:wordInfo.m_Word.c_str()];
  }
  else
  {
    cell.textLabel.text = @"";
  }

  return cell;
}

- (NSIndexPath *)tableView:(UITableView *)tableView
  willSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  if (indexPath.row < GetSloynikEngine()->WordCount())
    return indexPath;
  else
    return nil;
}


- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  sl::SloynikEngine::WordId const wordId = indexPath.row;
  if (wordId < GetSloynikEngine()->WordCount())
  {
    [self willShowArticle];
    [self.articleVC setArticleById:wordId];
    [self showArticle];
  }
}

- (void)willShowArticle
{
  if (!self.articleVC)
    self.articleVC = [[[ArticleVC alloc] init] autorelease];
}

- (void)showArticle
{
  [self.resultsView deselectRowAtIndexPath:[self.resultsView indexPathForSelectedRow] animated:NO];

  self.articleVC.searchVC = self;
  UIView * superView = self.view.superview;

  [self.articleVC viewWillAppear:YES];
  [self viewWillDisappear:YES];

  [self.view removeFromSuperview];
  [superView addSubview:[self.articleVC view]];

  [self viewDidDisappear:YES];
  [self.articleVC viewDidAppear:YES];

  CATransition * animation = [CATransition animation];
	animation.duration = 0.2;
  animation.type = kCATransitionPush;
	animation.subtype = kCATransitionFromRight;
  animation.timingFunction =
  [CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseInEaseOut];
	[[superView layer] addAnimation:animation forKey:@"SwitchToArticleView"];
}

- (void)menuButtonPressed
{
}

@end
