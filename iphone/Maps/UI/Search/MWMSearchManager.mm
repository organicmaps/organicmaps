#import "MWMSearchManager.h"
#import "MWMCommon.h"
#import "MWMFrameworkListener.h"
#import "MWMMapViewControlsManager.h"
#import "MWMNoMapsViewController.h"
#import "MWMRoutePoint+CPP.h"
#import "MWMRouter.h"
#import "MWMSearchChangeModeView.h"
#import "MWMSearchFilterTransitioningManager.h"
#import "MWMSearchManager+Filter.h"
#import "MWMSearchManager+Layout.h"
#import "MWMSearchTabButtonsView.h"
#import "MWMSearchTabbedViewController.h"
#import "MWMSearchTableViewController.h"
#import "MapViewController.h"
#import "Statistics.h"
#import "3party/Alohalytics/src/alohalytics_objc.h"

extern NSString * const kAlohalyticsTapEventKey;
extern NSString * const kSearchStateKey = @"SearchStateKey";

namespace
{
typedef NS_ENUM(NSUInteger, MWMSearchManagerActionBarState) {
  MWMSearchManagerActionBarStateHidden,
  MWMSearchManagerActionBarStateTabBar,
  MWMSearchManagerActionBarStateModeFilter
};

using Observer = id<MWMSearchManagerObserver>;
using Observers = NSHashTable<Observer>;
}  // namespace

@interface MWMMapViewControlsManager ()

@property(nonatomic) MWMSearchManager * searchManager;

@end

@interface MWMSearchManager ()<MWMSearchTableViewProtocol, MWMSearchTabbedViewProtocol,
                               MWMSearchTabButtonsViewProtocol, UITextFieldDelegate,
                               MWMFrameworkStorageObserver, MWMSearchObserver>

@property(weak, nonatomic, readonly) UIViewController * ownerController;

@property(nonatomic) IBOutlet UIView * searchBarView;

@property(nonatomic) IBOutlet UIView * actionBarView;

@property(weak, nonatomic) IBOutlet NSLayoutConstraint * actionBarViewHeight;
@property(nonatomic) MWMSearchManagerActionBarState actionBarState;
@property(weak, nonatomic) IBOutlet UIButton * actionBarViewFilterButton;
@property(nonatomic) IBOutlet UIView * tabBarView;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * tabBarViewHeight;
@property(nonatomic) IBOutlet MWMSearchChangeModeView * changeModeView;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * changeModeViewHeight;
@property(weak, nonatomic) IBOutlet UIButton * filterButton;

@property(nonatomic) IBOutlet UIView * contentView;

@property(nonatomic) NSLayoutConstraint * actionBarViewBottom;

@property(nonatomic) IBOutletCollection(MWMSearchTabButtonsView) NSArray * tabButtons;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * scrollIndicatorOffset;
@property(weak, nonatomic) IBOutlet UIView * scrollIndicator;

@property(nonatomic) UINavigationController * navigationController;
@property(nonatomic) MWMSearchTabbedViewController * tabbedController;
@property(nonatomic) MWMSearchTableViewController * tableViewController;
@property(nonatomic) MWMNoMapsViewController * noMapsController;

@property(nonatomic) MWMSearchFilterTransitioningManager * filterTransitioningManager;

@property(nonatomic) Observers * observers;

@end

@implementation MWMSearchManager

+ (MWMSearchManager *)manager { return [MWMMapViewControlsManager manager].searchManager; }
- (nullable instancetype)init
{
  self = [super init];
  if (self)
  {
    [NSBundle.mainBundle loadNibNamed:@"MWMSearchView" owner:self options:nil];
    self.state = MWMSearchManagerStateHidden;
    [MWMSearch addObserver:self];
    _observers = [Observers weakObjectsHashTable];
  }
  return self;
}

- (void)mwm_refreshUI
{
  [self.searchBarView mwm_refreshUI];
  [self.actionBarView mwm_refreshUI];
  [self.tabbedController mwm_refreshUI];
  [self.tableViewController mwm_refreshUI];
  [self.noMapsController mwm_refreshUI];
  if ([MWMSearch hasFilter])
    [[MWMSearch getFilter] mwm_refreshUI];
}

- (void)beginSearch
{
  if (self.state != MWMSearchManagerStateHidden)
    self.state = MWMSearchManagerStateTableSearch;
}

- (void)endSearch
{
  if (self.state != MWMSearchManagerStateHidden)
    self.state = MWMSearchManagerStateDefault;
  self.searchTextField.text = @"";
  [MWMSearch clear];
}

#pragma mark - Actions

- (IBAction)textFieldDidEndEditing:(UITextField *)textField
{
  if (textField.text.length == 0)
    [self endSearch];
}

