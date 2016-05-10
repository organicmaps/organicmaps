#import "BookmarksRootVC.h"
#import "BookmarksVC.h"
#import "Common.h"
#import "EAGLView.h"
#import "MapsAppDelegate.h"
#import "MapViewController.h"
#import "MWMAlertViewController.h"
#import "MWMAPIBar.h"
#import "MWMAuthorizationCommon.h"
#import "MWMAuthorizationLoginViewController.h"
#import "MWMAuthorizationWebViewLoginViewController.h"
#import "MWMEditorViewController.h"
#import "MWMFirstLaunchController.h"
#import "MWMFrameworkListener.h"
#import "MWMFrameworkObservers.h"
#import "MWMMapDownloadDialog.h"
#import "MWMMapDownloaderViewController.h"
#import "MWMMapViewControlsManager.h"
#import "MWMPageController.h"
#import "MWMPlacePageEntity.h"
#import "MWMStorage.h"
#import "MWMTableViewController.h"
#import "MWMTextToSpeech.h"
#import "MWMWhatsNewEditorController.h"
#import "RouteState.h"
#import "Statistics.h"
#import "UIColor+MapsMeColor.h"
#import "UIFont+MapsMeFonts.h"
#import "UIViewController+Navigation.h"
#import <MyTargetSDKCorp/MTRGManager_Corp.h>

#import "3party/Alohalytics/src/alohalytics_objc.h"

#include "indexer/osm_editor.hpp"

#include "Framework.h"

#include "../Statistics/Statistics.h"

#include "map/user_mark.hpp"

#include "drape_frontend/user_event_stream.hpp"

#include "platform/file_logging.hpp"
#include "platform/local_country_file_utils.hpp"
#include "platform/platform.hpp"
#include "platform/settings.hpp"

// If you have a "missing header error" here, then please run configure.sh script in the root repo folder.
#import "../../../private.h"

extern NSString * const kAlohalyticsTapEventKey = @"$onClick";
extern NSString * const kMap2OsmLoginSegue = @"Map2OsmLogin";
extern NSString * const kMap2FBLoginSegue = @"Map2FBLogin";
extern NSString * const kMap2GoogleLoginSegue = @"Map2GoogleLogin";
extern char const * kAdForbiddenSettingsKey;
extern char const * kAdServerForbiddenKey;

typedef NS_ENUM(NSUInteger, ForceRoutingStateChange)
{
  ForceRoutingStateChangeNone,
  ForceRoutingStateChangeRestoreRoute,
  ForceRoutingStateChangeStartFollowing
};

typedef NS_ENUM(NSUInteger, UserTouchesAction)
{
  UserTouchesActionNone,
  UserTouchesActionDrag,
  UserTouchesActionScale
};

namespace
{
NSString * const kDownloaderSegue = @"Map2MapDownloaderSegue";
NSString * const kMigrationSegue = @"Map2MigrationSegue";
NSString * const kEditorSegue = @"Map2EditorSegue";
NSString * const kUDViralAlertWasShown = @"ViralAlertWasShown";

// The first launch after process started. Used to skip "Not follow, no position" state and to run locator.
BOOL gIsFirstMyPositionMode = YES;
} // namespace

@interface NSValueWrapper : NSObject

-(NSValue *)getInnerValue;

@end

@implementation NSValueWrapper
{
  NSValue * m_innerValue;
}

-(NSValue *)getInnerValue
{
  return m_innerValue;
}

-(id)initWithValue:(NSValue *)value
{
  self = [super init];
  if (self)
    m_innerValue = value;
  return self;
}

-(BOOL)isEqual:(id)anObject
{
  return [anObject isMemberOfClass:[NSValueWrapper class]];
}

@end

@interface MapViewController ()<MTRGNativeAppwallAdDelegate, MWMFrameworkRouteBuilderObserver,
                                MWMFrameworkDrapeObserver, MWMFrameworkStorageObserver,
                                MWMPageControllerProtocol>

@property (nonatomic, readwrite) MWMMapViewControlsManager * controlsManager;
@property (nonatomic) MWMBottomMenuState menuRestoreState;

@property (nonatomic) ForceRoutingStateChange forceRoutingStateChange;
@property (nonatomic) BOOL disableStandbyOnLocationStateMode;

@property (nonatomic) UserTouchesAction userTouchesAction;
@property (nonatomic) MWMPageController * pageViewController;
@property (nonatomic) MWMMapDownloadDialog * downloadDialog;

