#import "MapViewController.h"
#import <CoreApi/MWMBookmarksManager.h>
#import "EAGLView.h"
#import "MWMAuthorizationCommon.h"
#import "MWMAutoupdateController.h"
#import "MWMEditorViewController.h"
#import "MWMFrameworkListener.h"
#import "MWMFrameworkObservers.h"
#import "MWMLocationHelpers.h"
#import "MWMLocationModeListener.h"
#import "MWMMapDownloadDialog.h"
#import "MWMMapViewControlsManager.h"
#import "MWMNetworkPolicy+UI.h"
#import "MWMPlacePageProtocol.h"
#import "MapsAppDelegate.h"
#import "SwiftBridge.h"

#import <CoreApi/Framework.h>
#import <CoreApi/MWMFrameworkHelper.h>
#import <CoreApi/PlacePageData.h>

#include "drape_frontend/user_event_stream.hpp"

#include "geometry/mercator.hpp"

// If you have a "missing header error" here, then please run configure.sh script in the root repo
// folder.
#import "../../../private.h"

extern NSString *const kMap2FBLoginSegue = @"Map2FBLogin";
extern NSString *const kMap2GoogleLoginSegue = @"Map2GoogleLogin";

static CGFloat kPlacePageCompactWidth = 350;
static CGFloat kPlacePageLeadingOffset = IPAD ? 20 : 0;

typedef NS_ENUM(NSUInteger, UserTouchesAction) { UserTouchesActionNone, UserTouchesActionDrag, UserTouchesActionScale };

namespace {
NSString *const kDownloaderSegue = @"Map2MapDownloaderSegue";
NSString *const kEditorSegue = @"Map2EditorSegue";
NSString *const kUDViralAlertWasShown = @"ViralAlertWasShown";
NSString *const kPP2BookmarkEditingSegue = @"PP2BookmarkEditing";
NSString *const kSettingsSegue = @"Map2Settings";
}  // namespace

@interface NSValueWrapper : NSObject

- (NSValue *)getInnerValue;

@end

@implementation NSValueWrapper {
  NSValue *m_innerValue;
}

- (NSValue *)getInnerValue {
  return m_innerValue;
}
- (id)initWithValue:(NSValue *)value {
  self = [super init];
  if (self)
    m_innerValue = value;
  return self;
}

- (BOOL)isEqual:(id)anObject {
  return [anObject isMemberOfClass:[NSValueWrapper class]];
}

@end

@interface MapViewController () <MWMFrameworkDrapeObserver,
                                 MWMKeyboardObserver,
                                 MWMBookmarksObserver,
                                 UIGestureRecognizerDelegate>

@property(nonatomic, readwrite) MWMMapViewControlsManager *controlsManager;
@property(nonatomic, readwrite) SearchOnMapManager *searchManager;

@property(nonatomic) BOOL disableStandbyOnLocationStateMode;

@property(nonatomic) UserTouchesAction userTouchesAction;
@property(nonatomic) CGPoint pointerLocation API_AVAILABLE(ios(14.0));
@property(nonatomic) CGFloat currentScale;
@property(nonatomic) CGFloat currentRotation;

@property(nonatomic, readwrite) MWMMapDownloadDialog *downloadDialog;

@property(nonatomic) BOOL skipForceTouch;

@property(strong, nonatomic) IBOutlet NSLayoutConstraint *visibleAreaBottom;
@property(strong, nonatomic) IBOutlet NSLayoutConstraint *visibleAreaKeyboard;
@property(strong, nonatomic) IBOutlet NSLayoutConstraint *placePageAreaKeyboard;
@property(strong, nonatomic) IBOutlet NSLayoutConstraint *sideButtonsAreaBottom;
@property(strong, nonatomic) IBOutlet NSLayoutConstraint *sideButtonsAreaKeyboard;
@property(strong, nonatomic) IBOutlet UIView *carplayPlaceholderView;
@property(strong, nonatomic) BookmarksCoordinator * bookmarksCoordinator;

@property(strong, nonatomic) NSHashTable<id<MWMLocationModeListener>> *listeners;

@property(nonatomic) BOOL needDeferFocusNotification;
@property(nonatomic) BOOL deferredFocusValue;
@property(nonatomic) PlacePageViewController *placePageVC;
@property(nonatomic) UIView *placePageContainer;

@property(nonatomic) NSLayoutConstraint *placePageWidthConstraint;
@property(nonatomic) NSLayoutConstraint *placePageLeadingConstraint;
@property(nonatomic) NSLayoutConstraint *placePageTrailingConstraint;

@end

@implementation MapViewController
@synthesize mapView, controlsView;

+ (MapViewController *)sharedController {
  return [MapsAppDelegate theApp].mapViewController;
}

