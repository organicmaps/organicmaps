#import "Common.h"
#import "Macros.h"
#import "MWMSearchHistoryClearCell.h"
#import "MWMSearchHistoryManager.h"
#import "MWMSearchHistoryRequestCell.h"

#include "Framework.h"

static NSString * const kRequestCellIdentifier = @"MWMSearchHistoryRequestCell";
static NSString * const kClearCellIdentifier = @"MWMSearchHistoryClearCell";

@interface MWMSearchHistoryManager ()

@property (weak, nonatomic) MWMSearchTabbedCollectionViewCell * cell;

@property (nonatomic) MWMSearchHistoryRequestCell * sizingCell;

@end

@implementation MWMSearchHistoryManager

- (void)attachCell:(MWMSearchTabbedCollectionViewCell *)cell
{
  self.cell = cell;
  UITableView * tableView = cell.tableView;
  tableView.alpha = cell.noResultsView.alpha = 1.0;
  if (GetFramework().GetLastSearchQueries().empty())
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
  return @([self queryAtIndex:index].second.c_str());
}

#pragma mark - UITableViewDataSource

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  return GetFramework().GetLastSearchQueries().size() + 1;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  if (indexPath.row < GetFramework().GetLastSearchQueries().size())
    return [tableView dequeueReusableCellWithIdentifier:kRequestCellIdentifier];
  else
    return [tableView dequeueReusableCellWithIdentifier:kClearCellIdentifier];
}

#pragma mark - UITableViewDelegate

- (CGFloat)tableView:(UITableView *)tableView estimatedHeightForRowAtIndexPath:(NSIndexPath *)indexPath
{
  if (indexPath.row < GetFramework().GetLastSearchQueries().size())
    return MWMSearchHistoryRequestCell.defaultCellHeight;
  else
    return MWMSearchHistoryClearCell.cellHeight;
}

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath
{
  if (indexPath.row < GetFramework().GetLastSearchQueries().size())
  {
    [self.sizingCell config:[self stringAtIndex:indexPath.row]];
    return self.sizingCell.cellHeight;
  }
  else
    return MWMSearchHistoryClearCell.cellHeight;
}

- (void)tableView:(UITableView *)tableView willDisplayCell:(MWMSearchHistoryRequestCell *)cell
forRowAtIndexPath:(NSIndexPath *)indexPath
{
  if (indexPath.row < GetFramework().GetLastSearchQueries().size())
    [cell config:[self stringAtIndex:indexPath.row]];
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  Framework & f = GetFramework();
  if (indexPath.row < f.GetLastSearchQueries().size())
  {
    search::QuerySaver::TSearchRequest const & query = [self queryAtIndex:indexPath.row];
    [self.delegate searchText:@(query.second.c_str()) forInputLocale:@(query.first.c_str())];
  }
  else
  {
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
