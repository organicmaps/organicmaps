#import "MWMSearchCategoriesManager.h"
#import <MyTrackerSDK/MRMyTracker.h>
#import "MWMSearchCategoryCell.h"
#import "Statistics.h"
#import "SwiftBridge.h"

#include "Framework.h"

static NSInteger const kRutaxiIndex = 6;

@interface MWMSearchCategoriesManager () <MWMSearchBannerCellDelegate>

@end

@implementation MWMSearchCategoriesManager
{
  vector<string> m_categories;
  bool m_rutaxi;
}

- (void)attachCell:(MWMSearchTabbedCollectionViewCell *)cell
{
  if (m_categories.empty())
  {
    m_categories = GetFramework().GetDisplayedCategories().GetKeys();
    m_rutaxi = GetFramework().HasRuTaxiCategoryBanner();
  }
  [cell removeNoResultsView];
  UITableView * tableView = cell.tableView;
  tableView.estimatedRowHeight = 44.;
  tableView.rowHeight = UITableViewAutomaticDimension;
  tableView.alpha = 1.0;
  tableView.hidden = NO;
  tableView.delegate = self;
  tableView.dataSource = self;
  [tableView registerWithCellClass:[MWMSearchCategoryCell class]];
  [tableView registerWithCellClass:[MWMSearchBannerCell class]];
  [tableView reloadData];
}

- (void)resetCategories
{
  m_categories.clear();
  m_rutaxi = false;
}

#pragma mark - UITableViewDataSource

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  return m_categories.size() + (m_rutaxi ? 1 : 0);
}

- (UITableViewCell *)tableView:(UITableView *)tableView
         cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  if (m_rutaxi && indexPath.row == kRutaxiIndex)
  {
    auto cell = static_cast<MWMSearchBannerCell *>([tableView
                                                    dequeueReusableCellWithCellClass:[MWMSearchBannerCell class]
                                                    indexPath:indexPath]);
    cell.delegate = self;
    return cell;
  }

  auto tCell = static_cast<MWMSearchCategoryCell *>([tableView
      dequeueReusableCellWithCellClass:[MWMSearchCategoryCell class]
                             indexPath:indexPath]);
  [tCell setCategory:@(m_categories[[self adjustedIndex:indexPath.row]].c_str())];
  return tCell;
}

- (NSInteger)adjustedIndex:(NSInteger)index {
  if (m_rutaxi)
    return index > kRutaxiIndex ? index - 1 : index;
  else
    return index;
}

#pragma mark - UITableViewDelegate

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  NSString * string = @(m_categories[[self adjustedIndex:indexPath.row]].c_str());
  auto query = [L(string) stringByAppendingString:@" "];

  [Statistics logEvent:kStatEventName(kStatSearch, kStatSelectResult)
        withParameters:@{kStatValue : string, kStatScreen : kStatCategories}];
  id<MWMSearchTabbedViewProtocol> delegate = self.delegate;
  [delegate searchText:query forInputLocale:[[AppInfo sharedInfo] languageId]];
  [delegate dismissKeyboard];
}

- (NSIndexPath *)tableView:(UITableView *)tableView willSelectRowAtIndexPath:(NSIndexPath *)indexPath {
  if (m_rutaxi && indexPath.row == kRutaxiIndex)
    return nil;

  return indexPath;
}

#pragma mark - MWMSearchBannerCellDelegate

- (void)cellDidPressAction:(MWMSearchBannerCell *)cell
{
  [[UIApplication sharedApplication] openURL:
   [NSURL URLWithString:@"https://go.onelink.me/2944814706/86db6339"]];
}

- (void)cellDidPressClose:(MWMSearchBannerCell *)cell
{
  [[MapViewController sharedController] showRemoveAds];
}

@end