#pragma mark - Map Navigation

- (void)showOrUpdatePlacePage:(PlacePageData *)data {
  if (self.searchManager.isSearching)
    [self.searchManager setPlaceOnMapSelected:YES];

  self.controlsManager.trafficButtonHidden = YES;
  if (self.placePageVC != nil) {
    [PlacePageBuilder update:(PlacePageViewController *)self.placePageVC with:data];
    return;
  }
  [self showPlacePageFor:data];
}

- (void)showPlacePageFor:(PlacePageData *)data {
  self.placePageContainer.hidden = NO;
  self.placePageVC = [PlacePageBuilder buildFor:data];
  [self.placePageContainer addSubview:self.placePageVC.view];
  [self addChildViewController:self.placePageVC];
  [self.placePageVC didMoveToParentViewController:self];
  self.placePageVC.view.translatesAutoresizingMaskIntoConstraints = NO;
  [NSLayoutConstraint activateConstraints:@[
    [self.placePageVC.view.topAnchor constraintEqualToAnchor:self.placePageContainer.topAnchor],
    [self.placePageVC.view.leadingAnchor constraintEqualToAnchor:self.placePageContainer.leadingAnchor],
    [self.placePageVC.view.bottomAnchor constraintEqualToAnchor:self.placePageContainer.bottomAnchor],
    [self.placePageVC.view.trailingAnchor constraintEqualToAnchor:self.placePageContainer.trailingAnchor]
  ]];
  [self updatePlacePageContainerConstraints];
}

- (void)setupPlacePageContainer {
  self.placePageContainer = [[TouchTransparentView alloc] initWithFrame:self.view.bounds];
  [self.view addSubview:self.placePageContainer];
  [self.view bringSubviewToFront:self.placePageContainer];

  self.placePageContainer.translatesAutoresizingMaskIntoConstraints = NO;
  self.placePageLeadingConstraint = [self.placePageContainer.leadingAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.leadingAnchor constant:kPlacePageLeadingOffset];
  self.placePageLeadingConstraint.active = YES;

  self.placePageWidthConstraint = [self.placePageContainer.widthAnchor constraintEqualToConstant:0];
  self.placePageTrailingConstraint = [self.placePageContainer.trailingAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.trailingAnchor];

  [self.placePageContainer.topAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.topAnchor].active = YES;
  if (IPAD) {
    self.placePageLeadingConstraint.priority = UILayoutPriorityDefaultLow;
    [self.placePageContainer.bottomAnchor constraintLessThanOrEqualToAnchor:self.view.bottomAnchor].active = YES;
  }
  else {
    [self.placePageContainer.bottomAnchor constraintEqualToAnchor:self.view.bottomAnchor].active = YES;
  }

  [self updatePlacePageContainerConstraints];
}

- (void)updatePlacePageContainerConstraints {
  const BOOL isLimitedWidth = IPAD || self.traitCollection.verticalSizeClass == UIUserInterfaceSizeClassCompact;
  [self.placePageWidthConstraint setConstant:kPlacePageCompactWidth];

  if (IPAD && self.searchViewContainer != nil) {
    NSLayoutConstraint * leadingToSearchConstraint = [self.placePageContainer.leadingAnchor constraintEqualToAnchor:self.searchViewContainer.trailingAnchor constant:kPlacePageLeadingOffset];
    leadingToSearchConstraint.priority = UILayoutPriorityDefaultHigh;
    leadingToSearchConstraint.active = isLimitedWidth;
  }

  [self.placePageWidthConstraint setActive:isLimitedWidth];
  [self.placePageTrailingConstraint setActive:!isLimitedWidth];
  [self.view layoutIfNeeded];
}

- (void)dismissPlacePage {
  GetFramework().DeactivateMapSelection();
}

- (void)hideRegularPlacePage {
  [self.placePageVC closeAnimatedWithCompletion:^{
    [self.placePageVC.view removeFromSuperview];
    [self.placePageVC willMoveToParentViewController:nil];
    [self.placePageVC removeFromParentViewController];
    self.placePageVC = nil;
    self.placePageContainer.hidden = YES;
    [self setPlacePageTopBound:0 duration:0];
  }];
}

- (void)hidePlacePage {
  if (self.placePageVC != nil) {
    [self hideRegularPlacePage];
  }
  self.controlsManager.trafficButtonHidden = NO;
}

- (void)onMapObjectDeselected {
  [self hidePlacePage];

  BOOL const isSearching = self.searchManager.isSearching;
  BOOL const isNavigationDashboardHidden = [MWMNavigationDashboardManager sharedManager].state == MWMNavigationDashboardStateHidden;
  if (isSearching)
    [self.searchManager setPlaceOnMapSelected:!isNavigationDashboardHidden];
  // Always show the controls during the navigation or planning mode.
  if (!isNavigationDashboardHidden)
    self.controlsManager.hidden = NO;
}

