#import "MapViewController.h"
#import "BookmarksVC.h"
#import "EAGLView.h"
#import "MWMAPIBar.h"
#import "MWMAuthorizationCommon.h"
#import "MWMAuthorizationWebViewLoginViewController.h"
#import "MWMAutoupdateController.h"
#import "MWMBookmarksManager.h"
#import "MWMCommon.h"
#import "MWMEditBookmarkController.h"
#import "MWMEditorViewController.h"
#import "MWMFacilitiesController.h"
#import "MWMFrameworkListener.h"
#import "MWMLocationHelpers.h"
#import "MWMMapDownloadDialog.h"
#import "MWMMapDownloaderViewController.h"
#import "MWMMapViewControlsManager.h"
#import "MWMPlacePageProtocol.h"
#import "MapsAppDelegate.h"
#import "SwiftBridge.h"

#include "Framework.h"

#include "drape_frontend/user_event_stream.hpp"

#import <Crashlytics/Crashlytics.h>

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
NSString * const kHotelFacilitiesSegue = @"Map2FacilitiesSegue";

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
                                MWMWelcomePageControllerProtocol, MWMKeyboardObserver,
                                RemoveAdsViewControllerDelegate>

@property(nonatomic, readwrite) MWMMapViewControlsManager * controlsManager;

@property(nonatomic) BOOL disableStandbyOnLocationStateMode;

@property(nonatomic) UserTouchesAction userTouchesAction;

@property(nonatomic) MWMMapDownloadDialog * downloadDialog;

@property(nonatomic) BOOL skipForceTouch;

@property(weak, nonatomic) IBOutlet NSLayoutConstraint * visibleAreaBottom;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * visibleAreaKeyboard;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * placePageAreaKeyboard;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * sideButtonsAreaBottom;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * sideButtonsAreaKeyboard;

@end

@implementation MapViewController

+ (MapViewController *)sharedController { return [MapsAppDelegate theApp].mapViewController; }

- (BOOL)isLaunchByDeepLink { return [(EAGLView *)self.view isLaunchByDeepLink]; }

- (void)setLaunchByDeepLink:(BOOL)launchByDeepLink { [(EAGLView *)self.view setLaunchByDeepLink:launchByDeepLink]; }

#pragma mark - Map Navigation

