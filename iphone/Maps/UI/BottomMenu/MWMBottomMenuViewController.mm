#import "MWMBottomMenuViewController.h"
#import <Pushwoosh/PushNotificationManager.h>
#import "EAGLView.h"
#import "MWMActivityViewController.h"
#import "MWMBottomMenuCollectionViewCell.h"
#import "MWMBottomMenuLayout.h"
#import "MWMBottomMenuView.h"
#import "MWMButton.h"
#import "MWMCommon.h"
#import "MWMFrameworkListener.h"
#import "MWMFrameworkObservers.h"
#import "MWMLocationManager.h"
#import "MWMMapViewControlsManager.h"
#import "MWMRouter.h"
#import "MWMSearchManager.h"
#import "MWMSettingsViewController.h"
#import "MWMTextToSpeech.h"
#import "MWMTrafficManager.h"
#import "MapViewController.h"
#import "MapsAppDelegate.h"
#import "Statistics.h"
#import "SwiftBridge.h"
#import "UIImageView+Coloring.h"
#import "UIViewController+Navigation.h"

#import "3party/Alohalytics/src/alohalytics_objc.h"

#include "Framework.h"

#include "platform/mwm_version.hpp"

extern NSString * const kAlohalyticsTapEventKey;
extern NSString * const kSearchStateWillChangeNotification;
extern NSString * const kSearchStateKey;