- (void)onSwitchFullScreen {
  BOOL const isNavigationDashboardHidden = MWMNavigationDashboardManager.sharedManager.state == MWMNavigationDashboardStateHidden;
  if (!self.searchManager.isSearching && isNavigationDashboardHidden) {
    if (!self.controlsManager.hidden)
      [self dismissPlacePage];
    self.controlsManager.hidden = !self.controlsManager.hidden;
  }
}

- (void)onMapObjectSelected {
  if (!PlacePageData.hasData) {
    return;
  }
  PlacePageData * data = [[PlacePageData alloc] initWithLocalizationProvider:[[OpeinigHoursLocalization alloc] init]];
  [self showOrUpdatePlacePage:data];
}

- (void)onMapObjectUpdated {
  //  [self.controlsManager updatePlacePage];
}

- (void)checkMaskedPointer:(UITouch *)touch withEvent:(df::TouchEvent &)e {
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

- (void)sendTouchType:(df::TouchEvent::ETouchType)type withTouches:(NSSet *)touches andEvent:(UIEvent *)event {
  if ([MWMCarPlayService shared].isCarplayActivated) {
    return;
  }

  if (self.searchManager.isSearching && type == df::TouchEvent::TOUCH_MOVE)
    [self.searchManager setMapIsDragging];

  NSArray *allTouches = [[event allTouches] allObjects];
  if ([allTouches count] < 1)
    return;

  UIView *v = self.mapView;
  CGFloat const scaleFactor = v.contentScaleFactor;

  df::TouchEvent e;
  UITouch *touch = [allTouches objectAtIndex:0];
  CGPoint const pt = [touch locationInView:v];

  e.SetTouchType(type);

  df::Touch t0;
  t0.m_location = m2::PointD(pt.x * scaleFactor, pt.y * scaleFactor);
  t0.m_id = reinterpret_cast<int64_t>(touch);
  if ([self hasForceTouch])
    t0.m_force = touch.force / touch.maximumPossibleForce;
  e.SetFirstTouch(t0);

  if (allTouches.count > 1) {
    UITouch *touch = [allTouches objectAtIndex:1];
    CGPoint const pt = [touch locationInView:v];

    df::Touch t1;
    t1.m_location = m2::PointD(pt.x * scaleFactor, pt.y * scaleFactor);
    t1.m_id = reinterpret_cast<int64_t>(touch);
    if ([self hasForceTouch])
      t1.m_force = touch.force / touch.maximumPossibleForce;
    e.SetSecondTouch(t1);
  }

  NSArray *toggledTouches = [touches allObjects];
  if (toggledTouches.count > 0)
    [self checkMaskedPointer:[toggledTouches objectAtIndex:0] withEvent:e];

  if (toggledTouches.count > 1)
    [self checkMaskedPointer:[toggledTouches objectAtIndex:1] withEvent:e];

  Framework &f = GetFramework();
  f.TouchEvent(e);
}

- (BOOL)hasForceTouch {
  return self.view.traitCollection.forceTouchCapability == UIForceTouchCapabilityAvailable;
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event {
  [self sendTouchType:df::TouchEvent::TOUCH_DOWN withTouches:touches andEvent:event];
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event {
  [self sendTouchType:df::TouchEvent::TOUCH_MOVE withTouches:nil andEvent:event];
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event {
  [self sendTouchType:df::TouchEvent::TOUCH_UP withTouches:touches andEvent:event];
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event {
  [self sendTouchType:df::TouchEvent::TOUCH_CANCEL withTouches:touches andEvent:event];
}

#pragma mark - ViewController lifecycle

- (void)viewWillTransitionToSize:(CGSize)size
       withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator {
  [super viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
  [self.alertController viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
  [self.controlsManager viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
}

- (void)traitCollectionDidChange:(UITraitCollection *)previousTraitCollection {
  [super traitCollectionDidChange:previousTraitCollection];
  if (self.traitCollection.verticalSizeClass != previousTraitCollection.verticalSizeClass)
    [self updatePlacePageContainerConstraints];
}

- (void)didReceiveMemoryWarning {
  GetFramework().MemoryWarning();
  [super didReceiveMemoryWarning];
}

- (void)onTerminate {
  [self.mapView deallocateNative];
}

- (void)onGetFocus:(BOOL)isOnFocus {
  self.needDeferFocusNotification = (self.mapView == nil);
  self.deferredFocusValue = isOnFocus;
  [self.mapView setPresentAvailable:isOnFocus];
}

- (void)viewWillAppear:(BOOL)animated {
  [super viewWillAppear:animated];

  if ([MWMNavigationDashboardManager sharedManager].state == MWMNavigationDashboardStateHidden)
    self.controlsManager.menuState = self.controlsManager.menuRestoreState;

  [self updateStatusBarStyle];
  GetFramework().SetRenderingEnabled();
  GetFramework().InvalidateRendering();
  [self showViralAlertIfNeeded];
  [self checkAuthorization];
  [MWMRouter updateRoute];
}

- (void)viewDidLoad {
  [super viewDidLoad];
  [self setupPlacePageContainer];

  if (@available(iOS 14.0, *))
    [self setupTrackPadGestureRecognizers];

  self.title = L(@"map");

  // On iOS 10 (it was reproduced, it may be also on others), mapView can be uninitialized
  // when onGetFocus is called, it can lead to missing of onGetFocus call and a deadlock on the start.
  // As soon as mapView must exist before onGetFocus, so we have to defer onGetFocus call.
  if (self.needDeferFocusNotification)
    [self onGetFocus:self.deferredFocusValue];

  [MWMRouter restoreRouteIfNeeded];

  self.view.clipsToBounds = YES;
  [MWMKeyboard addObserver:self];

  if ([FirstSession isFirstSession])
  {
    [MWMLocationManager start];
    dispatch_async(dispatch_get_main_queue(), ^{
      [MWMFrameworkHelper processFirstLaunch:[MWMLocationManager isStarted]];
    });
  }
  else
  {
    [self processMyPositionStateModeEvent:[MWMLocationManager isLocationProhibited] ?
        MWMMyPositionModeNotFollowNoPosition : MWMMyPositionModePendingPosition];
  }

  if ([MWMNavigationDashboardManager sharedManager].state == MWMNavigationDashboardStateHidden)
    self.controlsManager.menuState = self.controlsManager.menuRestoreState;

  // Added in https://github.com/organicmaps/organicmaps/pull/7333
  // After all users migrate to OAuth2 we can remove next code
  [self migrateOAuthCredentials];

  /// @todo: Uncomment update dialog when will be ready to handle big traffic bursts.
  /*
  if (!DeepLinkHandler.shared.isLaunchedByDeeplink)
  {
    auto const todo = GetFramework().ToDoAfterUpdate();
    switch (todo) {
      case Framework::DoAfterUpdate::Migrate:
      case Framework::DoAfterUpdate::Nothing:
        break;
      case Framework::DoAfterUpdate::AutoupdateMaps:
      case Framework::DoAfterUpdate::AskForUpdateMaps:
        [self presentViewController:[MWMAutoupdateController instanceWithPurpose:todo] animated:YES completion:nil];
        break;
    }
  }
  */
}

- (void)viewDidAppear:(BOOL)animated {
  [super viewDidAppear:animated];
  // Cold start deep links should be handled when the map is initialized.
  // Otherwise PP container view is nil, or there is no animation/selection of the point.
  if (DeepLinkHandler.shared.isLaunchedByDeeplink)
    (void)[DeepLinkHandler.shared handleDeepLinkAndReset];
}

- (void)viewDidLayoutSubviews {
  [super viewDidLayoutSubviews];
  if (!self.mapView.drapeEngineCreated && !MapsAppDelegate.isTestsEnvironment)
    [self.mapView createDrapeEngine];
}

- (void)applyTheme {
  [super applyTheme];
  [MapsAppDelegate customizeAppearance];
}

- (void)setupTrackPadGestureRecognizers API_AVAILABLE(ios(14.0)) {
  if (!NSProcessInfo.processInfo.isiOSAppOnMac)
    return;
  // Mouse zoom
  UIPanGestureRecognizer * panRecognizer = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(handlePan:)];
  panRecognizer.allowedScrollTypesMask = UIScrollTypeMaskAll;
  panRecognizer.allowedTouchTypes = @[@(UITouchTypeIndirect)];
  [self.view addGestureRecognizer:panRecognizer];

  // Trackpad zoom
  UIPinchGestureRecognizer * pinchRecognizer = [[UIPinchGestureRecognizer alloc] initWithTarget:self action:@selector(handlePinch:)];
  pinchRecognizer.allowedTouchTypes = @[@(UITouchTypeIndirect)];
  pinchRecognizer.delegate = self;
  [self.view addGestureRecognizer:pinchRecognizer];

  // Trackpad rotation
  UIRotationGestureRecognizer * rotationRecognizer = [[UIRotationGestureRecognizer alloc] initWithTarget:self action:@selector(handleRotation:)];
  rotationRecognizer.allowedTouchTypes = @[@(UITouchTypeIndirect | UITouchTypeDirect)];
  rotationRecognizer.delegate = self;
  [self.view addGestureRecognizer:rotationRecognizer];

  // Pointer location
  UIHoverGestureRecognizer * hoverRecognizer = [[UIHoverGestureRecognizer alloc] initWithTarget:self action:@selector(handlePointerHover:)];
  hoverRecognizer.allowedTouchTypes = @[@(UITouchTypeIndirectPointer)];
  [self.view addGestureRecognizer:hoverRecognizer];
}

- (void)showViralAlertIfNeeded {
  NSUserDefaults *ud = NSUserDefaults.standardUserDefaults;

  using namespace osm_auth_ios;
  if (!AuthorizationIsNeedCheck() || [ud objectForKey:kUDViralAlertWasShown] || !AuthorizationHaveCredentials())
    return;

  if (osm::Editor::Instance().GetStats().m_edits.size() < 2)
    return;

  if (!Platform::IsConnected())
    return;

  [self.alertController presentEditorViralAlert];

  [ud setObject:[NSDate date] forKey:kUDViralAlertWasShown];
}

- (void)viewWillDisappear:(BOOL)animated {
  [super viewWillDisappear:animated];

  if ([MWMNavigationDashboardManager sharedManager].state == MWMNavigationDashboardStateHidden)
    self.controlsManager.menuRestoreState = self.controlsManager.menuState;
  GetFramework().SetRenderingDisabled(false);
}

- (BOOL)prefersStatusBarHidden {
  return NO;
}
- (UIStatusBarStyle)preferredStatusBarStyle {
  MWMMapViewControlsManager * manager = self.controlsManager;
  if (manager)
    return manager.preferredStatusBarStyle;
  return UIStatusBarStyleDefault;
}

- (void)updateStatusBarStyle {
  [self setNeedsStatusBarAppearanceUpdate];
}

- (void)migrateOAuthCredentials {
  if (osm_auth_ios::AuthorizationHaveOAuth1Credentials())
  {
    osm_auth_ios::AuthorizationClearOAuth1Credentials();
    [self.alertController presentOsmReauthAlert];
  }
}

- (id)initWithCoder:(NSCoder *)coder {
  NSLog(@"MapViewController initWithCoder Started");
  self = [super initWithCoder:coder];
  if (self)
    [self initialize];

  NSLog(@"MapViewController initWithCoder Ended");
  return self;
}

- (void)initialize {
  self.listeners = [NSHashTable<id<MWMLocationModeListener>> weakObjectsHashTable];
  Framework &f = GetFramework();
  // TODO: Review and improve this code.
  f.SetPlacePageListeners([self]() { [self onMapObjectSelected]; },
                          [self]() { [self onMapObjectDeselected]; },
                          [self]() { [self onMapObjectUpdated]; },
                          [self]() { [self onSwitchFullScreen]; });
  // TODO: Review and improve this code.
  f.SetMyPositionModeListener([self](location::EMyPositionMode mode, bool routingActive) {
    // TODO: Two global listeners are subscribed to the same event from the core.
    // Probably it's better to subscribe only wnen needed and usubscribe in other cases.
    // May be better solution would be multiobservers support in the C++ core.
    [self processMyPositionStateModeEvent:location_helpers::mwmMyPositionMode(mode)];
  });

  self.userTouchesAction = UserTouchesActionNone;
  [[MWMBookmarksManager sharedManager] addObserver:self];
  [[MWMBookmarksManager sharedManager] loadBookmarks];
  [MWMFrameworkListener addObserver:self];
}

- (void)dealloc {
  [[MWMBookmarksManager sharedManager] removeObserver:self];
  [MWMFrameworkListener removeObserver:self];
}

- (void)addListener:(id<MWMLocationModeListener>)listener {
  [self.listeners addObject:listener];
}

- (void)removeListener:(id<MWMLocationModeListener>)listener {
  [self.listeners removeObject:listener];
}

#pragma mark - Open controllers
- (void)openMenu {
  [self.controlsManager.tabBarController onMenuButtonPressed:self];
}

- (void)openSettings {
  [self performSegueWithIdentifier:kSettingsSegue sender:nil];
}

- (void)openMapsDownloader:(MWMMapDownloaderMode)mode {
  [self performSegueWithIdentifier:kDownloaderSegue sender:@(mode)];
}

- (void)openEditor {
  [self performSegueWithIdentifier:kEditorSegue sender:self.controlsManager.featureHolder];
}

- (void)openBookmarkEditor {
  [self performSegueWithIdentifier:kPP2BookmarkEditingSegue sender:nil];
}

- (void)openFullPlaceDescriptionWithHtml:(NSString *)htmlString {
  WebViewController *descriptionViewController =
    [[PlacePageDescriptionViewController alloc] initWithHtml:htmlString
                                                     baseUrl:nil
                                                       title:L(@"place_description_title")];
  descriptionViewController.openInSafari = YES;
  [self.navigationController pushViewController:descriptionViewController animated:YES];
}

- (void)searchText:(NSString *)text {
  [self.controlsManager searchText:text forInputLocale:[[AppInfo sharedInfo] languageId]];
}

- (void)openDrivingOptions {
  UIStoryboard *sb = [UIStoryboard instance:MWMStoryboardDrivingOptions];
  UIViewController *vc = [sb instantiateInitialViewController];
  [self.navigationController pushViewController:vc animated:YES];
}

- (void)processMyPositionStateModeEvent:(MWMMyPositionMode)mode {
  self.currentPositionMode = mode;
  [MWMLocationManager setMyPositionMode:mode];
  [[MWMSideButtons buttons] processMyPositionStateModeEvent:mode];
  NSArray<id<MWMLocationModeListener>> *objects = self.listeners.allObjects;
  for (id<MWMLocationModeListener> object in objects) {
    [object processMyPositionStateModeEvent:mode];
  }
  self.disableStandbyOnLocationStateMode = NO;
  switch (mode) {
    case MWMMyPositionModeNotFollowNoPosition:
      [MWMLocationManager stop];
      break;
    case MWMMyPositionModePendingPosition:
      [MWMLocationManager start];
      if (![MWMLocationManager isStarted])
        [self processMyPositionStateModeEvent:MWMMyPositionModeNotFollowNoPosition];
      break;
    case MWMMyPositionModeNotFollow:
      break;
    case MWMMyPositionModeFollow:
    case MWMMyPositionModeFollowAndRotate:
      self.disableStandbyOnLocationStateMode = YES;
      break;
  }
}

#pragma mark - MWMFrameworkDrapeObserver

- (void)processViewportCountryEvent:(CountryId const &)countryId {
  [self.downloadDialog processViewportCountryEvent:countryId];
}

#pragma mark - Authorization

- (void)checkAuthorization {
  using namespace osm_auth_ios;
  BOOL const isAfterEditing = AuthorizationIsNeedCheck() && !AuthorizationHaveCredentials();
  if (isAfterEditing) {
    AuthorizationSetNeedCheck(NO);
    if (!Platform::IsConnected())
      return;
    [self.alertController presentOsmAuthAlert];
  }
}

#pragma mark - 3d touch

- (void)performAction:(NSString *)action {
  [self.navigationController popToRootViewControllerAnimated:NO];
  if (self.isViewLoaded) {
    [MWMRouter stopRouting];
    if ([action isEqualToString:@"app.organicmaps.3daction.bookmarks"])
      [self.bookmarksCoordinator open];
    else if ([action isEqualToString:@"app.organicmaps.3daction.search"])
      [self.searchManager startSearchingWithIsRouting:NO];
    else if ([action isEqualToString:@"app.organicmaps.3daction.route"])
      [self.controlsManager onRoutePrepare];
  } else {
    dispatch_async(dispatch_get_main_queue(), ^{
      [self performAction:action];
    });
  }
}

#pragma mark - ShowDialog callback

- (void)presentDisabledLocationAlert {
  [self.alertController presentDisabledLocationAlert];
}
- (void)setDisableStandbyOnLocationStateMode:(BOOL)disableStandbyOnLocationStateMode {
  if (_disableStandbyOnLocationStateMode == disableStandbyOnLocationStateMode)
    return;
  _disableStandbyOnLocationStateMode = disableStandbyOnLocationStateMode;
  if (disableStandbyOnLocationStateMode)
    [[MapsAppDelegate theApp] disableStandby];
  else
    [[MapsAppDelegate theApp] enableStandby];
}

#pragma mark - Segue

- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender {
  if ([segue.identifier isEqualToString:kEditorSegue]) {
    MWMEditorViewController *dvc = segue.destinationViewController;
    [dvc setFeatureToEdit:static_cast<id<MWMFeatureHolder>>(sender).featureId];
  } else if ([segue.identifier isEqualToString:kDownloaderSegue]) {
    MWMDownloadMapsViewController *dvc = segue.destinationViewController;
    NSNumber *mode = sender;
    dvc.mode = (MWMMapDownloaderMode)mode.integerValue;
  }
}

#pragma mark - MWMKeyboard

- (void)onKeyboardWillAnimate {
  [self.view setNeedsLayout];
}
- (void)onKeyboardAnimation {
  auto const kbHeight = [MWMKeyboard keyboardHeight];
  self.sideButtonsAreaKeyboard.constant = kbHeight;
  if (IPAD) {
    self.visibleAreaKeyboard.constant = kbHeight;
    self.placePageAreaKeyboard.constant = kbHeight;
  }
}
#pragma mark - Properties

- (MWMMapViewControlsManager *)controlsManager {
  if (!self.isViewLoaded) {
    // TODO: Returns nil when called from MapViewController.preferredStatusBarStyle.
    return nil;
  }
  if (!_controlsManager)
    _controlsManager = [[MWMMapViewControlsManager alloc] initWithParentController:self];
  return _controlsManager;
}

- (SearchOnMapManager *)searchManager {
  if (!_searchManager)
    _searchManager = [[SearchOnMapManager alloc] initWithNavigationController:self.navigationController];
  return _searchManager;
}

- (UIView * _Nullable)searchViewContainer {
  return self.searchManager.viewController.view;
}

- (BOOL)hasNavigationBar {
  return NO;
}

- (MWMMapDownloadDialog *)downloadDialog {
  if (!_downloadDialog)
    _downloadDialog = [MWMMapDownloadDialog dialogForController:self];
  return _downloadDialog;
}

- (void)setPlacePageTopBound:(CGFloat)bound duration:(double)duration {
  self.visibleAreaBottom.constant = bound;
  self.sideButtonsAreaBottom.constant = bound;
  [UIView animateWithDuration:duration delay:0 options:UIViewAnimationOptionBeginFromCurrentState animations:^{
    [self.view layoutIfNeeded];
  } completion:nil];
}

+ (void)setViewport:(double)lat lon:(double)lon zoomLevel:(int)zoomLevel {
  Framework &f = GetFramework();

  f.StopLocationFollow();

  auto const center = mercator::FromLatLon(lat, lon);
  f.SetViewportCenter(center, zoomLevel, false);
}

- (BookmarksCoordinator *)bookmarksCoordinator {
  if (!_bookmarksCoordinator)
    _bookmarksCoordinator = [[BookmarksCoordinator alloc] initWithNavigationController:self.navigationController
                                                                       controlsManager:self.controlsManager
                                                                     navigationManager:[MWMNavigationDashboardManager sharedManager]];
  return _bookmarksCoordinator;
}

#pragma mark - CarPlay map append/remove

- (void)disableCarPlayRepresentation {
  self.carplayPlaceholderView.hidden = YES;
  self.mapView.frame = self.view.bounds;
  [self.view insertSubview:self.mapView atIndex:0];
  [[self.mapView.topAnchor constraintEqualToAnchor:self.view.topAnchor] setActive:YES];
  [[self.mapView.bottomAnchor constraintEqualToAnchor:self.view.bottomAnchor] setActive:YES];
  [[self.mapView.leadingAnchor constraintEqualToAnchor:self.view.leadingAnchor] setActive:YES];
  [[self.mapView.trailingAnchor constraintEqualToAnchor:self.view.trailingAnchor] setActive:YES];
  self.controlsView.hidden = NO;
  [MWMFrameworkHelper setVisibleViewport:self.view.bounds scaleFactor:self.view.contentScaleFactor];
}

- (void)enableCarPlayRepresentation {
  UIViewController *presentedController = self.presentedViewController;
  if (presentedController != nil) {
    [presentedController dismissViewControllerAnimated:NO completion:nil];
  }
  [[MapsAppDelegate theApp] showMap];
  [self loadViewIfNeeded];
  [self.mapView removeFromSuperview];
  if (!self.controlsView.isHidden) {
    self.controlsView.hidden = YES;
  }
  self.carplayPlaceholderView.hidden = NO;
}

#pragma mark - MWMBookmarksObserver
- (void)onBookmarksFileLoadSuccess {
  [[MWMAlertViewController activeAlertController] presentInfoAlert:L(@"load_kmz_title") text:L(@"load_kmz_successful")];
}

- (void)onBookmarksFileLoadError {
  [[MWMAlertViewController activeAlertController] presentInfoAlert:L(@"load_kmz_title") text:L(@"load_kmz_failed")];
}

- (BOOL)canBecomeFirstResponder {
  return YES;
}

- (NSArray *)keyCommands {
  NSArray *commands = @[
    [UIKeyCommand keyCommandWithInput:UIKeyInputDownArrow modifierFlags:0 action:@selector(zoomOut)], // Alternative, not shown when holding CMD
    [UIKeyCommand keyCommandWithInput:@"-" modifierFlags:UIKeyModifierCommand action:@selector(zoomOut) discoverabilityTitle:@"Zoom Out"],
    [UIKeyCommand keyCommandWithInput:UIKeyInputUpArrow modifierFlags:0 action:@selector(zoomIn)], // Alternative, not shown when holding CMD
    [UIKeyCommand keyCommandWithInput:@"=" modifierFlags:UIKeyModifierCommand action:@selector(zoomIn)], // Alternative, not shown when holding CMD
    [UIKeyCommand keyCommandWithInput:@"+" modifierFlags:UIKeyModifierCommand action:@selector(zoomIn) discoverabilityTitle:@"Zoom In"],
    [UIKeyCommand keyCommandWithInput:UIKeyInputEscape modifierFlags:0 action:@selector(goBack) discoverabilityTitle:@"Go Back"],
    [UIKeyCommand keyCommandWithInput:@"0" modifierFlags:UIKeyModifierCommand action:@selector(switchPositionMode) discoverabilityTitle:@"Switch position mode"]
  ];

  if (@available(iOS 15, *)) {
    for (UIKeyCommand *command in commands) {
      command.wantsPriorityOverSystemBehavior = YES;
    }
  }

  return commands;
}

- (void)zoomOut {
  GetFramework().Scale(Framework::SCALE_MIN, true);
}

- (void)zoomIn {
  GetFramework().Scale(Framework::SCALE_MAG, true);
}

- (void)switchPositionMode {
  GetFramework().SwitchMyPositionNextMode();
}

- (void)goBack {
  NSString *backURL = [DeepLinkHandler.shared getBackUrl];
  if (backURL != nil) {
    [[UIApplication sharedApplication] openURL:[NSURL URLWithString: backURL] options:@{} completionHandler:nil];
  }
}

// MARK: - Handle macOS trackpad gestures

- (void)handlePan:(UIPanGestureRecognizer *)recognizer API_AVAILABLE(ios(14.0)) {
  switch (recognizer.state) {
    case UIGestureRecognizerStateBegan:
    case UIGestureRecognizerStateChanged:
    {
      CGPoint translation = [recognizer translationInView:self.view];
      if (translation.x == 0 && CGPointEqualToPoint(translation, CGPointZero))
        return;
      self.userTouchesAction = UserTouchesActionScale;
      static const CGFloat kScaleFactor = 0.9;
      const CGFloat factor = translation.y > 0 ? 1 / kScaleFactor : kScaleFactor;
      GetFramework().Scale(factor, [self getZoomPoint], false);
      [recognizer setTranslation:CGPointZero inView:self.view];
      break;
    }
    case UIGestureRecognizerStateEnded:
      self.userTouchesAction = UserTouchesActionNone;
      break;
    default:
      break;
  }
}

- (void)handlePinch:(UIPinchGestureRecognizer *)recognizer API_AVAILABLE(ios(14.0)) {
  switch (recognizer.state) {
    case UIGestureRecognizerStateBegan:
      self.currentScale = 1.0;
    case UIGestureRecognizerStateChanged:
    {
      const CGFloat scale = [recognizer scale];
      static const CGFloat kScaleDeltaMultiplier = 4.0; // map trackpad scale to the map scale
      const CGFloat delta = scale - self.currentScale;
      const CGFloat scaleFactor = 1 + delta * kScaleDeltaMultiplier;
      GetFramework().Scale(scaleFactor, [self getZoomPoint], false);
      self.currentScale = scale;
      break;
    }
    case UIGestureRecognizerStateEnded:
      self.userTouchesAction = UserTouchesActionNone;
      break;
    default:
      break;
  }
}

- (void)handleRotation:(UIRotationGestureRecognizer *)recognizer API_AVAILABLE(ios(14.0)) {
  switch (recognizer.state) {
    case UIGestureRecognizerStateBegan:
    case UIGestureRecognizerStateChanged:
    {
      self.userTouchesAction = UserTouchesActionDrag;
      GetFramework().Rotate(self.currentRotation == 0 ? recognizer.rotation : self.currentRotation + recognizer.rotation, false);
      break;
    }
    case UIGestureRecognizerStateEnded:
      self.currentRotation += recognizer.rotation;
      self.userTouchesAction = UserTouchesActionNone;
      break;
    default:
      break;
  }
}

- (void)handlePointerHover:(UIHoverGestureRecognizer *)recognizer API_AVAILABLE(ios(14.0)) {
  self.pointerLocation = [recognizer locationInView:self.view];
}

- (m2::PointD)getZoomPoint API_AVAILABLE(ios(14.0)) {
  const CGFloat scale = [UIScreen mainScreen].scale;
  return m2::PointD(self.pointerLocation.x * scale, self.pointerLocation.y * scale);
}

// MARK: - UIGestureRecognizerDelegate

- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer *)otherGestureRecognizer {
  return YES;
}

@end