- (void)dismissPlacePage { [self.controlsManager dismissPlacePage]; }
- (void)onMapObjectDeselected:(bool)switchFullScreenMode
{
  [self dismissPlacePage];

  if (!switchFullScreenMode)
    return;

  if ([MapsAppDelegate theApp].hasApiURL)
    return;

  BOOL const isSearchHidden = ([MWMSearchManager manager].state == MWMSearchManagerStateHidden);
  BOOL const isNavigationDashboardHidden =
      ([MWMNavigationDashboardManager manager].state == MWMNavigationDashboardStateHidden);
  if (isSearchHidden && isNavigationDashboardHidden)
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

- (void)viewWillTransitionToSize:(CGSize)size
       withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator
{
  [super viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
  [self.alertController viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
  [self.controlsManager viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
  [self.welcomePageController viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
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

  if ([MWMNavigationDashboardManager manager].state == MWMNavigationDashboardStateHidden)
    self.controlsManager.menuState = self.controlsManager.menuRestoreState;

  [self updateStatusBarStyle];
  GetFramework().InvalidateRendering();
  [self.welcomePageController show];
  [self showViralAlertIfNeeded];
  [self checkAuthorization];
}

- (void)viewDidLoad
{
  [super viewDidLoad];
  
  self.view.clipsToBounds = YES;
  [self processMyPositionStateModeEvent:MWMMyPositionModePendingPosition];
  [MWMKeyboard addObserver:self];
  self.welcomePageController = [MWMWelcomePageController controllerWithParent:self];
  if ([MWMNavigationDashboardManager manager].state == MWMNavigationDashboardStateHidden)
    self.controlsManager.menuState = self.controlsManager.menuRestoreState;

  [NSNotificationCenter.defaultCenter addObserver:self
                                         selector:@selector(didBecomeActive)
                                             name:UIApplicationDidBecomeActiveNotification
                                           object:nil];
}

- (void)didBecomeActive
{
  if (!self.welcomePageController)
    [self.controlsManager showTutorialIfNeeded];
}

- (void)viewDidLayoutSubviews
{
  [super viewDidLayoutSubviews];
  EAGLView * renderingView = (EAGLView *)self.view;
  if (!renderingView.drapeEngineCreated)
    [renderingView createDrapeEngine];
}

- (void)mwm_refreshUI
{
  [MapsAppDelegate customizeAppearance];
  [self.navigationController.navigationBar mwm_refreshUI];
  [self.controlsManager mwm_refreshUI];
  [self.downloadDialog mwm_refreshUI];
}

- (void)closePageController:(MWMWelcomePageController *)pageController
{
  if ([pageController isEqual:self.welcomePageController])
    self.welcomePageController = nil;

  auto const todo = GetFramework().ToDoAfterUpdate();
  
  switch (todo)
  {
  case Framework::DoAfterUpdate::Nothing:
    break;
    
  case Framework::DoAfterUpdate::Migrate:
    [self openMigration];
    break;

  case Framework::DoAfterUpdate::AutoupdateMaps:
  case Framework::DoAfterUpdate::AskForUpdateMaps:
    [self presentViewController:[MWMAutoupdateController instanceWithPurpose:todo] animated:YES completion:nil];
    break;
  }
}

- (void)showViralAlertIfNeeded
{
  NSUserDefaults * ud = NSUserDefaults.standardUserDefaults;

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
}

- (BOOL)prefersStatusBarHidden { return NO; }
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
    [self processMyPositionStateModeEvent:location_helpers::mwmMyPositionMode(mode)];
  });

  self.userTouchesAction = UserTouchesActionNone;
  [[MWMBookmarksManager sharedManager] loadBookmarks];
  [MWMFrameworkListener addObserver:self];
}

#pragma mark - Open controllers

- (void)openMigration { [self performSegueWithIdentifier:kMigrationSegue sender:self]; }
- (void)openBookmarks
{
  [self.navigationController pushViewController:[[MWMBookmarksTabViewController alloc] init]
                                       animated:YES];
}

- (void)openMapsDownloader:(MWMMapDownloaderMode)mode
{
  [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"downloader"];
  [self performSegueWithIdentifier:kDownloaderSegue sender:@(mode)];
}

- (void)openEditor
{
  using namespace osm_auth_ios;

  auto const & featureID = [self.controlsManager.featureHolder featureId];

  [Statistics logEvent:kStatEditorEditStart
        withParameters:@{
          kStatIsAuthenticated: @(AuthorizationHaveCredentials()),
          kStatIsOnline: Platform::IsConnected() ? kStatYes : kStatNo,
          kStatEditorMWMName: @(featureID.GetMwmName().c_str()),
          kStatEditorMWMVersion: @(featureID.GetMwmVersion())
        }];
  [self performSegueWithIdentifier:kEditorSegue sender:self.controlsManager.featureHolder];
}

- (void)openHotelFacilities
{
  [self performSegueWithIdentifier:kHotelFacilitiesSegue sender:self.controlsManager.bookingInfoHolder];
}

- (void)openBookmarkEditorWithData:(MWMPlacePageData *)data
{
  [self performSegueWithIdentifier:kPP2BookmarkEditingSegue sender:data];
}

- (void)openFullPlaceDescriptionWithHtml:(NSString *)htmlString
{
  [Statistics logEvent:kStatPlacePageDescriptionMore];
  WebViewController * descriptionViewController =
  [[PlacePageDescriptionViewController alloc] initWithHtml:htmlString baseUrl:nil title:L(@"place_description_title")];
  descriptionViewController.openInSafari = YES;
  [self.navigationController pushViewController:descriptionViewController animated:YES];
}

- (void)showUGCAuth
{
  [Statistics logEvent:kStatUGCReviewAuthShown];
  if (IPAD)
  {
    auto controller = [[MWMAuthorizationViewController alloc]
                       initWithPopoverSourceView:self.controlsManager.anchorView
                       sourceComponent:MWMAuthorizationSourceUGC
                       permittedArrowDirections:UIPopoverArrowDirectionDown
                       successHandler:nil
                       errorHandler:nil
                       completionHandler:nil];

    [self presentViewController:controller animated:YES completion:nil];
    return;
  }

  auto controller = [[MWMAuthorizationViewController alloc]
                     initWithBarButtonItem:nil
                           sourceComponent:MWMAuthorizationSourceUGC
                            successHandler:nil
                              errorHandler:nil
                         completionHandler:nil];

  [self presentViewController:controller animated:YES completion:nil];
}

- (void)showBookmarksLoadedAlert:(UInt64)categoryId
{
  for (UIViewController * vc in self.navigationController.viewControllers)
  {
    if ([vc isMemberOfClass:MWMBookmarksTabViewController.class])
    {
      auto alert = [[BookmarksLoadedViewController alloc] init];
      alert.onViewBlock = ^{
        [self dismissViewControllerAnimated:YES completion:nil];
        [self.navigationController popToRootViewControllerAnimated:YES];
        GetFramework().ShowBookmarkCategory(categoryId);
      };
      alert.onCancelBlock = ^{
        [self.navigationController dismissViewControllerAnimated:YES completion:nil];
      };
      [self.navigationController presentViewController:alert animated:YES completion:nil];
      return;
    }
  }
  if (![MWMRouter isOnRoute])
      [[MWMToast toastWithText:L(@"guide_downloaded_title")] show];
}

- (void)openCatalogAnimated:(BOOL)animated
{
  [self openCatalogDeeplink:nil animated:animated];
}

- (void)openCatalogDeeplink:(NSURL *)deeplinkUrl animated:(BOOL)animated
{
  [self.navigationController popToRootViewControllerAnimated:NO];
  auto bookmarks = [[MWMBookmarksTabViewController alloc] init];
  bookmarks.activeTab = ActiveTabCatalog;
  MWMCatalogWebViewController *catalog;
  if (deeplinkUrl)
    catalog = [[MWMCatalogWebViewController alloc] init:deeplinkUrl];
  else
    catalog = [[MWMCatalogWebViewController alloc] init];

  NSMutableArray<UIViewController *> * controllers = [self.navigationController.viewControllers mutableCopy];
  [controllers addObjectsFromArray:@[bookmarks, catalog]];
  [self.navigationController setViewControllers:controllers animated:animated];
}

- (void)searchText:(NSString *)text
{
  [self.controlsManager searchText:text forInputLocale:[[AppInfo sharedInfo] languageId]];
}

- (void)showRemoveAds
{
  auto removeAds = [[RemoveAdsViewController alloc] init];
  removeAds.delegate = self;
  [self.navigationController presentViewController:removeAds animated:YES completion:nil];
}

- (void)processMyPositionStateModeEvent:(MWMMyPositionMode)mode
{
  [MWMLocationManager setMyPositionMode:mode];
  [[MWMSideButtons buttons] processMyPositionStateModeEvent:mode];
  self.disableStandbyOnLocationStateMode = NO;
  switch (mode)
  {
  case location::NotFollowNoPosition:
  {
    BOOL const hasLocation = [MWMLocationManager lastLocation] != nil;
    if (hasLocation)
    {
      GetFramework().SwitchMyPositionNextMode();
      break;
    }
    if ([Alohalytics isFirstSession])
      break;
    if (gIsFirstMyPositionMode)
    {
      GetFramework().SwitchMyPositionNextMode();
      break;
    }
    BOOL const isMapVisible = (self.navigationController.visibleViewController == self);
    if (isMapVisible && ![MWMLocationManager isLocationProhibited])
    {
      [self.alertController presentLocationNotFoundAlertWithOkBlock:^{
        GetFramework().SwitchMyPositionNextMode();
      }];
    }
    break;
  }
  case location::PendingPosition:
  case location::NotFollow: break;
  case location::Follow:
  case location::FollowAndRotate: self.disableStandbyOnLocationStateMode = YES; break;
  }
  gIsFirstMyPositionMode = NO;
}

#pragma mark - MWMRemoveAdsViewControllerDelegate

- (void)didCompleteSubscribtion:(RemoveAdsViewController *)viewController
{
  [self.navigationController dismissViewControllerAnimated:YES completion:nil];
  GetFramework().DeactivateMapSelection(true);
  [self.controlsManager hideSearch];
}

- (void)didCancelSubscribtion:(RemoveAdsViewController *)viewController
{
  [self.navigationController dismissViewControllerAnimated:YES completion:nil];
}

#pragma mark - MWMFrameworkDrapeObserver

- (void)processViewportCountryEvent:(TCountryId const &)countryId
{
  [self.downloadDialog processViewportCountryEvent:countryId];
}

#pragma mark - MWMFrameworkStorageObserver

- (void)processCountryEvent:(TCountryId const &)countryId
{
  if (countryId.empty())
  {
#ifdef OMIM_PRODUCTION
    auto err = [[NSError alloc] initWithDomain:kMapsmeErrorDomain code:1
                      userInfo:@{@"Description" : @"attempt to get info from empty countryId"}];
    [[Crashlytics sharedInstance] recordError:err];
#endif
    return;
  }

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
  if (self.isViewLoaded)
  {
    auto searchState = MWMSearchManagerStateHidden;
    [MWMRouter stopRouting];
    if ([action isEqualToString:@"me.maps.3daction.bookmarks"])
      [self openBookmarks];
    else if ([action isEqualToString:@"me.maps.3daction.search"])
      searchState = MWMSearchManagerStateDefault;
    else if ([action isEqualToString:@"me.maps.3daction.route"])
      [self.controlsManager onRoutePrepare];
    [MWMSearchManager manager].state = searchState;
  }
  else
  {
    dispatch_async(dispatch_get_main_queue(), ^{
      [self performAction:action];
    });
  }
}

#pragma mark - API bar

- (MWMAPIBar *)apiBar
{
  if (!_apiBar)
    _apiBar = [[MWMAPIBar alloc] initWithController:self];
  return _apiBar;
}

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
                       mode:static_cast<MWMMapDownloaderMode>(mode.integerValue)];
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
  else if ([segue.identifier isEqualToString:kHotelFacilitiesSegue])
  {
    MWMFacilitiesController * dvc = segue.destinationViewController;
    auto bookingInfo = id<MWMBookingInfoHolder>(sender);
    dvc.facilities = bookingInfo.hotelFacilities;
    dvc.hotelName = bookingInfo.hotelName;
  }
}

#pragma mark - MWMKeyboard

- (void)onKeyboardWillAnimate { [self.view setNeedsLayout]; }
- (void)onKeyboardAnimation
{
  auto const kbHeight = [MWMKeyboard keyboardHeight];
  self.sideButtonsAreaKeyboard.constant = kbHeight;
  if (IPAD)
  {
    self.visibleAreaKeyboard.constant = kbHeight;
    self.placePageAreaKeyboard.constant = kbHeight;
  }
  [self.view layoutIfNeeded];
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

- (void)setPlacePageTopBound:(CGFloat)bound;
{
  self.visibleAreaBottom.constant = bound;
  self.sideButtonsAreaBottom.constant = bound;
}

@end
