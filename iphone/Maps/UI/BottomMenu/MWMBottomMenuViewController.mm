#import "MWMBottomMenuViewController.h"
#import "MWMActivityViewController.h"
#import "MWMBottomMenuCollectionViewCell.h"
#import "MWMBottomMenuControllerProtocol.h"
#import "MWMBottomMenuLayout.h"
#import "MWMButton.h"
#import "MWMCommon.h"
#import "MWMMapViewControlsManager.h"
#import "MapViewController.h"
#import "MapsAppDelegate.h"
#import "SwiftBridge.h"

#include "Framework.h"

extern NSString * const kAlohalyticsTapEventKey;
extern NSString * const kSearchStateKey;

namespace
{
CGFloat constexpr kLayoutThreshold = 420.0;
}  // namespace

typedef NS_ENUM(NSUInteger, MWMBottomMenuViewCell) {
  MWMBottomMenuViewCellAddPlace,
  MWMBottomMenuViewCellDownload,
  MWMBottomMenuViewCellSettings,
  MWMBottomMenuViewCellShare,
  MWMBottomMenuViewCellCount
};

@interface MWMMapViewControlsManager ()

@property(nonatomic) MWMBottomMenuViewController * menuController;

@end

@interface MWMBottomMenuViewController ()<UICollectionViewDataSource, UICollectionViewDelegate,
                                          MWMNavigationDashboardObserver, MWMSearchManagerObserver>

@property(nonatomic) MWMBottomMenuState restoreState;
@property(nonatomic) MWMDimBackground * dimBackground;
@property(nonatomic, readonly) NSUInteger additionalButtonsCount;
@property(weak, nonatomic) IBOutlet MWMButton * searchButton;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * mainButtonsHeight;
@property(weak, nonatomic) IBOutlet UICollectionView * additionalButtons;
@property(weak, nonatomic) IBOutlet UIView * downloadBadge;
@property(weak, nonatomic) MapViewController * controller;
@property(weak, nonatomic) id<MWMBottomMenuControllerProtocol> delegate;

@end

@implementation MWMBottomMenuViewController

+ (MWMBottomMenuViewController *)controller
{
  return [MWMMapViewControlsManager manager].menuController;
}

+ (void)updateAvailableArea:(CGRect)frame
{
  auto view = static_cast<MWMBottomMenuView *>([self controller].view);
  [view updateAvailableArea:frame];
}

- (instancetype)initWithParentController:(MapViewController *)controller
                                delegate:(id<MWMBottomMenuControllerProtocol>)delegate
{
  self = [super init];
  if (self)
  {
    _controller = controller;
    _delegate = delegate;
    [controller addChildViewController:self];
    [controller.view addSubview:self.view];
  }
  return self;
}

- (void)dealloc { [[NSNotificationCenter defaultCenter] removeObserver:self]; }
- (void)viewDidLoad
{
  [super viewDidLoad];
  UICollectionView * bcv = self.additionalButtons;
  [bcv registerWithCellClass:[MWMBottomMenuCollectionViewPortraitCell class]];
  [bcv registerWithCellClass:[MWMBottomMenuCollectionViewLandscapeCell class]];
  MWMBottomMenuLayout * cvLayout =
      (MWMBottomMenuLayout *)self.additionalButtons.collectionViewLayout;
  cvLayout.layoutThreshold = kLayoutThreshold;
  self.menuView.layoutThreshold = kLayoutThreshold;

  [MWMSearchManager addObserver:self];
  [MWMNavigationDashboardManager addObserver:self];
}

- (void)viewWillAppear:(BOOL)animated
{
  [super viewWillAppear:animated];
  [self refreshLayout];
}

- (void)mwm_refreshUI { [self.view mwm_refreshUI]; }
- (void)updateBadgeVisible:(BOOL)visible { self.downloadBadge.hidden = !visible; }
#pragma mark - Refresh Collection View layout

- (void)refreshLayout
{
  MWMBottomMenuLayout * cvLayout =
      (MWMBottomMenuLayout *)self.additionalButtons.collectionViewLayout;
  cvLayout.buttonsCount = [self collectionView:self.additionalButtons numberOfItemsInSection:0];
  [self.additionalButtons reloadData];
  [self.menuView refreshLayout];
}

#pragma mark - Layout

- (void)viewWillTransitionToSize:(CGSize)size
       withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator
{
  [self.additionalButtons reloadData];
}

#pragma mark - MWMNavigationDashboardObserver

- (void)onNavigationDashboardStateChanged
{
  auto const navigationState = [MWMNavigationDashboardManager manager].state;
  if (navigationState == MWMNavigationDashboardStateHidden)
    self.state = MWMBottomMenuStateInactive;
  else
    self.state = MWMBottomMenuStateHidden;
}

