#import "LocationManager.h"
#import "Macros.h"
#import "MapsAppDelegate.h"
#import "MWMSearchCommonCell.h"
#import "MWMSearchShowOnMapCell.h"
#import "MWMSearchSuggestionCell.h"
#import "MWMSearchTableView.h"
#import "MWMSearchTableViewController.h"
#import "Statistics.h"
#import "ToastView.h"

#include "std/vector.hpp"

static NSString * const kTableShowOnMapCell = @"MWMSearchShowOnMapCell";
static NSString * const kTableSuggestionCell = @"MWMSearchSuggestionCell";
static NSString * const kTableCommonCell = @"MWMSearchCommonCell";

typedef NS_ENUM(NSUInteger, MWMSearchTableCellType)
{
  MWMSearchTableCellTypeOnMap,
  MWMSearchTableCellTypeSuggestion,
  MWMSearchTableCellTypeCommon
};

NSString * identifierForType(MWMSearchTableCellType type)
{
  switch (type)
  {
    case MWMSearchTableCellTypeOnMap:
      return kTableShowOnMapCell;
    case MWMSearchTableCellTypeSuggestion:
      return kTableSuggestionCell;
    case MWMSearchTableCellTypeCommon:
      return kTableCommonCell;
  }
}

@interface MWMSearchTableViewController () <UITableViewDataSource, UITableViewDelegate,
LocationObserver>

@property (weak, nonatomic) IBOutlet UITableView * tableView;

@property (nonatomic) BOOL watchLocationUpdates;

@property (nonatomic) MWMSearchCommonCell * commonSizingCell;

@property (weak, nonatomic) id<MWMSearchTableViewProtocol> delegate;

@end

@implementation MWMSearchTableViewController
{
  search::SearchParams searchParams;
  search::Results searchResults;
}

- (nonnull instancetype)initWithDelegate:(id<MWMSearchTableViewProtocol>)delegate
{
  self = [super init];
  if (self)
  {
    self.delegate = delegate;
    [self setupSearchParams];
  }
  return self;
}

- (search::SearchParams const &)searchParams
{
  return searchParams;
}

- (void)viewDidLoad
{
  [super viewDidLoad];
  [self setupTableView];
}

- (void)refresh
{
  [self.view refresh];
}

- (void)setupTableView
{
  [self.tableView registerNib:[UINib nibWithNibName:kTableShowOnMapCell bundle:nil]
       forCellReuseIdentifier:kTableShowOnMapCell];
  [self.tableView registerNib:[UINib nibWithNibName:kTableSuggestionCell bundle:nil]
       forCellReuseIdentifier:kTableSuggestionCell];
  [self.tableView registerNib:[UINib nibWithNibName:kTableCommonCell bundle:nil]
       forCellReuseIdentifier:kTableCommonCell];
}

- (void)setupSearchParams
{
  __weak auto weakSelf = self;
  searchParams.m_callback = ^(search::Results const & results)
  {
    __strong auto self = weakSelf;
    if (!self)
      return;
    dispatch_async(dispatch_get_main_queue(), [=]()
    {
      if (!results.IsEndMarker())
      {
        searchResults = results;
        [self updateSearchResultsInTable];
      }
      else if (results.IsEndedNormal())
      {
        [self completeSearch];
        if (IPAD)
          GetFramework().UpdateUserViewportChanged();
      }
    });
  };
}

- (MWMSearchTableCellType)cellTypeForIndexPath:(NSIndexPath *)indexPath
{
  size_t const numSuggests = searchResults.GetSuggestsCount();
  if (numSuggests > 0)
  {
    return indexPath.row < numSuggests ? MWMSearchTableCellTypeSuggestion : MWMSearchTableCellTypeCommon;
  }
  else
  {
    MWMRoutingPlaneMode const m = MapsAppDelegate.theApp.routingPlaneMode;
    if (IPAD || m == MWMRoutingPlaneModeSearchSource || m == MWMRoutingPlaneModeSearchDestination)
      return MWMSearchTableCellTypeCommon;
    else
      return indexPath.row == 0 ? MWMSearchTableCellTypeOnMap : MWMSearchTableCellTypeCommon;
  }
}

