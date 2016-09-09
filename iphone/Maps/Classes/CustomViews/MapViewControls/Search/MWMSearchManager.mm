#import "MWMSearchManager.h"
#import "MWMConsole.h"
#import "MWMFrameworkListener.h"
#import "MWMLocationManager.h"
#import "MWMMapViewControlsManager.h"
#import "MWMNoMapsViewController.h"
#import "MWMRouter.h"
#import "MWMSearch.h"
#import "MWMSearchTabButtonsView.h"
#import "MWMSearchTabbedViewController.h"
#import "MWMSearchTableViewController.h"
#import "MapsAppDelegate.h"
#import "Statistics.h"

#import "3party/Alohalytics/src/alohalytics_objc.h"

#include "MWMRoutePoint.h"

#include "storage/storage_helpers.hpp"

#include "Framework.h"

extern NSString * const kAlohalyticsTapEventKey;
extern NSString * const kSearchStateWillChangeNotification = @"SearchStateWillChangeNotification";
extern NSString * const kSearchStateKey = @"SearchStateKey";

@interface MWMSearchManager ()<MWMSearchTableViewProtocol, MWMSearchTabbedViewProtocol,
                               MWMSearchTabButtonsViewProtocol, UITextFieldDelegate,
                               MWMFrameworkStorageObserver>

@property(weak, nonatomic) UIView * parentView;
@property(nonatomic) IBOutlet MWMSearchView * rootView;
@property(weak, nonatomic) IBOutlet UIView * contentView;

@property(nonatomic) IBOutletCollection(MWMSearchTabButtonsView) NSArray * tabButtons;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * scrollIndicatorOffset;
@property(weak, nonatomic) IBOutlet UIView * scrollIndicator;

@property(nonatomic) UINavigationController * navigationController;
@property(nonatomic) MWMSearchTabbedViewController * tabbedController;
@property(nonatomic) MWMSearchTableViewController * tableViewController;
@property(nonatomic) MWMNoMapsViewController * noMapsController;

@end

@implementation MWMSearchManager

- (nullable instancetype)initWithParentView:(nonnull UIView *)view
{
  self = [super init];
  if (self)
  {
    [NSBundle.mainBundle loadNibNamed:@"MWMSearchView" owner:self options:nil];
    self.parentView = view;
    self.state = MWMSearchManagerStateHidden;
  }
  return self;
}

- (void)mwm_refreshUI
{
  [self.rootView mwm_refreshUI];
  if (self.state == MWMSearchManagerStateHidden)
    return;
  [self.tabbedController mwm_refreshUI];
  [self.tableViewController mwm_refreshUI];
}