#pragma mark - MWMSearchManagerObserver

- (void)onSearchManagerStateChanged
{
  auto state = [MWMSearchManager manager].state;
  self.searchButton.selected = (state != MWMSearchManagerStateHidden);
}

#pragma mark - UICollectionViewDataSource

- (NSInteger)collectionView:(nonnull UICollectionView *)collectionView
     numberOfItemsInSection:(NSInteger)section
{
  return MWMBottomMenuViewCellCount;
}

- (nonnull UICollectionViewCell *)collectionView:(nonnull UICollectionView *)collectionView
                          cellForItemAtIndexPath:(nonnull NSIndexPath *)indexPath
{
  BOOL const isWideMenu = self.view.width > kLayoutThreshold;
  Class cls = isWideMenu ? [MWMBottomMenuCollectionViewLandscapeCell class]
                         : [MWMBottomMenuCollectionViewPortraitCell class];
  auto cell = static_cast<MWMBottomMenuCollectionViewCell *>(
      [collectionView dequeueReusableCellWithCellClass:cls indexPath:indexPath]);
  NSInteger item = indexPath.item;
  if (isWideMenu && isInterfaceRightToLeft())
    item = [self collectionView:collectionView numberOfItemsInSection:indexPath.section] - item - 1;
  switch (item)
  {
  case MWMBottomMenuViewCellAddPlace:
  {
    BOOL const isEnabled =
        [MWMNavigationDashboardManager manager].state == MWMNavigationDashboardStateHidden &&
        GetFramework().CanEditMap();
    [cell configureWithImageName:@"ic_add_place"
                           label:L(@"placepage_add_place_button")
                      badgeCount:0
                       isEnabled:isEnabled];
    break;
  }
  case MWMBottomMenuViewCellDownload:
  {
    [cell configureWithImageName:@"ic_menu_download"
                           label:L(@"download_maps")
                      badgeCount:[[MapsAppDelegate theApp] badgeNumber]
                       isEnabled:YES];
  }
  break;
  case MWMBottomMenuViewCellSettings:
    [cell configureWithImageName:@"ic_menu_settings"
                           label:L(@"settings")
                      badgeCount:0
                       isEnabled:YES];
    break;
  case MWMBottomMenuViewCellShare:
    [cell configureWithImageName:@"ic_menu_share"
                           label:L(@"share_my_location")
                      badgeCount:0
                       isEnabled:YES];
    break;
  }
  return cell;
}

#pragma mark - UICollectionViewDelegate

- (void)collectionView:(nonnull UICollectionView *)collectionView
    didSelectItemAtIndexPath:(nonnull NSIndexPath *)indexPath
{
  MWMBottomMenuCollectionViewCell * cell = static_cast<MWMBottomMenuCollectionViewCell *>(
      [collectionView cellForItemAtIndexPath:indexPath]);
  if (!cell.isEnabled)
    return;
  switch (indexPath.item)
  {
  case MWMBottomMenuViewCellAddPlace: [self menuActionAddPlace]; break;
  case MWMBottomMenuViewCellDownload: [self menuActionDownloadMaps]; break;
  case MWMBottomMenuViewCellSettings: [self menuActionOpenSettings]; break;
  case MWMBottomMenuViewCellShare: [self menuActionShareLocation]; break;
  }
}

#pragma mark - Buttons actions

- (void)menuActionAddPlace
{
  [Statistics logEvent:kStatEditorAddClick withParameters:@{kStatValue : kStatMenu}];
  GetPlatform().GetMarketingService().SendPushWooshTag(marketing::kEditorAddDiscovered);
  self.state = self.restoreState;
  [self.delegate addPlace:NO hasPoint:NO point:m2::PointD()];
}

- (void)menuActionDownloadMaps
{
  [Statistics logEvent:kStatMenu withParameters:@{kStatButton : kStatDownloadMaps}];
  self.state = MWMBottomMenuStateInactive;
  [self.delegate actionDownloadMaps:MWMMapDownloaderModeDownloaded];
}

- (IBAction)menuActionOpenSettings
{
  [Statistics logEvent:kStatMenu withParameters:@{kStatButton : kStatSettings}];
  self.state = self.restoreState;
  [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"settingsAndMore"];
  [self.controller performSegueWithIdentifier:@"Map2Settings" sender:nil];
}