- (search::Result &)searchResultForIndexPath:(NSIndexPath *)indexPath
{
  MWMSearchTableCellType firstCellType =
      [self cellTypeForIndexPath:[NSIndexPath indexPathForRow:0 inSection:0]];
  size_t const searchPosition =
      indexPath.row - (firstCellType == MWMSearchTableCellTypeOnMap ? 1 : 0);
  return searchResults.GetResult(searchPosition);
}

#pragma mark - Layout

- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation
                                duration:(NSTimeInterval)duration
{
  dispatch_async(dispatch_get_main_queue(), ^
  {
    [self updateSearchResultsInTable];
  });
}

- (void)viewWillTransitionToSize:(CGSize)size
       withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator
{
  [coordinator animateAlongsideTransition:^(id<UIViewControllerTransitionCoordinatorContext> context)
  {
    [self updateSearchResultsInTable];
  }
  completion:nil];
}

#pragma mark - UITableViewDataSource

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  MWMSearchTableCellType firstCellType = [self cellTypeForIndexPath:[NSIndexPath indexPathForRow:0 inSection:0]];
  NSInteger const count = searchResults.GetCount();
  BOOL const showOnMap = firstCellType == MWMSearchTableCellTypeOnMap;
  return count + (showOnMap ? 1 : 0);
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  MWMSearchTableCellType const cellType = [self cellTypeForIndexPath:indexPath];
  return [tableView dequeueReusableCellWithIdentifier:identifierForType(cellType)];
}

#pragma mark - Config cells

- (void)configSuggestionCell:(MWMSearchSuggestionCell *)cell result:(search::Result &)result
                  isLastCell:(BOOL)isLastCell
{
  [cell config:result];
  cell.isLastCell = isLastCell;
}

#pragma mark - UITableViewDelegate

- (CGFloat)tableView:(UITableView *)tableView estimatedHeightForRowAtIndexPath:(NSIndexPath *)indexPath
{
  switch ([self cellTypeForIndexPath:indexPath])
  {
    case MWMSearchTableCellTypeOnMap:
      return MWMSearchShowOnMapCell.cellHeight;
    case MWMSearchTableCellTypeSuggestion:
      return MWMSearchSuggestionCell.cellHeight;
    case MWMSearchTableCellTypeCommon:
      return MWMSearchCommonCell.defaultCellHeight;
  }
}

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath
{
  MWMSearchTableCellType const cellType = [self cellTypeForIndexPath:indexPath];
  switch (cellType)
  {
    case MWMSearchTableCellTypeOnMap:
      return MWMSearchShowOnMapCell.cellHeight;
    case MWMSearchTableCellTypeSuggestion:
      return MWMSearchSuggestionCell.cellHeight;
    case MWMSearchTableCellTypeCommon:
      [self.commonSizingCell config:[self searchResultForIndexPath:indexPath] forHeight:YES];
      return self.commonSizingCell.cellHeight;
  }
}

