#import "MWMSearchTableViewController.h"
#import "MWMLocationManager.h"
#import "MWMSearchChangeModeView.h"
#import "MWMSearchCommonCell.h"
#import "MWMSearchShowOnMapCell.h"
#import "MWMSearchSuggestionCell.h"
#import "MWMSearchTableView.h"
#import "Macros.h"
#import "MapsAppDelegate.h"
#import "Statistics.h"

static NSString * const kTableSuggestionCell = @"MWMSearchSuggestionCell";
static NSString * const kTableCommonCell = @"MWMSearchCommonCell";

namespace
{
typedef NS_ENUM(NSUInteger, MWMSearchTableCellType) {
  MWMSearchTableCellTypeSuggestion,
  MWMSearchTableCellTypeCommon
};

NSString * identifierForType(MWMSearchTableCellType type)
{
  switch (type)
  {
  case MWMSearchTableCellTypeSuggestion: return kTableSuggestionCell;
  case MWMSearchTableCellTypeCommon: return kTableCommonCell;
  }
}
}  // namespace

@interface MWMSearchTableViewController ()<UITableViewDataSource, UITableViewDelegate>

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

- (void)viewWillAppear:(BOOL)animated
{
  [super viewWillAppear:animated];
  self.tableView.hidden = NO;
  [(MWMSearchTableView *)self.view hideNoResultsView:YES];
  [self reloadData];
}

- (void)mwm_refreshUI { [self.view mwm_refreshUI]; }
- (void)setupTableView
{
  UITableView * tableView = self.tableView;
  [tableView registerNib:[UINib nibWithNibName:kTableSuggestionCell bundle:nil]
      forCellReuseIdentifier:kTableSuggestionCell];
  [tableView registerNib:[UINib nibWithNibName:kTableCommonCell bundle:nil]
      forCellReuseIdentifier:kTableCommonCell];
}

- (MWMSearchTableCellType)cellTypeForIndexPath:(NSIndexPath *)indexPath
{
  size_t const numSuggests = [MWMSearch suggestionsCount];
  if (numSuggests > 0 && indexPath.row < numSuggests)
    return MWMSearchTableCellTypeSuggestion;
  return MWMSearchTableCellTypeCommon;
}

- (search::Result const &)searchResultForIndexPath:(NSIndexPath *)indexPath
{
  return [MWMSearch resultAtIndex:indexPath.row];
}

- (void)reloadData { [self.tableView reloadData]; }
#pragma mark - Layout

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
  return [MWMSearch resultsCount];
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
  case MWMSearchTableCellTypeSuggestion: return MWMSearchSuggestionCell.cellHeight;
  case MWMSearchTableCellTypeCommon: return MWMSearchCommonCell.defaultCellHeight;
  }
}

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath
{
  MWMSearchTableCellType const cellType = [self cellTypeForIndexPath:indexPath];
  switch (cellType)
  {
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
  id<MWMSearchTableViewProtocol> delegate = self.delegate;
  search::Result const & result = [self searchResultForIndexPath:indexPath];
  if (cellType == MWMSearchTableCellTypeSuggestion)
  {
    NSString * suggestionString = @(result.GetSuggestionString());
    [Statistics logEvent:kStatEventName(kStatSearch, kStatSelectResult)
          withParameters:@{kStatValue : suggestionString, kStatScreen : kStatSearch}];
    [delegate searchText:suggestionString forInputLocale:nil];
  }
  else
  {
    MWMSearchTextField * textField = delegate.searchTextField;
    [MWMSearch saveQuery:textField.text forInputLocale:textField.textInputMode.primaryLanguage];
    [delegate processSearchWithResult:result];
  }
}

#pragma mark - MWMSearchObserver

- (void)onSearchCompleted
{
  BOOL const noResults = [MWMSearch resultsCount] == 0;
  self.tableView.hidden = noResults;
  [(MWMSearchTableView *)self.view hideNoResultsView:!noResults];
}

- (void)onSearchResultsUpdated
{
  if (!IPAD && [MWMSearch isSearchOnMap])
    return;

  self.commonSizingCell = nil;
  [self reloadData];
}

#pragma mark - Properties

- (MWMSearchCommonCell *)commonSizingCell
{
  if (!_commonSizingCell)
    _commonSizingCell = [self.tableView dequeueReusableCellWithIdentifier:kTableCommonCell];
  return _commonSizingCell;
}

@end
