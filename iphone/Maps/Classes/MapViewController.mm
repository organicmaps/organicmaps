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
#import "MWMEditorViewController.h"
#import "MWMMapViewControlsManager.h"
#import "MWMPageController.h"
#import "MWMPlacePageEntity.h"
#import "MWMTableViewController.h"
#import "MWMTextToSpeech.h"
#import "RouteState.h"
#import "Statistics.h"
#import "UIFont+MapsMeFonts.h"
#import "UIViewController+Navigation.h"
#import <MyTargetSDKCorp/MTRGManager_Corp.h>

#import "UIColor+MapsMeColor.h"

#import "3party/Alohalytics/src/alohalytics_objc.h"

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
extern NSString * const kUDWhatsNewWasShown = @"WhatsNewWithNightModeWasShown";
extern char const * kAdForbiddenSettingsKey;
extern char const * kAdServerForbiddenKey;
extern char const * kAutoDownloadEnabledKey;

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
NSString * const kAuthorizationSegue = @"Map2AuthorizationSegue";
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
                                MWMFrameworkMyPositionObserver, MWMFrameworkUserMarkObserver>

@property (nonatomic, readwrite) MWMMapViewControlsManager * controlsManager;
@property (nonatomic) MWMBottomMenuState menuRestoreState;

@property (nonatomic) ForceRoutingStateChange forceRoutingStateChange;
@property (nonatomic) BOOL disableStandbyOnLocationStateMode;

@property (nonatomic) UserTouchesAction userTouchesAction;
@property (nonatomic) MWMPageController * pageViewController;

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
      [[MapsAppDelegate theApp].m_locationManager stop:self];
      break;
    }
    case location::ENotSupported:
    {
      [self.alertController presentLocationServiceNotSupportedAlert];
      [[MapsAppDelegate theApp].m_locationManager stop:self];
      break;
    }
    default:
      break;
  }
}

