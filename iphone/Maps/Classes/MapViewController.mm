#import "MapViewController.h"
#import <CoreApi/MWMBookmarksManager.h>
#import "EAGLView.h"
#import "MWMAuthorizationCommon.h"
#import "MWMAuthorizationWebViewLoginViewController.h"
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

#include <CoreApi/Framework.h>
#import <CoreApi/MWMFrameworkHelper.h>
#import <CoreApi/PlacePageData.h>

#include "drape_frontend/user_event_stream.hpp"

#include "geometry/mercator.hpp"

#import <FirebaseCrashlytics/FirebaseCrashlytics.h>

// If you have a "missing header error" here, then please run configure.sh script in the root repo
// folder.
#import "../../../private.h"

extern NSString *const kMap2FBLoginSegue = @"Map2FBLogin";
extern NSString *const kMap2GoogleLoginSegue = @"Map2GoogleLogin";

typedef NS_ENUM(NSUInteger, UserTouchesAction) { UserTouchesActionNone, UserTouchesActionDrag, UserTouchesActionScale };

namespace {
NSString *const kDownloaderSegue = @"Map2MapDownloaderSegue";
NSString *const kEditorSegue = @"Map2EditorSegue";
NSString *const kUDViralAlertWasShown = @"ViralAlertWasShown";
NSString *const kPP2BookmarkEditingSegue = @"PP2BookmarkEditing";
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
                                 MWMStorageObserver,
                                 MWMWelcomePageControllerProtocol,
                                 MWMKeyboardObserver,
                                 RemoveAdsViewControllerDelegate,
                                 MWMBookmarksObserver>

@property(nonatomic, readwrite) MWMMapViewControlsManager *controlsManager;

@property(nonatomic) BOOL disableStandbyOnLocationStateMode;

@property(nonatomic) UserTouchesAction userTouchesAction;

@property(nonatomic, readwrite) MWMMapDownloadDialog *downloadDialog;

@property(nonatomic) BOOL skipForceTouch;

@property(strong, nonatomic) IBOutlet NSLayoutConstraint *visibleAreaBottom;
@property(strong, nonatomic) IBOutlet NSLayoutConstraint *visibleAreaKeyboard;
@property(strong, nonatomic) IBOutlet NSLayoutConstraint *placePageAreaKeyboard;
@property(strong, nonatomic) IBOutlet NSLayoutConstraint *sideButtonsAreaBottom;
@property(strong, nonatomic) IBOutlet NSLayoutConstraint *sideButtonsAreaKeyboard;
@property(strong, nonatomic) IBOutlet NSLayoutConstraint *guidesNavigationBarAreaBottom;
@property(strong, nonatomic) IBOutlet NSLayoutConstraint *guidesVisibleConstraint;;
@property(strong, nonatomic) IBOutlet NSLayoutConstraint *guidesHiddenConstraint;
@property(strong, nonatomic) IBOutlet UIImageView *carplayPlaceholderLogo;
@property(strong, nonatomic) BookmarksCoordinator * bookmarksCoordinator;

@property(strong, nonatomic) NSHashTable<id<MWMLocationModeListener>> *listeners;

@property(nonatomic) BOOL needDeferFocusNotification;
@property(nonatomic) BOOL deferredFocusValue;
@property(nonatomic) UIViewController *placePageVC;
@property(nonatomic) UIViewController *guidesGalleryVC;
@property(nonatomic) IBOutlet UIView *placePageContainer;
@property(nonatomic) IBOutlet UIView *guidesCollectionContainer;

@end

@implementation MapViewController
@synthesize mapView, controlsView;

+ (MapViewController *)sharedController {
  return [MapsAppDelegate theApp].mapViewController;
}

#pragma mark - Map Navigation

- (void)showRegularPlacePage {
  self.placePageVC = [PlacePageBuilder build];
  self.placePageContainer.hidden = NO;
  [self.placePageContainer addSubview:self.placePageVC.view];
  [self.view bringSubviewToFront:self.placePageContainer];
  [NSLayoutConstraint activateConstraints:@[
    [self.placePageVC.view.topAnchor constraintEqualToAnchor:self.placePageContainer.safeAreaLayoutGuide.topAnchor],
    [self.placePageVC.view.leftAnchor constraintEqualToAnchor:self.placePageContainer.leftAnchor],
    [self.placePageVC.view.bottomAnchor constraintEqualToAnchor:self.placePageContainer.bottomAnchor],
    [self.placePageVC.view.rightAnchor constraintEqualToAnchor:self.placePageContainer.rightAnchor]
  ]];
  self.placePageVC.view.translatesAutoresizingMaskIntoConstraints = NO;
  [self addChildViewController:self.placePageVC];
  [self.placePageVC didMoveToParentViewController:self];
}

- (void)showGuidesGallery {
  self.guidesGalleryVC = [MWMGuidesGalleryBuilder build];
  self.guidesCollectionContainer.hidden = NO;
  [self.guidesCollectionContainer addSubview:self.guidesGalleryVC.view];
  [NSLayoutConstraint activateConstraints:@[
    [self.guidesGalleryVC.view.topAnchor constraintEqualToAnchor:self.guidesCollectionContainer.topAnchor],
    [self.guidesGalleryVC.view.leftAnchor constraintEqualToAnchor:self.guidesCollectionContainer.leftAnchor],
    [self.guidesGalleryVC.view.bottomAnchor constraintEqualToAnchor:self.guidesCollectionContainer.bottomAnchor],
    [self.guidesGalleryVC.view.rightAnchor constraintEqualToAnchor:self.guidesCollectionContainer.rightAnchor]
  ]];
  self.guidesGalleryVC.view.translatesAutoresizingMaskIntoConstraints = NO;
  [self addChildViewController:self.guidesGalleryVC];
  [self.guidesGalleryVC didMoveToParentViewController:self];
  self.guidesVisibleConstraint.priority = UILayoutPriorityDefaultLow;
  self.guidesHiddenConstraint.priority = UILayoutPriorityDefaultHigh;
  self.guidesCollectionContainer.alpha = 0;
  [self.view layoutIfNeeded];
  [UIView animateWithDuration:kDefaultAnimationDuration animations:^{
    self.guidesVisibleConstraint.priority = UILayoutPriorityDefaultHigh;
    self.guidesHiddenConstraint.priority = UILayoutPriorityDefaultLow;
    [self.view layoutIfNeeded];
    self.guidesCollectionContainer.alpha = 1;
  }];
  [self setPlacePageTopBound:self.view.height - self.guidesCollectionContainer.minY duration:kDefaultAnimationDuration];
}

- (void)showPlacePage {
  if (!PlacePageData.hasData) {
    return;
  }
  
  self.controlsManager.trafficButtonHidden = YES;
  if (PlacePageData.isGuide) {
    [self showGuidesGallery];
  } else {
    [self showRegularPlacePage];
  }
}

- (void)dismissPlacePage {
  GetFramework().DeactivateMapSelection(true);
}

- (void)hideRegularPlacePage {
  [self.placePageVC.view removeFromSuperview];
  [self.placePageVC willMoveToParentViewController:nil];
  [self.placePageVC removeFromParentViewController];
  self.placePageVC = nil;
  self.placePageContainer.hidden = YES;
  [self setPlacePageTopBound:0 duration:0];
}

- (void)hideGuidesGallery {
  [self.view layoutIfNeeded];
  [UIView animateWithDuration:kDefaultAnimationDuration animations:^{
    self.guidesVisibleConstraint.priority = UILayoutPriorityDefaultLow;
    self.guidesHiddenConstraint.priority = UILayoutPriorityDefaultHigh;
    [self.view layoutIfNeeded];
    self.guidesCollectionContainer.alpha = 0;
  } completion:^(BOOL finished) {
    [self.guidesGalleryVC.view removeFromSuperview];
    [self.guidesGalleryVC willMoveToParentViewController:nil];
    [self.guidesGalleryVC removeFromParentViewController];
    self.guidesGalleryVC = nil;
    self.guidesCollectionContainer.hidden = YES;
    self.guidesVisibleConstraint.constant = 48;
  }];
  [self setPlacePageTopBound:0 duration:kDefaultAnimationDuration];
}

- (void)hidePlacePage {
  if (self.placePageVC != nil) {
    [self hideRegularPlacePage];
  } else if (self.guidesGalleryVC != nil) {
    [self hideGuidesGallery];
  }
  self.controlsManager.trafficButtonHidden = NO;
}

- (void)onMapObjectDeselected:(bool)switchFullScreenMode {
  [self hidePlacePage];

  BOOL const isSearchResult = [MWMSearchManager manager].state == MWMSearchManagerStateResult;
  BOOL const isNavigationDashboardHidden = [MWMNavigationDashboardManager sharedManager].state == MWMNavigationDashboardStateHidden;
  if (isSearchResult) {
    if (isNavigationDashboardHidden) {
      [MWMSearchManager manager].state = MWMSearchManagerStateMapSearch;
    } else {
      [MWMSearchManager manager].state = MWMSearchManagerStateHidden;
    }
  }

  if (!switchFullScreenMode)
    return;

  if (DeepLinkHandler.shared.isLaunchedByDeeplink)
    return;

  BOOL const isSearchHidden = [MWMSearchManager manager].state == MWMSearchManagerStateHidden;
  if (isSearchHidden && isNavigationDashboardHidden) {
    self.controlsManager.hidden = !self.controlsManager.hidden;
  }
}

- (void)onMapObjectSelected {
  [self hidePlacePage];
  [[MWMNetworkPolicy sharedPolicy] callOnlineApi:^(BOOL) {
    [self showPlacePage];
  }];
}

- (void)onMapObjectUpdated {
  //  [self.controlsManager updatePlacePage];
}

- (IBAction)onGudesGalleryPan:(UIPanGestureRecognizer *)sender {
  CGFloat originalConstant = 48;
  UIView *galleryView = self.guidesCollectionContainer;
  switch (sender.state) {
    case UIGestureRecognizerStatePossible:
    case UIGestureRecognizerStateBegan:
      break;
    case UIGestureRecognizerStateChanged: {
      CGFloat dy = [sender translationInView:galleryView].y;
      [sender setTranslation:CGPointZero inView:galleryView];
      CGFloat constant = MIN(self.guidesVisibleConstraint.constant - dy, originalConstant);
      self.guidesVisibleConstraint.constant = constant;
      galleryView.alpha = (constant + galleryView.frame.size.height) / (galleryView.frame.size.height + originalConstant);
      break;
    }
    case UIGestureRecognizerStateEnded:
    case UIGestureRecognizerStateCancelled:
    case UIGestureRecognizerStateFailed:
      CGFloat velocity = [sender velocityInView:galleryView].y;
      if (velocity < 0 || (velocity == 0 && galleryView.alpha > 0.8)) {
        [self.view layoutIfNeeded];
        [UIView animateWithDuration:kDefaultAnimationDuration animations:^{
          self.guidesVisibleConstraint.constant = originalConstant;
          galleryView.alpha = 1;
          [self.view layoutIfNeeded];
        }];
      } else {
        [self dismissPlacePage];
      }
      break;
  }
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
  if (@available(iOS 12.0, *)) {
    if ([MWMCarPlayService shared].isCarplayActivated) {
      return;
    }
  }
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
  [self.welcomePageController viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
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

  if ([MWMNavigationDashboardManager sharedManager].state == MWMNavigationDashboardStateHidden &&
      [MWMSearchManager manager].state == MWMSearchManagerStateHidden &&
      [MWMMapViewControlsManager manager].guidesNavigationBarHidden == NO)
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

  // On iOS 10 (it was reproduced, it may be also on others), mapView can be uninitialized
  // when onGetFocus is called, it can lead to missing of onGetFocus call and a deadlock on the start.
  // As soon as mapView must exist before onGetFocus, so we have to defer onGetFocus call.
  if (self.needDeferFocusNotification)
    [self onGetFocus:self.deferredFocusValue];

  [self.mapView setLaunchByDeepLink:DeepLinkHandler.shared.isLaunchedByDeeplink];
  [MWMRouter restoreRouteIfNeeded];

  self.view.clipsToBounds = YES;
  [MWMKeyboard addObserver:self];
  self.welcomePageController = [MWMWelcomePageController controllerWithParent:self];
  [self processMyPositionStateModeEvent:[MWMLocationManager isLocationProhibited] ? MWMMyPositionModeNotFollowNoPosition
                                                                                  : MWMMyPositionModePendingPosition];
  if ([MWMNavigationDashboardManager sharedManager].state == MWMNavigationDashboardStateHidden)
    self.controlsManager.menuState = self.controlsManager.menuRestoreState;

  [NSNotificationCenter.defaultCenter addObserver:self
                                         selector:@selector(didBecomeActive)
                                             name:UIApplicationDidBecomeActiveNotification
                                           object:nil];
  [self.welcomePageController show];
  if (!self.welcomePageController) {
    [DeepLinkHandler.shared handleDeeplink];
  }
}

- (void)didBecomeActive {
  if (!self.welcomePageController)
    [self.controlsManager showAdditionalViewsIfNeeded];
}

- (void)viewDidLayoutSubviews {
  [super viewDidLayoutSubviews];
  if (!self.mapView.drapeEngineCreated)
    [self.mapView createDrapeEngine];
}

- (void)applyTheme {
  [super applyTheme];
  [MapsAppDelegate customizeAppearance];
}

- (void)closePageController:(MWMWelcomePageController *)pageController {
  if ([pageController isEqual:self.welcomePageController])
    self.welcomePageController = nil;

  auto const todo = GetFramework().ToDoAfterUpdate();

  switch (todo) {
    case Framework::DoAfterUpdate::Nothing:
      break;
    case Framework::DoAfterUpdate::AutoupdateMaps:
    case Framework::DoAfterUpdate::AskForUpdateMaps:
      [self presentViewController:[MWMAutoupdateController instanceWithPurpose:todo] animated:YES completion:nil];
      break;
  }
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
  [ud synchronize];
}

- (void)viewWillDisappear:(BOOL)animated {
  [super viewWillDisappear:animated];

  if ([MWMNavigationDashboardManager sharedManager].state == MWMNavigationDashboardStateHidden &&
      [MWMSearchManager manager].state == MWMSearchManagerStateHidden)
    self.controlsManager.menuRestoreState = self.controlsManager.menuState;
  GetFramework().SetRenderingDisabled(false);
}

- (BOOL)prefersStatusBarHidden {
  return NO;
}
- (UIStatusBarStyle)preferredStatusBarStyle {
  return [self.controlsManager preferredStatusBarStyle];
}

- (void)updateStatusBarStyle {
  [self setNeedsStatusBarAppearanceUpdate];
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
                          [self](bool switchFullScreen) { [self onMapObjectDeselected:switchFullScreen]; },
                          [self]() { [self onMapObjectUpdated]; });
  // TODO: Review and improve this code.
  f.SetMyPositionModeListener([self](location::EMyPositionMode mode, bool routingActive) {
    // TODO: Two global listeners are subscribed to the same event from the core.
    // Probably it's better to subscribe only wnen needed and usubscribe in other cases.
    // May be better solution would be multiobservers support in the C++ core.
    [self processMyPositionStateModeEvent:location_helpers::mwmMyPositionMode(mode)];
  });
  f.SetMyPositionPendingTimeoutListener([self] { [self processMyPositionPendingTimeout]; });

  self.userTouchesAction = UserTouchesActionNone;
  [[MWMBookmarksManager sharedManager] addObserver:self];
  [[MWMBookmarksManager sharedManager] loadBookmarks];
  [MWMFrameworkListener addObserver:self];
  [[MWMStorage sharedStorage] addObserver:self];
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

- (void)openMapsDownloader:(MWMMapDownloaderMode)mode {
  [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"downloader"];
  [self performSegueWithIdentifier:kDownloaderSegue sender:@(mode)];
}

- (void)openEditor {
  using namespace osm_auth_ios;

  auto const &featureID = GetFramework().GetCurrentPlacePageInfo().GetID();

  [Statistics logEvent:kStatEditorEditStart
        withParameters:@{
          kStatIsAuthenticated: @(AuthorizationHaveCredentials()),
          kStatIsOnline: Platform::IsConnected() ? kStatYes : kStatNo,
          kStatMWMName: @(featureID.GetMwmName().c_str()),
          kStatEditorMWMVersion: @(featureID.GetMwmVersion())
        }];
  [self performSegueWithIdentifier:kEditorSegue sender:self.controlsManager.featureHolder];
}

- (void)openBookmarkEditor {
  [self performSegueWithIdentifier:kPP2BookmarkEditingSegue sender:nil];
}

- (void)openFullPlaceDescriptionWithHtml:(NSString *)htmlString {
  [Statistics logEvent:kStatPlacePageDescriptionMore
        withParameters:@{kStatSource: kStatWikipedia}
           withChannel:StatisticsChannelRealtime];
  WebViewController *descriptionViewController =
    [[PlacePageDescriptionViewController alloc] initWithHtml:htmlString
                                                     baseUrl:nil
                                                       title:L(@"place_description_title")];
  descriptionViewController.openInSafari = YES;
  [self.navigationController pushViewController:descriptionViewController animated:YES];
}

- (void)showUGCAuth {
  if (IPAD) {
    auto controller = [[MWMAuthorizationViewController alloc] initWithPopoverSourceView:self.controlsManager.anchorView
                                                                                 source:AuthorizationSourceAfterSaveReview
                                                               permittedArrowDirections:UIPopoverArrowDirectionDown
                                                                         successHandler:nil
                                                                           errorHandler:nil
                                                                      completionHandler:nil];

    [self presentViewController:controller animated:YES completion:nil];
    return;
  }

  auto controller = [[MWMAuthorizationViewController alloc] initWithBarButtonItem:nil
                                                                           source:AuthorizationSourceAfterSaveReview
                                                                   successHandler:nil
                                                                     errorHandler:nil
                                                                completionHandler:nil];

  [self presentViewController:controller animated:YES completion:nil];
}

- (void)showBookmarksLoadedAlert:(UInt64)categoryId {
  for (UIViewController *vc in self.navigationController.viewControllers) {
    if ([vc isMemberOfClass:MWMCatalogWebViewController.class]) {
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

- (void)openCatalogAnimated:(BOOL)animated utm:(MWMUTM)utm {
  [Statistics logEvent:kStatCatalogOpen withParameters:@{kStatFrom: kStatMenu}];
  [self openCatalogAbsoluteUrl:nil animated:animated utm:utm];
}

- (void)openCatalogInternal:(MWMCatalogWebViewController *)catalog animated:(BOOL)animated utm:(MWMUTM)utm {
  [self.navigationController popToRootViewControllerAnimated:NO];
  auto bookmarks = [[MWMBookmarksTabViewController alloc] initWithCoordinator:self.bookmarksCoordinator];
  bookmarks.activeTab = ActiveTabCatalog;
  NSMutableArray<UIViewController *> *controllers = [self.navigationController.viewControllers mutableCopy];
  [controllers addObjectsFromArray:@[bookmarks, catalog]];
  [self.navigationController setViewControllers:controllers animated:animated];
}

- (void)openCatalogDeeplink:(NSURL *)deeplinkUrl animated:(BOOL)animated utm:(MWMUTM)utm {
  MWMCatalogWebViewController *catalog;
  catalog = [MWMCatalogWebViewController catalogFromDeeplink:deeplinkUrl utm:utm];
  [self openCatalogInternal:catalog animated:animated utm:utm];
}

- (void)openCatalogAbsoluteUrl:(NSURL *)url animated:(BOOL)animated utm:(MWMUTM)utm {
  MWMCatalogWebViewController *catalog;
  catalog = [MWMCatalogWebViewController catalogFromAbsoluteUrl:url utm:utm];
  NSMutableArray<UIViewController *> *controllers = [self.navigationController.viewControllers mutableCopy];
  [controllers addObjectsFromArray:@[catalog]];
  [self.navigationController setViewControllers:controllers animated:animated];
}

- (void)searchText:(NSString *)text {
  [self.controlsManager searchText:text forInputLocale:[[AppInfo sharedInfo] languageId]];
}

- (void)openDrivingOptions {
  UIStoryboard *sb = [UIStoryboard instance:MWMStoryboardDrivingOptions];
  UIViewController *vc = [sb instantiateInitialViewController];
  [self.navigationController pushViewController:vc animated:YES];
}

- (void)showRemoveAds {
  auto removeAds = [[RemoveAdsViewController alloc] init];
  removeAds.delegate = self;
  [self.navigationController presentViewController:removeAds animated:YES completion:nil];
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
      break;
    case MWMMyPositionModePendingPosition:
      if (self.welcomePageController && [Alohalytics isFirstSession]) {
        break;
      }
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

- (void)processMyPositionPendingTimeout {
  [MWMLocationManager stop];
  NSArray<id<MWMLocationModeListener>> *objects = self.listeners.allObjects;
  for (id<MWMLocationModeListener> object in objects) {
    [object processMyPositionPendingTimeout];
  }
  BOOL const isMapVisible = (self.navigationController.visibleViewController == self);
  if (isMapVisible && ![MWMLocationManager isLocationProhibited]) {
    if (self.welcomePageController) {
      GetFramework().SwitchMyPositionNextMode();
    } else {
      [self.alertController presentLocationNotFoundAlertWithOkBlock:^{
        GetFramework().SwitchMyPositionNextMode();
      }];
    }
  }
}

#pragma mark - MWMRemoveAdsViewControllerDelegate

- (void)didCompleteSubscribtion:(RemoveAdsViewController *)viewController {
  [self.navigationController dismissViewControllerAnimated:YES completion:nil];
  GetFramework().DeactivateMapSelection(true);
  [self.controlsManager hideSearch];
}

- (void)didCancelSubscribtion:(RemoveAdsViewController *)viewController {
  [self.navigationController dismissViewControllerAnimated:YES completion:nil];
}

#pragma mark - MWMFrameworkDrapeObserver

- (void)processViewportCountryEvent:(CountryId const &)countryId {
  [self.downloadDialog processViewportCountryEvent:countryId];
}

#pragma mark - MWMStorageObserver

- (void)processCountryEvent:(NSString *)countryId {
  if (countryId.length == 0) {
#ifdef OMIM_PRODUCTION
    auto err = [[NSError alloc] initWithDomain:kMapsmeErrorDomain
                                          code:1
                                      userInfo:@{@"Description": @"attempt to get info from empty countryId"}];
    [[FIRCrashlytics crashlytics] recordError:err];
#endif
    return;
  }

  NodeStatuses nodeStatuses{};
  GetFramework().GetStorage().GetNodeStatuses(countryId.UTF8String, nodeStatuses);
  if (nodeStatuses.m_status != NodeStatus::Error)
    return;
  switch (nodeStatuses.m_error) {
    case NodeErrorCode::NoError:
      break;
    case NodeErrorCode::UnknownError:
      [Statistics logEvent:kStatDownloaderMapError withParameters:@{kStatType: kStatUnknownError}];
      break;
    case NodeErrorCode::OutOfMemFailed:
      [Statistics logEvent:kStatDownloaderMapError withParameters:@{kStatType: kStatNoSpace}];
      break;
    case NodeErrorCode::NoInetConnection:
      [Statistics logEvent:kStatDownloaderMapError withParameters:@{kStatType: kStatNoConnection}];
      break;
  }
}

#pragma mark - Authorization

- (void)checkAuthorization {
  using namespace osm_auth_ios;
  BOOL const isAfterEditing = AuthorizationIsNeedCheck() && !AuthorizationHaveCredentials();
  if (isAfterEditing) {
    AuthorizationSetNeedCheck(NO);
    if (!Platform::IsConnected())
      return;
    [Statistics logEvent:kStatEventName(kStatPlacePage, kStatEditTime)
          withParameters:@{kStatValue: kStatAuthorization}];
    [self.alertController presentOsmAuthAlert];
  }
}

#pragma mark - 3d touch

- (void)performAction:(NSString *)action {
  [self.navigationController popToRootViewControllerAnimated:NO];
  if (self.isViewLoaded) {
    auto searchState = MWMSearchManagerStateHidden;
    [MWMRouter stopRouting];
    if ([action isEqualToString:@"me.maps.3daction.bookmarks"])
      [self.bookmarksCoordinator open];
    else if ([action isEqualToString:@"me.maps.3daction.search"])
      searchState = MWMSearchManagerStateDefault;
    else if ([action isEqualToString:@"me.maps.3daction.route"])
      [self.controlsManager onRoutePrepare];
    [MWMSearchManager manager].state = searchState;
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
  } else if ([segue.identifier isEqualToString:kMap2FBLoginSegue]) {
    MWMAuthorizationWebViewLoginViewController *dvc = segue.destinationViewController;
    dvc.authType = MWMWebViewAuthorizationTypeFacebook;
  } else if ([segue.identifier isEqualToString:kMap2GoogleLoginSegue]) {
    MWMAuthorizationWebViewLoginViewController *dvc = segue.destinationViewController;
    dvc.authType = MWMWebViewAuthorizationTypeGoogle;
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
  if (!self.isViewLoaded)
    return nil;
  if (!_controlsManager)
    _controlsManager = [[MWMMapViewControlsManager alloc] initWithParentController:self];
  return _controlsManager;
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
  self.guidesNavigationBarAreaBottom.constant = bound;
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
  self.carplayPlaceholderLogo.hidden = YES;
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
  self.carplayPlaceholderLogo.hidden = NO;
}

#pragma mark - MWMBookmarksObserver
- (void)onBookmarksFileLoadSuccess {
  [[MWMAlertViewController activeAlertController] presentInfoAlert:L(@"load_kmz_title") text:L(@"load_kmz_successful")];
  [Statistics logEvent:kStatEventName(kStatApplication, kStatImport) withParameters:@{kStatValue: kStatImport}];
}

- (void)onBookmarksFileLoadError {
  [[MWMAlertViewController activeAlertController] presentInfoAlert:L(@"load_kmz_title") text:L(@"load_kmz_failed")];
}

- (BOOL)canBecomeFirstResponder {
  return YES;
}

- (NSArray *)keyCommands {
   return @[[UIKeyCommand keyCommandWithInput:UIKeyInputDownArrow modifierFlags:0 action:@selector(zoomOut) discoverabilityTitle:@"Zoom Out"],
    [UIKeyCommand keyCommandWithInput:UIKeyInputUpArrow modifierFlags:0 action:@selector(zoomIn) discoverabilityTitle:@"Zoom In"],
    [UIKeyCommand keyCommandWithInput:UIKeyInputEscape modifierFlags:0 action:@selector(goBack) discoverabilityTitle:@"Go Back"]];
}

- (void)zoomOut {
  GetFramework().Scale(Framework::SCALE_MIN, true);
}

- (void)zoomIn {
  GetFramework().Scale(Framework::SCALE_MAG, true);
}

- (void)goBack {
   NSString *backURL = [DeepLinkHandler.shared getBackUrl];
   BOOL canOpenURL = [[UIApplication sharedApplication] canOpenURL:[NSURL URLWithString:backURL]];
   if (canOpenURL){
     [[UIApplication sharedApplication] openURL:[NSURL URLWithString:backURL] options:@{} completionHandler:nil];
   }
}

@end
