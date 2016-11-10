#import "MapViewController.h"
#import "BookmarksRootVC.h"
#import "BookmarksVC.h"
#import "Common.h"
#import "EAGLView.h"
#import "MWMAPIBar.h"
#import "MWMAlertViewController.h"
#import "MWMAuthorizationCommon.h"
#import "MWMAuthorizationLoginViewController.h"
#import "MWMAuthorizationWebViewLoginViewController.h"
#import "MWMEditBookmarkController.h"
#import "MWMEditorViewController.h"
#import "MWMFirstLaunchController.h"
#import "MWMFrameworkListener.h"
#import "MWMFrameworkObservers.h"
#import "MWMLocationHelpers.h"
#import "MWMLocationManager.h"
#import "MWMMapDownloadDialog.h"
#import "MWMMapDownloaderViewController.h"
#import "MWMMapViewControlsManager.h"
#import "MWMPlacePageData.h"
#import "MWMPlacePageEntity.h"
#import "MWMPlacePageProtocol.h"
#import "MWMRouter.h"
#import "MWMRouterSavedState.h"
#import "MWMSettings.h"
#import "MWMStorage.h"
#import "MWMTableViewController.h"
#import "MWMWhatsNewUberController.h"
#import "MapsAppDelegate.h"
#import "Statistics.h"
#import "UIColor+MapsMeColor.h"
#import "UIFont+MapsMeFonts.h"
#import "UIViewController+Navigation.h"

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

// If you have a "missing header error" here, then please run configure.sh script in the root repo
// folder.
#import "../../../private.h"

extern NSString * const kAlohalyticsTapEventKey = @"$onClick";
extern NSString * const kMap2OsmLoginSegue = @"Map2OsmLogin";
extern NSString * const kMap2FBLoginSegue = @"Map2FBLogin";
extern NSString * const kMap2GoogleLoginSegue = @"Map2GoogleLogin";

typedef NS_ENUM(NSUInteger, UserTouchesAction) {
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
NSString * const kPP2BookmarkEditingSegue = @"PP2BookmarkEditing";

// The first launch after process started. Used to skip "Not follow, no position" state and to run
// locator.
BOOL gIsFirstMyPositionMode = YES;
}  // namespace

@interface NSValueWrapper : NSObject

- (NSValue *)getInnerValue;

@end

@implementation NSValueWrapper
{
  NSValue * m_innerValue;
}

- (NSValue *)getInnerValue { return m_innerValue; }
- (id)initWithValue:(NSValue *)value
{
  self = [super init];
  if (self)
    m_innerValue = value;
  return self;
}

- (BOOL)isEqual:(id)anObject { return [anObject isMemberOfClass:[NSValueWrapper class]]; }
@end

@interface MapViewController ()<MWMFrameworkDrapeObserver, MWMFrameworkStorageObserver,
                                MWMPageControllerProtocol>

@property(nonatomic, readwrite) MWMMapViewControlsManager * controlsManager;

@property(nonatomic) BOOL disableStandbyOnLocationStateMode;

@property(nonatomic) UserTouchesAction userTouchesAction;

@property(nonatomic) MWMMapDownloadDialog * downloadDialog;

@property(nonatomic) BOOL skipForceTouch;

@end

@implementation MapViewController

+ (MapViewController *)controller { return [MapsAppDelegate theApp].mapViewController; }
#pragma mark - Map Navigation

- (void)dismissPlacePage { [self.controlsManager dismissPlacePage]; }
- (void)onMapObjectDeselected:(bool)switchFullScreenMode
{
  [self dismissPlacePage];

  if (!switchFullScreenMode)
    return;

  if ([MapsAppDelegate theApp].hasApiURL)
    return;

  MWMMapViewControlsManager * cm = self.controlsManager;
  if (cm.searchHidden && cm.navigationState == MWMNavigationDashboardStateHidden)
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
  if (e.GetFirstTouch().m_id == id)
    pointerIndex = 0;
  else if (e.GetSecondTouch().m_id == id)
    pointerIndex = 1;

  if (e.GetFirstMaskedPointer() == df::TouchEvent::INVALID_MASKED_POINTER)
    e.SetFirstMaskedPointer(pointerIndex);
  else
    e.SetSecondMaskedPointer(pointerIndex);
}

