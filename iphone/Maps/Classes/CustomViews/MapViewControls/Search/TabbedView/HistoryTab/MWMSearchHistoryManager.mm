#import "MWMSearchHistoryManager.h"
#import "Common.h"
#import "MWMLocationManager.h"
#import "MWMSearchHistoryClearCell.h"
#import "MWMSearchHistoryMyPositionCell.h"
#import "MWMSearchHistoryRequestCell.h"
#import "Macros.h"
#import "MapsAppDelegate.h"
#import "Statistics.h"

#include "Framework.h"

static NSString * const kRequestCellIdentifier = @"MWMSearchHistoryRequestCell";
static NSString * const kClearCellIdentifier = @"MWMSearchHistoryClearCell";
static NSString * const kMyPositionCellIdentifier = @"MWMSearchHistoryMyPositionCell";

@interface MWMSearchHistoryManager ()

@property(weak, nonatomic) MWMSearchTabbedCollectionViewCell * cell;

@property(nonatomic) MWMSearchHistoryRequestCell * sizingCell;

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
  tableView.alpha = cell.noResultsView.alpha = 1.0;
  BOOL const isRouteSearch = self.isRouteSearchMode;
  if (GetFramework().GetLastSearchQueries().empty() && !isRouteSearch)
  {
    tableView.hidden = YES;
    cell.noResultsView.hidden = NO;
  }
  else
  {
    cell.noResultsView.hidden = YES;
    tableView.hidden = NO;
    tableView.delegate = self;
    tableView.dataSource = self;
    [tableView registerNib:[UINib nibWithNibName:kRequestCellIdentifier bundle:nil]
        forCellReuseIdentifier:kRequestCellIdentifier];
    [tableView registerNib:[UINib nibWithNibName:kClearCellIdentifier bundle:nil]
        forCellReuseIdentifier:kClearCellIdentifier];
    if (isRouteSearch)
    {
      [tableView registerNib:[UINib nibWithNibName:kMyPositionCellIdentifier bundle:nil]
          forCellReuseIdentifier:kMyPositionCellIdentifier];
    }
    [tableView reloadData];
  }
  cell.noResultsImage.image = [UIImage imageNamed:@"img_search_history"];
  cell.noResultsTitle.text = L(@"search_history_title");
  cell.noResultsText.text = L(@"search_history_text");
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
    return [tableView dequeueReusableCellWithIdentifier:kRequestCellIdentifier];
  else
    return [tableView dequeueReusableCellWithIdentifier:self.isRouteSearchMode
                                                            ? kMyPositionCellIdentifier
                                                            : kClearCellIdentifier];
}

#pragma mark - UITableViewDelegate

- (CGFloat)tableView:(UITableView *)tableView
    estimatedHeightForRowAtIndexPath:(NSIndexPath *)indexPath
{
  if ([self isRequestCell:indexPath])
    return MWMSearchHistoryRequestCell.defaultCellHeight;
  else
    return self.isRouteSearchMode ? MWMSearchHistoryMyPositionCell.cellHeight
                                  : MWMSearchHistoryClearCell.cellHeight;
}

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath
{
  NSUInteger const row = indexPath.row;
  if ([self isRequestCell:indexPath])
  {
    [self.sizingCell config:[self stringAtIndex:row]];
    return self.sizingCell.cellHeight;
  }
  else
    return self.isRouteSearchMode ? MWMSearchHistoryMyPositionCell.cellHeight
                                  : MWMSearchHistoryClearCell.cellHeight;
}

- (void)tableView:(UITableView *)tableView
      willDisplayCell:(UITableViewCell *)cell
    forRowAtIndexPath:(NSIndexPath *)indexPath
{
  if (![cell isKindOfClass:[MWMSearchHistoryRequestCell class]])
    return;
  MWMSearchHistoryRequestCell * tCell = static_cast<MWMSearchHistoryRequestCell *>(cell);
  [tCell config:[self stringAtIndex:indexPath.row]];
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  BOOL const isRouteSearch = self.isRouteSearchMode;
  if ([self isRequestCell:indexPath])
  {
    search::QuerySaver::TSearchRequest const & query =
        [self queryAtIndex:isRouteSearch ? indexPath.row - 1 : indexPath.row];
    NSString * queryText = @(query.second.c_str());
    [Statistics logEvent:kStatEventName(kStatSearch, kStatSelectResult)
          withParameters:@{kStatValue : queryText, kStatScreen : kStatHistory}];
    [self.delegate searchText:queryText forInputLocale:@(query.first.c_str())];
  }
  else
  {
    if (isRouteSearch)
    {
      [Statistics logEvent:kStatEventName(kStatSearch, kStatSelectResult)
            withParameters:@{kStatValue : kStatMyPosition, kStatScreen : kStatHistory}];
      [self.delegate tapMyPositionFromHistory];
      return;
    }
    [Statistics logEvent:kStatEventName(kStatSearch, kStatSelectResult)
          withParameters:@{kStatValue : kStatClear, kStatScreen : kStatHistory}];
    GetFramework().ClearSearchHistory();
    MWMSearchTabbedCollectionViewCell * cell = self.cell;
    [UIView animateWithDuration:kDefaultAnimationDuration
        animations:^{
          cell.tableView.alpha = 0.0;
          cell.noResultsView.alpha = 1.0;
        }
        completion:^(BOOL finished) {
          cell.tableView.hidden = YES;
          cell.noResultsView.hidden = NO;
        }];
  }
}

#pragma mark - Properties

- (MWMSearchHistoryRequestCell *)sizingCell
{
  if (!_sizingCell)
    _sizingCell = [self.cell.tableView dequeueReusableCellWithIdentifier:kRequestCellIdentifier];
  return _sizingCell;
}

@end
