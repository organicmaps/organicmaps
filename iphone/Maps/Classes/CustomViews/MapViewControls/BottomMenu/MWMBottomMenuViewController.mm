#import "MWMBottomMenuViewController.h"
#import "Common.h"
#import "EAGLView.h"
#import "MWMActivityViewController.h"
#import "MWMBottomMenuCollectionViewCell.h"
#import "MWMBottomMenuLayout.h"
#import "MWMBottomMenuView.h"
#import "MWMButton.h"
#import "MWMFrameworkListener.h"
#import "MWMFrameworkObservers.h"
#import "MWMLocationManager.h"
#import "MWMMapViewControlsManager.h"
#import "MWMRouter.h"
#import "MWMSearchManager.h"
#import "MWMTextToSpeech.h"
#import "MapViewController.h"
#import "MapsAppDelegate.h"
#import "SettingsAndMoreVC.h"
#import "Statistics.h"
#import "TimeUtils.h"
#import "UIColor+MapsMeColor.h"
#import "UIFont+MapsMeFonts.h"
#import "UIImageView+Coloring.h"
#import "UIKitCategories.h"

#import "3party/Alohalytics/src/alohalytics_objc.h"

#include "Framework.h"

#include "platform/mwm_version.hpp"

extern NSString * const kAlohalyticsTapEventKey;
extern NSString * const kSearchStateWillChangeNotification;
extern NSString * const kSearchStateKey;

namespace
{
NSString * const kCollectionCellPortrait = @"MWMBottomMenuCollectionViewPortraitCell";
NSString * const kCollectionCelllandscape = @"MWMBottomMenuCollectionViewLandscapeCell";
}  // namespace

static CGFloat const kLayoutThreshold = 420.0;

typedef NS_ENUM(NSUInteger, MWMBottomMenuViewCell) {
  MWMBottomMenuViewCellAddPlace,
  MWMBottomMenuViewCellDownload,
  MWMBottomMenuViewCellSettings,
  MWMBottomMenuViewCellShare,
  MWMBottomMenuViewCellAd,
  MWMBottomMenuViewCellCount
};

@interface MWMBottomMenuViewController ()<UICollectionViewDataSource, UICollectionViewDelegate>

@property(weak, nonatomic) MapViewController * controller;
@property(weak, nonatomic) IBOutlet UICollectionView * buttonsCollectionView;

@property(weak, nonatomic) IBOutlet UICollectionView * additionalButtons;

@property(weak, nonatomic) id<MWMBottomMenuControllerProtocol> delegate;

@property(nonatomic) BOOL searchIsActive;

@property(nonatomic) SolidTouchView * dimBackground;

@property(nonatomic) MWMBottomMenuState restoreState;

@property(nonatomic, readonly) NSUInteger additionalButtonsCount;

@property(weak, nonatomic) MWMNavigationDashboardEntity * routingInfo;

@property(weak, nonatomic) IBOutlet UILabel * speedLabel;
@property(weak, nonatomic) IBOutlet UILabel * timeLabel;
@property(weak, nonatomic) IBOutlet UILabel * distanceLabel;
@property(weak, nonatomic) IBOutlet UILabel * speedLegendLabel;
@property(weak, nonatomic) IBOutlet UILabel * distanceLegendLabel;
@property(weak, nonatomic) IBOutlet UILabel * speedWithLegendLabel;
@property(weak, nonatomic) IBOutlet UILabel * distanceWithLegendLabel;
@property(weak, nonatomic) IBOutlet UIPageControl * routingInfoPageControl;

@property(weak, nonatomic) IBOutlet UIView * progressView;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * routingProgress;
@property(weak, nonatomic) IBOutlet MWMButton * ttsSoundButton;

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
  }
  return self;
}

- (void)dealloc { [[NSNotificationCenter defaultCenter] removeObserver:self]; }
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

  NSNotificationCenter * nc = [NSNotificationCenter defaultCenter];
  [nc addObserver:self
         selector:@selector(searchStateWillChange:)
             name:kSearchStateWillChangeNotification
           object:nil];
  [nc addObserver:self
         selector:@selector(ttsButtonStatusChanged:)
             name:[MWMTextToSpeech ttsStatusNotificationKey]
           object:nil];
}

