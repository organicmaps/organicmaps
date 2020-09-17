#import "MWMSearchManager.h"
#import "MWMFrameworkListener.h"
#import "MWMMapViewControlsManager.h"
#import "MWMNoMapsViewController.h"
#import "MWMRoutePoint+CPP.h"
#import "MWMRouter.h"
#import "MWMSearchManager+Filter.h"
#import "MWMSearchManager+Layout.h"
#import "MWMSearchTableViewController.h"
#import "MapViewController.h"
#import "Statistics.h"
#import "SwiftBridge.h"

namespace {
typedef NS_ENUM(NSUInteger, MWMSearchManagerActionBarState) {
  MWMSearchManagerActionBarStateHidden,
  MWMSearchManagerActionBarStateTabBar,
  MWMSearchManagerActionBarStateModeFilter
};

using Observer = id<MWMSearchManagerObserver>;
using Observers = NSHashTable<Observer>;
}  // namespace

@interface MWMMapViewControlsManager ()

@property(nonatomic) MWMSearchManager *searchManager;

@end

@interface MWMSearchManager () <MWMSearchTableViewProtocol,
                                MWMSearchTabViewControllerDelegate,
                                UITextFieldDelegate,
                                MWMStorageObserver,
                                MWMSearchObserver,
                                DatePickerViewControllerDelegate,
                                GuestsPickerViewControllerDelegate>

@property(weak, nonatomic, readonly) UIViewController *ownerController;
@property(weak, nonatomic, readonly) UIView *searchViewContainer;
@property(weak, nonatomic, readonly) UIView *actionBarContainer;
@property(weak, nonatomic, readonly) MWMMapViewControlsManager *controlsManager;

@property(nonatomic) IBOutlet SearchBar *searchBarView;
@property(weak, nonatomic) IBOutlet SearchActionBarView *actionBarView;
@property(nonatomic) IBOutlet UIView *contentView;
@property(strong, nonatomic) IBOutlet UIView *tableViewContainer;

@property(nonatomic) NSLayoutConstraint *contentViewTopHidden;
@property(nonatomic) NSLayoutConstraint *contentViewBottomHidden;
@property(nonatomic) NSLayoutConstraint *actionBarViewBottomKeyboard;
@property(nonatomic) NSLayoutConstraint *actionBarViewBottomNormal;

@property(nonatomic) UINavigationController *navigationController;
@property(nonatomic) MWMSearchTableViewController *tableViewController;
@property(nonatomic) MWMNoMapsViewController *noMapsController;

@property(nonatomic) Observers *observers;
@property(nonatomic) NSDateFormatter *dateFormatter;

@end

@implementation MWMSearchManager

+ (MWMSearchManager *)manager {
  return [MWMMapViewControlsManager manager].searchManager;
}
- (nullable instancetype)init {
  self = [super init];
  if (self) {
    [NSBundle.mainBundle loadNibNamed:@"MWMSearchView" owner:self options:nil];
    self.state = MWMSearchManagerStateHidden;
    [MWMSearch addObserver:self];
    _observers = [Observers weakObjectsHashTable];
    _dateFormatter = [[NSDateFormatter alloc] init];
    _dateFormatter.dateFormat = @"yyyy-MM-dd";
  }
  return self;
}

- (void)beginSearch {
  if (self.state != MWMSearchManagerStateHidden)
    self.state = MWMSearchManagerStateTableSearch;
}

- (void)endSearch {
  if (self.state != MWMSearchManagerStateHidden)
    self.state = MWMSearchManagerStateDefault;
  self.searchTextField.text = @"";
  [self.searchBarView resetDates];
  [self.searchBarView resetGuestCount];
  [MWMSearch clear];
}

#pragma mark - Actions

- (IBAction)textFieldDidEndEditing:(UITextField *)textField {
  if (textField.text.length == 0)
    [self endSearch];
}

- (IBAction)textFieldTextDidChange:(UITextField *)textField {
  NSString *text = textField.text;
  if (text.length > 0) {
    [self clearFilter];
    [self beginSearch];
    [MWMSearch searchQuery:text forInputLocale:textField.textInputMode.primaryLanguage];
  } else {
    [self endSearch];
  }
}

- (IBAction)cancelButtonPressed {
  [Statistics logEvent:kStatEventName(kStatSearch, kStatCancel)];
  [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"searchCancel"];
  self.state = MWMSearchManagerStateHidden;
}

