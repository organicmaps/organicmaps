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

- (void)dealloc
{
  delete m_pSloynikData;
  [searchBar release];
  [resultsView release];

  [super dealloc];
}

// Implement viewDidLoad to do additional setup after loading the view, typically from a nib.
- (void)viewDidLoad
{
  [super viewDidLoad];

  self.searchBar.delegate = self;

  self.resultsView.dataSource = self;
  self.resultsView.delegate = self;
  
  m_pSloynikData = new SloynikData;
  GetSloynikEngine()->Search("", m_pSloynikData->m_SearchResult);

  [self onEmptySearch];
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
  self.searchBar = nil;
  self.resultsView = nil;
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
    [self onEmptySearch];
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
    ArticleVC * articleVC = [[[ArticleVC alloc] init] autorelease];
    [self willShowArticleVC:articleVC];
    [articleVC setArticleById:wordId];

    [self.resultsView deselectRowAtIndexPath:[self.resultsView indexPathForSelectedRow] animated:NO];

    CATransition * animation = [CATransition animation];
    animation.duration = 0.2;
    animation.type = kCATransitionPush;
    animation.subtype = kCATransitionFromRight;
    animation.timingFunction =
    [CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseInEaseOut];
    [[self.view.superview layer] addAnimation:animation forKey:@"SwitchToArticleView"];

    [self presentModalViewController:articleVC animated:NO];
  }
}

- (void)willShowArticleVC:(ArticleVC *) articleVC
{
}

- (void)onEmptySearch
{
  self.resultsView.hidden = YES;
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
  return YES;  // All orientations are supported.
}

@end