- (IBAction)textFieldTextDidChange:(UITextField *)textField
{
  NSString * text = textField.text;
  if (text.length > 0)
  {
    [self clearFilter];
    [self beginSearch];
    [MWMSearch searchQuery:text forInputLocale:textField.textInputMode.primaryLanguage];
  }
  else
  {
    [self endSearch];
  }
}

- (IBAction)cancelButtonPressed
{
  [Statistics logEvent:kStatEventName(kStatSearch, kStatCancel)];
  [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"searchCancel"];
  self.state = MWMSearchManagerStateHidden;
}

- (void)tabButtonPressed:(MWMSearchTabButtonsView *)sender
{
  [self.searchTextField resignFirstResponder];
  [self.tabbedController tabButtonPressed:sender];
}

#pragma mark - Layout

- (void)viewWillTransitionToSize:(CGSize)size
       withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator
{
  [self.navigationController viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
}

#pragma mark - UITextFieldDelegate

- (void)textFieldDidBeginEditing:(UITextField *)textField
{
  BOOL const isEmpty = (textField.text.length == 0);
  self.state = isEmpty ? MWMSearchManagerStateDefault : MWMSearchManagerStateTableSearch;
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField
{
  textField.text = [[textField.text stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceCharacterSet] stringByAppendingString:@" "];
  [self textFieldTextDidChange:textField];
  [textField resignFirstResponder];
  return YES;
}

#pragma mark - MWMSearchTabbedViewProtocol

- (void)searchText:(NSString *)text forInputLocale:(NSString *)locale
{
  [self beginSearch];
  self.searchTextField.text = text;
  NSString * inputLocale = locale ?: self.searchTextField.textInputMode.primaryLanguage;
  [MWMSearch searchQuery:text forInputLocale:inputLocale];
}

- (void)dismissKeyboard { [self.searchTextField resignFirstResponder]; }
- (void)processSearchWithResult:(search::Result const &)result
{
  if (self.routingTooltipSearch == MWMSearchManagerRoutingTooltipSearchNone)
  {
    [MWMSearch showResult:result];
  }
  else
  {
    BOOL const isStart = self.routingTooltipSearch == MWMSearchManagerRoutingTooltipSearchStart;
    auto point = [[MWMRoutePoint alloc]
            initWithPoint:result.GetFeatureCenter()
                    title:@(result.GetString().c_str())
                 subtitle:@(result.GetAddress().c_str())
                     type:isStart ? MWMRoutePointTypeStart : MWMRoutePointTypeFinish
        intermediateIndex:0];
    if (isStart)
      [MWMRouter buildFromPoint:point bestRouter:NO];
    else
      [MWMRouter buildToPoint:point bestRouter:NO];
  }
  if (!IPAD || [MWMNavigationDashboardManager manager].state != MWMNavigationDashboardStateHidden)
    self.state = MWMSearchManagerStateHidden;
}

#pragma mark - MWMFrameworkStorageObserver

- (void)processCountryEvent:(storage::TCountryId const &)countryId
{
  using namespace storage;
  NodeStatuses nodeStatuses{};
  GetFramework().GetStorage().GetNodeStatuses(countryId, nodeStatuses);
  if (nodeStatuses.m_status != NodeStatus::OnDisk)
    return;
  [self updateTopController];
  if (self.state == MWMSearchManagerStateTableSearch ||
      self.state == MWMSearchManagerStateMapSearch)
  {
    NSString * text = self.searchTextField.text;
    if (text.length != 0)
      [MWMSearch searchQuery:text
              forInputLocale:self.searchTextField.textInputMode.primaryLanguage];
  }
}

#pragma mark - State changes

- (void)updateTopController
{
  UIViewController * selfTopVC = self.topController;
  if ([selfTopVC isEqual:self.navigationController.topViewController])
    return;
  NSMutableArray * viewControllers = [self.navigationController.viewControllers mutableCopy];
  viewControllers[0] = selfTopVC;
  [self.navigationController setViewControllers:viewControllers animated:NO];
}

- (void)changeToHiddenState
{
  self.routingTooltipSearch = MWMSearchManagerRoutingTooltipSearchNone;
  [self endSearch];
  [self.tabbedController resetSelectedTab];

  [self viewHidden:YES];
}

- (void)changeToDefaultState
{
  [self.tabbedController resetCategories];
  [self.navigationController popToRootViewControllerAnimated:NO];

  [self animateConstraints:^{
    self.actionBarViewBottom.priority = UILayoutPriorityDefaultLow;
  }];
  [self viewHidden:NO];

  if (self.topController != self.noMapsController)
  {
    self.actionBarState = MWMSearchManagerActionBarStateTabBar;
    [self.searchTextField becomeFirstResponder];
  }
  else
  {
    self.actionBarState = MWMSearchManagerActionBarStateHidden;
  }
}

- (void)changeToTableSearchState
{
  [self.navigationController popToRootViewControllerAnimated:NO];

  [self updateTableSearchActionBar];
  [self viewHidden:NO];
  [MWMSearch setSearchOnMap:NO];
  [self.tableViewController reloadData];

  if (![self.navigationController.viewControllers containsObject:self.tableViewController])
    [self.navigationController pushViewController:self.tableViewController animated:NO];
}

- (void)changeToMapSearchState
{
  [self.navigationController popToRootViewControllerAnimated:NO];

  self.actionBarState = MWMSearchManagerActionBarStateModeFilter;
  [self animateConstraints:^{
    self.actionBarViewBottom.priority = UILayoutPriorityDefaultHigh;
  }];
  auto const navigationManagerState = [MWMNavigationDashboardManager manager].state;
  [self viewHidden:navigationManagerState != MWMNavigationDashboardStateHidden];
  [MWMSearch setSearchOnMap:YES];
  [self.tableViewController reloadData];

  GetFramework().DeactivateMapSelection(true);
  [self.searchTextField resignFirstResponder];

  if (navigationManagerState == MWMNavigationDashboardStateNavigation)
  {
    self.searchTextField.text = @"";
    [self.tabbedController resetSelectedTab];
  }
}

- (void)animateConstraints:(MWMVoidBlock)block
{
  UIView * parentView = self.ownerController.view;
  [parentView layoutIfNeeded];
  block();
  [UIView animateWithDuration:kDefaultAnimationDuration
                   animations:^{
                     [parentView layoutIfNeeded];
                   }];
}

#pragma mark - MWMSearchObserver

- (void)onSearchCompleted
{
  if (self.state != MWMSearchManagerStateTableSearch)
     return;
  [self.tableViewController onSearchCompleted];
  [self updateTableSearchActionBar];
}

- (void)onSearchResultsUpdated
{
  [self.tableViewController reloadData];
}

- (void)updateTableSearchActionBar
{
  if (self.state != MWMSearchManagerStateTableSearch)
    return;
  [self animateConstraints:^{
    BOOL hideActionBar = NO;
    if ([MWMSearch resultsCount] == 0)
      hideActionBar = YES;
    else if (IPAD)
      hideActionBar = !([MWMSearch isHotelResults] || [MWMSearch hasFilter]);
    else
      hideActionBar = ([MWMSearch suggestionsCount] != 0);
    self.actionBarState = hideActionBar ? MWMSearchManagerActionBarStateHidden
                                        : MWMSearchManagerActionBarStateModeFilter;

    self.actionBarViewBottom.priority = UILayoutPriorityDefaultLow;
  }];
}

#pragma mark - Add/Remove Observers

+ (void)addObserver:(id<MWMSearchManagerObserver>)observer
{
  [[MWMSearchManager manager].observers addObject:observer];
}

+ (void)removeObserver:(id<MWMSearchManagerObserver>)observer
{
  [[MWMSearchManager manager].observers removeObject:observer];
}

#pragma mark - MWMSearchManagerObserver

- (void)onSearchManagerStateChanged
{
  for (Observer observer in self.observers)
    [observer onSearchManagerStateChanged];
}

#pragma mark - Filters

- (IBAction)changeMode
{
  switch (self.state)
  {
  case MWMSearchManagerStateTableSearch: self.state = MWMSearchManagerStateMapSearch; break;
  case MWMSearchManagerStateMapSearch: self.state = MWMSearchManagerStateTableSearch; break;
  default: break;
  }
}

#pragma mark - Properties

- (UINavigationController *)navigationController
{
  if (!_navigationController)
  {
    _navigationController =
        [[UINavigationController alloc] initWithRootViewController:self.topController];
    [self.contentView addSubview:_navigationController.view];
    _navigationController.navigationBarHidden = YES;
  }
  return _navigationController;
}

- (UIViewController *)topController
{
  [MWMFrameworkListener removeObserver:self];
  self.noMapsController = nil;
  switch (self.state)
  {
  case MWMSearchManagerStateHidden: return self.tabbedController;
  case MWMSearchManagerStateDefault:
    if (GetFramework().GetStorage().HaveDownloadedCountries())
      return self.tabbedController;
    self.noMapsController = [MWMNoMapsViewController controller];
    [MWMFrameworkListener addObserver:self];
    return self.noMapsController;
  case MWMSearchManagerStateTableSearch:
    return self.tableViewController;
  case MWMSearchManagerStateMapSearch: return self.tableViewController;
  }
}

- (MWMSearchTabbedViewController *)tabbedController
{
  if (!_tabbedController)
  {
    _tabbedController = [[MWMSearchTabbedViewController alloc] init];
    _tabbedController.scrollIndicatorOffset = self.scrollIndicatorOffset;
    _tabbedController.scrollIndicator = self.scrollIndicator;
    _tabbedController.tabButtons = self.tabButtons;
    _tabbedController.delegate = self;
  }
  return _tabbedController;
}

- (MWMSearchTableViewController *)tableViewController
{
  if (!_tableViewController)
    _tableViewController = [[MWMSearchTableViewController alloc] initWithDelegate:self];
  return _tableViewController;
}

- (void)setScrollIndicatorOffset:(NSLayoutConstraint *)scrollIndicatorOffset
{
  _scrollIndicatorOffset = self.tabbedController.scrollIndicatorOffset = scrollIndicatorOffset;
}

- (void)setScrollIndicator:(UIView *)scrollIndicator
{
  _scrollIndicator = self.tabbedController.scrollIndicator = scrollIndicator;
}

- (void)setTabButtons:(NSArray *)tabButtons
{
  _tabButtons = self.tabbedController.tabButtons = tabButtons;
}

- (void)setState:(MWMSearchManagerState)state
{
  if (_state == state)
    return;
  if (_state == MWMSearchManagerStateHidden)
    [self endSearch];
  _state = state;
  [self updateTopController];
  switch (state)
  {
  case MWMSearchManagerStateHidden:
    [Statistics logEvent:kStatSearchEnteredState withParameters:@{kStatName : kStatClose}];
    [self changeToHiddenState];
    break;
  case MWMSearchManagerStateDefault:
    [Statistics logEvent:kStatSearchEnteredState withParameters:@{kStatName : kStatOpen}];
    [self changeToDefaultState];
    break;
  case MWMSearchManagerStateTableSearch:
    [Statistics logEvent:kStatSearchEnteredState withParameters:@{kStatName : kStatTable}];
    [self changeToTableSearchState];
    break;
  case MWMSearchManagerStateMapSearch:
    [Statistics logEvent:kStatSearchEnteredState withParameters:@{kStatName : kStatMapSearch}];
    [self changeToMapSearchState];
    break;
  }
  [self onSearchManagerStateChanged];
  [self.changeModeView updateForState:state];
  [[MapViewController controller] updateStatusBarStyle];
}

- (void)viewHidden:(BOOL)hidden
{
  UIView * searchBarView = self.searchBarView;
  UIView * actionBarView = self.actionBarView;
  UIView * contentView = self.contentView;
  UIView * parentView = self.ownerController.view;

  if (!hidden)
  {
    if (searchBarView.superview)
    {
      [parentView bringSubviewToFront:searchBarView];
      [parentView bringSubviewToFront:actionBarView];
      [parentView bringSubviewToFront:contentView];
      return;
    }
    [parentView addSubview:searchBarView];
    [parentView addSubview:actionBarView];
    [parentView addSubview:contentView];
    [self layoutTopViews];
  }
  [UIView animateWithDuration:kDefaultAnimationDuration
      animations:^{
        CGFloat const alpha = hidden ? 0 : 1;
        contentView.alpha = alpha;
        actionBarView.alpha = alpha;
        searchBarView.alpha = alpha;
      }
      completion:^(BOOL finished) {
        if (!hidden)
          return;
        [contentView removeFromSuperview];
        [actionBarView removeFromSuperview];
        [searchBarView removeFromSuperview];
      }];
}

- (void)setChangeModeView:(MWMSearchChangeModeView *)changeModeView
{
  _changeModeView = changeModeView;
  [changeModeView updateForState:self.state];
}

- (void)setActionBarState:(MWMSearchManagerActionBarState)actionBarState
{
  _actionBarState = actionBarState;
  switch (actionBarState)
  {
  case MWMSearchManagerActionBarStateHidden:
    self.tabBarView.hidden = YES;
    self.changeModeView.hidden = YES;
    self.actionBarViewHeight.priority = UILayoutPriorityDefaultHigh;
    self.tabBarViewHeight.priority = UILayoutPriorityDefaultLow;
    self.changeModeViewHeight.priority = UILayoutPriorityDefaultLow;
    break;
  case MWMSearchManagerActionBarStateTabBar:
    self.tabBarView.hidden = NO;
    self.changeModeView.hidden = YES;
    self.actionBarViewHeight.priority = UILayoutPriorityDefaultLow;
    self.tabBarViewHeight.priority = UILayoutPriorityDefaultHigh;
    self.changeModeViewHeight.priority = UILayoutPriorityDefaultLow;
    break;
  case MWMSearchManagerActionBarStateModeFilter:
    self.tabBarView.hidden = YES;
    self.changeModeView.hidden = NO;
    self.actionBarViewHeight.priority = UILayoutPriorityDefaultLow;
    self.tabBarViewHeight.priority = UILayoutPriorityDefaultLow;
    self.changeModeViewHeight.priority = UILayoutPriorityDefaultHigh;
    break;
  }
}

- (UIViewController *)ownerController { return [MapViewController controller]; }
@end