- (void)beginSearch
{
  if (self.state == MWMSearchManagerStateDefault)
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

- (IBAction)textFieldDidBeginEditing:(UITextField *)textField
{
  if (self.state == MWMSearchManagerStateMapSearch)
    self.state = MWMSearchManagerStateTableSearch;
}

- (IBAction)textFieldDidEndEditing:(UITextField *)textField
{
  if (textField.text.length == 0 && self.state != MWMSearchManagerStateHidden)
    [self endSearch];
}

- (IBAction)textFieldTextDidChange:(UITextField *)textField
{
  NSString * text = textField.text;
  if (text.length > 0)
  {
    if ([MWMConsole performCommand:text])
    {
      self.state = MWMSearchManagerStateHidden;
    }
    else
    {
      [self beginSearch];
      [MWMSearch searchQuery:text forInputLocale:textField.textInputMode.primaryLanguage];
    }
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
  MapsAppDelegate * a = MapsAppDelegate.theApp;
  MWMRoutingPlaneMode const m = a.routingPlaneMode;
  if (m == MWMRoutingPlaneModeSearchDestination || m == MWMRoutingPlaneModeSearchSource)
    a.routingPlaneMode = MWMRoutingPlaneModePlacePage;
}

- (void)tabButtonPressed:(MWMSearchTabButtonsView *)sender
{
  [self.searchTextField resignFirstResponder];
  [self.tabbedController tabButtonPressed:sender];
}

#pragma mark - Layout

- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation
                                duration:(NSTimeInterval)duration
{
  [self.navigationController willRotateToInterfaceOrientation:toInterfaceOrientation
                                                     duration:duration];
}

- (void)viewWillTransitionToSize:(CGSize)size
       withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator
{
  [self.navigationController viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
}

#pragma mark - UITextFieldDelegate

- (BOOL)textFieldShouldReturn:(nonnull UITextField *)textField
{
  [textField resignFirstResponder];
  if (textField.text.length != 0)
    self.state = MWMSearchManagerStateMapSearch;
  return YES;
}

#pragma mark - MWMSearchTabbedViewProtocol

- (void)searchText:(NSString *)text forInputLocale:(NSString *)locale
{
  [self beginSearch];
  self.searchTextField.text = text;
  NSString * inputLocale = locale ? locale : self.searchTextField.textInputMode.primaryLanguage;
  [MWMSearch searchQuery:text forInputLocale:inputLocale];
}

- (void)tapMyPositionFromHistory
{
  MapsAppDelegate * a = MapsAppDelegate.theApp;
  MWMRoutePoint const p = MWMRoutePoint::MWMRoutePoint([MWMLocationManager lastLocation].mercator);
  if (a.routingPlaneMode == MWMRoutingPlaneModeSearchSource)
    [[MWMRouter router] buildFromPoint:p bestRouter:YES];
  else if (a.routingPlaneMode == MWMRoutingPlaneModeSearchDestination)
    [[MWMRouter router] buildToPoint:p bestRouter:YES];
  else
    NSAssert(false, @"Incorrect state for process my position tap");
  if (!IPAD)
    a.routingPlaneMode = MWMRoutingPlaneModePlacePage;
  self.state = MWMSearchManagerStateHidden;
}

- (void)processSearchWithResult:(search::Result const &)result
{
  MapsAppDelegate * a = MapsAppDelegate.theApp;
  MWMRoutingPlaneMode const m = a.routingPlaneMode;
  if (m == MWMRoutingPlaneModeSearchSource)
  {
    MWMRoutePoint const p = { result.GetFeatureCenter(), @(result.GetString().c_str()) };
    [[MWMRouter router] buildFromPoint:p bestRouter:YES];
  }
  else if (m == MWMRoutingPlaneModeSearchDestination)
  {
    MWMRoutePoint const p = { result.GetFeatureCenter(), @(result.GetString().c_str()) };
    [[MWMRouter router] buildToPoint:p bestRouter:YES];
  }
  else
  {
    [MWMSearch showResult:result];
  }
  if (!IPAD && a.routingPlaneMode != MWMRoutingPlaneModeNone)
    a.routingPlaneMode = MWMRoutingPlaneModePlacePage;
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
  self.rootView.tabBarIsVisible =
      self.state == MWMSearchManagerStateDefault && [selfTopVC isEqual:self.tabbedController];
  if ([selfTopVC isEqual:self.navigationController.topViewController])
    return;
  NSMutableArray * viewControllers = [self.navigationController.viewControllers mutableCopy];
  viewControllers[0] = selfTopVC;
  [self.navigationController setViewControllers:viewControllers animated:NO];
}

- (void)changeToHiddenState
{
  [self endSearch];
  [self.tabbedController resetSelectedTab];
  self.tableViewController = nil;
  self.noMapsController = nil;
  self.rootView.isVisible = NO;
}

- (void)changeToDefaultState
{
  self.view.alpha = 1.;
  [self updateTopController];
  [self.navigationController popToRootViewControllerAnimated:NO];
  [self.parentView addSubview:self.rootView];
  self.rootView.compact = NO;
  self.rootView.isVisible = YES;
  if (self.topController != self.noMapsController)
    [self.searchTextField becomeFirstResponder];
}

- (void)changeToTableSearchState
{
  [self changeToDefaultState];
  self.rootView.compact = NO;
  self.rootView.tabBarIsVisible = NO;
  [MWMSearch setSearchOnMap:NO];
  if (![self.navigationController.viewControllers containsObject:self.tableViewController])
    [self.navigationController pushViewController:self.tableViewController animated:NO];
}

- (void)changeToMapSearchState
{
  [self changeToDefaultState];
  GetFramework().DeactivateMapSelection(true);
  [self.searchTextField resignFirstResponder];
  self.rootView.compact = YES;
  [MWMSearch setSearchOnMap:YES];

  if ([MWMNavigationDashboardManager manager].state == MWMNavigationDashboardStateNavigation)
  {
    self.searchTextField.text = @"";
    [self.tabbedController resetSelectedTab];
    self.tableViewController = nil;
    self.noMapsController = nil;
    self.rootView.isVisible = NO;
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
  if (self.state == MWMSearchManagerStateHidden ||
      GetFramework().GetStorage().HaveDownloadedCountries())
  {
    return self.tabbedController;
  }
  else
  {
    if (!self.noMapsController)
    {
      UIStoryboard * storyboard =
          [UIStoryboard storyboardWithName:@"Mapsme" bundle:[NSBundle mainBundle]];
      self.noMapsController =
          [storyboard instantiateViewControllerWithIdentifier:@"MWMNoMapsViewController"];
    }
    return self.noMapsController;
  }
}

- (void)setNoMapsController:(MWMNoMapsViewController *)noMapsController
{
  _noMapsController = noMapsController;
  if (noMapsController)
    [MWMFrameworkListener addObserver:self];
  else
    [MWMFrameworkListener removeObserver:self];
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
  [[NSNotificationCenter defaultCenter] postNotificationName:kSearchStateWillChangeNotification
                                                      object:nil
                                                    userInfo:@{
                                                      kSearchStateKey : @(state)
                                                    }];
  _state = state;
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
  [[MWMMapViewControlsManager manager] searchViewDidEnterState:state];
}

- (UIView *)view { return self.rootView; }
@end
