#import "MWMSearchHistoryManager.h"
#import "MWMCommon.h"
#import "MWMLocationManager.h"
#import "MWMSearchHistoryClearCell.h"
#import "MWMSearchHistoryMyPositionCell.h"
#import "MWMSearchHistoryRequestCell.h"
#import "MWMSearchNoResults.h"
#import "MapsAppDelegate.h"
#import "Statistics.h"
#import "SwiftBridge.h"

#include "Framework.h"

@interface MWMSearchHistoryManager ()

@property(weak, nonatomic) MWMSearchTabbedCollectionViewCell * cell;

@property(nonatomic) MWMSearchNoResults * noResultsView;

@end

@implementation MWMSearchHistoryManager

- (BOOL)isRouteSearchMode
{
  MWMRoutingPlaneMode const m = MapsAppDelegate.theApp.routingPlaneMode;
  CLLocation * lastLocation = [MWMLocationManager lastLocation];
  return lastLocation &&
         (m == MWMRoutingPlaneModeSearchSource || m == MWMRoutingPlaneModeSearchDestination);
}

- (void)attachCell:(MWMSearchTabbedCollectionViewCell *)cell
{
  self.cell = cell;
  UITableView * tableView = cell.tableView;
  tableView.estimatedRowHeight = 44.;
  tableView.rowHeight = UITableViewAutomaticDimension;
  tableView.alpha = 1.0;
  BOOL const isRouteSearch = self.isRouteSearchMode;
  if (GetFramework().GetLastSearchQueries().empty() && !isRouteSearch)
  {
    tableView.hidden = YES;
    [cell addNoResultsView:self.noResultsView];
  }
  else
  {
    [cell removeNoResultsView];
    tableView.hidden = NO;
    tableView.delegate = self;
    tableView.dataSource = self;
    [tableView registerWithCellClass:[MWMSearchHistoryRequestCell class]];
    [tableView registerWithCellClass:[MWMSearchHistoryClearCell class]];
    if (isRouteSearch)
      [tableView registerWithCellClass:[MWMSearchHistoryMyPositionCell class]];
    [tableView reloadData];
  }
}

- (search::QuerySaver::TSearchRequest const &)queryAtIndex:(NSInteger)index
{
  Framework & f = GetFramework();
  NSAssert(index >= 0 && index < f.GetLastSearchQueries().size(), @"Invalid search history index");
  auto it = f.GetLastSearchQueries().cbegin();
  advance(it, index);
  return *it;
}

- (NSString *)stringAtIndex:(NSInteger)index
{
  NSUInteger const i = self.isRouteSearchMode ? index - 1 : index;
  return @([self queryAtIndex:i].second.c_str());
}

- (BOOL)isRequestCell:(NSIndexPath *)indexPath
{
  NSUInteger const row = indexPath.row;
  BOOL const isRouteSearch = self.isRouteSearchMode;
  return isRouteSearch ? row > 0 : row < GetFramework().GetLastSearchQueries().size();
}

#pragma mark - UITableViewDataSource

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  return GetFramework().GetLastSearchQueries().size() + 1;
}

- (UITableViewCell *)tableView:(UITableView *)tableView
         cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  if ([self isRequestCell:indexPath])
  {
    auto tCell = static_cast<MWMSearchHistoryRequestCell *>([tableView
        dequeueReusableCellWithCellClass:[MWMSearchHistoryRequestCell class]
                               indexPath:indexPath]);
    [tCell config:[self stringAtIndex:indexPath.row]];
    return tCell;
  }
  Class cls = self.isRouteSearchMode ? [MWMSearchHistoryMyPositionCell class]
                                     : [MWMSearchHistoryClearCell class];
  return [tableView dequeueReusableCellWithCellClass:cls indexPath:indexPath];
}

#pragma mark - UITableViewDelegate

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  BOOL const isRouteSearch = self.isRouteSearchMode;
  id<MWMSearchTabbedViewProtocol> delegate = self.delegate;
  if ([self isRequestCell:indexPath])
  {
    search::QuerySaver::TSearchRequest const & query =
        [self queryAtIndex:isRouteSearch ? indexPath.row - 1 : indexPath.row];
    NSString * queryText = @(query.second.c_str());
    [Statistics logEvent:kStatEventName(kStatSearch, kStatSelectResult)
          withParameters:@{kStatValue : queryText, kStatScreen : kStatHistory}];
    [delegate searchText:queryText forInputLocale:@(query.first.c_str())];
  }
  else
  {
    if (isRouteSearch)
    {
      [Statistics logEvent:kStatEventName(kStatSearch, kStatSelectResult)
            withParameters:@{kStatValue : kStatMyPosition, kStatScreen : kStatHistory}];
      [delegate tapMyPositionFromHistory];
      return;
    }
    [Statistics logEvent:kStatEventName(kStatSearch, kStatSelectResult)
          withParameters:@{kStatValue : kStatClear, kStatScreen : kStatHistory}];
    GetFramework().ClearSearchHistory();
    MWMSearchTabbedCollectionViewCell * cell = self.cell;
    [UIView animateWithDuration:kDefaultAnimationDuration
        animations:^{
          cell.tableView.alpha = 0.0;
        }
        completion:^(BOOL finished) {
          cell.tableView.hidden = YES;
          [cell addNoResultsView:self.noResultsView];
        }];
  }
}

#pragma mark - Properties

- (MWMSearchNoResults *)noResultsView
{
  if (!_noResultsView)
  {
    _noResultsView = [MWMSearchNoResults viewWithImage:[UIImage imageNamed:@"img_search_history"]
                                                 title:L(@"search_history_title")
                                                  text:L(@"search_history_text")];
  }
  return _noResultsView;
}

@end