- (void)viewWillAppear:(BOOL)animated
{
  [super viewWillAppear:animated];
  [self refreshLayout];
}

- (void)mwm_refreshUI { [self.view mwm_refreshUI]; }
#pragma mark - MWMNavigationDashboardInfoProtocol

- (void)updateNavigationInfo:(MWMNavigationDashboardEntity *)info
{
  NSDictionary * routingNumberAttributes = @{
    NSForegroundColorAttributeName : [UIColor blackPrimaryText],
    NSFontAttributeName : [UIFont bold24]
  };
  NSDictionary * routingLegendAttributes = @{
    NSForegroundColorAttributeName : [UIColor blackSecondaryText],
    NSFontAttributeName : [UIFont bold14]
  };

  self.routingInfo = info;
  if (self.routingInfoPageControl.currentPage == 0)
  {
    self.timeLabel.text = [NSDateFormatter estimatedArrivalTimeWithSeconds:@(info.timeToTarget)];
  }
  else
  {
    NSDate * arrivalDate = [[NSDate date] dateByAddingTimeInterval:info.timeToTarget];
    self.timeLabel.text = [NSDateFormatter localizedStringFromDate:arrivalDate
                                                         dateStyle:NSDateFormatterNoStyle
                                                         timeStyle:NSDateFormatterShortStyle];
  }
  self.distanceLabel.text = info.targetDistance;
  self.distanceLegendLabel.text = info.targetUnits;
  NSMutableAttributedString * distance =
      [[NSMutableAttributedString alloc] initWithString:info.targetDistance
                                             attributes:routingNumberAttributes];
  [distance
      appendAttributedString:[[NSAttributedString alloc] initWithString:info.targetUnits
                                                             attributes:routingLegendAttributes]];
  self.distanceWithLegendLabel.attributedText = distance;

  NSString * currentSpeed = info.speed;
  if (currentSpeed.length != 0)
  {
    self.speedLabel.text = currentSpeed;
    self.speedLegendLabel.text = info.speedUnits;
    NSMutableAttributedString * speed =
        [[NSMutableAttributedString alloc] initWithString:currentSpeed
                                               attributes:routingNumberAttributes];
    [speed
        appendAttributedString:[[NSAttributedString alloc] initWithString:info.speedUnits
                                                               attributes:routingLegendAttributes]];
    self.speedWithLegendLabel.attributedText = speed;
  }
  else
  {
    self.speedLabel.text = @"";
    self.speedLegendLabel.text = @"";
    self.speedWithLegendLabel.text = @"";
  }

  [self.progressView layoutIfNeeded];
  [UIView animateWithDuration:kDefaultAnimationDuration
                   animations:^{
                     self.routingProgress.constant = self.progressView.width * info.progress / 100.;
                     [self.progressView layoutIfNeeded];
                   }];
}

#pragma mark - Routing

- (IBAction)toggleInfoTouchUpInside
{
  self.routingInfoPageControl.currentPage =
      (self.routingInfoPageControl.currentPage + 1) % self.routingInfoPageControl.numberOfPages;
  [self updateNavigationInfo:self.routingInfo];
}

