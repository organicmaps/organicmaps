#import "Macros.h"
#import "MWMSearchCategoriesManager.h"
#import "MWMSearchCategoryCell.h"
#import "Statistics.h"
#import "AppInfo.h"

#include "search/displayed_categories.hpp"

#include "base/macros.hpp"

static NSString * const kCellIdentifier = @"MWMSearchCategoryCell";

@implementation MWMSearchCategoriesManager
{
  vector<string> m_categories;
}

- (instancetype)init
{
  self = [super init];
  if (self)
    m_categories = search::DisplayedCategories::GetKeys();
  return self;
}

- (void)attachCell:(MWMSearchTabbedCollectionViewCell *)cell
{
  cell.noResultsView.hidden = YES;
  UITableView * tableView = cell.tableView;
  tableView.alpha = 1.0;
  tableView.hidden = NO;
  tableView.delegate = self;
  tableView.dataSource = self;
  [tableView registerNib:[UINib nibWithNibName:kCellIdentifier bundle:nil]
  forCellReuseIdentifier:kCellIdentifier];
  [tableView reloadData];
}

#pragma mark - UITableViewDataSource

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  return m_categories.size();
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  return [tableView dequeueReusableCellWithIdentifier:kCellIdentifier];
}

#pragma mark - UITableViewDelegate

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath
{
  return 44.0;
}

- (void)tableView:(UITableView *)tableView willDisplayCell:(MWMSearchCategoryCell *)cell
forRowAtIndexPath:(NSIndexPath *)indexPath
{
  [cell setCategory:@(m_categories[indexPath.row].c_str())];
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  NSString * string = @(m_categories[indexPath.row].c_str());
  [Statistics logEvent:kStatEventName(kStatSearch, kStatSelectResult)
                   withParameters:@{kStatValue : string, kStatScreen : kStatCategories}];
  [self.delegate searchText:[L(string) stringByAppendingString:@" "] forInputLocale:[[AppInfo sharedInfo] languageId]];
}

@end
