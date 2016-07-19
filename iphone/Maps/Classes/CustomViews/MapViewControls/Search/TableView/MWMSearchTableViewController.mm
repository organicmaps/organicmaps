#import "MWMSearchTableViewController.h"
#import "MWMLocationManager.h"
#import "MWMSearch.h"
#import "MWMSearchCommonCell.h"
#import "MWMSearchShowOnMapCell.h"
#import "MWMSearchSuggestionCell.h"
#import "MWMSearchTableView.h"
#import "Macros.h"
#import "MapsAppDelegate.h"
#import "Statistics.h"
#import "ToastView.h"

static NSString * const kTableShowOnMapCell = @"MWMSearchShowOnMapCell";
static NSString * const kTableSuggestionCell = @"MWMSearchSuggestionCell";
static NSString * const kTableCommonCell = @"MWMSearchCommonCell";

typedef NS_ENUM(NSUInteger, MWMSearchTableCellType) {
  MWMSearchTableCellTypeOnMap,
  MWMSearchTableCellTypeSuggestion,
  MWMSearchTableCellTypeCommon
};

NSString * identifierForType(MWMSearchTableCellType type)
{
  switch (type)
  {
  case MWMSearchTableCellTypeOnMap: return kTableShowOnMapCell;
  case MWMSearchTableCellTypeSuggestion: return kTableSuggestionCell;
  case MWMSearchTableCellTypeCommon: return kTableCommonCell;
  }
}

@interface MWMSearchTableViewController ()<UITableViewDataSource, UITableViewDelegate,
                                           MWMSearchObserver>

@property(weak, nonatomic) IBOutlet UITableView * tableView;

@property(nonatomic) MWMSearchCommonCell * commonSizingCell;

@property(weak, nonatomic) id<MWMSearchTableViewProtocol> delegate;

@end

@implementation MWMSearchTableViewController

- (nonnull instancetype)initWithDelegate:(id<MWMSearchTableViewProtocol>)delegate
{
  self = [super init];
  if (self)
    _delegate = delegate;
  return self;
}

- (void)viewDidLoad
{
  [super viewDidLoad];
  [self setupTableView];
}

- (void)viewDidAppear:(BOOL)animated
{
  [super viewDidAppear:animated];
  [MWMSearch addObserver:self];
}

- (void)viewWillDisappear:(BOOL)animated
{
  [super viewWillDisappear:animated];
  [MWMSearch removeObserver:self];
}

- (void)mwm_refreshUI { [self.view mwm_refreshUI]; }
- (void)setupTableView
{
  [self.tableView registerNib:[UINib nibWithNibName:kTableShowOnMapCell bundle:nil]
       forCellReuseIdentifier:kTableShowOnMapCell];
  [self.tableView registerNib:[UINib nibWithNibName:kTableSuggestionCell bundle:nil]
       forCellReuseIdentifier:kTableSuggestionCell];
  [self.tableView registerNib:[UINib nibWithNibName:kTableCommonCell bundle:nil]
       forCellReuseIdentifier:kTableCommonCell];
}

- (MWMSearchTableCellType)cellTypeForIndexPath:(NSIndexPath *)indexPath
{
  size_t const numSuggests = [MWMSearch suggestionsCount];
  if (numSuggests > 0)
  {
    return indexPath.row < numSuggests ? MWMSearchTableCellTypeSuggestion
                                       : MWMSearchTableCellTypeCommon;
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

- (search::Result const &)searchResultForIndexPath:(NSIndexPath *)indexPath
{
  MWMSearchTableCellType firstCellType =
      [self cellTypeForIndexPath:[NSIndexPath indexPathForRow:0 inSection:0]];
  NSUInteger const searchPosition =
      indexPath.row - (firstCellType == MWMSearchTableCellTypeOnMap ? 1 : 0);
  return [MWMSearch resultAtIndex:searchPosition];
}

#pragma mark - Layout

- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation
                                duration:(NSTimeInterval)duration
{
  dispatch_async(dispatch_get_main_queue(), ^{
    [self onSearchResultsUpdated];
  });
}

- (void)viewWillTransitionToSize:(CGSize)size
       withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator
{
  [coordinator
      animateAlongsideTransition:^(id<UIViewControllerTransitionCoordinatorContext> context) {
        [self onSearchResultsUpdated];
      }
                      completion:nil];
}

#pragma mark - UITableViewDataSource

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  MWMSearchTableCellType firstCellType =
      [self cellTypeForIndexPath:[NSIndexPath indexPathForRow:0 inSection:0]];
  BOOL const showOnMap = firstCellType == MWMSearchTableCellTypeOnMap;
  return [MWMSearch resultsCount] + (showOnMap ? 1 : 0);
}

- (UITableViewCell *)tableView:(UITableView *)tableView
         cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  MWMSearchTableCellType const cellType = [self cellTypeForIndexPath:indexPath];
  return [tableView dequeueReusableCellWithIdentifier:identifierForType(cellType)];
}