- (void)sendTouchType:(df::TouchEvent::ETouchType)type
          withTouches:(NSSet *)touches
             andEvent:(UIEvent *)event
{
  NSArray * allTouches = [[event allTouches] allObjects];
  if ([allTouches count] < 1)
    return;

  UIView * v = self.view;
  CGFloat const scaleFactor = v.contentScaleFactor;

  df::TouchEvent e;
  UITouch * touch = [allTouches objectAtIndex:0];
  CGPoint const pt = [touch locationInView:v];

  e.SetTouchType(type);

  df::Touch t0;
  t0.m_location = m2::PointD(pt.x * scaleFactor, pt.y * scaleFactor);
  t0.m_id = reinterpret_cast<int64_t>(touch);
  if ([self hasForceTouch])
    t0.m_force = touch.force / touch.maximumPossibleForce;
  e.SetFirstTouch(t0);

  if (allTouches.count > 1)
  {
    UITouch * touch = [allTouches objectAtIndex:1];
    CGPoint const pt = [touch locationInView:v];

    df::Touch t1;
    t1.m_location = m2::PointD(pt.x * scaleFactor, pt.y * scaleFactor);
    t1.m_id = reinterpret_cast<int64_t>(touch);
    if ([self hasForceTouch])
      t1.m_force = touch.force / touch.maximumPossibleForce;
    e.SetSecondTouch(t1);
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
  if (isIOS8)
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

- (void)dealloc { [[NSNotificationCenter defaultCenter] removeObserver:self]; }
- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
  return YES;  // We support all orientations
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
  [self.alertController willRotateToInterfaceOrientation:(size.width > size.height)
                                                             ? UIInterfaceOrientationLandscapeLeft
                                                             : UIInterfaceOrientationPortrait
                                                duration:kDefaultAnimationDuration];
  [self.controlsManager viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
  [self.pageViewController viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
}

- (void)didReceiveMemoryWarning
{
  GetFramework().MemoryWarning();
  [super didReceiveMemoryWarning];
}

- (void)onTerminate { [(EAGLView *)self.view deallocateNative]; }
- (void)onGetFocus:(BOOL)isOnFocus { [(EAGLView *)self.view setPresentAvailable:isOnFocus]; }
- (void)viewWillAppear:(BOOL)animated
{
  [super viewWillAppear:animated];
  [[NSNotificationCenter defaultCenter] removeObserver:self
                                                  name:UIDeviceOrientationDidChangeNotification
                                                object:nil];

  self.controlsManager.menuState = self.controlsManager.menuRestoreState;

  [self updateStatusBarStyle];
  GetFramework().InvalidateRendering();
  [self showWelcomeScreenIfNeeded];
  [self showViralAlertIfNeeded];
  [self checkAuthorization];
}

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.view.clipsToBounds = YES;
  [self processMyPositionStateModeEvent:location::PendingPosition];
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
  Class<MWMWelcomeControllerProtocol> whatsNewClass = [MWMWhatsNewUberController class];
  BOOL const isFirstSession = [Alohalytics isFirstSession];
  Class<MWMWelcomeControllerProtocol> welcomeClass =
      isFirstSession ? [MWMFirstLaunchController class] : whatsNewClass;

  NSUserDefaults * ud = [NSUserDefaults standardUserDefaults];
  if ([ud boolForKey:[welcomeClass udWelcomeWasShownKey]])
    return;
  
  self.pageViewController =
      [MWMPageController pageControllerWithParent:self welcomeClass:welcomeClass];
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
  if (!AuthorizationIsNeedCheck() || [ud objectForKey:kUDViralAlertWasShown] ||
      !AuthorizationHaveCredentials())
    return;

  if (osm::Editor::Instance().GetStats().m_edits.size() < 2)
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
  self.controlsManager.menuRestoreState = self.controlsManager.menuState;
  [[NSNotificationCenter defaultCenter] addObserver:self
                                           selector:@selector(orientationChanged:)
                                               name:UIDeviceOrientationDidChangeNotification
                                             object:nil];
}

- (void)orientationChanged:(NSNotification *)notification
{
  [self willRotateToInterfaceOrientation:self.interfaceOrientation duration:0.];
}

- (BOOL)prefersStatusBarHidden { return self.apiBar.isVisible; }
- (UIStatusBarStyle)preferredStatusBarStyle
{
  return [self.controlsManager preferredStatusBarStyle];
}

- (void)updateStatusBarStyle { [self setNeedsStatusBarAppearanceUpdate]; }
- (id)initWithCoder:(NSCoder *)coder
{
  NSLog(@"MapViewController initWithCoder Started");
  self = [super initWithCoder:coder];
  if (self)
    [self initialize];

  NSLog(@"MapViewController initWithCoder Ended");
  return self;
}

- (void)initialize
{
  Framework & f = GetFramework();
  // TODO: Review and improve this code.
  f.SetMapSelectionListeners(
      [self](place_page::Info const & info) { [self onMapObjectSelected:info]; },
      [self](bool switchFullScreen) { [self onMapObjectDeselected:switchFullScreen]; });
  // TODO: Review and improve this code.
  f.SetMyPositionModeListener([self](location::EMyPositionMode mode, bool routingActive) {
    // TODO: Two global listeners are subscribed to the same event from the core.
    // Probably it's better to subscribe only wnen needed and usubscribe in other cases.
    // May be better solution would be multiobservers support in the C++ core.
    [self processMyPositionStateModeEvent:mode];
  });

  self.userTouchesAction = UserTouchesActionNone;
  GetFramework().LoadBookmarks();
  [MWMFrameworkListener addObserver:self];
}

#pragma mark - Open controllers

- (void)openMigration { [self performSegueWithIdentifier:kMigrationSegue sender:self]; }
- (void)openBookmarks
{
  BOOL const oneCategory = (GetFramework().GetBmCategoriesCount() == 1);
  MWMTableViewController * vc =
      oneCategory ? [[BookmarksVC alloc] initWithCategory:0] : [[BookmarksRootVC alloc] init];
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

  auto const & featureID = self.controlsManager.featureHolder.featureId;

  [Statistics logEvent:kStatEditorEditStart
        withParameters:@{
          kStatEditorIsAuthenticated : @(AuthorizationHaveCredentials()),
          kStatIsOnline : Platform::IsConnected() ? kStatYes : kStatNo,
          kStatEditorMWMName : @(featureID.GetMwmName().c_str()),
          kStatEditorMWMVersion : @(featureID.GetMwmVersion())
        }];
  [self performSegueWithIdentifier:kEditorSegue sender:self.controlsManager.featureHolder];
}

- (void)openBookmarkEditorWithData:(MWMPlacePageData *)data
{
  [self performSegueWithIdentifier:kPP2BookmarkEditingSegue sender:data];
}

- (void)processMyPositionStateModeEvent:(location::EMyPositionMode)mode
{
  [MWMLocationManager setMyPositionMode:mode];
  [self.controlsManager processMyPositionStateModeEvent:mode];
  self.disableStandbyOnLocationStateMode = NO;
  switch (mode)
  {
  case location::NotFollowNoPosition:
    if (gIsFirstMyPositionMode && ![Alohalytics isFirstSession])
    {
      GetFramework().SwitchMyPositionNextMode();
    }
    else
    {
      BOOL const isMapVisible = (self.navigationController.visibleViewController == self);
      if (isMapVisible && !location_helpers::isLocationProhibited())
      {
        [self.alertController presentLocationNotFoundAlertWithOkBlock:^{
          GetFramework().SwitchMyPositionNextMode();
        }];
      }
    }
    break;
  case location::PendingPosition:
  case location::NotFollow: break;
  case location::Follow:
  case location::FollowAndRotate: self.disableStandbyOnLocationStateMode = YES; break;
  }
  gIsFirstMyPositionMode = NO;
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
  GetFramework().GetStorage().GetNodeStatuses(countryId, nodeStatuses);
  if (nodeStatuses.m_status != NodeStatus::Error)
    return;
  switch (nodeStatuses.m_error)
  {
  case NodeErrorCode::NoError: break;
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
  [[MWMRouter router] stop];
  if ([action isEqualToString:@"me.maps.3daction.bookmarks"])
    [self openBookmarks];
  else if ([action isEqualToString:@"me.maps.3daction.search"])
    self.controlsManager.searchHidden = NO;
  else if ([action isEqualToString:@"me.maps.3daction.route"])
    [self.controlsManager onRoutePrepare];
}

#pragma mark - API bar

- (MWMAPIBar *)apiBar
{
  if (!_apiBar)
    _apiBar = [[MWMAPIBar alloc] initWithController:self];
  return _apiBar;
}

- (void)showAPIBar { self.apiBar.isVisible = YES; }
#pragma mark - ShowDialog callback

- (void)presentDisabledLocationAlert { [self.alertController presentDisabledLocationAlert]; }
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
    [dvc setFeatureToEdit:static_cast<id<MWMFeatureHolder>>(sender).featureId];
  }
  else if ([segue.identifier isEqualToString:kPP2BookmarkEditingSegue])
  {
    MWMEditBookmarkController * dvc = segue.destinationViewController;
    dvc.data = static_cast<MWMPlacePageData *>(sender);
  }
  else if ([segue.identifier isEqualToString:kDownloaderSegue])
  {
    MWMMapDownloaderViewController * dvc = segue.destinationViewController;
    NSNumber * mode = sender;
    [dvc setParentCountryId:@(GetFramework().GetStorage().GetRootId().c_str())
                       mode:static_cast<mwm::DownloaderMode>(mode.integerValue)];
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
  else if ([segue.identifier isEqualToString:@"PP2BookmarkEditingIPAD"])
  {
    UINavigationController * nav = segue.destinationViewController;
    MWMEditBookmarkController * dvc = nav.viewControllers.firstObject;
    dvc.manager = sender;
  }
  else if ([segue.identifier isEqualToString:@"PP2BookmarkEditing"])
  {
    MWMEditBookmarkController * dvc = segue.destinationViewController;
    dvc.manager = sender;
  }
}

#pragma mark - Properties

- (MWMMapViewControlsManager *)controlsManager
{
  if (!self.isViewLoaded)
    return nil;
  if (!_controlsManager)
    _controlsManager = [[MWMMapViewControlsManager alloc] initWithParentController:self];
  return _controlsManager;
}

- (BOOL)hasNavigationBar { return NO; }
- (MWMMapDownloadDialog *)downloadDialog
{
  if (!_downloadDialog)
    _downloadDialog = [MWMMapDownloadDialog dialogForController:self];
  return _downloadDialog;
}

@end