@property (nonatomic) BOOL skipForceTouch;

@end

@implementation MapViewController

#pragma mark - LocationManager Callbacks

- (void)onLocationError:(location::TLocationError)errorCode
{
  GetFramework().OnLocationError(errorCode);

  switch (errorCode)
  {
    case location::EDenied:
    {
      [self.alertController presentLocationAlert];
      [[MapsAppDelegate theApp].locationManager stop:self];
      break;
    }
    case location::ENotSupported:
    {
      [self.alertController presentLocationServiceNotSupportedAlert];
      [[MapsAppDelegate theApp].locationManager stop:self];
      break;
    }
    default:
      break;
  }
}

- (void)onLocationUpdate:(location::GpsInfo const &)info
{
  if (info.m_source != location::EPredictor)
    [m_predictor reset:info];
  Framework & frm = GetFramework();
  frm.OnLocationUpdate(info);
  LOG_MEMORY_INFO();

  [self updateRoutingInfo];

  if (self.forceRoutingStateChange == ForceRoutingStateChangeRestoreRoute)
    [self restoreRoute];
}

- (void)updateRoutingInfo
{
  Framework & frm = GetFramework();
  if (!frm.IsRoutingActive())
    return;

  location::FollowingInfo res;
  frm.GetRouteFollowingInfo(res);

  if (res.IsValid())
    [self.controlsManager setupRoutingDashboard:res];

  if (frm.IsOnRoute())
    [[MWMTextToSpeech tts] playTurnNotifications];
}

- (void)onCompassUpdate:(location::CompassInfo const &)info
{
  GetFramework().OnCompassUpdate(info);
}

#pragma mark - Restore route

- (void)restoreRoute
{
  self.forceRoutingStateChange = ForceRoutingStateChangeStartFollowing;
  [self.controlsManager restoreRouteTo:self.restoreRouteDestination];
}

#pragma mark - Map Navigation

- (void)dismissPlacePage
{
  [self.controlsManager dismissPlacePage];
}

- (void)onMapObjectDeselected:(bool)switchFullScreenMode
{
  [self dismissPlacePage];

  auto & f = GetFramework();
  if (switchFullScreenMode && self.controlsManager.searchHidden && !f.IsRouteNavigable())
    self.controlsManager.hidden = !self.controlsManager.hidden;
}

- (void)onMapObjectSelected:(place_page::Info const &)info
{
  self.controlsManager.hidden = NO;
  [self.controlsManager showPlacePage:info];
}

- (void)checkMaskedPointer:(UITouch *)touch withEvent:(df::TouchEvent &)e
{
  int64_t id = reinterpret_cast<int64_t>(touch);
  int8_t pointerIndex = df::TouchEvent::INVALID_MASKED_POINTER;
  if (e.m_touches[0].m_id == id)
    pointerIndex = 0;
  else if (e.m_touches[1].m_id == id)
    pointerIndex = 1;

  if (e.GetFirstMaskedPointer() == df::TouchEvent::INVALID_MASKED_POINTER)
    e.SetFirstMaskedPointer(pointerIndex);
  else
    e.SetSecondMaskedPointer(pointerIndex);
}

- (void)sendTouchType:(df::TouchEvent::ETouchType)type withTouches:(NSSet *)touches andEvent:(UIEvent *)event
{
  NSArray * allTouches = [[event allTouches] allObjects];
  if ([allTouches count] < 1)
    return;

  UIView * v = self.view;
  CGFloat const scaleFactor = v.contentScaleFactor;

  df::TouchEvent e;
  UITouch * touch = [allTouches objectAtIndex:0];
  CGPoint const pt = [touch locationInView:v];
  e.m_type = type;
  e.m_touches[0].m_id = reinterpret_cast<int64_t>(touch);
  e.m_touches[0].m_location = m2::PointD(pt.x * scaleFactor, pt.y * scaleFactor);
  if ([self hasForceTouch])
    e.m_touches[0].m_force = touch.force / touch.maximumPossibleForce;
  if (allTouches.count > 1)
  {
    UITouch * touch = [allTouches objectAtIndex:1];
    CGPoint const pt = [touch locationInView:v];
    e.m_touches[1].m_id = reinterpret_cast<int64_t>(touch);
    e.m_touches[1].m_location = m2::PointD(pt.x * scaleFactor, pt.y * scaleFactor);
    if ([self hasForceTouch])
      e.m_touches[1].m_force = touch.force / touch.maximumPossibleForce;
  }

  NSArray * toggledTouches = [touches allObjects];
  if (toggledTouches.count > 0)
    [self checkMaskedPointer:[toggledTouches objectAtIndex:0] withEvent:e];

  if (toggledTouches.count > 1)
    [self checkMaskedPointer:[toggledTouches objectAtIndex:1] withEvent:e];

  Framework & f = GetFramework();
  f.TouchEvent(e);
}