- (void)onLocationUpdate:(location::GpsInfo const &)info
{
  // TODO: Remove this hack for location changing bug
  if (self.navigationController.visibleViewController == self)
  {
    if (info.m_source != location::EPredictor)
      [m_predictor reset:info];
    Framework & frm = GetFramework();
    frm.OnLocationUpdate(info);
    LOG_MEMORY_INFO();

    [self showPopover];
    [self updateRoutingInfo];

    if (self.forceRoutingStateChange == ForceRoutingStateChangeRestoreRoute)
      [self restoreRoute];
  }
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
  // TODO: Remove this hack for orientation changing bug
  if (self.navigationController.visibleViewController == self)
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

- (void)onMyPositionClicked:(id)sender
{
  GetFramework().SwitchMyPositionNextMode();
}

- (void)popoverControllerDidDismissPopover:(UIPopoverController *)popoverController
{
  [self destroyPopover];
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
  [self destroyPopover];
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

- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation
{
  [self showPopover];
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
  if (self.isDaemon)
    return;
  // Notify about entering foreground (should be called on the first launch too).
  GetFramework().EnterForeground();
}

- (void)viewWillAppear:(BOOL)animated
{
  [super viewWillAppear:animated];
  if (self.isDaemon)
    return;
  [[NSNotificationCenter defaultCenter] removeObserver:self name:UIDeviceOrientationDidChangeNotification object:nil];

  self.skipPlacePageDismissOnViewDisappear = NO;
  [self.controlsManager reloadPlacePage];
  self.controlsManager.menuState = self.menuRestoreState;

  [self refreshAd];

  [self updateStatusBarStyle];
  GetFramework().InvalidateRendering();
  [self showWhatsNewIfNeeded];
}

- (void)viewDidAppear:(BOOL)animated
{
  [super viewDidAppear:animated];
  [self checkAuthorization];
}

- (void)viewDidLoad
{
  [super viewDidLoad];
  if (self.isDaemon)
    return;
  self.view.clipsToBounds = YES;
  [MTRGManager setMyCom:YES];
  self.controlsManager = [[MWMMapViewControlsManager alloc] initWithParentController:self];
}

- (void)mwm_refreshUI
{
  [MapsAppDelegate customizeAppearance];
  [self.navigationController.navigationBar mwm_refreshUI];
  [self.controlsManager mwm_refreshUI];
}

- (void)showWhatsNewIfNeeded
{
  if (isIOS7)
    return;
  NSUserDefaults * ud = [NSUserDefaults standardUserDefaults];
  BOOL const whatsNewWasShown = [ud boolForKey:kUDWhatsNewWasShown];
  if (whatsNewWasShown)
    return;

  if (![Alohalytics isFirstSession])
    [self configureAndShowPageController];

  [ud setBool:YES forKey:kUDWhatsNewWasShown];
  [ud synchronize];
}

- (void)configureAndShowPageController
{
  self.pageViewController = [MWMPageController pageControllerWithParent:self];
  [self.pageViewController show];
}

- (void)viewWillDisappear:(BOOL)animated
{
  [super viewWillDisappear:animated];
  self.menuRestoreState = self.controlsManager.menuState;
  if (!self.skipPlacePageDismissOnViewDisappear)
    [self dismissPlacePage];
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

- (BOOL)isDaemon
{
  return MapsAppDelegate.theApp.m_locationManager.isDaemonMode;
}

- (id)initWithCoder:(NSCoder *)coder
{
  NSLog(@"MapViewController initWithCoder Started");
  self = [super initWithCoder:coder];
  if (self && !self.isDaemon)
    [self initialize];

  NSLog(@"MapViewController initWithCoder Ended");
  return self;
}

- (void)initialize
{
  m_predictor = [[LocationPredictor alloc] initWithObserver:self];
  self.forceRoutingStateChange = ForceRoutingStateChangeNone;
  self.userTouchesAction = UserTouchesActionNone;
  self.menuRestoreState = MWMBottomMenuStateInactive;
  GetFramework().LoadBookmarks();
  [[MWMFrameworkListener listener] addObserver:self];
}

- (void)openMapsDownloader
{
  [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"downloader"];
  [self performSegueWithIdentifier:@"Map2MapDownloaderSegue" sender:self];
}

#pragma mark - MWMFrameworkMyPositionObserver

- (void)processMyPositionStateModeChange:(location::EMyPositionMode)mode
{
  [m_predictor setMode:mode];

  switch (mode)
  {
    case location::MODE_UNKNOWN_POSITION:
    {
      self.disableStandbyOnLocationStateMode = NO;
      [[MapsAppDelegate theApp].m_locationManager stop:self];
      break;
    }
    case location::MODE_PENDING_POSITION:
      self.disableStandbyOnLocationStateMode = NO;
      [[MapsAppDelegate theApp].m_locationManager start:self];
      break;
    case location::MODE_NOT_FOLLOW:
      self.disableStandbyOnLocationStateMode = NO;
      break;
    case location::MODE_FOLLOW:
    case location::MODE_ROTATE_AND_FOLLOW:
      self.disableStandbyOnLocationStateMode = YES;
      break;
  }
}

#pragma mark - MWMFrameworkRouteBuilderObserver

- (void)processRouteBuilderEvent:(routing::IRouter::ResultCode)code
                       countries:(storage::TCountriesVec const &)absentCountries
                          routes:(storage::TCountriesVec const &)absentRoutes
{
  switch (code)
  {
    case routing::IRouter::ResultCode::NoError:
    {
      GetFramework().ActivateUserMark(nullptr, true);
      if (self.forceRoutingStateChange == ForceRoutingStateChangeStartFollowing)
        [self.controlsManager routingNavigation];
      else
        [self.controlsManager routingReady];
      [self updateRoutingInfo];
      bool isDisclaimerApproved = false;
      (void)Settings::Get("IsDisclaimerApproved", isDisclaimerApproved);
      if (!isDisclaimerApproved)
      {
        [self presentRoutingDisclaimerAlert];
        Settings::Set("IsDisclaimerApproved", true);
      }
      break;
    }
    case routing::IRouter::RouteFileNotExist:
    case routing::IRouter::InconsistentMWMandRoute:
    case routing::IRouter::NeedMoreMaps:
    case routing::IRouter::FileTooOld:
    case routing::IRouter::RouteNotFound:
    {
      [self presentDownloaderAlert:code countries:absentCountries routes:absentRoutes block:[=]
      {
        auto & s = GetFramework().Storage();
        for (auto const & countryId : absentCountries)
          s.DownloadNode(countryId);
        for (auto const & countryId : absentRoutes)
          s.DownloadNode(countryId);
        [self openMapsDownloader];
      }];
      break;
    }
    case routing::IRouter::Cancelled:
      break;
    default:
      [self presentDefaultAlert:code];
      break;
  }
  self.forceRoutingStateChange = ForceRoutingStateChangeNone;
}

#pragma mark - MWMFrameworkUserMarkObserver

- (void)processUserMarkActivation:(UserMark const *)mark
{
  if (mark == nullptr)
  {
    [self dismissPlacePage];

    auto & f = GetFramework();
    if (!f.HasActiveUserMark() && self.controlsManager.searchHidden && !f.IsRouteNavigable())
      self.controlsManager.hidden = !self.controlsManager.hidden;
  }
  else
  {
    self.controlsManager.hidden = NO;
    [self.controlsManager showPlacePage];
  }
}

#pragma mark - Bookmarks

- (void)openBookmarks
{
  BOOL const oneCategory = (GetFramework().GetBmCategoriesCount() == 1);
  MWMTableViewController * vc =
      oneCategory ? [[BookmarksVC alloc] initWithCategory:0] : [[BookmarksRootVC alloc] init];
  [self.navigationController pushViewController:vc animated:YES];
}

#pragma mark - Authorization

- (void)checkAuthorization
{
  using namespace osm_auth_ios;
  BOOL const isAfterFirstEdit = AuthorizationIsNeedCheck() && !AuthorizationHaveCredentials() && !AuthorizationIsUserSkip();
  if (isAfterFirstEdit)
  {
    [[Statistics instance] logEvent:kStatEventName(kStatPlacePage, kStatEditTime)
                     withParameters:@{kStatValue : kStatAuthorization}];
    [self performSegueWithIdentifier:kAuthorizationSegue sender:nil];
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
  (void)Settings::Get(kAdServerForbiddenKey, adServerForbidden);
  bool adForbidden = false;
  (void)Settings::Get(kAdForbiddenSettingsKey, adForbidden);
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
                        routes:(storage::TCountriesVec const &)routes
                         block:(TMWMVoidBlock)block
{
  if (countries.size() || routes.size())
    [self.alertController presentDownloaderAlertWithCountries:countries routes:routes code:code block:block];
  else
    [self presentDefaultAlert:code];
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

- (void)destroyPopover
{
  self.popoverVC = nil;
}

- (void)showPopover
{
  Framework & f = GetFramework();
  if (self.popoverVC)
    f.ActivateUserMark(nullptr, true);

  CGFloat const sf = self.view.contentScaleFactor;

  m2::PointD tmp = m2::PointD(f.GtoP(m2::PointD(m_popoverPos.x, m_popoverPos.y)));

  [self.popoverVC presentPopoverFromRect:CGRectMake(tmp.x / sf, tmp.y / sf, 1, 1) inView:self.view permittedArrowDirections:UIPopoverArrowDirectionAny animated:YES];
}

- (void)dismissPopover
{
  [self.popoverVC dismissPopoverAnimated:YES];
  [self destroyPopover];
}

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
  if ([segue.identifier isEqualToString:@"Map2EditorSegue"])
  {
    self.skipPlacePageDismissOnViewDisappear = YES;
    UINavigationController * dvc = segue.destinationViewController;
    MWMEditorViewController * editorVC = (MWMEditorViewController *)[dvc topViewController];
    editorVC.entity = sender;
  }
  else if ([segue.identifier isEqualToString:kAuthorizationSegue])
  {
    UINavigationController * dvc = segue.destinationViewController;
    MWMAuthorizationLoginViewController * authVC = (MWMAuthorizationLoginViewController *)[dvc topViewController];
    authVC.isCalledFromSettings = NO;
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

@end