namespace
{
CGFloat constexpr kLayoutThreshold = 420.0;
NSTimeInterval constexpr kRoutingDiminishInterval = 5.0;
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
                                          MWMTrafficManagerObserver>

@property(weak, nonatomic) MapViewController * controller;
@property(weak, nonatomic) IBOutlet UICollectionView * buttonsCollectionView;

@property(weak, nonatomic) IBOutlet UICollectionView * additionalButtons;

@property(weak, nonatomic) id<MWMBottomMenuControllerProtocol> delegate;

@property(nonatomic) BOOL searchIsActive;

@property(nonatomic) SolidTouchView * dimBackground;

@property(nonatomic) MWMBottomMenuState restoreState;

@property(nonatomic, readonly) NSUInteger additionalButtonsCount;

@property(weak, nonatomic) MWMNavigationDashboardEntity * navigationInfo;

@property(copy, nonatomic) NSString * routingErrorMessage;

@property(weak, nonatomic) IBOutlet UILabel * speedLabel;
@property(weak, nonatomic) IBOutlet UILabel * timeLabel;
@property(weak, nonatomic) IBOutlet UILabel * distanceLabel;
@property(weak, nonatomic) IBOutlet UILabel * speedLegendLabel;
@property(weak, nonatomic) IBOutlet UILabel * distanceLegendLabel;
@property(weak, nonatomic) IBOutlet UILabel * speedWithLegendLabel;
@property(weak, nonatomic) IBOutlet UILabel * distanceWithLegendLabel;
@property(weak, nonatomic) IBOutlet UIPageControl * routingInfoPageControl;
@property(weak, nonatomic) IBOutlet UILabel * estimateLabel;

@property(weak, nonatomic) IBOutlet UIView * progressView;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * routingProgress;
@property(weak, nonatomic) IBOutlet MWMButton * ttsSoundButton;
@property(weak, nonatomic) IBOutlet MWMButton * trafficButton;

@property(weak, nonatomic) IBOutlet NSLayoutConstraint * mainButtonsHeight;

@end

@implementation MWMBottomMenuViewController

+ (MWMBottomMenuViewController *)controller
{
  return [MWMMapViewControlsManager manager].menuController;
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
  UICollectionView * bcv = self.buttonsCollectionView;
  [bcv registerWithCellClass:[MWMBottomMenuCollectionViewPortraitCell class]];
  [bcv registerWithCellClass:[MWMBottomMenuCollectionViewLandscapeCell class]];
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
  [MWMTrafficManager addObserver:self];
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
  if ([MWMRouter isTaxi])
    return;

  NSDictionary * routingNumberAttributes = @{
    NSForegroundColorAttributeName : [UIColor blackPrimaryText],
    NSFontAttributeName : [UIFont bold24]
  };
  NSDictionary * routingLegendAttributes = @{
    NSForegroundColorAttributeName : [UIColor blackSecondaryText],
    NSFontAttributeName : [UIFont bold14]
  };

  self.estimateLabel.attributedText = info.estimate;
  self.navigationInfo = info;
  if (self.routingInfoPageControl.currentPage == 0)
  {
    self.timeLabel.text = [NSDateComponentsFormatter etaStringFrom:info.timeToTarget];
  }
  else
  {
    NSDate * arrivalDate = [[NSDate date] dateByAddingTimeInterval:info.timeToTarget];
    self.timeLabel.text = [NSDateFormatter localizedStringFromDate:arrivalDate
                                                         dateStyle:NSDateFormatterNoStyle
                                                         timeStyle:NSDateFormatterShortStyle];
  }
  NSString * targetDistance = info.targetDistance;
  NSMutableAttributedString * distance;
  if (targetDistance)
  {
    self.distanceLabel.text = targetDistance;
    distance = [[NSMutableAttributedString alloc] initWithString:targetDistance
                                           attributes:routingNumberAttributes];
  }

  NSString * targetUnits = info.targetUnits;
  if (targetUnits)
  {
    self.distanceLegendLabel.text = targetUnits;
    if (distance)
    {
      [distance appendAttributedString:[[NSAttributedString alloc] initWithString:targetUnits
                                                                       attributes:routingLegendAttributes]];
      self.distanceWithLegendLabel.attributedText = distance;
    }
  }

  NSString * currentSpeed = info.speed ?: @"0";
  self.speedLabel.text = currentSpeed;
  self.speedLegendLabel.text = info.speedUnits;
  NSMutableAttributedString * speed =
      [[NSMutableAttributedString alloc] initWithString:currentSpeed
                                             attributes:routingNumberAttributes];
  [speed
      appendAttributedString:[[NSAttributedString alloc] initWithString:info.speedUnits
                                                             attributes:routingLegendAttributes]];
  self.speedWithLegendLabel.attributedText = speed;

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
  [self updateNavigationInfo:self.navigationInfo];
  [self refreshRoutingDiminishTimer];
}

- (IBAction)routingStartTouchUpInside { [MWMRouter startRouting]; }
- (IBAction)routingStopTouchUpInside { [MWMRouter stopRouting]; }
- (IBAction)soundTouchUpInside:(MWMButton *)sender
{
  BOOL const isEnabled = sender.selected;
  [Statistics logEvent:kStatMenu withParameters:@{kStatTTS : isEnabled ? kStatOn : kStatOff}];
  [MWMTextToSpeech tts].active = !isEnabled;
  [self refreshRoutingDiminishTimer];
}

#pragma mark - MWMTrafficManagerObserver

- (void)onTrafficStateUpdated
{
  MWMButton * tb = self.trafficButton;
  BOOL const enabled = ([MWMTrafficManager state] != TrafficManager::TrafficState::Disabled);
  tb.selected = enabled;
}

- (IBAction)trafficTouchUpInside:(MWMButton *)sender
{
  BOOL const switchOn = ([MWMTrafficManager state] == TrafficManager::TrafficState::Disabled);
  [Statistics logEvent:kStatMenu withParameters:@{kStatTraffic : switchOn ? kStatOn : kStatOff}];
  [MWMTrafficManager enableTraffic:switchOn];
  [self refreshRoutingDiminishTimer];
}

#pragma mark - Refresh Collection View layout

- (void)refreshLayout
{
  MWMBottomMenuLayout * cvLayout =
      (MWMBottomMenuLayout *)self.buttonsCollectionView.collectionViewLayout;
  cvLayout.buttonsCount = [self collectionView:self.additionalButtons numberOfItemsInSection:0];
  [self.additionalButtons reloadData];
  [(MWMBottomMenuView *)self.view refreshLayout];
}

#pragma mark - Layout

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
    auto & s = GetFramework().GetStorage();
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
  self.state = self.restoreState;
  [self.delegate actionDownloadMaps:mwm::DownloaderMode::Downloaded];
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
    [MWMRouter stopRouting];
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
  [self.delegate closeInfoScreens];
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
  case MWMBottomMenuStateRoutingError:
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

- (void)refreshRoutingDiminishTimer
{
  SEL const diminishFunction = @selector(diminishRoutingState);
  [NSObject cancelPreviousPerformRequestsWithTarget:self selector:diminishFunction object:self];
  runAsyncOnMainQueue(^{
    if (self.state == MWMBottomMenuStateRoutingExpanded)
      [self performSelector:diminishFunction withObject:self afterDelay:kRoutingDiminishInterval];
  });
}

- (void)diminishRoutingState { self.state = MWMBottomMenuStateRouting; }
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

- (MWMTaxiCollectionView *)taxiCollectionView
{
  return static_cast<MWMBottomMenuView *>(self.view).taxiCollectionView;
}

- (void)setState:(MWMBottomMenuState)state
{
  runAsyncOnMainQueue(^{
    [self.controller setNeedsStatusBarAppearanceUpdate];
  });
  [self refreshRoutingDiminishTimer];
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
    dispatch_async(dispatch_get_main_queue(), ^{
      self.p2pButton.selected =
          ([MWMNavigationDashboardManager manager].state == MWMNavigationDashboardStatePrepare);
    });
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

  if (state == MWMBottomMenuStateRoutingError)
    self.estimateLabel.text = self.routingErrorMessage;
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
- (void)setTtsSoundButton:(MWMButton *)ttsSoundButton
{
  _ttsSoundButton = ttsSoundButton;
  [ttsSoundButton setImage:[UIImage imageNamed:@"ic_voice_off"] forState:UIControlStateNormal];
  [ttsSoundButton setImage:[UIImage imageNamed:@"ic_voice_on"] forState:UIControlStateSelected];
  [ttsSoundButton setImage:[UIImage imageNamed:@"ic_voice_on"]
                  forState:UIControlStateSelected | UIControlStateHighlighted];
  [self ttsButtonStatusChanged:nil];
}

- (void)setTrafficButton:(MWMButton *)trafficButton
{
  _trafficButton = trafficButton;
  [trafficButton setImage:[UIImage imageNamed:@"ic_setting_traffic_off"]
                 forState:UIControlStateNormal];
  [trafficButton setImage:[UIImage imageNamed:@"ic_setting_traffic_on"]
                 forState:UIControlStateSelected];
  [trafficButton setImage:[UIImage imageNamed:@"ic_setting_traffic_on"]
                 forState:UIControlStateSelected | UIControlStateHighlighted];
}

- (CGFloat)mainStateHeight { return self.mainButtonsHeight.constant; }
@end
