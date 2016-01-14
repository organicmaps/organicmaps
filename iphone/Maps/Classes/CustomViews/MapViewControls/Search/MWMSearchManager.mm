#import "LocationManager.h"
#import "MapsAppDelegate.h"
#import "MapsObservers.h"
#import "MWMConsole.h"
#import "MWMRoutingProtocol.h"
#import "MWMSearchDownloadViewController.h"
#import "MWMSearchManager.h"
#import "MWMSearchTabbedViewController.h"
#import "MWMSearchTabButtonsView.h"
#import "MWMSearchTableViewController.h"
#import "Statistics.h"

#import "3party/Alohalytics/src/alohalytics_objc.h"

#include "Framework.h"
#include "map/active_maps_layout.hpp"

extern NSString * const kAlohalyticsTapEventKey;
extern NSString * const kSearchStateWillChangeNotification = @"SearchStateWillChangeNotification";
extern NSString * const kSearchStateKey = @"SearchStateKey";

@interface MWMSearchManager ()<MWMSearchTableViewProtocol, MWMSearchDownloadProtocol,
                               MWMSearchTabbedViewProtocol, ActiveMapsObserverProtocol,
                               MWMSearchTabButtonsViewProtocol, UITextFieldDelegate>

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
{
  unique_ptr<ActiveMapsObserver> m_mapsObserver;
  int m_mapsObserverSlotId;
}

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

- (void)refresh
{
  [self.rootView refresh];
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
  [[Statistics instance] logEvent:kStatEventName(kStatSearch, kStatCancel)];
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
  MWMRoutePoint const p = MWMRoutePoint::MWMRoutePoint(a.m_locationManager.lastLocation.mercator);
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
  MWMRoutePoint const p = {result.GetFeatureCenter(), @(result.GetString())};
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

#pragma mark - MWMSearchDownloadMapRequest

- (void)selectMapsAction
{
  [self.delegate actionDownloadMaps];
}

#pragma mark - ActiveMapsObserverProtocol

- (void)countryStatusChangedAtPosition:(int)position inGroup:(ActiveMapsLayout::TGroup const &)group
{
  auto const status =
      GetFramework().GetCountryTree().GetActiveMapLayout().GetCountryStatus(group, position);
  [self updateTopController];
  if (status == TStatus::EDownloadFailed)
    [self.downloadController setDownloadFailed];
  if (self.state == MWMSearchManagerStateTableSearch ||
      self.state == MWMSearchManagerStateMapSearch)
  {
    NSString * text = self.searchTextField.text;
    if (text.length > 0)
      [self.tableViewController searchText:text
                            forInputLocale:self.searchTextField.textInputMode.primaryLanguage];
  }
}

- (void)countryDownloadingProgressChanged:(LocalAndRemoteSizeT const &)progress
                               atPosition:(int)position
                                  inGroup:(ActiveMapsLayout::TGroup const &)group
{
  CGFloat const normProgress = (CGFloat)progress.first / progress.second;
  ActiveMapsLayout & activeMapLayout = GetFramework().GetCountryTree().GetActiveMapLayout();
  NSString * countryName =
      @(activeMapLayout.GetFormatedCountryName(activeMapLayout.GetCoreIndex(group, position))
            .c_str());
  [self.downloadController downloadProgress:normProgress countryName:countryName];
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

- (void)changeFromHiddenState
{
  __weak auto weakSelf = self;
  m_mapsObserver.reset(new ActiveMapsObserver(weakSelf));
  m_mapsObserverSlotId = GetFramework().GetCountryTree().GetActiveMapLayout().AddListener(m_mapsObserver.get());
}

- (void)changeToHiddenState
{
  [self endSearch];
  GetFramework().GetCountryTree().GetActiveMapLayout().RemoveListener(m_mapsObserverSlotId);
  [self.tabbedController resetSelectedTab];
  self.tableViewController = nil;
  self.downloadController = nil;
  self.rootView.isVisible = NO;
}

- (void)changeToDefaultState
{
  self.view.alpha = 1.;
  GetFramework().PrepareSearch();
  [self updateTopController];
  [self.navigationController popToRootViewControllerAnimated:NO];
  [self.parentView addSubview:self.rootView];
  self.rootView.compact = NO;
  self.rootView.isVisible = YES;
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
  Framework & f = GetFramework();
  UITextField * textField = self.searchTextField;

  string const locale = textField.textInputMode.primaryLanguage ?
            textField.textInputMode.primaryLanguage.UTF8String :
            self.tableViewController.searchParams.m_inputLocale;
  f.SaveSearchQuery(make_pair(locale, textField.text.precomposedStringWithCompatibilityMapping.UTF8String));
  f.ActivateUserMark(nullptr, true);
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
  auto & f = GetFramework();
  auto & activeMapLayout = f.GetCountryTree().GetActiveMapLayout();
  int const outOfDate = activeMapLayout.GetCountInGroup(storage::ActiveMapsLayout::TGroup::EOutOfDate);
  int const upToDate = activeMapLayout.GetCountInGroup(storage::ActiveMapsLayout::TGroup::EUpToDate);
  BOOL const haveMap = outOfDate > 0 || upToDate > 0;
  return haveMap ? self.tabbedController : self.downloadController;
}

- (MWMSearchDownloadViewController *)downloadController
{
  if (!_downloadController)
    _downloadController = [[MWMSearchDownloadViewController alloc] initWithDelegate:self];
  return _downloadController;
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
  if (_state == MWMSearchManagerStateHidden)
    [self changeFromHiddenState];
  _state = state;
  switch (state)
  {
  case MWMSearchManagerStateHidden:
    [[Statistics instance] logEvent:kStatSearchEnteredState withParameters:@{kStatName : kStatClose}];
    [self changeToHiddenState];
    break;
  case MWMSearchManagerStateDefault:
    [[Statistics instance] logEvent:kStatSearchEnteredState withParameters:@{kStatName : kStatOpen}];
    [self changeToDefaultState];
    break;
  case MWMSearchManagerStateTableSearch:
    [[Statistics instance] logEvent:kStatSearchEnteredState withParameters:@{kStatName : kStatTable}];
    [self changeToTableSearchState];
    break;
  case MWMSearchManagerStateMapSearch:
    [[Statistics instance] logEvent:kStatSearchEnteredState withParameters:@{kStatName : kStatMap}];
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
