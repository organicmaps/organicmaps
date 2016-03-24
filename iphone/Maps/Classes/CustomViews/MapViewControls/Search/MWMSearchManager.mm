#import "LocationManager.h"
#import "MapsAppDelegate.h"
#import "MWMConsole.h"
#import "MWMFrameworkListener.h"
#import "MWMRoutingProtocol.h"
#import "MWMSearchDownloadViewController.h"
#import "MWMSearchManager.h"
#import "MWMSearchTabbedViewController.h"
#import "MWMSearchTabButtonsView.h"
#import "MWMSearchTableViewController.h"
#import "Statistics.h"

#import "3party/Alohalytics/src/alohalytics_objc.h"

#include "storage/storage_helpers.hpp"

#include "Framework.h"

extern NSString * const kAlohalyticsTapEventKey;
extern NSString * const kSearchStateWillChangeNotification = @"SearchStateWillChangeNotification";
extern NSString * const kSearchStateKey = @"SearchStateKey";

@interface MWMSearchManager ()<MWMSearchTableViewProtocol, MWMSearchDownloadProtocol,
                               MWMSearchTabbedViewProtocol, MWMSearchTabButtonsViewProtocol,
                               UITextFieldDelegate, MWMFrameworkStorageObserver>

@property (weak, nonatomic) UIView * parentView;
@property (nonatomic) IBOutlet MWMSearchView * rootView;
@property (weak, nonatomic) IBOutlet UIView * contentView;

@property (nonatomic) IBOutletCollection(MWMSearchTabButtonsView) NSArray * tabButtons;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * scrollIndicatorOffset;
@property (weak, nonatomic) IBOutlet UIView * scrollIndicator;

@property (nonatomic) UINavigationController * navigationController;
@property (nonatomic) MWMSearchTabbedViewController * tabbedController;
@property (nonatomic) MWMSearchTableViewController * tableViewController;
@property (nonatomic) MWMSearchDownloadViewController * downloadController;

@end

@implementation MWMSearchManager