- (void)menuActionShareLocation
{
  [Statistics logEvent:kStatMenu withParameters:@{kStatButton : kStatShare}];
  [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"share@"];
  CLLocation * lastLocation = [MWMLocationManager lastLocation];
  if (!lastLocation)
  {
    [[[UIAlertView alloc] initWithTitle:L(@"unknown_current_position")
                                message:nil
                               delegate:nil
                      cancelButtonTitle:L(@"ok")
                      otherButtonTitles:nil] show];
    return;
  }
  CLLocationCoordinate2D const coord = lastLocation.coordinate;
  NSIndexPath * cellIndex = [NSIndexPath indexPathForItem:MWMBottomMenuViewCellShare inSection:0];
  MWMBottomMenuCollectionViewCell * cell =
      (MWMBottomMenuCollectionViewCell *)[self.additionalButtons cellForItemAtIndexPath:cellIndex];
  MWMActivityViewController * shareVC =
      [MWMActivityViewController shareControllerForMyPosition:coord];
  [shareVC presentInParentViewController:self.controller anchorView:cell.icon];
}

- (IBAction)point2PointButtonTouchUpInside:(UIButton *)sender
{
  [Statistics logEvent:kStatMenu withParameters:@{kStatButton : kStatPointToPoint}];
  BOOL const isSelected = !sender.isSelected;
  [MWMRouter enableAutoAddLastLocation:NO];
  if (isSelected)
    [[MWMMapViewControlsManager manager] onRoutePrepare];
  else
    [MWMRouter stopRouting];
}

- (IBAction)searchButtonTouchUpInside
{
  [Statistics logEvent:kStatMenu withParameters:@{kStatButton : kStatSearch}];
  [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"search"];
  self.state = MWMBottomMenuStateInactive;
  auto searchManager = [MWMSearchManager manager];
  if (searchManager.state == MWMSearchManagerStateHidden)
    searchManager.state = MWMSearchManagerStateDefault;
  else
    searchManager.state = MWMSearchManagerStateHidden;
}

- (IBAction)bookmarksButtonTouchUpInside
{
  [Statistics logEvent:kStatMenu withParameters:@{kStatButton : kStatBookmarks}];
  [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"bookmarks"];
  self.state = MWMBottomMenuStateInactive;
  [self.controller openBookmarks];
}

- (IBAction)menuButtonTouchUpInside
{
  switch (self.state)
  {
  case MWMBottomMenuStateHidden: NSAssert(false, @"Incorrect state"); break;
  case MWMBottomMenuStateInactive:
    if ([self.menuView isCompact])
    {
      [Statistics logEvent:kStatMenu withParameters:@{kStatButton : kStatRegular}];
      if (IPAD)
      {
        [MWMSearchManager manager].state = MWMSearchManagerStateHidden;
        [MWMRouter stopRouting];
      }
    }
    else
    {
      [Statistics logEvent:kStatMenu withParameters:@{kStatButton : kStatExpand}];
      self.state = MWMBottomMenuStateActive;
    }
    break;
  case MWMBottomMenuStateActive:
    [Statistics logEvent:kStatMenu withParameters:@{kStatButton : kStatCollapse}];
    self.state = MWMBottomMenuStateInactive;
    break;
  }
}

#pragma mark - Properties

- (MWMBottomMenuView *)menuView { return (MWMBottomMenuView *)self.view; }
- (MWMDimBackground *)dimBackground
{
  if (!_dimBackground)
    _dimBackground = [[MWMDimBackground alloc] initWithMainView:self.view];
  return _dimBackground;
}

- (void)setState:(MWMBottomMenuState)state
{
  dispatch_async(dispatch_get_main_queue(), ^{
    [self.controller setNeedsStatusBarAppearanceUpdate];
  });
  MWMBottomMenuView * view = self.menuView;
  BOOL const menuActive = (state == MWMBottomMenuStateActive);
  if (menuActive)
    [self.controller.view bringSubviewToFront:view];

  __weak auto wSelf = self;
  [self.dimBackground setVisible:menuActive
                       tapAction:^{
                         // In case when there are 2 touch events (dimBackgroundTap &
                         // menuButtonTouchUpInside)
                         // if dimBackgroundTap is processed first then menuButtonTouchUpInside
                         // behaves as if menu is
                         // inactive this is wrong case, so we postpone dimBackgroundTap to make
                         // sure
                         // menuButtonTouchUpInside processed first
                         dispatch_async(dispatch_get_main_queue(), ^{
                           wSelf.state = MWMBottomMenuStateInactive;
                         });
                       }];
  view.state = state;
  [self updateBadgeVisible:[[MapsAppDelegate theApp] badgeNumber] != 0];
}

- (MWMBottomMenuState)state { return self.menuView.state; }
@end
