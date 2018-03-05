#import "MWMSearchCategoriesManager.h"
#import <MyTrackerSDK/MRMyTracker.h>
#import "MWMSearchCategoryCell.h"
#import "Statistics.h"
#import "SwiftBridge.h"

#include "Framework.h"

extern NSString * const kLuggageCategory = @"luggagehero";

@implementation MWMSearchCategoriesManager
{
  vector<string> m_categories;
}

- (void)attachCell:(MWMSearchTabbedCollectionViewCell *)cell
{
  if (m_categories.empty())
    m_categories = GetFramework().GetDisplayedCategories().GetKeys();
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

- (void)resetCategories { m_categories.clear(); }
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

- (void)tableView:(UITableView *)tableView
      willDisplayCell:(UITableViewCell *)cell
    forRowAtIndexPath:(NSIndexPath *)indexPath
{
  NSString * string = @(m_categories[indexPath.row].c_str());
  if ([string isEqualToString:kLuggageCategory])
  {
    [MRMyTracker trackEventWithName:@"Search_SponsoredCategory_shown_LuggageHero"];
    [Statistics logEvent:kStatSearchSponsoredShow withParameters:@{kStatProvider : kStatLuggageHero}];
  }
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  NSString * string = @(m_categories[indexPath.row].c_str());
  [Statistics logEvent:kStatEventName(kStatSearch, kStatSelectResult)
        withParameters:@{kStatValue : string, kStatScreen : kStatCategories}];
  id<MWMSearchTabbedViewProtocol> delegate = self.delegate;
  [delegate searchText:[L(string) stringByAppendingString:@" "]
        forInputLocale:[[AppInfo sharedInfo] languageId]];
  [delegate dismissKeyboard];
  if ([string isEqualToString:kLuggageCategory])
  {
    if (!IPAD)
      delegate.state = MWMSearchManagerStateMapSearch;
    [MRMyTracker trackEventWithName:@"Search_SponsoredCategory_selected_LuggageHero"];
    [Statistics logEvent:kStatSearchSponsoredSelect withParameters:@{kStatProvider : kStatLuggageHero}];
  }
}

@end