- (IBAction)backButtonPressed {
  NSString *statFrom = kStatUnknown;
  if (self.state == MWMSearchManagerStateResult) {
    statFrom = kStatSearchMapOneResult;
  } else if (self.state == MWMSearchManagerStateResult) {
    statFrom = kStatSearchMapSearch;
  }
  if (GetFramework().HasPlacePageInfo()) {
    [Statistics logEvent:kStatBackClick
          withParameters:@{kStatFrom: statFrom, kStatTo: kStatSearchResults, kStatPlacePage: kStatPreview}];
  } else {
    [Statistics logEvent:kStatBackClick withParameters:@{kStatFrom: statFrom, kStatTo: kStatSearchResults}];
  }
  self.state = MWMSearchManagerStateTableSearch;
}

- (IBAction)onBookingDateButtonPressed:(id)sender {
  [self.searchTextField resignFirstResponder];

  if (Platform::ConnectionStatus() == Platform::EConnectionType::CONNECTION_NONE) {
    [MWMAlertViewController.activeAlertController presentSearchQuickFilterNoConnectionAlert];
    [Statistics logEvent:kStatSearchQuickFilterError withParameters:@{kStatCategory: kStatHotel,
                                                                      kStatError: @"no internet",
                                                                      kStatFilter: kStatDate}];
    return;
  }

  DatePickerViewController *controller = [[DatePickerViewController alloc] init];
  controller.delegate = self;
  MWMHotelParams *filter = [MWMSearch getFilter];
  if (filter != nil && filter.checkInDate != nil && filter.checkOutDate != nil) {
    controller.initialCheckInDate = filter.checkInDate;
    controller.initialCheckOutDate = filter.checkOutDate;
  }
  controller.popoverPresentationController.sourceView = self.searchBarView;
  controller.popoverPresentationController.permittedArrowDirections = UIPopoverArrowDirectionUp;
  [[MapViewController sharedController] presentViewController:controller animated:YES completion:nil];
}

- (IBAction)onBookingGuestsButtonPressed:(id)sender {
  [self.searchTextField resignFirstResponder];

  if (Platform::ConnectionStatus() == Platform::EConnectionType::CONNECTION_NONE) {
    [MWMAlertViewController.activeAlertController presentSearchQuickFilterNoConnectionAlert];
    [Statistics logEvent:kStatSearchQuickFilterError withParameters:@{kStatCategory: kStatHotel,
                                                                      kStatError: @"no internet",
                                                                      kStatFilter: kStatSearchRooms}];
    return;
  }

  GuestsPickerViewController *controller = [[GuestsPickerViewController alloc] init];
  controller.delegate = self;
  MWMHotelParams *filter = [MWMSearch getFilter];
  if (filter != nil) {
    if (filter.numberOfRooms > 0) {
      controller.roomsInitialCount = filter.numberOfRooms;
    }
    if (filter.numberOfAdults > 0) {
      controller.adultsInitialCount = filter.numberOfAdults;
    }
    controller.childrenInitialCount = filter.numberOfChildren;
    controller.infantsInitialCount = filter.numberOfInfants;
  }
  controller.popoverPresentationController.sourceView = self.searchBarView;
  controller.popoverPresentationController.permittedArrowDirections = UIPopoverArrowDirectionUp;
  [[MapViewController sharedController] presentViewController:controller animated:YES completion:nil];
}

#pragma mark - Layout