- (nullable instancetype)initWithParentView:(nonnull UIView *)view
                                   delegate:(nonnull id<MWMSearchManagerProtocol, MWMSearchViewProtocol, MWMRoutingProtocol>)delegate
{
  self = [super init];
  if (self)
  {
    [NSBundle.mainBundle loadNibNamed:@"MWMSearchView" owner:self options:nil];
    self.delegate = delegate;
    self.rootView.delegate = delegate;
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
  self.searchTextField.isSearching = YES;
}

- (void)endSearch
{
  GetFramework().CancelInteractiveSearch();
  if (self.state != MWMSearchManagerStateHidden)
    self.state = MWMSearchManagerStateDefault;
  self.searchTextField.isSearching = NO;
  self.searchTextField.text = @"";
  [self.tableViewController searchText:@"" forInputLocale:nil];
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
    if ([MWMConsole performCommand:text])
    {
      self.state = MWMSearchManagerStateHidden;
    }
    else
    {
      [self beginSearch];
      [self.tableViewController searchText:text
                            forInputLocale:textField.textInputMode.primaryLanguage];
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
  [self.tableViewController searchText:text forInputLocale:inputLocale];
}

- (void)tapMyPositionFromHistory
{
  MapsAppDelegate * a = MapsAppDelegate.theApp;
  MWMRoutePoint const p = MWMRoutePoint::MWMRoutePoint(a.locationManager.lastLocation.mercator);
  if (a.routingPlaneMode == MWMRoutingPlaneModeSearchSource)
    [self.delegate buildRouteFrom:p];
  else if (a.routingPlaneMode == MWMRoutingPlaneModeSearchDestination)
    [self.delegate buildRouteTo:p];
  else
    NSAssert(false, @"Incorrect state for process my position tap");
  if (!IPAD)
    a.routingPlaneMode = MWMRoutingPlaneModePlacePage;
  self.state = MWMSearchManagerStateHidden;
}

- (void)processSearchWithResult:(search::Result const &)result query:(search::QuerySaver::TSearchRequest const &)query
{
  auto & f = GetFramework();
  f.SaveSearchQuery(query);
  MapsAppDelegate * a = MapsAppDelegate.theApp;
  MWMRoutingPlaneMode const m = a.routingPlaneMode;
  MWMRoutePoint const p = {result.GetFeatureCenter(), @(result.GetString().c_str())};
  if (m == MWMRoutingPlaneModeSearchSource)
    [self.delegate buildRouteFrom:p];
  else if (m == MWMRoutingPlaneModeSearchDestination)
     [self.delegate buildRouteTo:p];
  else
    f.ShowSearchResult(result);
  if (!IPAD && a.routingPlaneMode != MWMRoutingPlaneModeNone)
    a.routingPlaneMode = MWMRoutingPlaneModePlacePage;
  self.state = MWMSearchManagerStateHidden;
}

#pragma mark - MWMFrameworkStorageObserver

- (void)processCountryEvent:(storage::TCountryId const &)countryId
{
  using namespace storage;
  [self updateTopController];
  NodeStatuses nodeStatuses{};
  GetFramework().Storage().GetNodeStatuses(countryId, nodeStatuses);
  if (nodeStatuses.m_status == NodeStatus::Error)
    [self.downloadController setDownloadFailed];
  if (self.state == MWMSearchManagerStateTableSearch || self.state == MWMSearchManagerStateMapSearch)
  {
    NSString * text = self.searchTextField.text;
    if (text.length > 0)
      [self.tableViewController searchText:text
                            forInputLocale:self.searchTextField.textInputMode.primaryLanguage];
  }
}

- (void)processCountry:(storage::TCountryId const &)countryId progress:(storage::TLocalAndRemoteSize const &)progress
{
  [self.downloadController downloadProgress:static_cast<CGFloat>(progress.first) / progress.second];
}

#pragma mark - MWMSearchDownloadProtocol

- (MWMAlertViewController *)alertController
{
  return self.delegate.alertController;
}

- (void)selectMapsAction
{
  [self.delegate actionDownloadMaps];
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
  self.downloadController = nil;
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
  if (![self.topController isEqual:self.downloadController])
    [self.searchTextField becomeFirstResponder];
}

- (void)changeToTableSearchState
{
  self.rootView.tabBarIsVisible = NO;
  self.tableViewController.searchOnMap = NO;
  [self.navigationController pushViewController:self.tableViewController animated:NO];
}

- (void)changeToMapSearchState
{
  auto & f = GetFramework();
  UITextField * textField = self.searchTextField;

  string const locale = textField.textInputMode.primaryLanguage ?
            textField.textInputMode.primaryLanguage.UTF8String :
            self.tableViewController.searchParams.m_inputLocale;
  f.SaveSearchQuery(make_pair(locale, textField.text.precomposedStringWithCompatibilityMapping.UTF8String));
  f.DeactivateMapSelection(true);
  [self.searchTextField resignFirstResponder];
  self.rootView.compact = YES;
  self.tableViewController.searchOnMap = YES;
}

#pragma mark - Properties

- (UINavigationController *)navigationController
{
  if (!_navigationController)
  {
    _navigationController = [[UINavigationController alloc] initWithRootViewController:self.topController];
    [self.contentView addSubview:_navigationController.view];
    _navigationController.navigationBarHidden = YES;
  }
  return _navigationController;
}

- (UIViewController *)topController
{
  if (self.state == MWMSearchManagerStateHidden || GetFramework().Storage().HaveDownloadedCountries())
  {
    return self.tabbedController;
  }
  else
  {
    if (!self.downloadController)
      self.downloadController = [[MWMSearchDownloadViewController alloc] initWithDelegate:self];
    return self.downloadController;
  }
}

- (void)setDownloadController:(MWMSearchDownloadViewController *)downloadController
{
  _downloadController = downloadController;
  if (downloadController)
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
  [self.delegate searchViewDidEnterState:state];
}

- (UIView *)view
{
  return self.rootView;
}

@end
