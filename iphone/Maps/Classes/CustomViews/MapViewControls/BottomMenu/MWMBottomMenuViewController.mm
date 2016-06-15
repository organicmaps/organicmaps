#import "Common.h"
#import "EAGLView.h"
#import "MapsAppDelegate.h"
#import "MapViewController.h"
#import "MWMActivityViewController.h"
#import "MWMBottomMenuCollectionViewCell.h"
#import "MWMBottomMenuLayout.h"
#import "MWMBottomMenuView.h"
#import "MWMBottomMenuViewController.h"
#import "MWMButton.h"
#import "MWMFrameworkListener.h"
#import "MWMFrameworkObservers.h"
#import "MWMMapViewControlsManager.h"
#import "MWMSearchManager.h"
#import "SettingsAndMoreVC.h"
#import "Statistics.h"
#import "UIColor+MapsMeColor.h"
#import "UIImageView+Coloring.h"
#import "UIKitCategories.h"

#import "3party/Alohalytics/src/alohalytics_objc.h"

#include "Framework.h"

#include "platform/mwm_version.hpp"

extern NSString * const kAlohalyticsTapEventKey;
extern NSString * const kSearchStateWillChangeNotification;
extern NSString * const kSearchStateKey;

static NSString * const kCollectionCellPortrait = @"MWMBottomMenuCollectionViewPortraitCell";
static NSString * const kCollectionCelllandscape = @"MWMBottomMenuCollectionViewLandscapeCell";

static CGFloat const kLayoutThreshold = 420.0;

typedef NS_ENUM(NSUInteger, MWMBottomMenuViewCell)
{
  MWMBottomMenuViewCellAddPlace,
  MWMBottomMenuViewCellDownload,
  MWMBottomMenuViewCellSettings,
  MWMBottomMenuViewCellShare,
  MWMBottomMenuViewCellAd,
  MWMBottomMenuViewCellCount
};

@interface MWMBottomMenuViewController () <UICollectionViewDataSource, UICollectionViewDelegate>

@property (weak, nonatomic) MapViewController * controller;
@property (weak, nonatomic) IBOutlet UICollectionView * buttonsCollectionView;

@property (weak, nonatomic) IBOutlet UICollectionView * additionalButtons;
@property (weak, nonatomic) IBOutlet UILabel * streetLabel;

@property (weak, nonatomic) id<MWMBottomMenuControllerProtocol> delegate;

@property (nonatomic) BOOL searchIsActive;

@property (nonatomic) SolidTouchView * dimBackground;

@property (nonatomic) MWMBottomMenuState restoreState;

@property (nonatomic, readonly) NSUInteger additionalButtonsCount;

@end

@implementation MWMBottomMenuViewController

- (instancetype)initWithParentController:(MapViewController *)controller
                                delegate:(id<MWMBottomMenuControllerProtocol>)delegate
{
  self = [super init];
  if (self)
  {
    _controller = controller;
    _delegate = delegate;
    [controller addChildViewController:self];
    MWMBottomMenuView * view = (MWMBottomMenuView *)self.view;
    [controller.view addSubview:view];
    view.maxY = controller.view.height;
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(searchStateWillChange:)
                                                 name:kSearchStateWillChangeNotification
                                               object:nil];
  }
  return self;
}

