#import "Common.h"
#import "LocationManager.h"
#import "Macros.h"
#import "MapsAppDelegate.h"
#import "MWMSearchHistoryClearCell.h"
#import "MWMSearchHistoryManager.h"
#import "MWMSearchHistoryMyPositionCell.h"
#import "MWMSearchHistoryRequestCell.h"
#import "Statistics.h"

#include "Framework.h"

static NSString * const kRequestCellIdentifier = @"MWMSearchHistoryRequestCell";
static NSString * const kClearCellIdentifier = @"MWMSearchHistoryClearCell";
static NSString * const kMyPositionCellIdentifier = @"MWMSearchHistoryMyPositionCell";

@interface MWMSearchHistoryManager ()

@property (weak, nonatomic) MWMSearchTabbedCollectionViewCell * cell;

@property (nonatomic) MWMSearchHistoryRequestCell * sizingCell;

@end

@implementation MWMSearchHistoryManager

- (BOOL)isRouteSearchMode
{
  MWMRoutingPlaneMode const m = MapsAppDelegate.theApp.routingPlaneMode;
  return (m == MWMRoutingPlaneModeSearchSource ||
         m == MWMRoutingPlaneModeSearchDestination) &&
         MapsAppDelegate.theApp.m_locationManager.lastLocationIsValid;
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
  cell.noResultsImage.image = [UIImage imageNamed:@"img_no_history_light"];
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

#pragma mark - UITableViewDataSource

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  return GetFramework().GetLastSearchQueries().size() + 1;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  NSUInteger const row = indexPath.row;
  BOOL const isRouteSearch = self.isRouteSearchMode;
  BOOL const isRequestCell = isRouteSearch ? row > 0 : row < GetFramework().GetLastSearchQueries().size();
  if (isRequestCell)
    return [tableView dequeueReusableCellWithIdentifier:kRequestCellIdentifier];
  else
    return [tableView dequeueReusableCellWithIdentifier:isRouteSearch ? kMyPositionCellIdentifier : kClearCellIdentifier];
}

#pragma mark - UITableViewDelegate

- (CGFloat)tableView:(UITableView *)tableView estimatedHeightForRowAtIndexPath:(NSIndexPath *)indexPath
{
  NSUInteger const row = indexPath.row;
  BOOL const isRouteSearch = self.isRouteSearchMode;
  BOOL const isRequestCell = isRouteSearch ? row > 0 : row < GetFramework().GetLastSearchQueries().size();
  if (isRequestCell)
    return MWMSearchHistoryRequestCell.defaultCellHeight;
  else
    return isRouteSearch ? MWMSearchHistoryMyPositionCell.cellHeight : MWMSearchHistoryClearCell.cellHeight;
}

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath
{
  NSUInteger const row = indexPath.row;
  BOOL const isRouteSearch = self.isRouteSearchMode;
  BOOL const isRequestCell = isRouteSearch ? row > 0 : row < GetFramework().GetLastSearchQueries().size();
  if (isRequestCell)
  {
    [self.sizingCell config:[self stringAtIndex:row]];
    return self.sizingCell.cellHeight;
  }
  else
    return isRouteSearch ? MWMSearchHistoryMyPositionCell.cellHeight : MWMSearchHistoryClearCell.cellHeight;
}

- (void)tableView:(UITableView *)tableView willDisplayCell:(MWMSearchHistoryRequestCell *)cell
forRowAtIndexPath:(NSIndexPath *)indexPath
{
  size_t const size = GetFramework().GetLastSearchQueries().size() + (self.isRouteSearchMode ? 1 : 0);
  NSUInteger const row = indexPath.row;
  BOOL const isRequestCell = self.isRouteSearchMode ? row != 0 : row < size;
  if (isRequestCell)
    [cell config:[self stringAtIndex:indexPath.row]];
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  Framework & f = GetFramework();
  NSUInteger const row = indexPath.row;
  BOOL const isRouteSearch = self.isRouteSearchMode;
  BOOL const isRequestCell = isRouteSearch ? row != 0 : row < f.GetLastSearchQueries().size();
  if (isRequestCell)
  {
    search::QuerySaver::TSearchRequest const & query = [self queryAtIndex:isRouteSearch ? indexPath.row - 1 : indexPath.row];
    NSString * queryText = @(query.second.c_str());
    [[Statistics instance] logEvent:kStatEventName(kStatSearch, kStatSelectResult)
                     withParameters:@{kStatValue : queryText, kStatScreen : kStatHistory}];
    [self.delegate searchText:queryText forInputLocale:@(query.first.c_str())];
  }
  else
  {
    if (isRouteSearch)
    {
      [[Statistics instance] logEvent:kStatEventName(kStatSearch, kStatSelectResult)
                       withParameters:@{kStatValue : kStatMyPosition, kStatScreen : kStatHistory}];
      [self.delegate tapMyPositionFromHistory];
      return;
    }
    [[Statistics instance] logEvent:kStatEventName(kStatSearch, kStatSelectResult)
                     withParameters:@{kStatValue : kStatClear, kStatScreen : kStatHistory}];
    f.ClearSearchHistory();
    MWMSearchTabbedCollectionViewCell * cell = self.cell;
    [UIView animateWithDuration:kDefaultAnimationDuration animations:^
    {
      cell.tableView.alpha = 0.0;
      cell.noResultsView.alpha = 1.0;
    }
    completion:^(BOOL finished)
    {
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