- (void)tableView:(UITableView *)tableView willDisplayCell:(UITableViewCell *)cell
forRowAtIndexPath:(NSIndexPath *)indexPath
{
  switch ([self cellTypeForIndexPath:indexPath])
  {
    case MWMSearchTableCellTypeOnMap:
      break;
    case MWMSearchTableCellTypeSuggestion:
      [self configSuggestionCell:(MWMSearchSuggestionCell *)cell
                          result:[self searchResultForIndexPath:indexPath]
                      isLastCell:indexPath.row == searchResults.GetSuggestsCount() - 1];
      break;
    case MWMSearchTableCellTypeCommon:
      [(MWMSearchCommonCell *)cell config:[self searchResultForIndexPath:indexPath] forHeight:NO];
      break;
  }
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  MWMSearchTableCellType cellType = [self cellTypeForIndexPath:indexPath];
  if (cellType == MWMSearchTableCellTypeOnMap)
  {
    self.delegate.state = MWMSearchManagerStateMapSearch;
  }
  else
  {
    search::Result const & result = [self searchResultForIndexPath:indexPath];
    if (cellType == MWMSearchTableCellTypeSuggestion)
    {
      NSString * suggestionString = @(result.GetSuggestionString());
      [[Statistics instance] logEvent:kStatEventName(kStatSearch, kStatSelectResult)
                       withParameters:@{kStatValue : suggestionString, kStatScreen : kStatSearch}];
      [self.delegate searchText:suggestionString forInputLocale:nil];
    }
    else
      [self.delegate processSearchWithResult:result query:make_pair(searchParams.m_inputLocale, searchParams.m_query)];
  }
}

- (void)completeSearch
{
  self.delegate.searchTextField.isSearching = NO;
  MWMSearchTableView * view = (MWMSearchTableView *)self.view;
  if (searchResults.GetCount() == 0)
  {
    view.tableView.hidden = YES;
    view.noResultsView.hidden = NO;
    view.noResultsText.text = L(@"search_not_found_query");
    if (self.searchOnMap)
      [[[ToastView alloc] initWithMessage:view.noResultsText.text] show];
  }
  else
  {
    view.tableView.hidden = NO;
    view.noResultsView.hidden = YES;
  }
}

- (void)updateSearchResultsInTable
{
  if (!IPAD && _searchOnMap)
    return;

  self.commonSizingCell = nil;
  [self.tableView reloadData];
}

#pragma mark - LocationObserver

- (void)onLocationUpdate:(location::GpsInfo const &)info
{
  searchParams.SetPosition(info.m_latitude, info.m_longitude);
  [self updateSearchResultsInTable];
}

#pragma mark - Search

- (void)searchText:(nonnull NSString *)text forInputLocale:(nullable NSString *)locale
{
  if (!text)
    return;

  if (locale)
    searchParams.SetInputLocale(locale.UTF8String);
  searchParams.m_query = text.precomposedStringWithCompatibilityMapping.UTF8String;
  searchParams.SetForceSearch(true);

  [self updateSearch:YES];
}

- (void)updateSearch:(BOOL)textChanged
{
  Framework & f = GetFramework();
  if (!searchParams.m_query.empty())
  {
    self.watchLocationUpdates = YES;

    if (self.searchOnMap)
    {
      if (textChanged)
        f.CancelInteractiveSearch();

      f.StartInteractiveSearch(searchParams);
    }

    if (!_searchOnMap)
    {
      f.Search(searchParams);
    }
    else
    {
      if (textChanged)
      {
        self.delegate.searchTextField.isSearching = NO;
        f.UpdateUserViewportChanged();
      }
      else
        f.ShowAllSearchResults(searchResults);
    }
  }
  else
  {
    self.watchLocationUpdates = NO;
    f.CancelInteractiveSearch();
  }
}

#pragma mark - Properties

- (void)setWatchLocationUpdates:(BOOL)watchLocationUpdates
{
  if (_watchLocationUpdates == watchLocationUpdates)
    return;
  _watchLocationUpdates = watchLocationUpdates;
  if (watchLocationUpdates)
    [[MapsAppDelegate theApp].m_locationManager start:self];
  else
    [[MapsAppDelegate theApp].m_locationManager stop:self];
}

@synthesize searchOnMap = _searchOnMap;
- (void)setSearchOnMap:(BOOL)searchOnMap
{
  _searchOnMap = searchOnMap;
  [self updateSearch:NO];
}

- (BOOL)searchOnMap
{
  return IPAD || _searchOnMap;
}

- (MWMSearchCommonCell *)commonSizingCell
{
  if (!_commonSizingCell)
    _commonSizingCell = [self.tableView dequeueReusableCellWithIdentifier:kTableCommonCell];
  return _commonSizingCell;
}

@end