- (void)dealloc
{
  [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)viewDidLoad
{
  [super viewDidLoad];
  [self.buttonsCollectionView registerNib:[UINib nibWithNibName:kCollectionCellPortrait bundle:nil]
               forCellWithReuseIdentifier:kCollectionCellPortrait];
  [self.buttonsCollectionView registerNib:[UINib nibWithNibName:kCollectionCelllandscape bundle:nil]
               forCellWithReuseIdentifier:kCollectionCelllandscape];
  MWMBottomMenuLayout * cvLayout =
  (MWMBottomMenuLayout *)self.buttonsCollectionView.collectionViewLayout;
  cvLayout.layoutThreshold = kLayoutThreshold;
  ((MWMBottomMenuView *)self.view).layoutThreshold = kLayoutThreshold;
}

- (void)viewWillAppear:(BOOL)animated
{
  [super viewWillAppear:animated];
  [self refreshLayout];
}

- (void)mwm_refreshUI
{
  [self.view mwm_refreshUI];
}

#pragma mark - Refresh Collection View layout

- (void)refreshLayout
{
  MWMBottomMenuLayout * cvLayout =
      (MWMBottomMenuLayout *)self.buttonsCollectionView.collectionViewLayout;
  cvLayout.buttonsCount = [self additionalButtonsCount];
  [self.additionalButtons reloadData];
  [(MWMBottomMenuView *)self.view refreshLayout];
}

#pragma mark - Layout

- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation
                                duration:(NSTimeInterval)duration
{
  [self.additionalButtons reloadData];
}

- (void)viewWillTransitionToSize:(CGSize)size
       withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator
{
  [self.additionalButtons reloadData];
}

#pragma mark - Routing state

- (void)setStreetName:(NSString *)streetName
{
  self.state = MWMBottomMenuStateText;
  self.streetLabel.text = streetName;
}

- (void)setInactive
{
  self.p2pButton.selected = NO;
  self.state = self.restoreState = MWMBottomMenuStateInactive;
}

- (void)setPlanning
{
  if (IPAD)
    return;
  self.state = MWMBottomMenuStatePlanning;
}

- (void)setGo
{
  if (IPAD)
    return;
  self.state = MWMBottomMenuStateGo;
}

#pragma mark - Notifications

- (void)searchStateWillChange:(NSNotification *)notification
{
  MWMSearchManagerState state =
      MWMSearchManagerState([[notification userInfo][kSearchStateKey] unsignedIntegerValue]);
  self.searchIsActive = state != MWMSearchManagerStateHidden;
}

#pragma mark - UICollectionViewDataSource

- (NSInteger)collectionView:(nonnull UICollectionView *)collectionView
     numberOfItemsInSection:(NSInteger)section
{
  return [self additionalButtonsCount];
}

- (nonnull UICollectionViewCell *)collectionView:(nonnull UICollectionView *)collectionView
                          cellForItemAtIndexPath:(nonnull NSIndexPath *)indexPath
{
  BOOL const isWideMenu = self.view.width > kLayoutThreshold;
  MWMBottomMenuCollectionViewCell * cell =
      [collectionView dequeueReusableCellWithReuseIdentifier:isWideMenu ? kCollectionCelllandscape
                                                                        : kCollectionCellPortrait
                                                forIndexPath:indexPath];
  switch (indexPath.item)
  {
  case MWMBottomMenuViewCellAddPlace:
  {
    BOOL const isEnabled = self.controller.controlsManager.navigationState == MWMNavigationDashboardStateHidden &&
                           GetFramework().CanEditMap();
    [cell configureWithImageName:@"ic_add_place"
                           label:L(@"placepage_add_place_button")
                      badgeCount:0
                       isEnabled:isEnabled];
    break;
  }
  case MWMBottomMenuViewCellDownload:
  {
    auto & s = GetFramework().Storage();
    storage::Storage::UpdateInfo updateInfo{};
    s.GetUpdateInfo(s.GetRootId(), updateInfo);
    [cell configureWithImageName:@"ic_menu_download"
                           label:L(@"download_maps")
                      badgeCount:updateInfo.m_numberOfMwmFilesToUpdate
                       isEnabled:YES];
  }
  break;
  case MWMBottomMenuViewCellSettings:
    [cell configureWithImageName:@"ic_menu_settings" label:L(@"settings") badgeCount:0 isEnabled:YES];
    break;
  case MWMBottomMenuViewCellShare:
    [cell configureWithImageName:@"ic_menu_share" label:L(@"share_my_location") badgeCount:0 isEnabled:YES];
    break;
  case MWMBottomMenuViewCellAd:
  {
    MTRGNativeAppwallBanner * banner = [self.controller.appWallAd.banners firstObject];
    [self.controller.appWallAd handleShow:banner];
    [cell configureWithImageName:@"ic_menu_showcase" label:L(@"showcase_more_apps") badgeCount:0 isEnabled:YES];
  }
    break;
  case MWMBottomMenuViewCellCount:
    break;
  }
  return cell;
}

#pragma mark - UICollectionViewDelegate

- (void)collectionView:(nonnull UICollectionView *)collectionView
    didSelectItemAtIndexPath:(nonnull NSIndexPath *)indexPath
{
  MWMBottomMenuCollectionViewCell * cell = static_cast<MWMBottomMenuCollectionViewCell *>([collectionView cellForItemAtIndexPath:indexPath]);
  if (!cell.isEnabled)
    return;
  switch (indexPath.item)
  {
  case MWMBottomMenuViewCellAddPlace:
    [self menuActionAddPlace];
    break;
  case MWMBottomMenuViewCellDownload:
    [self menuActionDownloadMaps];
    break;
  case MWMBottomMenuViewCellSettings:
    [self menuActionOpenSettings];
    break;
  case MWMBottomMenuViewCellShare:
    [self menuActionShareLocation];
    break;
  case MWMBottomMenuViewCellAd:
    [self menuActionOpenAd];
    break;
  case MWMBottomMenuViewCellCount:
    break;
  }
}

#pragma mark - Buttons actions

- (void)menuActionAddPlace
{
  [Statistics logEvent:kStatEditorAddClick withParameters:@{kStatValue : kStatMenu}];
  self.state = self.restoreState;
  [self.delegate addPlace:NO hasPoint:NO point:m2::PointD()];
}

- (void)menuActionDownloadMaps
{
  [Statistics logEvent:kStatMenu withParameters:@{kStatButton : kStatDownloadMaps}];
  self.state = self.restoreState;
  [self.delegate actionDownloadMaps:mwm::DownloaderMode::Downloaded];
}

- (void)menuActionOpenSettings
{
  [Statistics logEvent:kStatMenu withParameters:@{kStatButton : kStatSettings}];
  self.state = self.restoreState;
  [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"settingsAndMore"];
  SettingsAndMoreVC * const vc = [[SettingsAndMoreVC alloc] initWithStyle:UITableViewStyleGrouped];
  [self.controller.navigationController pushViewController:vc animated:YES];
}

- (void)menuActionShareLocation
{
  [Statistics logEvent:kStatMenu withParameters:@{kStatButton : kStatShare}];
  [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"share@"];
  CLLocation * location = [MapsAppDelegate theApp].locationManager.lastLocation;
  if (!location)
  {
    [[[UIAlertView alloc] initWithTitle:L(@"unknown_current_position")
                                message:nil
                               delegate:nil
                      cancelButtonTitle:L(@"ok")
                      otherButtonTitles:nil] show];
    return;
  }
  CLLocationCoordinate2D const coord = location.coordinate;
  NSIndexPath * cellIndex = [NSIndexPath indexPathForItem:MWMBottomMenuViewCellShare inSection:0];
  MWMBottomMenuCollectionViewCell * cell =
      (MWMBottomMenuCollectionViewCell *)[self.additionalButtons cellForItemAtIndexPath:cellIndex];
  MWMActivityViewController * shareVC =
      [MWMActivityViewController shareControllerForLocationTitle:nil location:coord myPosition:YES];
  [shareVC presentInParentViewController:self.controller anchorView:cell.icon];
}

- (void)menuActionOpenAd
{
  NSArray<MTRGNativeAppwallBanner *> * banners = self.controller.appWallAd.banners;
  NSAssert(banners.count != 0, @"Banners collection can not be empty!");
  [Statistics logEvent:kStatMenu withParameters:@{kStatButton : kStatMoreApps}];
  self.state = self.restoreState;
  [self.controller.appWallAd showWithController:self.controller onComplete:^
  {
    [Statistics logEvent:kStatMyTargetAppsDisplayed withParameters:@{kStatCount : @(banners.count)}];
    NSMutableArray<NSString *> * appNames = [@[] mutableCopy];
    for (MTRGNativeAppwallBanner * banner in banners)
    {
      [Statistics logEvent:kStatMyTargetAppsDisplayed withParameters:@{kStatName : banner.title}];
      [appNames addObject:banner.title];
    }
    NSString * appNamesString = [appNames componentsJoinedByString:@";"];
    [Alohalytics logEvent:kStatMyTargetAppsDisplayed
           withDictionary:@{
             kStatCount : @(banners.count),
             kStatName : appNamesString
           }];
  }
  onError:^(NSError * error)
  {
    NSMutableArray<NSString *> * appNames = [@[] mutableCopy];
    for (MTRGNativeAppwallBanner * banner in banners)
      [appNames addObject:banner.title];
    NSString * appNamesString = [appNames componentsJoinedByString:@";"];
    [Statistics logEvent:kStatMyTargetAppsDisplayed
                     withParameters:@{
                       kStatError : error,
                       kStatCount : @(banners.count),
                       kStatName : appNamesString
                     }];
  }];
}

- (IBAction)locationButtonTouchUpInside:(UIButton *)sender
{
  [Statistics logEvent:kStatMenu withParameters:@{kStatButton : kStatLocation}];
  GetFramework().SwitchMyPositionNextMode();
}

- (IBAction)point2PointButtonTouchUpInside:(UIButton *)sender
{
  [Statistics logEvent:kStatMenu withParameters:@{kStatButton : kStatPointToPoint}];
  self.state = self.restoreState;
  BOOL const isSelected = !sender.isSelected;
  sender.selected = isSelected;
  MapsAppDelegate * theApp = [MapsAppDelegate theApp];
  if (isSelected)
  {
    theApp.routingPlaneMode = MWMRoutingPlaneModePlacePage;
    [self.controller.controlsManager routingPrepare];
  }
  else
  {
    if (theApp.routingPlaneMode == MWMRoutingPlaneModeSearchDestination || theApp.routingPlaneMode == MWMRoutingPlaneModeSearchSource)
      self.controller.controlsManager.searchHidden = YES;
    [self.controller.controlsManager routingHidden];
  }
}

- (IBAction)searchButtonTouchUpInside:(UIButton *)sender
{
  [Statistics logEvent:kStatMenu withParameters:@{kStatButton : kStatSearch}];
  [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"search"];
  self.state = self.restoreState;
  self.controller.controlsManager.searchHidden = self.searchIsActive;
}

- (IBAction)bookmarksButtonTouchUpInside:(UIButton *)sender
{
  [Statistics logEvent:kStatMenu withParameters:@{kStatButton : kStatBookmarks}];
  [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"bookmarks"];
  self.state = self.restoreState;
  [self.controller openBookmarks];
}

- (IBAction)menuButtonTouchUpInside:(UIButton *)sender
{
  switch (self.state)
  {
  case MWMBottomMenuStateHidden:
    NSAssert(false, @"Incorrect state");
    break;
  case MWMBottomMenuStateInactive:
  case MWMBottomMenuStatePlanning:
  case MWMBottomMenuStateGo:
  case MWMBottomMenuStateText:
    [Statistics logEvent:kStatMenu withParameters:@{kStatButton : kStatExpand}];
    self.state = MWMBottomMenuStateActive;
    break;
  case MWMBottomMenuStateActive:
    [Statistics logEvent:kStatMenu withParameters:@{kStatButton : kStatCollapse}];
    self.state = self.restoreState;
    break;
  case MWMBottomMenuStateCompact:
    [Statistics logEvent:kStatMenu withParameters:@{kStatButton : kStatRegular}];
    [self.delegate closeInfoScreens];
    break;
  }
}
- (IBAction)goButtonTouchUpInside:(UIButton *)sender
{
  [self.controller.controlsManager routingNavigation];
}

- (void)dimBackgroundTap
{
  // In case when there are 2 touch events (dimBackgroundTap & menuButtonTouchUpInside)
  // if dimBackgroundTap is processed first then menuButtonTouchUpInside behaves as if menu is
  // inactive this is wrong case, so we postpone dimBackgroundTap to make sure
  // menuButtonTouchUpInside processed first
  dispatch_async(dispatch_get_main_queue(), ^{ self.state = self.restoreState; });
}

- (void)toggleDimBackgroundVisible:(BOOL)visible
{
  if (visible)
    [self.controller.view insertSubview:self.dimBackground belowSubview:self.view];
  self.dimBackground.alpha = visible ? 0.0 : 0.8;
  [UIView animateWithDuration:kDefaultAnimationDuration animations:^
  {
    self.dimBackground.alpha = visible ? 0.8 : 0.0;
  }
  completion:^(BOOL finished)
  {
    if (!visible)
    {
      [self.dimBackground removeFromSuperview];
      self.dimBackground = nil;
    }
  }];
}

#pragma mark - Properties

- (SolidTouchView *)dimBackground
{
  if (!_dimBackground)
  {
    _dimBackground = [[SolidTouchView alloc] initWithFrame:self.controller.view.bounds];
    _dimBackground.backgroundColor = [UIColor fadeBackground];
    _dimBackground.autoresizingMask =
        UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    UITapGestureRecognizer * tap =
        [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(dimBackgroundTap)];
    [_dimBackground addGestureRecognizer:tap];
  }
  return _dimBackground;
}

- (void)setState:(MWMBottomMenuState)state
{
  MWMBottomMenuView * view = (MWMBottomMenuView *)self.view;
  if (state != view.state)
    [self.controller.view bringSubviewToFront:view];
  [self toggleDimBackgroundVisible:state == MWMBottomMenuStateActive];
  if (view.state == MWMBottomMenuStateCompact &&
      (state == MWMBottomMenuStatePlanning || state == MWMBottomMenuStateGo ||
       state == MWMBottomMenuStateText))
    self.restoreState = state;
  else
    view.state = state;
}

- (MWMBottomMenuState)state
{
  return ((MWMBottomMenuView *)self.view).state;
}

- (void)setRestoreState:(MWMBottomMenuState)restoreState
{
  ((MWMBottomMenuView *)self.view).restoreState = restoreState;
}

- (MWMBottomMenuState)restoreState
{
  return ((MWMBottomMenuView *)self.view).restoreState;
}

- (void)setLeftBound:(CGFloat)leftBound
{
  ((MWMBottomMenuView *)self.view).leftBound = leftBound;
  ((EAGLView *)self.controller.view).widgetsManager.leftBound = leftBound;
}

- (CGFloat)leftBound
{
  return ((MWMBottomMenuView *)self.view).leftBound;
}

- (void)setSearchIsActive:(BOOL)searchIsActive
{
  ((MWMBottomMenuView *)self.view).searchIsActive = searchIsActive;
}

- (BOOL)searchIsActive
{
  return ((MWMBottomMenuView *)self.view).searchIsActive;
}

- (NSUInteger)additionalButtonsCount
{
  return MWMBottomMenuViewCellCount - (self.controller.isAppWallAdActive ? 0 : 1);
}

@end