- (IBAction)routingStartTouchUpInside { [[MWMRouter router] start]; }
- (IBAction)routingStopTouchUpInside { [[MWMRouter router] stop]; }
- (IBAction)soundTouchUpInside:(UIButton *)sender
{
  BOOL const isEnable = !sender.selected;
  [Statistics logEvent:kStatEventName(kStatNavigationDashboard, isEnable ? kStatOn : kStatOff)];
  [MWMTextToSpeech tts].active = isEnable;
  sender.selected = isEnable;
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

#pragma mark - Notifications

- (void)searchStateWillChange:(NSNotification *)notification
{
  MWMSearchManagerState state =
      MWMSearchManagerState([[notification userInfo][kSearchStateKey] unsignedIntegerValue]);
  self.searchIsActive = state != MWMSearchManagerStateHidden;
}

- (void)ttsButtonStatusChanged:(NSNotification *)notification
{
  auto & f = GetFramework();
  if (!f.IsRoutingActive())
    return;
  BOOL const isPedestrianRouting = f.GetRouter() == routing::RouterType::Pedestrian;
  MWMButton * ttsButton = self.ttsSoundButton;
  ttsButton.hidden = isPedestrianRouting || ![MWMTextToSpeech isTTSEnabled];
  if (!ttsButton.hidden)
    ttsButton.selected = [MWMTextToSpeech tts].active;
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
    BOOL const isEnabled =
        self.controller.controlsManager.navigationState == MWMNavigationDashboardStateHidden &&
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
  case MWMBottomMenuViewCellAd:
  {
    MTRGNativeAppwallBanner * banner = [self.controller.appWallAd.banners firstObject];
    [self.controller.appWallAd handleShow:banner];
    [cell configureWithImageName:@"ic_menu_showcase"
                           label:L(@"showcase_more_apps")
                      badgeCount:0
                       isEnabled:YES];
  }
  break;
  case MWMBottomMenuViewCellCount: break;
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
  case MWMBottomMenuViewCellAd: [self menuActionOpenAd]; break;
  case MWMBottomMenuViewCellCount: break;
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

- (IBAction)menuActionOpenSettings
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
      [MWMActivityViewController shareControllerForLocationTitle:nil location:coord myPosition:YES];
  [shareVC presentInParentViewController:self.controller anchorView:cell.icon];
}

- (void)menuActionOpenAd
{
  NSArray<MTRGNativeAppwallBanner *> * banners = self.controller.appWallAd.banners;
  NSAssert(banners.count != 0, @"Banners collection can not be empty!");
  [Statistics logEvent:kStatMenu withParameters:@{kStatButton : kStatMoreApps}];
  self.state = self.restoreState;
  [self.controller.appWallAd showWithController:self.controller
      onComplete:^{
        [Statistics logEvent:kStatMyTargetAppsDisplayed
              withParameters:@{
                kStatCount : @(banners.count)
              }];
        NSMutableArray<NSString *> * appNames = [@[] mutableCopy];
        for (MTRGNativeAppwallBanner * banner in banners)
        {
          [Statistics logEvent:kStatMyTargetAppsDisplayed
                withParameters:@{kStatName : banner.title}];
          [appNames addObject:banner.title];
        }
        NSString * appNamesString = [appNames componentsJoinedByString:@";"];
        [Alohalytics logEvent:kStatMyTargetAppsDisplayed
               withDictionary:@{
                 kStatCount : @(banners.count),
                 kStatName : appNamesString
               }];
      }
      onError:^(NSError * error) {
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

- (IBAction)point2PointButtonTouchUpInside:(UIButton *)sender
{
  [Statistics logEvent:kStatMenu withParameters:@{kStatButton : kStatPointToPoint}];
  self.state = self.restoreState;
  BOOL const isSelected = !sender.isSelected;
  sender.selected = isSelected;
  MapsAppDelegate * theApp = [MapsAppDelegate theApp];
  if (isSelected)
  {
    [[MWMMapViewControlsManager manager] onRoutePrepare];
  }
  else
  {
    if (theApp.routingPlaneMode == MWMRoutingPlaneModeSearchDestination ||
        theApp.routingPlaneMode == MWMRoutingPlaneModeSearchSource)
      self.controller.controlsManager.searchHidden = YES;
    [[MWMRouter router] stop];
  }
}

- (IBAction)searchButtonTouchUpInside
{
  [Statistics logEvent:kStatMenu withParameters:@{kStatButton : kStatSearch}];
  [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"search"];
  self.state = self.restoreState;
  self.controller.controlsManager.searchHidden = self.searchIsActive;
}

- (IBAction)bookmarksButtonTouchUpInside
{
  [Statistics logEvent:kStatMenu withParameters:@{kStatButton : kStatBookmarks}];
  [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"bookmarks"];
  self.state = self.restoreState;
  [self.controller openBookmarks];
}

- (IBAction)menuButtonTouchUpInside
{
  switch (self.state)
  {
  case MWMBottomMenuStateHidden: NSAssert(false, @"Incorrect state"); break;
  case MWMBottomMenuStateInactive:
  case MWMBottomMenuStatePlanning:
  case MWMBottomMenuStateGo:
    [Statistics logEvent:kStatMenu withParameters:@{kStatButton : kStatExpand}];
    self.state = MWMBottomMenuStateActive;
    break;
  case MWMBottomMenuStateActive:
  case MWMBottomMenuStateRoutingExpanded:
    [Statistics logEvent:kStatMenu withParameters:@{kStatButton : kStatCollapse}];
    self.state = self.restoreState;
    break;
  case MWMBottomMenuStateCompact:
    [Statistics logEvent:kStatMenu withParameters:@{kStatButton : kStatRegular}];
    [self.delegate closeInfoScreens];
    break;
  case MWMBottomMenuStateRouting: self.state = MWMBottomMenuStateRoutingExpanded; break;
  }
}

- (void)dimBackgroundTap
{
  // In case when there are 2 touch events (dimBackgroundTap & menuButtonTouchUpInside)
  // if dimBackgroundTap is processed first then menuButtonTouchUpInside behaves as if menu is
  // inactive this is wrong case, so we postpone dimBackgroundTap to make sure
  // menuButtonTouchUpInside processed first
  dispatch_async(dispatch_get_main_queue(), ^{
    self.state = self.restoreState;
  });
}

- (void)toggleDimBackgroundVisible:(BOOL)visible
{
  if (visible)
    [self.controller.view insertSubview:self.dimBackground belowSubview:self.view];
  self.dimBackground.alpha = visible ? 0.0 : 0.8;
  [UIView animateWithDuration:kDefaultAnimationDuration
      animations:^{
        self.dimBackground.alpha = visible ? 0.8 : 0.0;
      }
      completion:^(BOOL finished) {
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
  BOOL const menuActive =
      (state == MWMBottomMenuStateActive || state == MWMBottomMenuStateRoutingExpanded);
  if (menuActive)
    [self.controller.view bringSubviewToFront:view];
  [self toggleDimBackgroundVisible:menuActive];

  if (state == MWMBottomMenuStateRoutingExpanded)
    [self ttsButtonStatusChanged:nil];

  if (state == MWMBottomMenuStateInactive || state == MWMBottomMenuStateRouting)
  {
    self.p2pButton.selected = NO;
    view.state = self.restoreState = state;
    return;
  }
  if (IPAD && (state == MWMBottomMenuStatePlanning || state == MWMBottomMenuStateGo))
    return;
  if (view.state == MWMBottomMenuStateCompact &&
      (state == MWMBottomMenuStatePlanning || state == MWMBottomMenuStateGo ||
       state == MWMBottomMenuStateRouting))
    self.restoreState = state;
  else
    view.state = state;
}

- (MWMBottomMenuState)state { return ((MWMBottomMenuView *)self.view).state; }
- (void)setRestoreState:(MWMBottomMenuState)restoreState
{
  ((MWMBottomMenuView *)self.view).restoreState = restoreState;
}

- (MWMBottomMenuState)restoreState { return ((MWMBottomMenuView *)self.view).restoreState; }
- (void)setLeftBound:(CGFloat)leftBound
{
  ((MWMBottomMenuView *)self.view).leftBound = leftBound;
  ((EAGLView *)self.controller.view).widgetsManager.leftBound = leftBound;
}

- (CGFloat)leftBound { return ((MWMBottomMenuView *)self.view).leftBound; }
- (void)setSearchIsActive:(BOOL)searchIsActive
{
  ((MWMBottomMenuView *)self.view).searchIsActive = searchIsActive;
}

- (BOOL)searchIsActive { return ((MWMBottomMenuView *)self.view).searchIsActive; }
- (NSUInteger)additionalButtonsCount
{
  return MWMBottomMenuViewCellCount - (self.controller.isAppWallAdActive ? 0 : 1);
}

@end