- (void)viewWillTransitionToSize:(CGSize)size
       withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator {
  [self.navigationController viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
}

#pragma mark - UITextFieldDelegate

- (void)textFieldDidBeginEditing:(UITextField *)textField {
  BOOL const isEmpty = (textField.text.length == 0);
  self.state = isEmpty ? MWMSearchManagerStateDefault : MWMSearchManagerStateTableSearch;
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField {
  textField.text = [[textField.text stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceCharacterSet]
    stringByAppendingString:@" "];
  [self textFieldTextDidChange:textField];
  [textField resignFirstResponder];
  return YES;
}

#pragma mark - MWMSearchTabbedViewProtocol

- (void)searchText:(NSString *)text forInputLocale:(NSString *)locale {
  [self beginSearch];
  self.searchTextField.text = text;
  NSString *inputLocale = locale ?: self.searchTextField.textInputMode.primaryLanguage;
  [MWMSearch searchQuery:text forInputLocale:inputLocale];
}

- (void)dismissKeyboard {
  [self.searchTextField resignFirstResponder];
}
- (void)processSearchWithResult:(search::Result const &)result {
  if (self.routingTooltipSearch == MWMSearchManagerRoutingTooltipSearchNone) {
    [MWMSearch showResult:result];
  } else {
    BOOL const isStart = self.routingTooltipSearch == MWMSearchManagerRoutingTooltipSearchStart;
    auto point = [[MWMRoutePoint alloc] initWithPoint:result.GetFeatureCenter()
                                                title:@(result.GetString().c_str())
                                             subtitle:@(result.GetAddress().c_str())
                                                 type:isStart ? MWMRoutePointTypeStart : MWMRoutePointTypeFinish
                                    intermediateIndex:0];
    if (isStart)
      [MWMRouter buildFromPoint:point bestRouter:NO];
    else
      [MWMRouter buildToPoint:point bestRouter:NO];
  }
  if (!IPAD || [MWMNavigationDashboardManager sharedManager].state != MWMNavigationDashboardStateHidden)
    self.state = MWMSearchManagerStateResult;
}

#pragma mark - MWMStorageObserver

- (void)processCountryEvent:(NSString *)countryId {
  using namespace storage;
  NodeStatuses nodeStatuses{};
  GetFramework().GetStorage().GetNodeStatuses(countryId.UTF8String, nodeStatuses);
  if (nodeStatuses.m_status != NodeStatus::OnDisk)
    return;
  [self updateTopController];
  if (self.state == MWMSearchManagerStateTableSearch || self.state == MWMSearchManagerStateMapSearch) {
    NSString *text = self.searchTextField.text;
    if (text.length != 0)
      [MWMSearch searchQuery:text forInputLocale:self.searchTextField.textInputMode.primaryLanguage];
  }
}

#pragma mark - State changes

- (void)updateTopController {
  UIViewController *selfTopVC = self.topController;
  if (!selfTopVC || [selfTopVC isEqual:self.navigationController.topViewController])
    return;
  NSMutableArray *viewControllers = [self.navigationController.viewControllers mutableCopy];
  viewControllers[0] = selfTopVC;
  [self.navigationController setViewControllers:viewControllers animated:NO];
}

- (void)changeToHiddenState {
  self.routingTooltipSearch = MWMSearchManagerRoutingTooltipSearchNone;
  [self endSearch];

  MWMMapViewControlsManager *controlsManager = self.controlsManager;
  auto const navigationManagerState = [MWMNavigationDashboardManager sharedManager].state;
  if (navigationManagerState == MWMNavigationDashboardStateHidden) {
    controlsManager.menuState = controlsManager.menuRestoreState;
  }
  [self viewHidden:YES];
  self.searchBarView.isBookingSearchViewHidden = YES;
}

- (void)changeToDefaultState {
  MWMMapViewControlsManager *controlsManager = self.controlsManager;

  [self.navigationController popToRootViewControllerAnimated:NO];

  self.searchBarView.state = SearchBarStateReady;
  GetFramework().DeactivateMapSelection(true);
  [self animateConstraints:^{
    self.contentViewTopHidden.priority = UILayoutPriorityDefaultLow;
    self.contentViewBottomHidden.priority = UILayoutPriorityDefaultLow;
  }];
  auto const navigationManagerState = [MWMNavigationDashboardManager sharedManager].state;
  if (navigationManagerState == MWMNavigationDashboardStateHidden) {
    controlsManager.menuState = controlsManager.menuRestoreState;
  }
  [self viewHidden:NO];
  self.searchBarView.isBookingSearchViewHidden = YES;
  self.actionBarState = MWMSearchManagerActionBarStateHidden;
  [self.searchTextField becomeFirstResponder];
  [self.searchBarView applyTheme];
}

- (void)changeToTableSearchState {
  MWMMapViewControlsManager *controlsManager = self.controlsManager;

  [self.navigationController popToRootViewControllerAnimated:NO];

  self.searchBarView.state = SearchBarStateReady;
  GetFramework().DeactivateMapSelection(true);
  [self updateTableSearchActionBar];
  auto const navigationManagerState = [MWMNavigationDashboardManager sharedManager].state;
  if (navigationManagerState == MWMNavigationDashboardStateHidden) {
    controlsManager.menuState = controlsManager.menuRestoreState;
  }
  [self viewHidden:NO];
  [MWMSearch setSearchOnMap:NO];
  [self.tableViewController reloadData];

  if (![self.navigationController.viewControllers containsObject:self.tableViewController])
    [self.navigationController pushViewController:self.tableViewController animated:NO];
}

- (void)changeToMapSearchState {
  [self.navigationController popToRootViewControllerAnimated:NO];

  self.searchBarView.state = SearchBarStateBack;
  self.actionBarState = MWMSearchManagerActionBarStateModeFilter;
  if (!IPAD) {
    [self animateConstraints:^{
      self.contentViewTopHidden.priority = UILayoutPriorityDefaultHigh;
      self.contentViewBottomHidden.priority = UILayoutPriorityDefaultHigh;
    }];
  }
  auto const navigationManagerState = [MWMNavigationDashboardManager sharedManager].state;
  [self viewHidden:navigationManagerState != MWMNavigationDashboardStateHidden];
  self.controlsManager.menuState = MWMBottomMenuStateHidden;
  GetFramework().DeactivateMapSelection(true);
  [MWMSearch setSearchOnMap:YES];
  [self.tableViewController reloadData];

  [self.searchTextField resignFirstResponder];

  if (navigationManagerState == MWMNavigationDashboardStateNavigation) {
    self.searchTextField.text = @"";
  }
}

- (void)changeToResultSearchState {
  [self.navigationController popToRootViewControllerAnimated:NO];

  self.searchBarView.state = SearchBarStateBack;
  self.actionBarState = MWMSearchManagerActionBarStateModeFilter;
  if (!IPAD) {
    [self animateConstraints:^{
      self.contentViewTopHidden.priority = UILayoutPriorityDefaultHigh;
      self.contentViewBottomHidden.priority = UILayoutPriorityDefaultHigh;

    }];
  }
  auto const navigationManagerState = [MWMNavigationDashboardManager sharedManager].state;
  [self viewHidden:navigationManagerState != MWMNavigationDashboardStateHidden];
  [self.tableViewController reloadData];

  [self.searchTextField resignFirstResponder];

  if (navigationManagerState == MWMNavigationDashboardStateNavigation) {
    self.searchTextField.text = @"";
  }
}

- (void)animateConstraints:(MWMVoidBlock)block {
  UIView *parentView = self.searchViewContainer;
  [parentView layoutIfNeeded];
  block();
  [UIView animateWithDuration:kDefaultAnimationDuration
                   animations:^{
                     [parentView layoutIfNeeded];
                   }];
}

#pragma mark - MWMSearchObserver

- (void)onSearchCompleted {
  if (self.state == MWMSearchManagerStateMapSearch || self.state == MWMSearchManagerStateResult) {
    self.searchBarView.state = SearchBarStateBack;
  } else {
    self.searchBarView.state = SearchBarStateReady;
  }

  self.searchBarView.isBookingSearchViewHidden = !([MWMSearch isHotelResults] || [MWMSearch hasAvailability]);
  [self.actionBarView updateFilterButtonWithShowFilter:[MWMSearch isHotelResults] || [MWMSearch hasFilter]
                                           filterCount:[MWMSearch filterCount]];
  if (self.state != MWMSearchManagerStateTableSearch)
    return;
  [self.tableViewController onSearchCompleted];
  [self updateTableSearchActionBar];
}

- (void)onSearchStarted {
  self.searchBarView.state = SearchBarStateSearching;
  self.searchBarView.isBookingSearchViewHidden = !([MWMSearch isHotelResults] || [MWMSearch hasAvailability]);
  [self.actionBarView updateFilterButtonWithShowFilter:[MWMSearch isHotelResults] || [MWMSearch hasFilter]
                                           filterCount:[MWMSearch filterCount]];
  if (self.state != MWMSearchManagerStateTableSearch)
    return;
  self.actionBarState = MWMSearchManagerActionBarStateModeFilter;
}

- (void)onSearchResultsUpdated {
  [self.tableViewController reloadData];
}

- (void)updateTableSearchActionBar {
  if (self.state != MWMSearchManagerStateTableSearch)
    return;
  [self animateConstraints:^{
    BOOL hideActionBar = NO;
    if ([MWMSearch resultsCount] == 0)
      hideActionBar = YES;
    else if (IPAD)
      hideActionBar = !([MWMSearch isHotelResults] || [MWMSearch hasFilter]);
    self.actionBarState =
      hideActionBar ? MWMSearchManagerActionBarStateHidden : MWMSearchManagerActionBarStateModeFilter;

    self.contentViewTopHidden.priority = UILayoutPriorityDefaultLow;
    self.contentViewBottomHidden.priority = UILayoutPriorityDefaultLow;
  }];
}

#pragma mark - Add/Remove Observers

+ (void)addObserver:(id<MWMSearchManagerObserver>)observer {
  [[MWMSearchManager manager].observers addObject:observer];
}

+ (void)removeObserver:(id<MWMSearchManagerObserver>)observer {
  [[MWMSearchManager manager].observers removeObject:observer];
}

#pragma mark - MWMSearchManagerObserver

- (void)onSearchManagerStateChanged {
  for (Observer observer in self.observers)
    [observer onSearchManagerStateChanged];
}

#pragma mark - DatePickerViewControllerDelegate

- (void)datePicker:(DatePickerViewController *)datePicker
  didSelectStartDate:(NSDate *)startDate
             endDate:(NSDate *)endDate {
  [self.searchBarView setDatesWithCheckin:startDate checkout:endDate];
  MWMHotelParams *filter = [MWMSearch getFilter];
  if (!filter) {
    filter = [MWMHotelParams new];
  }
  filter.checkInDate = startDate;
  filter.checkOutDate = endDate;
  [MWMSearch updateHotelFilterWithParams:filter];
  [[MapViewController sharedController] dismissViewControllerAnimated:YES completion:nil];
}

- (void)datePickerDidCancel:(DatePickerViewController *)datePicker {
  [[MapViewController sharedController] dismissViewControllerAnimated:YES completion:nil];
}

- (void)datePickerDidClick:(DatePickerViewController *)datePicker
        didSelectStartDate:(NSDate *)startDate
                   endDate:(NSDate *)endDate {
  NSString *startString = startDate ? [self.dateFormatter stringFromDate:startDate] : kStatNone;
  NSString *endString = endDate ? [self.dateFormatter stringFromDate:endDate] : kStatNone;
  [Statistics logEvent:kStatSearchQuickFilterClick withParameters:@{ kStatCategory: kStatHotel,
                                                                     kStatDate: @[startString, endString]}];
}

#pragma mark - GuestsPickerViewControllerDelegate

- (void)guestsPicker:(GuestsPickerViewController *)guestsPicker
      didSelectRooms:(NSInteger)rooms
              adults:(NSInteger)adults
            children:(NSInteger)children
             infants:(NSInteger)infants {
  [self.searchBarView setGuestCount:adults + children + infants];
  MWMHotelParams *filter = [MWMSearch getFilter];
  if (!filter) {
    filter = [MWMHotelParams new];
  }
  filter.numberOfRooms = rooms;
  filter.numberOfAdults = adults;
  filter.numberOfChildren = children;
  filter.numberOfInfants = infants;
  [MWMSearch updateHotelFilterWithParams:filter];
  [[MapViewController sharedController] dismissViewControllerAnimated:YES completion:nil];
}

- (void)guestPickerDidClick:(GuestsPickerViewController *)guestsPicker
             didSelectRooms:(NSInteger)rooms
                     adults:(NSInteger)adults
                   children:(NSInteger)children
                    infants:(NSInteger)infants {
  [Statistics logEvent:kStatSearchQuickFilterClick withParameters:@{kStatCategory: kStatHotel,
                                                                    kStatSearchRooms: @(rooms),
                                                                    kStatSearchAdults: @(adults),
                                                                    kStatSearchChildren: @(children),
                                                                    kStatSearchInfants: @(infants)}];
}

- (void)guestsPickerDidCancel:(GuestsPickerViewController *)guestsPicker {
  [[MapViewController sharedController] dismissViewControllerAnimated:YES completion:nil];
}

#pragma mark - Filters

- (IBAction)changeMode {
  switch (self.state) {
    case MWMSearchManagerStateTableSearch:
      self.state = MWMSearchManagerStateMapSearch;
      [Statistics logEvent:kStatSearchContextAreaClick withParameters:@{kStatValue: kStatMap}];
      break;
    case MWMSearchManagerStateMapSearch:
      self.state = MWMSearchManagerStateTableSearch;
      [Statistics logEvent:kStatSearchContextAreaClick withParameters:@{kStatValue: kStatList}];
      break;
    default:
      break;
  }
}

#pragma mark - Properties

- (UINavigationController *)navigationController {
  if (!_navigationController) {
    _navigationController = [[UINavigationController alloc] init];
    [self.contentView addSubview:_navigationController.view];
    _navigationController.navigationBarHidden = YES;
  }
  return _navigationController;
}

- (UIViewController *)topController {
  [[MWMStorage sharedStorage] removeObserver:self];
  self.noMapsController = nil;
  switch (self.state) {
    case MWMSearchManagerStateHidden:
      return nil;
    case MWMSearchManagerStateDefault: {
      if (GetFramework().GetStorage().HaveDownloadedCountries()) {
        MWMSearchTabViewController *tabViewController = [MWMSearchTabViewController new];
        tabViewController.delegate = self;
        return tabViewController;
      }
      self.noMapsController = [MWMNoMapsViewController controller];
      [[MWMStorage sharedStorage] addObserver:self];
      return self.noMapsController;
    }
    case MWMSearchManagerStateTableSearch:
      return self.tableViewController;
    case MWMSearchManagerStateMapSearch:
      return self.tableViewController;
    case MWMSearchManagerStateResult:
      return self.tableViewController;
  }
}

- (void)searchTabController:(MWMSearchTabViewController *)viewController didSearch:(NSString *)didSearch {
  [self searchText:didSearch forInputLocale:[[AppInfo sharedInfo] languageId]];
}

- (MWMSearchTableViewController *)tableViewController {
  if (!_tableViewController)
    _tableViewController = [[MWMSearchTableViewController alloc] initWithDelegate:self];
  return _tableViewController;
}

- (void)setState:(MWMSearchManagerState)state {
  if (_state == state)
    return;

  _state = state;
  [self updateTopController];
  switch (state) {
    case MWMSearchManagerStateHidden:
      [Statistics logEvent:kStatSearchEnteredState withParameters:@{kStatName: kStatClose}];
      [self changeToHiddenState];
      break;
    case MWMSearchManagerStateDefault:
      [Statistics logEvent:kStatSearchEnteredState withParameters:@{kStatName: kStatOpen}];
      [self changeToDefaultState];
      break;
    case MWMSearchManagerStateTableSearch:
      [Statistics logEvent:kStatSearchEnteredState withParameters:@{kStatName: kStatTable}];
      [self changeToTableSearchState];
      break;
    case MWMSearchManagerStateMapSearch:
      [Statistics logEvent:kStatSearchEnteredState withParameters:@{kStatName: kStatMapSearch}];
      [self changeToMapSearchState];
      break;
    case MWMSearchManagerStateResult:
      [Statistics logEvent:kStatSearchEnteredState withParameters:@{kStatName: kStatSearchMapOneResult}];
      [self changeToResultSearchState];
      break;
  }
  [self onSearchManagerStateChanged];
  [self.actionBarView updateForState:state];
  [[MapViewController sharedController] updateStatusBarStyle];
}

- (void)viewHidden:(BOOL)hidden {
  UIView *searchBarView = self.searchBarView;
  UIView *actionBarView = self.actionBarView;
  UIView *contentView = self.contentView;
  UIView *parentView = self.searchViewContainer;

  if (!hidden) {
    if (searchBarView.superview) {
      [parentView bringSubviewToFront:searchBarView];
      [parentView bringSubviewToFront:contentView];
      [parentView bringSubviewToFront:actionBarView];
      return;
    }
    [parentView addSubview:searchBarView];
    [parentView addSubview:contentView];
    [parentView addSubview:actionBarView];
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

- (void)setActionBarState:(MWMSearchManagerActionBarState)actionBarState {
  switch (actionBarState) {
    case MWMSearchManagerActionBarStateHidden:
      self.actionBarView.hidden = YES;
      break;
    case MWMSearchManagerActionBarStateTabBar:
      self.actionBarView.hidden = YES;
      break;
    case MWMSearchManagerActionBarStateModeFilter:
      self.actionBarView.hidden = NO;
      break;
  }
}

- (UIViewController *)ownerController {
  return [MapViewController sharedController];
}
- (UIView *)searchViewContainer {
  return [MapViewController sharedController].searchViewContainer;
}
- (UIView *)actionBarContainer {
  return [MapViewController sharedController].controlsView;
}

- (MWMMapViewControlsManager *)controlsManager {
  return [MWMMapViewControlsManager manager];
}
@end