#pragma mark - Config cells

- (void)configSuggestionCell:(MWMSearchSuggestionCell *)cell
                      result:(search::Result const &)result
                  isLastCell:(BOOL)isLastCell
{
  [cell config:result];
  cell.isLastCell = isLastCell;
}

#pragma mark - UITableViewDelegate

- (CGFloat)tableView:(UITableView *)tableView
    estimatedHeightForRowAtIndexPath:(NSIndexPath *)indexPath
{
  switch ([self cellTypeForIndexPath:indexPath])
  {
  case MWMSearchTableCellTypeOnMap: return MWMSearchShowOnMapCell.cellHeight;
  case MWMSearchTableCellTypeSuggestion: return MWMSearchSuggestionCell.cellHeight;
  case MWMSearchTableCellTypeCommon: return MWMSearchCommonCell.defaultCellHeight;
  }
}

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath
{
  MWMSearchTableCellType const cellType = [self cellTypeForIndexPath:indexPath];
  switch (cellType)
  {
  case MWMSearchTableCellTypeOnMap: return MWMSearchShowOnMapCell.cellHeight;
  case MWMSearchTableCellTypeSuggestion: return MWMSearchSuggestionCell.cellHeight;
  case MWMSearchTableCellTypeCommon:
    [self.commonSizingCell config:[self searchResultForIndexPath:indexPath] forHeight:YES];
    return self.commonSizingCell.cellHeight;
  }
}

- (void)tableView:(UITableView *)tableView
      willDisplayCell:(UITableViewCell *)cell
    forRowAtIndexPath:(NSIndexPath *)indexPath
{
  switch ([self cellTypeForIndexPath:indexPath])
  {
  case MWMSearchTableCellTypeOnMap: break;
  case MWMSearchTableCellTypeSuggestion:
    [self configSuggestionCell:(MWMSearchSuggestionCell *)cell
                        result:[self searchResultForIndexPath:indexPath]
                    isLastCell:indexPath.row == [MWMSearch suggestionsCount] - 1];
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
    MWMSearchTextField * textField = self.delegate.searchTextField;
    [MWMSearch saveQuery:textField.text forInputLocale:textField.textInputMode.primaryLanguage];
    self.delegate.state = MWMSearchManagerStateMapSearch;
  }
  else
  {
    search::Result const & result = [self searchResultForIndexPath:indexPath];
    if (cellType == MWMSearchTableCellTypeSuggestion)
    {
      NSString * suggestionString = @(result.GetSuggestionString());
      [Statistics logEvent:kStatEventName(kStatSearch, kStatSelectResult)
            withParameters:@{kStatValue : suggestionString, kStatScreen : kStatSearch}];
      [self.delegate searchText:suggestionString forInputLocale:nil];
    }
    else
    {
      MWMSearchTextField * textField = self.delegate.searchTextField;
      [MWMSearch saveQuery:textField.text forInputLocale:textField.textInputMode.primaryLanguage];
      [self.delegate processSearchWithResult:result];
    }
  }
}

#pragma mark - MWMSearchObserver

- (void)onSearchCompleted
{
  MWMSearchTableView * view = (MWMSearchTableView *)self.view;
  if ([MWMSearch resultsCount] == 0)
  {
    view.tableView.hidden = YES;
    view.noResultsView.hidden = NO;
    view.noResultsText.text = L(@"search_not_found_query");
    if ([MWMSearch isSearchOnMap])
      [[[ToastView alloc] initWithMessage:view.noResultsText.text] show];
  }
  else
  {
    view.tableView.hidden = NO;
    view.noResultsView.hidden = YES;
  }
}

- (void)onSearchResultsUpdated
{
  if (!IPAD && [MWMSearch isSearchOnMap])
    return;

  self.commonSizingCell = nil;
  [self.tableView reloadData];
}

#pragma mark - Properties

- (MWMSearchCommonCell *)commonSizingCell
{
  if (!_commonSizingCell)
    _commonSizingCell = [self.tableView dequeueReusableCellWithIdentifier:kTableCommonCell];
  return _commonSizingCell;
}

@end
