#import "MWMSearchTableViewController.h"
#import "MWMLocationManager.h"
#import "MWMSearchChangeModeView.h"
#import "MWMSearchCommonCell.h"
#import "MWMSearchSuggestionCell.h"
#import "MWMSearchTableView.h"
#import "MapsAppDelegate.h"
#import "Statistics.h"
#import "SwiftBridge.h"

@interface MWMSearchTableViewController ()<UITableViewDataSource, UITableViewDelegate>

@property(weak, nonatomic) IBOutlet UITableView * tableView;

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
  tableView.estimatedRowHeight = 80.;
  tableView.rowHeight = UITableViewAutomaticDimension;
  [tableView registerWithCellClass:[MWMSearchSuggestionCell class]];
  [tableView registerWithCellClass:[MWMSearchCommonCell class]];
}

- (Class)cellClassForIndexPath:(NSIndexPath *)indexPath
{
  size_t const numSuggests = [MWMSearch suggestionsCount];
  if (numSuggests > 0 && indexPath.row < numSuggests)
    return [MWMSearchSuggestionCell class];
  return [MWMSearchCommonCell class];
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
  Class cls = [self cellClassForIndexPath:indexPath];
  auto cell = [tableView dequeueReusableCellWithCellClass:cls indexPath:indexPath];
  if (cls == [MWMSearchSuggestionCell class])
  {
    auto tCell = static_cast<MWMSearchSuggestionCell *>(cell);
    [tCell config:[self searchResultForIndexPath:indexPath]];
    tCell.isLastCell = indexPath.row == [MWMSearch suggestionsCount] - 1;
  }
  else if (cls == [MWMSearchCommonCell class])
  {
    auto tCell = static_cast<MWMSearchCommonCell *>(cell);
    [tCell config:[self searchResultForIndexPath:indexPath]];
  }
  return cell;
}

#pragma mark - UITableViewDelegate

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  Class cls = [self cellClassForIndexPath:indexPath];
  id<MWMSearchTableViewProtocol> delegate = self.delegate;
  search::Result const & result = [self searchResultForIndexPath:indexPath];
  if (cls == [MWMSearchSuggestionCell class])
  {
    NSString * suggestionString = @(result.GetSuggestionString());
    [Statistics logEvent:kStatEventName(kStatSearch, kStatSelectResult)
          withParameters:@{kStatValue : suggestionString, kStatScreen : kStatSearch}];
    [delegate searchText:suggestionString forInputLocale:nil];
  }
  else if (cls == [MWMSearchCommonCell class])
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

  [self reloadData];
}

@end