- (BOOL)hasForceTouch
{
  if (isIOS7 || isIOS8)
    return NO;
  return self.view.traitCollection.forceTouchCapability == UIForceTouchCapabilityAvailable;
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
  [self sendTouchType:df::TouchEvent::TOUCH_DOWN withTouches:touches andEvent:event];
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
  [self sendTouchType:df::TouchEvent::TOUCH_MOVE withTouches:nil andEvent:event];
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
  [self sendTouchType:df::TouchEvent::TOUCH_UP withTouches:touches andEvent:event];
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
  [self sendTouchType:df::TouchEvent::TOUCH_CANCEL withTouches:touches andEvent:event];
}

#pragma mark - ViewController lifecycle

- (void)dealloc
{
  [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (BOOL)shouldAutorotateToInterfaceOrientation: (UIInterfaceOrientation)interfaceOrientation
{
  return YES; // We support all orientations
}

- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation
                                duration:(NSTimeInterval)duration
{
  [self.alertController willRotateToInterfaceOrientation:toInterfaceOrientation duration:duration];
  [self.controlsManager willRotateToInterfaceOrientation:toInterfaceOrientation duration:duration];
}

- (void)viewWillTransitionToSize:(CGSize)size
       withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator
{
  [self.alertController willRotateToInterfaceOrientation:(size.width > size.height) ?
                    UIInterfaceOrientationLandscapeLeft : UIInterfaceOrientationPortrait
                                                                        duration:kDefaultAnimationDuration];
  [self.controlsManager viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
  [self.pageViewController viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
}

- (void)didReceiveMemoryWarning
{
  GetFramework().MemoryWarning();
  [super didReceiveMemoryWarning];
}

- (void)onTerminate
{
  [(EAGLView *)self.view deallocateNative];
}

- (void)onEnterBackground
{
  // Save state and notify about entering background.
  GetFramework().EnterBackground();
}

- (void)setMapStyle:(MapStyle)mapStyle
{
  GetFramework().SetMapStyle(mapStyle);
}

- (void)onEnterForeground
{
  if (MapsAppDelegate.theApp.isDaemonMode)
    return;
  // Notify about entering foreground (should be called on the first launch too).
  GetFramework().EnterForeground();
}

- (void)onGetFocus:(BOOL)isOnFocus
{
  [(EAGLView *)self.view setPresentAvailable:isOnFocus];
}

- (void)viewWillAppear:(BOOL)animated
{
  [super viewWillAppear:animated];
  if (MapsAppDelegate.theApp.isDaemonMode)
    return;
  [[NSNotificationCenter defaultCenter] removeObserver:self name:UIDeviceOrientationDidChangeNotification object:nil];

  self.controlsManager.menuState = self.menuRestoreState;

  [self refreshAd];

  [self updateStatusBarStyle];
  GetFramework().InvalidateRendering();
  [self showWelcomeScreenIfNeeded];
  [self showViralAlertIfNeeded];
}

- (void)viewDidAppear:(BOOL)animated
{
  [super viewDidAppear:animated];
  [self checkAuthorization];
}

- (void)viewDidLoad
{
  [super viewDidLoad];
  if (MapsAppDelegate.theApp.isDaemonMode)
    return;
  self.view.clipsToBounds = YES;
  [MTRGManager setMyCom:YES];
}

- (void)mwm_refreshUI
{
  [MapsAppDelegate customizeAppearance];
  [self.navigationController.navigationBar mwm_refreshUI];
  [self.controlsManager mwm_refreshUI];
  [self.downloadDialog mwm_refreshUI];
}

- (void)showWelcomeScreenIfNeeded
{
  if (isIOS7)
    return;

  Class<MWMWelcomeControllerProtocol> whatsNewClass = [MWMWhatsNewEditorController class];
  BOOL const isFirstSession = [Alohalytics isFirstSession];
  Class<MWMWelcomeControllerProtocol> welcomeClass = isFirstSession ? [MWMFirstLaunchController class] : whatsNewClass;

  NSUserDefaults * ud = [NSUserDefaults standardUserDefaults];
  if ([ud boolForKey:[welcomeClass udWelcomeWasShownKey]])
    return;

  self.pageViewController = [MWMPageController pageControllerWithParent:self welcomeClass:welcomeClass];
  [self.pageViewController show];

  [ud setBool:YES forKey:[whatsNewClass udWelcomeWasShownKey]];
  [ud setBool:YES forKey:[welcomeClass udWelcomeWasShownKey]];
  [ud synchronize];
}

- (void)closePageController:(MWMPageController *)pageController
{
  if ([pageController isEqual:self.pageViewController])
    self.pageViewController = nil;
}

- (void)showViralAlertIfNeeded
{
  NSUserDefaults * ud = [NSUserDefaults standardUserDefaults];

  using namespace osm_auth_ios;
  if (!AuthorizationIsNeedCheck() || [ud objectForKey:kUDViralAlertWasShown] || !AuthorizationHaveCredentials())
    return;

  if (osm::Editor::Instance().GetStats().m_edits.size() != 2)
    return;

  if (!Platform::IsConnected())
    return;

  [self.alertController presentEditorViralAlert];

  [ud setObject:[NSDate date] forKey:kUDViralAlertWasShown];
  [ud synchronize];
}

- (void)viewWillDisappear:(BOOL)animated
{
  [super viewWillDisappear:animated];
  self.menuRestoreState = self.controlsManager.menuState;
  [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(orientationChanged:) name:UIDeviceOrientationDidChangeNotification object:nil];
}

- (void)presentViewController:(UIViewController *)viewControllerToPresent
                     animated:(BOOL)flag
                   completion:(TMWMVoidBlock)completion
{
  if (isIOS7)
    self.menuRestoreState = self.controlsManager.menuState;
  [super presentViewController:viewControllerToPresent animated:flag completion:completion];
}

- (void)orientationChanged:(NSNotification *)notification
{
  [self willRotateToInterfaceOrientation:self.interfaceOrientation duration:0.];
}

- (BOOL)prefersStatusBarHidden
{
  return self.apiBar.isVisible;
}

- (UIStatusBarStyle)preferredStatusBarStyle
{
  BOOL const isNightMode = [UIColor isNightMode];
  BOOL const isLight = !self.controlsManager.searchHidden ||
                       self.controlsManager.menuState == MWMBottomMenuStateActive ||
                       self.controlsManager.isDirectionViewShown ||
                       (isNightMode &&
                       self.controlsManager.navigationState != MWMNavigationDashboardStateHidden) ||
                       MapsAppDelegate.theApp.routingPlaneMode != MWMRoutingPlaneModeNone;
  return (isLight || (!isLight && isNightMode)) ? UIStatusBarStyleLightContent : UIStatusBarStyleDefault;
}

- (void)updateStatusBarStyle
{
  [self setNeedsStatusBarAppearanceUpdate];
}

- (id)initWithCoder:(NSCoder *)coder
{
  NSLog(@"MapViewController initWithCoder Started");
  self = [super initWithCoder:coder];
  if (self && !MapsAppDelegate.theApp.isDaemonMode)
    [self initialize];

  NSLog(@"MapViewController initWithCoder Ended");
  return self;
}

- (void)initialize
{
  Framework & f = GetFramework();
  // TODO: Review and improve this code.
  f.SetMapSelectionListeners([self](place_page::Info const & info) { [self onMapObjectSelected:info]; },
                             [self](bool switchFullScreen) { [self onMapObjectDeselected:switchFullScreen]; });
  // TODO: Review and improve this code.
  f.SetMyPositionModeListener([self](location::EMyPositionMode mode, bool routingActive)
  {
    // TODO: Two global listeners are subscribed to the same event from the core.
    // Probably it's better to subscribe only wnen needed and usubscribe in other cases.
    // May be better solution would be multiobservers support in the C++ core.
    [self processMyPositionStateModeEvent:mode];
    [self.controlsManager.menuController processMyPositionStateModeEvent:mode];
    [[MapsAppDelegate theApp].locationManager processMyPositionStateModeEvent:mode];
  });

  m_predictor = [[LocationPredictor alloc] initWithObserver:self];
  self.forceRoutingStateChange = ForceRoutingStateChangeNone;
  self.userTouchesAction = UserTouchesActionNone;
  self.menuRestoreState = MWMBottomMenuStateInactive;
  GetFramework().LoadBookmarks();
  [MWMFrameworkListener addObserver:self];
}

#pragma mark - Open controllers

- (void)openMigration
{
  [self performSegueWithIdentifier:kMigrationSegue sender:self];
}

- (void)openBookmarks
{
  BOOL const oneCategory = (GetFramework().GetBmCategoriesCount() == 1);
  MWMTableViewController * vc = oneCategory ? [[BookmarksVC alloc] initWithCategory:0] : [[BookmarksRootVC alloc] init];
  [self.navigationController pushViewController:vc animated:YES];
}

- (void)openMapsDownloader:(mwm::DownloaderMode)mode
{
  [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"downloader"];
  [self performSegueWithIdentifier:kDownloaderSegue sender:@(static_cast<NSInteger>(mode))];
}

- (void)openEditor
{
  using namespace osm_auth_ios;
  auto const & featureID = self.controlsManager.placePageEntity.info.GetID();

  [Statistics logEvent:kStatEditorEditStart withParameters:@{kStatEditorIsAuthenticated : @(AuthorizationHaveCredentials()),
                                                             kStatIsOnline : Platform::IsConnected() ? kStatYes : kStatNo,
                                                        kStatEditorMWMName : @(featureID.GetMwmName().c_str()),
                                                     kStatEditorMWMVersion : @(featureID.GetMwmVersion())}];
  [self performSegueWithIdentifier:kEditorSegue sender:self.controlsManager.placePageEntity];
}

- (void)processMyPositionStateModeEvent:(location::EMyPositionMode)mode
{
  [m_predictor setMode:mode];

  switch (mode)
  {
    case location::PendingPosition:
      self.disableStandbyOnLocationStateMode = NO;
      [[MapsAppDelegate theApp].locationManager start:self];
      break;
    case location::NotFollowNoPosition:
      if (gIsFirstMyPositionMode && ![Alohalytics isFirstSession])
      {
        GetFramework().SwitchMyPositionNextMode();
      }
      else
      {
        self.disableStandbyOnLocationStateMode = NO;
        BOOL const isLocationManagerStarted = [MapsAppDelegate theApp].locationManager.isStarted;
        BOOL const isMapVisible = (self.navigationController.visibleViewController == self);
        if (isLocationManagerStarted && isMapVisible && ![Alohalytics isFirstSession])
        {
          [[MapsAppDelegate theApp].locationManager stop:self];
          [self.alertController presentLocationNotFoundAlertWithOkBlock:^
          {
            GetFramework().SwitchMyPositionNextMode();
          }];
        }
      }
      break;
    case location::NotFollow:
      self.disableStandbyOnLocationStateMode = NO;
      break;
    case location::Follow:
    case location::FollowAndRotate:
      self.disableStandbyOnLocationStateMode = YES;
      break;
  }
  gIsFirstMyPositionMode = NO;
}

#pragma mark - MWMFrameworkRouteBuilderObserver

- (void)processRouteBuilderEvent:(routing::IRouter::ResultCode)code
                       countries:(storage::TCountriesVec const &)absentCountries
{
  switch (code)
  {
    case routing::IRouter::ResultCode::NoError:
    {
      GetFramework().DeactivateMapSelection(true);
      if (self.forceRoutingStateChange == ForceRoutingStateChangeStartFollowing)
        [self.controlsManager routingNavigation];
      else
        [self.controlsManager routingReady];
      [self updateRoutingInfo];
      bool isDisclaimerApproved = false;
      (void)settings::Get("IsDisclaimerApproved", isDisclaimerApproved);
      if (!isDisclaimerApproved)
      {
        [self presentRoutingDisclaimerAlert];
        settings::Set("IsDisclaimerApproved", true);
      }
      break;
    }
    case routing::IRouter::RouteFileNotExist:
    case routing::IRouter::InconsistentMWMandRoute:
    case routing::IRouter::NeedMoreMaps:
    case routing::IRouter::FileTooOld:
    case routing::IRouter::RouteNotFound:
      [self presentDownloaderAlert:code countries:absentCountries];
      break;
    case routing::IRouter::Cancelled:
      break;
    default:
      [self presentDefaultAlert:code];
      break;
  }
  self.forceRoutingStateChange = ForceRoutingStateChangeNone;
}

#pragma mark - MWMFrameworkDrapeObserver

- (void)processViewportCountryEvent:(TCountryId const &)countryId
{
  [self.downloadDialog processViewportCountryEvent:countryId];
}

#pragma mark - MWMFrameworkStorageObserver

- (void)processCountryEvent:(TCountryId const &)countryId
{
  NodeStatuses nodeStatuses{};
  GetFramework().Storage().GetNodeStatuses(countryId, nodeStatuses);
  if (nodeStatuses.m_status != NodeStatus::Error)
    return;
  switch (nodeStatuses.m_error)
  {
    case NodeErrorCode::NoError:
      break;
    case NodeErrorCode::UnknownError:
      [Statistics logEvent:kStatDownloaderMapError withParameters:@{kStatType : kStatUnknownError}];
      break;
    case NodeErrorCode::OutOfMemFailed:
      [Statistics logEvent:kStatDownloaderMapError withParameters:@{kStatType : kStatNoSpace}];
      break;
    case NodeErrorCode::NoInetConnection:
      [Statistics logEvent:kStatDownloaderMapError withParameters:@{kStatType : kStatNoConnection}];
      break;
  }
}

#pragma mark - Authorization

- (void)checkAuthorization
{
  using namespace osm_auth_ios;
  BOOL const isAfterEditing = AuthorizationIsNeedCheck() && !AuthorizationHaveCredentials();
  if (isAfterEditing)
  {
    AuthorizationSetNeedCheck(NO);
    if (!Platform::IsConnected())
      return;
    [Statistics logEvent:kStatEventName(kStatPlacePage, kStatEditTime)
                     withParameters:@{kStatValue : kStatAuthorization}];
    [self.alertController presentOsmAuthAlert];
  }
}

#pragma mark - 3d touch

- (void)performAction:(NSString *)action
{
  [self.navigationController popToRootViewControllerAnimated:NO];
  self.controlsManager.searchHidden = YES;
  [self.controlsManager routingHidden];
  if ([action isEqualToString:@"me.maps.3daction.bookmarks"])
  {
    [self openBookmarks];
  }
  else if ([action isEqualToString:@"me.maps.3daction.search"])
  {
    self.controlsManager.searchHidden = NO;
  }
  else if ([action isEqualToString:@"me.maps.3daction.route"])
  {
    [MapsAppDelegate theApp].routingPlaneMode = MWMRoutingPlaneModePlacePage;
    [self.controlsManager routingPrepare];
  }
}

#pragma mark - myTarget

- (void)refreshAd
{
  bool adServerForbidden = false;
  (void)settings::Get(kAdServerForbiddenKey, adServerForbidden);
  bool adForbidden = false;
  (void)settings::Get(kAdForbiddenSettingsKey, adForbidden);
  if (isIOS7 || adServerForbidden || adForbidden)
  {
    self.appWallAd = nil;
    return;
  }
  if (self.isAppWallAdActive)
    return;
  self.appWallAd = [[MTRGNativeAppwallAd alloc]initWithSlotId:@(MY_TARGET_KEY)];
  self.appWallAd.handleLinksInApp = YES;
  self.appWallAd.closeButtonTitle = L(@"close");
  self.appWallAd.delegate = self;
  [self.appWallAd load];
}

- (void)onLoadWithAppwallBanners:(NSArray *)appwallBanners appwallAd:(MTRGNativeAppwallAd *)appwallAd
{
  if (![appwallAd isEqual:self.appWallAd])
    return;
  if (appwallBanners.count == 0)
    self.appWallAd = nil;
  [self.controlsManager refreshLayout];
}

- (void)onNoAdWithReason:(NSString *)reason appwallAd:(MTRGNativeAppwallAd *)appwallAd
{
  if (![appwallAd isEqual:self.appWallAd])
    return;
  self.appWallAd = nil;
}

#pragma mark - API bar

- (MWMAPIBar *)apiBar
{
  if (!_apiBar)
    _apiBar = [[MWMAPIBar alloc] initWithController:self];
  return _apiBar;
}

- (void)showAPIBar
{
  self.apiBar.isVisible = YES;
}

#pragma mark - ShowDialog callback

- (void)presentDownloaderAlert:(routing::IRouter::ResultCode)code
                     countries:(storage::TCountriesVec const &)countries
{
  if (platform::migrate::NeedMigrate())
  {
    [self.alertController presentRoutingMigrationAlertWithOkBlock:^
    {
      [Statistics logEvent:kStatDownloaderMigrationDialogue
            withParameters:@{kStatFrom : kStatRouting}];
      [self openMigration];
    }];
  }
  else if (!countries.empty())
  {
    [self.alertController presentDownloaderAlertWithCountries:countries
        code:code
        cancelBlock:^
        {
          if (code != routing::IRouter::NeedMoreMaps)
            [self.controlsManager routingHidden];
        }
        downloadBlock:^(storage::TCountriesVec const & downloadCountries, TMWMVoidBlock onSuccess)
        {
          [MWMStorage downloadNodes:downloadCountries
                    alertController:self.alertController
                          onSuccess:onSuccess];
        }
        downloadCompleteBlock:^
        {
          [self.controlsManager buildRoute];
        }];
  }
  else
  {
    [self presentDefaultAlert:code];
  }
}

- (void)presentDisabledLocationAlert
{
  [self.alertController presentDisabledLocationAlert];
}

- (void)presentDefaultAlert:(routing::IRouter::ResultCode)type
{
  [self.alertController presentAlert:type];
}

- (void)presentRoutingDisclaimerAlert
{
  [self.alertController presentRoutingDisclaimerAlert];
}

#pragma mark - Private methods

- (void)setRestoreRouteDestination:(m2::PointD)restoreRouteDestination
{
  _restoreRouteDestination = restoreRouteDestination;
  self.forceRoutingStateChange = ForceRoutingStateChangeRestoreRoute;
}

- (void)setDisableStandbyOnLocationStateMode:(BOOL)disableStandbyOnLocationStateMode
{
  if (_disableStandbyOnLocationStateMode == disableStandbyOnLocationStateMode)
    return;
  _disableStandbyOnLocationStateMode = disableStandbyOnLocationStateMode;
  if (disableStandbyOnLocationStateMode)
    [[MapsAppDelegate theApp] disableStandby];
  else
    [[MapsAppDelegate theApp] enableStandby];
}

#pragma mark - Segue

- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender
{
  if ([segue.identifier isEqualToString:kEditorSegue])
  {
    MWMEditorViewController * dvc = segue.destinationViewController;
    [dvc setFeatureToEdit:static_cast<MWMPlacePageEntity *>(sender).featureID];
  }
  else if ([segue.identifier isEqualToString:kDownloaderSegue])
  {
    MWMMapDownloaderViewController * dvc = segue.destinationViewController;
    NSNumber * mode = sender;
    [dvc setParentCountryId:@(GetFramework().Storage().GetRootId().c_str()) mode:static_cast<mwm::DownloaderMode>(mode.integerValue)];
  }
  else if ([segue.identifier isEqualToString:kMap2FBLoginSegue])
  {
    MWMAuthorizationWebViewLoginViewController * dvc = segue.destinationViewController;
    dvc.authType = MWMWebViewAuthorizationTypeFacebook;
  }
  else if ([segue.identifier isEqualToString:kMap2GoogleLoginSegue])
  {
    MWMAuthorizationWebViewLoginViewController * dvc = segue.destinationViewController;
    dvc.authType = MWMWebViewAuthorizationTypeGoogle;
  }
}

#pragma mark - Properties

- (void)setAppWallAd:(MTRGNativeAppwallAd *)appWallAd
{
  _appWallAd = appWallAd;
  [self.controlsManager refreshLayout];
}

- (MWMMapViewControlsManager *)controlsManager
{
  if (!_controlsManager)
    _controlsManager = [[MWMMapViewControlsManager alloc] initWithParentController:self];
  return _controlsManager;
}

- (BOOL)isAppWallAdActive
{
  BOOL const haveAppWall = (self.appWallAd != nil);
  BOOL const haveBanners = (self.appWallAd.banners && self.appWallAd.banners != 0);
  return haveAppWall && haveBanners;
}

- (BOOL)hasNavigationBar
{
  return NO;
}

- (MWMMapDownloadDialog *)downloadDialog
{
  if (!_downloadDialog)
    _downloadDialog = [MWMMapDownloadDialog dialogForController:self];
  return _downloadDialog;
}

@end
