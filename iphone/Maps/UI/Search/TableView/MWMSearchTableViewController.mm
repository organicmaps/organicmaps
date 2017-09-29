#import "MWMSearchTableViewController.h"
#import "MWMSearchCommonCell.h"
#import "MWMSearchSuggestionCell.h"
#import "MWMSearchTableView.h"
#import "Statistics.h"
#import "SwiftBridge.h"

@interface MWMSearchTableViewController ()<UITableViewDataSource, UITableViewDelegate, MWMGoogleFallbackBannerDynamicSizeDelegate>

@property(weak, nonatomic) IBOutlet UITableView * tableView;

@property(weak, nonatomic) id<MWMSearchTableViewProtocol> delegate;

@end

@implementation MWMSearchTableViewController

- (nonnull instancetype)initWithDelegate:(id<MWMSearchTableViewProtocol>)delegate
{
  self = [super init];
  if (self)
  {
    _delegate = delegate;
    [MWMSearch addObserver:self];
  }
  return self;
}

- (void)dealloc
{
  [MWMSearch removeObserver:self];
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
  [tableView registerWithCellClass:[MWMAdBanner class]];
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
  if ([MWMSearch resultsCount] == 0)
  {
    NSAssert(false, @"Invalid reload with outdated SearchIndex");
    return [tableView dequeueReusableCellWithCellClass:[MWMSearchCommonCell class] indexPath:indexPath];
  }
  auto const row = indexPath.row;
  auto const containerIndex = [MWMSearch containerIndexWithRow:row];
  switch ([MWMSearch resultTypeWithRow:row])
  {
  case MWMSearchItemTypeRegular:
  {
    auto cell = static_cast<MWMSearchCommonCell *>([tableView
        dequeueReusableCellWithCellClass:[MWMSearchCommonCell class]
                               indexPath:indexPath]);
    auto const & result = [MWMSearch resultWithContainerIndex:containerIndex];
    auto const isLocalAds = [MWMSearch isLocalAdsWithContainerIndex:containerIndex];
    [cell config:result isLocalAds:isLocalAds];
    return cell;
  }
  case MWMSearchItemTypeMopub:
  case MWMSearchItemTypeFacebook:
  case MWMSearchItemTypeGoogle:
  {
    auto cell = static_cast<MWMAdBanner *>([tableView dequeueReusableCellWithCellClass:[MWMAdBanner class] indexPath:indexPath]);
    auto ad = [MWMSearch adWithContainerIndex:containerIndex];
    if ([ad isKindOfClass:[MWMGoogleFallbackBanner class]])
    {
      auto fallbackAd = static_cast<MWMGoogleFallbackBanner *>(ad);
      fallbackAd.cellIndexPath = indexPath;
      fallbackAd.dynamicSizeDelegate = self;
    }
    [cell configWithAd:ad containerType:MWMAdBannerContainerTypeSearch];
    return cell;
  }
  case MWMSearchItemTypeSuggestion:
  {
    auto cell = static_cast<MWMSearchSuggestionCell *>([tableView
        dequeueReusableCellWithCellClass:[MWMSearchSuggestionCell class]
                               indexPath:indexPath]);
    auto const & suggestion = [MWMSearch resultWithContainerIndex:containerIndex];
    [cell config:suggestion];
    cell.isLastCell = row == [MWMSearch suggestionsCount] - 1;
    return cell;
  }
  }
}

#pragma mark - MWMGoogleFallbackBannerDynamicSizeDelegate

- (void)dynamicSizeUpdatedWithBanner:(MWMGoogleFallbackBanner * _Nonnull)banner
{
  [self.tableView reloadRowsAtIndexPaths:@[banner.cellIndexPath] withRowAnimation:UITableViewRowAnimationFade];
}

#pragma mark - UITableViewDelegate

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  id<MWMSearchTableViewProtocol> delegate = self.delegate;
  auto const row = indexPath.row;
  auto const containerIndex = [MWMSearch containerIndexWithRow:row];
  switch ([MWMSearch resultTypeWithRow:row])
  {
  case MWMSearchItemTypeRegular:
  {
    MWMSearchTextField * textField = delegate.searchTextField;
    [MWMSearch saveQuery:textField.text forInputLocale:textField.textInputMode.primaryLanguage];
    auto const & result = [MWMSearch resultWithContainerIndex:containerIndex];
    [delegate processSearchWithResult:result];
    break;
  }
  case MWMSearchItemTypeMopub: 
  case MWMSearchItemTypeFacebook:
  case MWMSearchItemTypeGoogle: break;
  case MWMSearchItemTypeSuggestion:
  {
    auto const & suggestion = [MWMSearch resultWithContainerIndex:containerIndex];
    NSString * suggestionString = @(suggestion.GetSuggestionString());
    [Statistics logEvent:kStatEventName(kStatSearch, kStatSelectResult)
          withParameters:@{kStatValue : suggestionString, kStatScreen : kStatSearch}];
    [delegate searchText:suggestionString forInputLocale:nil];
  }
  }
}

#pragma mark - MWMSearchObserver

- (void)onSearchCompleted
{
  [self onSearchResultsUpdated];
  BOOL const noResults = [MWMSearch resultsCount] == 0;
  self.tableView.hidden = noResults;
  [(MWMSearchTableView *)self.view hideNoResultsView:!noResults];
  [self reloadData];
}

- (void)onSearchResultsUpdated
{
  [self reloadData];
}

@end
