#import "MWMSearchCategoriesManager.h"
#import "AppInfo.h"
#import "MWMSearchCategoryCell.h"
#import "Statistics.h"
#import "SwiftBridge.h"

#include "search/displayed_categories.hpp"

#include "base/macros.hpp"

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
  [cell removeNoResultsView];
  UITableView * tableView = cell.tableView;
  tableView.estimatedRowHeight = 44.;
  tableView.rowHeight = UITableViewAutomaticDimension;
  tableView.alpha = 1.0;
  tableView.hidden = NO;
  tableView.delegate = self;
  tableView.dataSource = self;
  [tableView registerWithCellClass:[MWMSearchCategoryCell class]];
  [tableView reloadData];
}

#pragma mark - UITableViewDataSource

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  return m_categories.size();
}

- (UITableViewCell *)tableView:(UITableView *)tableView
         cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  auto tCell = static_cast<MWMSearchCategoryCell *>([tableView
      dequeueReusableCellWithCellClass:[MWMSearchCategoryCell class]
                             indexPath:indexPath]);
  [tCell setCategory:@(m_categories[indexPath.row].c_str())];
  return tCell;
}

#pragma mark - UITableViewDelegate

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  NSString * string = @(m_categories[indexPath.row].c_str());
  [Statistics logEvent:kStatEventName(kStatSearch, kStatSelectResult)
        withParameters:@{kStatValue : string, kStatScreen : kStatCategories}];
  id<MWMSearchTabbedViewProtocol> delegate = self.delegate;
  [delegate searchText:[L(string) stringByAppendingString:@" "]
        forInputLocale:[[AppInfo sharedInfo] languageId]];
  [delegate dismissKeyboard];
}

@end
