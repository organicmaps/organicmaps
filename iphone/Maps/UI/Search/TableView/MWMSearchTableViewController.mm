#import "MWMSearchTableViewController.h"
#import "MWMSearchCommonCell.h"
#import "MWMSearchSuggestionCell.h"
#import "MWMSearchTableView.h"
#import "Statistics.h"
#import "SwiftBridge.h"

#include "platform/localization.hpp"

namespace {
NSString *GetLocalizedTypeName(search::Result const &result) {
  if (result.GetResultType() != search::Result::Type::Feature)
    return @"";

  auto const readableType = classif().GetReadableObjectName(result.GetFeatureType());

  return @(platform::GetLocalizedTypeName(readableType).c_str());
}
}

@interface MWMSearchTableViewController () <UITableViewDataSource, UITableViewDelegate>

@property(weak, nonatomic) IBOutlet UITableView *tableView;

@property(weak, nonatomic) id<MWMSearchTableViewProtocol> delegate;

@end

@implementation MWMSearchTableViewController

- (nonnull instancetype)initWithDelegate:(id<MWMSearchTableViewProtocol>)delegate {
  self = [super init];
  if (self)
    _delegate = delegate;
  return self;
}

- (void)viewDidLoad {
  [super viewDidLoad];
  [self setupTableView];
}

- (void)viewWillAppear:(BOOL)animated {
  [super viewWillAppear:animated];
  self.tableView.hidden = NO;
  self.tableView.insetsContentViewsToSafeArea = YES;
  [(MWMSearchTableView *)self.view hideNoResultsView:YES];
  [self reloadData];
}

- (void)setupTableView {
  UITableView *tableView = self.tableView;
  tableView.estimatedRowHeight = 80.;
  tableView.rowHeight = UITableViewAutomaticDimension;
  [tableView registerNibWithCellClass:[MWMSearchSuggestionCell class]];
  [tableView registerNibWithCellClass:[MWMSearchCommonCell class]];
  [tableView registerClassWithCellClass:[MWMAdBannerCell class]];
}

- (void)reloadData {
  [self.tableView reloadData];
}
#pragma mark - Layout

- (void)viewWillTransitionToSize:(CGSize)size
       withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator {
  [super viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
  [coordinator
    animateAlongsideTransition:^(id<UIViewControllerTransitionCoordinatorContext> context) {
      [self reloadData];
    }
                    completion:nil];
}

#pragma mark - UITableViewDataSource

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
  return [MWMSearch resultsCount];
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
  if ([MWMSearch resultsCount] == 0) {
    NSAssert(false, @"Invalid reload with outdated SearchIndex");
    return [tableView dequeueReusableCellWithCellClass:[MWMSearchCommonCell class] indexPath:indexPath];
  }
  auto const row = indexPath.row;
  auto const containerIndex = [MWMSearch containerIndexWithRow:row];
  switch ([MWMSearch resultTypeWithRow:row]) {
    case MWMSearchItemTypeRegular: {
      auto cell =
        static_cast<MWMSearchCommonCell *>([tableView dequeueReusableCellWithCellClass:[MWMSearchCommonCell class]
                                                                             indexPath:indexPath]);
      auto const &result = [MWMSearch resultWithContainerIndex:containerIndex];
      auto const isBookingAvailable = [MWMSearch isBookingAvailableWithContainerIndex:containerIndex];
      auto const isDealAvailable = [MWMSearch isDealAvailableWithContainerIndex:containerIndex];
      auto const &productInfo = [MWMSearch productInfoWithContainerIndex:containerIndex];
      auto const typeName = GetLocalizedTypeName(result);
      [cell config:result
              isAvailable:isBookingAvailable
               isHotOffer:isDealAvailable
              productInfo:productInfo
        localizedTypeName:typeName];
      return cell;
    }
    case MWMSearchItemTypeMopub:
    case MWMSearchItemTypeFacebook: {
      auto cell = static_cast<MWMAdBannerCell *>([tableView dequeueReusableCellWithCellClass:[MWMAdBannerCell class]
                                                                                   indexPath:indexPath]);
      auto ad = [MWMSearch adWithContainerIndex:containerIndex];
      [cell configWithAd:ad
           containerType:MWMAdBannerContainerTypeSearch
            canRemoveAds:[SubscriptionManager canMakePayments]
             onRemoveAds:^{
               [[MapViewController sharedController] showRemoveAds];
             }];
      return cell;
    }
    case MWMSearchItemTypeSuggestion: {
      auto cell = static_cast<MWMSearchSuggestionCell *>(
        [tableView dequeueReusableCellWithCellClass:[MWMSearchSuggestionCell class] indexPath:indexPath]);
      auto const &suggestion = [MWMSearch resultWithContainerIndex:containerIndex];
      [cell config:suggestion localizedTypeName:@""];
      cell.isLastCell = row == [MWMSearch suggestionsCount] - 1;
      return cell;
    }
  }
}

#pragma mark - UITableViewDelegate

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
  id<MWMSearchTableViewProtocol> delegate = self.delegate;
  auto const row = indexPath.row;
  auto const containerIndex = [MWMSearch containerIndexWithRow:row];
  switch ([MWMSearch resultTypeWithRow:row]) {
    case MWMSearchItemTypeRegular: {
      SearchTextField *textField = delegate.searchTextField;
      [MWMSearch saveQuery:textField.text forInputLocale:textField.textInputMode.primaryLanguage];
      auto const &result = [MWMSearch resultWithContainerIndex:containerIndex];
      [delegate processSearchWithResult:result];
      break;
    }
    case MWMSearchItemTypeMopub:
    case MWMSearchItemTypeFacebook:
      break;
    case MWMSearchItemTypeSuggestion: {
      auto const &suggestion = [MWMSearch resultWithContainerIndex:containerIndex];
      NSString *suggestionString = @(suggestion.GetSuggestionString().c_str());
      [Statistics logEvent:kStatEventName(kStatSearch, kStatSelectResult)
            withParameters:@{kStatValue: suggestionString, kStatScreen: kStatSearch}];
      [delegate searchText:suggestionString forInputLocale:nil];
    }
  }
}

#pragma mark - MWMSearchObserver

- (void)onSearchCompleted {
  [self reloadData];
  BOOL const noResults = [MWMSearch resultsCount] == 0;
  self.tableView.hidden = noResults;
  [(MWMSearchTableView *)self.view hideNoResultsView:!noResults];
}

@end
