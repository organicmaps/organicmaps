#import "MWMSearchTableViewController.h"
#import "MWMSearchCommonCell.h"
#import "MWMSearchSuggestionCell.h"
#import "MWMSearchTableView.h"
#import "SearchResult.h"
#import "SwiftBridge.h"

#include "platform/localization.hpp"

#include "search/result.hpp"

namespace {
NSString *GetLocalizedTypeName(search::Result const &result) {
  return @(result.GetLocalizedFeatureType().c_str());
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
  auto const & result = [MWMSearch resultWithContainerIndex:containerIndex];

  switch ([MWMSearch resultTypeWithRow:row])
  {
    case SearchItemTypeRegular:
    {
      auto cell = static_cast<MWMSearchCommonCell *>(
        [tableView dequeueReusableCellWithCellClass:[MWMSearchCommonCell class] indexPath:indexPath]);
      [cell configureWith:result];
      return cell;
    }
    case SearchItemTypeSuggestion:
    {
      auto cell = static_cast<MWMSearchSuggestionCell *>(
        [tableView dequeueReusableCellWithCellClass:[MWMSearchSuggestionCell class] indexPath:indexPath]);
      [cell configureWith:result];
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
  auto const & result = [MWMSearch resultWithContainerIndex:containerIndex];

  switch ([MWMSearch resultTypeWithRow:row])
  {
    case SearchItemTypeRegular:
    {
      SearchTextField const * textField = delegate.searchTextField;
      [MWMSearch saveQuery:textField.text forInputLocale:textField.textInputMode.primaryLanguage];
      [delegate processSearchResultAtIndex:indexPath.row];
      break;
    }
    case SearchItemTypeSuggestion:
    {
      [delegate searchText:result.suggestion
            forInputLocale:nil
              withCategory:result.isPureSuggest];
      break;
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
