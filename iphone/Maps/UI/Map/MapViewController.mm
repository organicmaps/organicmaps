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
#import "MWMObjectsCategorySelectorController.h"
#import "MWMPlacePageProtocol.h"
#import "MapsAppDelegate.h"
#import "SwiftBridge.h"
#import "TrackRecordingState.h"

#import <CoreApi/Framework.h>
#import <CoreApi/MWMFrameworkHelper.h>
#import <CoreApi/PlacePageData.h>

#include "drape_frontend/drape_engine.hpp"
#include "drape_frontend/screen_operations.hpp"
#include "drape_frontend/user_event_stream.hpp"

#include "geometry/mercator.hpp"
#include "geometry/parametrized_segment.hpp"

#include <algorithm>
#include <atomic>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <tuple>

#import "../../../private.h"

extern NSString * const kMap2FBLoginSegue = @"Map2FBLogin";
extern NSString * const kMap2GoogleLoginSegue = @"Map2GoogleLogin";

static CGFloat kPlacePageCompactWidth = 350;
static CGFloat const kPlacePageLeadingOffset = 20;

typedef NS_ENUM(NSUInteger, UserTouchesAction) {
  UserTouchesActionNone,
  UserTouchesActionDrag,
  UserTouchesActionScale
};

namespace
{
NSString * const kDownloaderSegue = @"Map2MapDownloaderSegue";
NSString * const kEditorSegue = @"Map2EditorSegue";
NSString * const kUDViralAlertWasShown = @"ViralAlertWasShown";
NSString * const kPP2BookmarkEditingSegue = @"PP2BookmarkEditing";
// Mirrors kMapToCategorySelectorSegue in MWMMapViewControlsManager.mm.
NSString * const kCategorySelectorSegue = @"MapToCategorySelectorSegue";

using MapAccessibilityID = std::tuple<FeatureID, kml::MarkId, kml::TrackId>;

struct MapAccessibilityRenderState
{
  std::atomic_uint64_t m_revision{0};
  std::atomic_bool m_notificationPending{false};
};

CGRect MapAccessibilityFrame(m2::RectD const & rect, CGFloat scale)
{
  return CGRectMake(rect.minX() / scale, rect.minY() / scale, rect.SizeX() / scale, rect.SizeY() / scale);
}

CGRect PaddedMapAccessibilityFrame(CGRect frame)
{
  frame = CGRectInset(frame, -8.0, -8.0);
  return CGRectInset(frame, -std::max(0.0, (44.0 - frame.size.width) / 2.0),
                     -std::max(0.0, (44.0 - frame.size.height) / 2.0));
}
}  // namespace

@interface NSValueWrapper : NSObject

- (NSValue *)getInnerValue;

@end

@implementation NSValueWrapper
{
  NSValue * m_innerValue;
}

- (NSValue *)getInnerValue
{
  return m_innerValue;
}
- (id)initWithValue:(NSValue *)value
{
  self = [super init];
  if (self)
    m_innerValue = value;
  return self;
}

- (BOOL)isEqual:(id)anObject
{
  return [anObject isMemberOfClass:[NSValueWrapper class]];
}

@end

@interface MapViewController (MapAccessibilityActivation)

- (BOOL)activateMapAccessibilityObject:(MapAccessibilityID const &)identity
                              mercator:(m2::PointD const &)mercator
                              revision:(NSUInteger)revision
                                  zoom:(int)zoom;

@end

@interface MWMMapFeatureAccessibilityElement : UIAccessibilityElement

- (instancetype)initWithContainer:(UIView *)container
                       controller:(MapViewController *)controller
                         identity:(MapAccessibilityID const &)identity;
- (MapAccessibilityID const &)mapIdentity;
- (void)updateWithObject:(df::AccessibleMapObject &)object
                 mapView:(UIView *)mapView
                  screen:(ScreenBase const &)screen
                viewport:(CGRect)viewport
                revision:(NSUInteger)revision
                    zoom:(int)zoom;
- (NSInteger)hitRankAtScreenPoint:(CGPoint)point;
- (NSInteger)mapSelectionPriority;
- (CGFloat)squaredDistanceToScreenPoint:(CGPoint)point;
- (void)setMapExplorationPoint:(CGPoint)point;

@end

@implementation MWMMapFeatureAccessibilityElement
{
  MapAccessibilityID m_identity;
  m2::PointD m_mercator;
  NSUInteger m_revision;
  int m_zoom;
  __weak MapViewController * m_controller;
  NSString * m_title;
  NSString * m_subtitle;
  BOOL m_textLoaded;
  std::vector<CGRect> m_drawnFrames;
  std::vector<CGRect> m_paddedFrames;
  UIBezierPath * m_drawnTrackPath;
  std::vector<df::ProjectedPolyline> m_trackPaths;
  std::optional<ScreenBase> m_trackScreen;
  std::optional<CGPoint> m_explorationPoint;
}

- (instancetype)initWithContainer:(UIView *)container
                       controller:(MapViewController *)controller
                         identity:(MapAccessibilityID const &)identity
{
  self = [super initWithAccessibilityContainer:container];
  if (self)
  {
    m_identity = identity;
    m_controller = controller;
    self.isAccessibilityElement = YES;
    self.accessibilityTraits = UIAccessibilityTraitButton;
  }
  return self;
}

- (MapAccessibilityID const &)mapIdentity
{
  return m_identity;
}

- (void)updateWithObject:(df::AccessibleMapObject &)object
                 mapView:(UIView *)mapView
                  screen:(ScreenBase const &)screen
                viewport:(CGRect)viewport
                revision:(NSUInteger)revision
                    zoom:(int)zoom
{
  m_mercator = object.m_mercator;
  m_revision = revision;
  m_zoom = zoom;
  m_textLoaded = NO;
  m_drawnFrames.clear();
  m_paddedFrames.clear();
  m_drawnTrackPath = nil;
  self.accessibilityPath = nil;

  CGFloat const scale = mapView.contentScaleFactor;
  if (object.m_trackId != kml::kInvalidTrackId)
  {
    m_trackScreen = screen;
    m_trackPaths = std::move(object.m_pixelPaths);
    m_explorationPoint.reset();
    UIBezierPath * line = [UIBezierPath bezierPath];
    for (auto const & path : m_trackPaths)
    {
      auto const & points = path.m_pixels;
      [line moveToPoint:CGPointMake(points.front().x / scale, points.front().y / scale)];
      for (size_t i = 1; i < points.size(); ++i)
        [line addLineToPoint:CGPointMake(points[i].x / scale, points[i].y / scale)];
    }

    CGFloat const width = object.m_lineWidth / scale;
    auto const strokeInScreenCoordinates = [line, mapView](CGFloat strokeWidth)
    {
      CGPathRef const stroke =
          CGPathCreateCopyByStrokingPath(line.CGPath, nullptr, strokeWidth, kCGLineCapRound, kCGLineJoinRound, 10.0);
      UIBezierPath * path = [UIBezierPath bezierPathWithCGPath:stroke];
      CGPathRelease(stroke);
      return UIAccessibilityConvertPathToScreenCoordinates(path, mapView);
    };
    m_drawnTrackPath = strokeInScreenCoordinates(width);
    self.accessibilityPath = strokeInScreenCoordinates(std::max(44.0, width));
    self.accessibilityFrameInContainerSpace =
        CGRectIntersection(CGRectInset(MapAccessibilityFrame(object.m_pixelRect, scale), -22.0, -22.0), viewport);
    return;
  }

  CGRect frame = CGRectNull;
  for (auto const & pixelRect : object.m_pixelRects)
  {
    CGRect const drawn = MapAccessibilityFrame(pixelRect, scale);
    CGRect const padded = CGRectIntersection(PaddedMapAccessibilityFrame(drawn), viewport);
    m_drawnFrames.push_back(UIAccessibilityConvertFrameToScreenCoordinates(drawn, mapView));
    m_paddedFrames.push_back(UIAccessibilityConvertFrameToScreenCoordinates(padded, mapView));
    frame = CGRectUnion(frame, padded);
  }
  self.accessibilityFrameInContainerSpace = frame;
}

- (void)loadTextIfNeeded
{
  if (m_textLoaded)
    return;
  m_textLoaded = YES;
  auto const & [featureId, markId, trackId] = m_identity;
  std::pair<std::string, std::string> text;
  auto & framework = GetFramework();
  if (markId != kml::kInvalidMarkId)
    text = framework.GetAccessibleBookmarkText(markId);
  else if (trackId != kml::kInvalidTrackId)
    text = framework.GetAccessibleTrackText(trackId);
  else
    text = framework.GetAccessibleFeatureText(featureId);
  auto const & [title, subtitle] = text;
  m_title = [NSString stringWithUTF8String:title.c_str()];
  m_subtitle = [NSString stringWithUTF8String:subtitle.c_str()];
}

- (NSString *)accessibilityLabel
{
  [self loadTextIfNeeded];
  return m_title;
}

- (NSString *)accessibilityValue
{
  [self loadTextIfNeeded];
  return m_subtitle;
}

- (void)setMapExplorationPoint:(CGPoint)point
{
  if (m_trackScreen)
    m_explorationPoint = point;
}

- (void)accessibilityElementDidLoseFocus
{
  [super accessibilityElementDidLoseFocus];
  m_explorationPoint.reset();
}

- (BOOL)accessibilityActivate
{
  auto mercator = m_mercator;
  UIView * mapView = self.accessibilityContainer;
  if (m_explorationPoint && mapView.window)
  {
    CGPoint const local = [mapView convertPoint:*m_explorationPoint
                            fromCoordinateSpace:mapView.window.screen.coordinateSpace];
    CGFloat const scale = mapView.contentScaleFactor;
    m2::PointD const pixel(local.x * scale, local.y * scale);
    double bestDistance = std::numeric_limits<double>::max();
    for (auto const & path : m_trackPaths)
    {
      auto const & points = path.m_pixels;
      for (size_t i = 1; i < points.size(); ++i)
      {
        m2::ParametrizedSegment<m2::PointD> const segment(points[i - 1], points[i]);
        auto const nearest = segment.ClosestPointTo(pixel);
        double const distance = nearest.SquaredLength(pixel);
        if (distance < bestDistance)
        {
          bestDistance = distance;
          mercator = m_trackScreen->PtoG(m_trackScreen->P3dtoP(nearest));
          mercator.x -= path.m_worldOffsetX;
        }
      }
    }
  }
  return [m_controller activateMapAccessibilityObject:m_identity mercator:mercator revision:m_revision zoom:m_zoom];
}

- (NSInteger)hitRankAtScreenPoint:(CGPoint)point
{
  if (!CGRectContainsPoint(self.accessibilityFrame, point))
    return 0;
  if (m_drawnTrackPath != nil)
  {
    if (CGPathContainsPoint(m_drawnTrackPath.CGPath, nullptr, point, false))
      return 2;
    return CGPathContainsPoint(self.accessibilityPath.CGPath, nullptr, point, false) ? 1 : 0;
  }

  auto const containsPoint = [point](CGRect rect) { return CGRectContainsPoint(rect, point); };
  if (std::any_of(m_drawnFrames.begin(), m_drawnFrames.end(), containsPoint))
    return 2;
  return std::any_of(m_paddedFrames.begin(), m_paddedFrames.end(), containsPoint) ? 1 : 0;
}

- (NSInteger)mapSelectionPriority
{
  if (std::get<1>(m_identity) != kml::kInvalidMarkId)
    return 2;
  return std::get<2>(m_identity) == kml::kInvalidTrackId ? 1 : 0;
}

- (CGFloat)squaredDistanceToScreenPoint:(CGPoint)point
{
  CGRect const frame = self.accessibilityFrame;
  CGFloat const dx = point.x - CGRectGetMidX(frame);
  CGFloat const dy = point.y - CGRectGetMidY(frame);
  return dx * dx + dy * dy;
}

@end

@interface MapViewController () <MWMFrameworkDrapeObserver,
                                 MWMKeyboardObserver,
                                 MWMBookmarksObserver,
                                 SearchOnMapManagerObserver,
                                 UIGestureRecognizerDelegate>

@property(nonatomic, readwrite) MWMMapViewControlsManager * controlsManager;
@property(nonatomic, readwrite) SearchOnMapManager * searchManager;
@property(nonatomic, readwrite) TrackRecordingManager * trackRecordingManager;

@property(nonatomic) BOOL disableStandbyOnLocationStateMode;

@property(nonatomic) UserTouchesAction userTouchesAction;
@property(nonatomic) CGPoint pointerLocation;
@property(nonatomic) CGFloat currentScale;
@property(nonatomic) CGFloat currentRotation;
@property(nonatomic) NSMapTable<NSObject *, NSValue *> * availableAreaInsetsMap;

@property(nonatomic, readwrite) MWMMapDownloadDialog * downloadDialog;

@property(nonatomic) BOOL skipForceTouch;

@property(strong, nonatomic) IBOutlet NSLayoutConstraint * visibleAreaBottom;
@property(strong, nonatomic) IBOutlet NSLayoutConstraint * visibleAreaLeading;
@property(strong, nonatomic) IBOutlet NSLayoutConstraint * visibleAreaTrailing;
@property(strong, nonatomic) IBOutlet NSLayoutConstraint * visibleAreaKeyboard;
@property(strong, nonatomic) IBOutlet NSLayoutConstraint * placePageAreaKeyboard;
@property(strong, nonatomic) IBOutlet NSLayoutConstraint * sideButtonsAreaBottom;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * sideButtonsAreaCompactBottom;  // Enabled only in landscape
@property(strong, nonatomic) IBOutlet NSLayoutConstraint * sideButtonsAreaKeyboard;
@property(strong, nonatomic) IBOutlet UIView * carplayPlaceholderView;
@property(strong, nonatomic) BookmarksCoordinator * bookmarksCoordinator;

@property(strong, nonatomic) NSHashTable<id<MWMLocationModeListener>> * listeners;

@property(nonatomic) BOOL needDeferFocusNotification;
@property(nonatomic) BOOL deferredFocusValue;
@property(nonatomic) PlacePageViewController * placePageVC;

@property(nonatomic) NSLayoutConstraint * placePageWidthConstraint;
@property(nonatomic) NSLayoutConstraint * placePageLeadingConstraint;
@property(nonatomic) NSLayoutConstraint * placePageTrailingConstraint;

@property(nonatomic) NSUInteger mapAccessibilityRevision;
@property(nonatomic) NSUInteger mapAccessibilityAppliedRevision;
@property(nonatomic) NSTimer * mapAccessibilityRefreshTimer;
@property(nonatomic, weak) MWMMapFeatureAccessibilityElement * lastActivatedMapAccessibilityElement;

- (void)scheduleMapAccessibilityRefresh;
- (void)stopMapAccessibilityUpdates;
- (void)applyMapAccessibilitySnapshot:(df::AccessibleMapSnapshot &)snapshot;

@property(nonatomic, readwrite) BOOL isMapVisible;

@end

@implementation MapViewController
{
  std::shared_ptr<MapAccessibilityRenderState> m_accessibilityRenderState;
  uint64_t m_accessibilityAppliedRenderRevision;
}
@synthesize mapView, controlsView;

+ (MapViewController *)sharedController
{
  return [MapsAppDelegate theApp].mapViewController;
}

#pragma mark - PlacePage

- (void)showOrUpdatePlacePage:(PlacePageData *)data
{
  // Map API requests deselect and select in one call: remove the closing page before showing the new one.
  [self.placePageVC finishClosingIfNeeded];

  if (self.searchManager.isSearching)
    [self.searchManager setPlaceOnMapSelected:YES];

  self.controlsManager.trafficButtonHidden = YES;
  if (self.placePageVC != nil)
  {
    [PlacePageBuilder update:self.placePageVC with:data];
    return;
  }

  [self showPlacePageFor:data];
}

- (void)showPlacePageFor:(PlacePageData *)data
{
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

- (void)setupPlacePageContainer
{
  self.placePageContainer = [[TouchTransparentView alloc] initWithFrame:self.view.bounds];
  self.placePageContainer.translatesAutoresizingMaskIntoConstraints = NO;
  [self.view addSubview:self.placePageContainer];
  [self.view bringSubviewToFront:self.placePageContainer];

  self.placePageLeadingConstraint =
      [self.placePageContainer.leadingAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.leadingAnchor
                                                            constant:IPAD ? kPlacePageLeadingOffset : 0];
  if (IPAD)
    self.placePageLeadingConstraint.priority = UILayoutPriorityDefaultLow;

  self.placePageWidthConstraint =
      [self.placePageContainer.widthAnchor constraintEqualToConstant:kPlacePageCompactWidth];
  self.placePageTrailingConstraint =
      [self.placePageContainer.trailingAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.trailingAnchor];

  NSLayoutConstraint * topConstraint =
      [self.placePageContainer.topAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.topAnchor];

  NSLayoutConstraint * bottomConstraint;
  if (IPAD)
    bottomConstraint = [self.placePageContainer.bottomAnchor constraintLessThanOrEqualToAnchor:self.view.bottomAnchor];
  else
    bottomConstraint = [self.placePageContainer.bottomAnchor constraintEqualToAnchor:self.view.bottomAnchor];

  [NSLayoutConstraint activateConstraints:@[
    self.placePageLeadingConstraint,
    topConstraint,
    bottomConstraint,
  ]];

  [self updatePlacePageContainerConstraints];
}

- (void)setupSearchContainer
{
  if (self.searchContainer != nil)
  {
    [self.view bringSubviewToFront:self.searchContainer];
    return;
  }
  self.searchContainer = [[TouchTransparentView alloc] initWithFrame:self.view.bounds];
  [self.view addSubview:self.searchContainer];
  [self.view bringSubviewToFront:self.searchContainer];
  self.searchContainer.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
}

- (void)updatePlacePageContainerConstraints
{
  const BOOL isLimitedWidth = IPAD || self.traitCollection.verticalSizeClass == UIUserInterfaceSizeClassCompact;

  if (IPAD)
  {
    if (self.searchViewAvailableArea)
    {
      NSLayoutConstraint * leadingToSearchConstraint = [self.placePageContainer.leadingAnchor
          constraintGreaterThanOrEqualToAnchor:self.searchViewAvailableArea.trailingAnchor
                                      constant:kPlacePageLeadingOffset];
      leadingToSearchConstraint.priority = UILayoutPriorityDefaultHigh;
      leadingToSearchConstraint.active = isLimitedWidth;
    }
  }

  if (isLimitedWidth)
  {
    [self.placePageTrailingConstraint setActive:NO];
    [self.placePageWidthConstraint setActive:YES];
  }
  else
  {
    [self.placePageWidthConstraint setActive:NO];
    [self.placePageTrailingConstraint setActive:YES];
  }
  [self.view layoutIfNeeded];
}

- (void)dismissPlacePage
{
  GetFramework().DeactivateMapSelection();
}

- (BOOL)isMapFullyVisible
{
  BOOL const isMapVisible = self.isMapVisible && self.navigationController.visibleViewController == self;
  BOOL const isPlacePageClosed = self.placePageVC == nil;
  BOOL const isNavigationDashboardClosed = self.navigationDashboardManager.state == MWMNavigationDashboardStateClosed;
  BOOL const isSearchingClosed = !self.searchManager.isSearching;
  BOOL const isMapDownloadDialogClosed = self.downloadDialog.superview == nil;
  BOOL const isAlertControllerClosed = !self.alertController.isAlertDisplayed;
  return isMapVisible && isPlacePageClosed && isNavigationDashboardClosed && isSearchingClosed &&
         isMapDownloadDialogClosed && isAlertControllerClosed;
}

- (void)hideRegularPlacePage
{
  [self stopObservingTrackRecordingUpdates];
  [self.placePageVC closeWithCompletion:^{
    [self.placePageVC willMoveToParentViewController:nil];
    [self.placePageVC.view removeFromSuperview];
    [self.placePageVC removeFromParentViewController];
    self.placePageVC = nil;
    self.placePageContainer.hidden = YES;
    [self scheduleMapAccessibilityRefresh];
  }];
}

- (void)hidePlacePage
{
  if (self.placePageVC != nil)
    [self hideRegularPlacePage];
  self.controlsManager.trafficButtonHidden = NO;
}

- (void)onMapObjectDeselected
{
  [self hidePlacePage];
  [self scheduleMapAccessibilityRefresh];
  BOOL const isSearching = self.searchManager.isSearching;
  BOOL const isNavigationDashboardHidden = self.navigationDashboardManager.state == MWMNavigationDashboardStateHidden ||
                                           self.navigationDashboardManager.state == MWMNavigationDashboardStateClosed;
  if (isSearching)
    [self.searchManager setPlaceOnMapSelected:!isNavigationDashboardHidden && PlacePageData.hasData];
  else if (isNavigationDashboardHidden)
    [self.navigationDashboardManager onSelectPlacePage:NO];
  // Always show the controls during the navigation or planning mode.
  if (!isNavigationDashboardHidden)
    self.controlsManager.hidden = NO;
}

- (void)onSwitchFullScreen
{
  BOOL const isNavigationDashboardHidden = self.navigationDashboardManager.state == MWMNavigationDashboardStateClosed;
  if (!self.searchManager.isSearching && isNavigationDashboardHidden)
  {
    if (!self.controlsManager.hidden)
    {
      [self dismissPlacePage];
      [Toast showWithText:L(@"long_tap_toast")];
    }
    self.controlsManager.hidden = !self.controlsManager.hidden;
  }
}

- (void)onMapObjectSelected
{
  if (!PlacePageData.hasData)
    return;
  self.mapView.accessibilityElementsHidden = UIAccessibilityIsVoiceOverRunning();
  [self.navigationDashboardManager onSelectPlacePage:YES];
  PlacePageData * data = [[PlacePageData alloc] initWithLocalizationProvider:[[OpeinigHoursLocalization alloc] init]];
  [self stopObservingTrackRecordingUpdates];
  [self showOrUpdatePlacePage:data];
}

- (void)onMapObjectUpdated
{
  if (!PlacePageData.hasData)
  {
    [self onMapObjectDeselected];
    return;
  }
  PlacePageData * data = [[PlacePageData alloc] initWithLocalizationProvider:[[OpeinigHoursLocalization alloc] init]];
  [self showOrUpdatePlacePage:data];
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

- (void)sendTouchType:(df::TouchEvent::ETouchType)type withTouches:(NSSet *)touches andEvent:(UIEvent *)event
{
  if ([MWMCarPlayService shared].isCarplayActivated)
    return;

  NSArray * allTouches = [[event allTouches] allObjects];
  if ([allTouches count] < 1)
    return;

  UIView * v = self.mapView;
  CGFloat const scaleFactor = v.contentScaleFactor;

  df::TouchEvent e;
  UITouch * touch = [allTouches objectAtIndex:0];
  CGPoint const pt = [touch locationInView:v];

  if (self.searchManager.isSearching && type == df::TouchEvent::TOUCH_MOVE)
    [self.searchManager setMapIsDragging];

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
}

- (void)updateMapFontScaleFactor
{
  double const scaleFactor =
      [MapFontScaleFactor valueForContentSizeCategory:self.traitCollection.preferredContentSizeCategory];
  [MWMFrameworkHelper setMapFontScaleFactor:scaleFactor];
}

- (void)traitCollectionDidChange:(UITraitCollection *)previousTraitCollection
{
  [super traitCollectionDidChange:previousTraitCollection];
  if (self.traitCollection.verticalSizeClass != previousTraitCollection.verticalSizeClass)
    [self updatePlacePageContainerConstraints];

  if (previousTraitCollection == nil || ![previousTraitCollection.preferredContentSizeCategory
                                            isEqualToString:self.traitCollection.preferredContentSizeCategory])
    [self updateMapFontScaleFactor];
}

- (void)didReceiveMemoryWarning
{
  GetFramework().MemoryWarning();
  [super didReceiveMemoryWarning];
}

- (void)onTerminate
{
  [self.mapView deallocateNative];
}

- (void)onGetFocus:(BOOL)isOnFocus
{
  self.needDeferFocusNotification = (self.mapView == nil);
  self.deferredFocusValue = isOnFocus;
  [self.mapView setPresentAvailable:isOnFocus];
}

- (void)viewWillAppear:(BOOL)animated
{
  [super viewWillAppear:animated];

  [self updateMapFontScaleFactor];

  if (self.navigationDashboardManager.state == MWMNavigationDashboardStateClosed)
    self.controlsManager.menuState = self.controlsManager.closedMenuState;

  [self updateStatusBarStyle];
  GetFramework().SetRenderingEnabled();
  GetFramework().InvalidateRendering();
  [self showViralAlertIfNeeded];
  [self checkAuthorization];
  [MWMRouter updateRoute];
  self.isMapVisible = YES;
}

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.mapView.isAccessibilityElement = NO;
  __weak MapViewController * weakSelf = self;
  self.mapView.accessibilityHitTestHandler = ^id(CGPoint point) {
    MapViewController * controller = weakSelf;
    if (controller == nil || ![controller isMapFullyVisible])
      return nil;

    MWMMapFeatureAccessibilityElement * selected = nil;
    std::tuple<NSInteger, NSInteger, CGFloat> bestScore;
    for (UIAccessibilityElement * candidate in controller.mapView.accessibilityElements)
    {
      if (![candidate isKindOfClass:[MWMMapFeatureAccessibilityElement class]])
        continue;
      auto * mapElement = (MWMMapFeatureAccessibilityElement *)candidate;
      NSInteger const rank = [mapElement hitRankAtScreenPoint:point];
      if (rank == 0)
        continue;
      auto const score =
          std::tuple{rank, mapElement.mapSelectionPriority, -[mapElement squaredDistanceToScreenPoint:point]};
      if (score > bestScore)
      {
        selected = mapElement;
        bestScore = score;
      }
    }
    [selected setMapExplorationPoint:point];
    return selected;
  };
  [[NSNotificationCenter defaultCenter] addObserver:self
                                           selector:@selector(onVoiceOverStatusChanged)
                                               name:UIAccessibilityVoiceOverStatusDidChangeNotification
                                             object:nil];
  [self.searchManager addObserver:self];
  [self setupPlacePageContainer];
  [self setupSearchContainer];

  [self setupTrackPadGestureRecognizers];

  self.title = L(@"map");

  // mapView can be uninitialized when onGetFocus is called, which can lead to missing
  // the onGetFocus call and a deadlock on start. Defer the call until mapView exists.
  if (self.needDeferFocusNotification)
    [self onGetFocus:self.deferredFocusValue];

  [MWMRouter restoreRouteIfNeeded];

  self.view.clipsToBounds = YES;
  [MWMKeyboard addObserver:self];

  if ([FirstSession isFirstSession])
  {
    [MWMLocationManager start];
    dispatch_async(dispatch_get_main_queue(),
                   ^{ [MWMFrameworkHelper processFirstLaunch:[MWMLocationManager isStarted]]; });
  }
  else
  {
    [self processMyPositionStateModeEvent:[MWMLocationManager isLocationProhibited]
                                              ? MWMMyPositionModeNotFollowNoPosition
                                              : MWMMyPositionModePendingPosition];
  }

  if (self.navigationDashboardManager.state == MWMNavigationDashboardStateClosed)
    self.controlsManager.menuState = self.controlsManager.closedMenuState;

  // Added in https://github.com/organicmaps/organicmaps/pull/7333
  // After all users migrate to OAuth2 we can remove next code
  [self migrateOAuthCredentials];

  if (self.trackRecordingManager.isActive)
    [self showTrackRecordingPlacePage];

  /// @todo: Uncomment update dialog when will be ready to handle big traffic bursts.
  /*
  if (!DeepLinkHandler.shared.hasPendingColdLaunchDeepLink)
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

- (void)viewDidAppear:(BOOL)animated
{
  [super viewDidAppear:animated];
  // Cold start deep links should be handled when the map is initialized.
  // Otherwise PP container view is nil, or there is no animation/selection of the point.
  if (DeepLinkHandler.shared.hasPendingColdLaunchDeepLink)
    (void)[DeepLinkHandler.shared handlePendingDeepLink];
  [self scheduleMapAccessibilityRefresh];
}

- (void)viewDidLayoutSubviews
{
  [super viewDidLayoutSubviews];
  if (!self.mapView.drapeEngineCreated && !MapsAppDelegate.isTestsEnvironment)
  {
    [self.mapView createDrapeEngine];
    [self scheduleMapAccessibilityRefresh];
  }
}

- (void)applyTheme
{
  [super applyTheme];
  [MapsAppDelegate customizeAppearance];
}

- (void)setupTrackPadGestureRecognizers
{
  if (!NSProcessInfo.processInfo.isiOSAppOnMac)
    return;
  // Mouse zoom
  UIPanGestureRecognizer * panRecognizer = [[UIPanGestureRecognizer alloc] initWithTarget:self
                                                                                   action:@selector(handlePan:)];
  panRecognizer.allowedScrollTypesMask = UIScrollTypeMaskAll;
  panRecognizer.allowedTouchTypes = @[@(UITouchTypeIndirect)];
  [self.view addGestureRecognizer:panRecognizer];

  // Trackpad zoom
  UIPinchGestureRecognizer * pinchRecognizer =
      [[UIPinchGestureRecognizer alloc] initWithTarget:self action:@selector(handlePinch:)];
  pinchRecognizer.allowedTouchTypes = @[@(UITouchTypeIndirect)];
  pinchRecognizer.delegate = self;
  [self.view addGestureRecognizer:pinchRecognizer];

  // Trackpad rotation
  UIRotationGestureRecognizer * rotationRecognizer =
      [[UIRotationGestureRecognizer alloc] initWithTarget:self action:@selector(handleRotation:)];
  rotationRecognizer.allowedTouchTypes = @[@(UITouchTypeIndirect | UITouchTypeDirect)];
  rotationRecognizer.delegate = self;
  [self.view addGestureRecognizer:rotationRecognizer];

  // Pointer location
  UIHoverGestureRecognizer * hoverRecognizer =
      [[UIHoverGestureRecognizer alloc] initWithTarget:self action:@selector(handlePointerHover:)];
  hoverRecognizer.allowedTouchTypes = @[@(UITouchTypeIndirectPointer)];
  [self.view addGestureRecognizer:hoverRecognizer];
}

- (void)showViralAlertIfNeeded
{
  NSUserDefaults * ud = NSUserDefaults.standardUserDefaults;

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

- (void)viewWillDisappear:(BOOL)animated
{
  [super viewWillDisappear:animated];
  [self stopMapAccessibilityUpdates];

  // The shared EAGLView may already be hosted by CarPlay while the phone controller disappears.
  if (![MWMCarPlayService shared].isCarplayActivated)
    GetFramework().SetRenderingDisabled(false);
  self.isMapVisible = NO;
}

- (BOOL)prefersStatusBarHidden
{
  return NO;
}
- (UIStatusBarStyle)preferredStatusBarStyle
{
  MWMMapViewControlsManager * manager = _controlsManager;
  if (manager)
    return manager.preferredStatusBarStyle;
  return UIStatusBarStyleDefault;
}

- (void)updateStatusBarStyle
{
  [self setNeedsStatusBarAppearanceUpdate];
}

- (void)migrateOAuthCredentials
{
  if (osm_auth_ios::AuthorizationHaveOAuth1Credentials())
  {
    osm_auth_ios::AuthorizationClearOAuth1Credentials();
    [self.alertController presentOsmReauthAlert];
  }
}

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
  self.listeners = [NSHashTable<id<MWMLocationModeListener>> weakObjectsHashTable];
  self.availableAreaInsetsMap = [NSMapTable weakToStrongObjectsMapTable];
  Framework & f = GetFramework();
  // TODO: Review and improve this code.
  f.SetPlacePageListeners([self]() { [self onMapObjectSelected]; }, [self]() { [self onMapObjectDeselected]; },
                          [self]() { [self onMapObjectUpdated]; }, [self]() { [self onSwitchFullScreen]; });
  // TODO: Review and improve this code.
  f.SetMyPositionModeListener([self](location::EMyPositionMode mode, bool routingActive)
  {
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

- (void)dealloc
{
  self.mapView.accessibilityHitTestHandler = nil;
  [self.mapAccessibilityRefreshTimer invalidate];
  [_searchManager removeObserver:self];
  [[NSNotificationCenter defaultCenter] removeObserver:self
                                                  name:UIAccessibilityVoiceOverStatusDidChangeNotification
                                                object:nil];
  [[MWMBookmarksManager sharedManager] removeObserver:self];
  [MWMFrameworkListener removeObserver:self];
}

- (void)addListener:(id<MWMLocationModeListener>)listener
{
  [self.listeners addObject:listener];
}

- (void)removeListener:(id<MWMLocationModeListener>)listener
{
  [self.listeners removeObject:listener];
}

#pragma mark - Open controllers
- (void)openMenu
{
  // The menu belongs to the map, so a link that arrives over another screen returns to the map first.
  // Open it after the pop: viewWillAppear restores the closed state of the map controls.
  UINavigationController * navigationController = self.navigationController;
  [navigationController popToRootViewControllerAnimated:YES];
  id<UIViewControllerTransitionCoordinator> coordinator = navigationController.transitionCoordinator;
  if (!coordinator)
  {
    [self.controlsManager openMenuFromDeepLink];
    return;
  }
  // A link handled during the pop replaces this one, so the menu must not cover its destination.
  NSURL * url = DeepLinkHandler.shared.url;
  [coordinator animateAlongsideTransition:nil
                               completion:^(id<UIViewControllerTransitionCoordinatorContext>) {
                                 if ([DeepLinkHandler.shared.url isEqual:url])
                                   [self.controlsManager openMenuFromDeepLink];
                               }];
}

- (void)refreshMapAccessibilityAfterMenuDismissal
{
  if (UIAccessibilityIsVoiceOverRunning())
    [self scheduleMapAccessibilityRefresh];
}

- (void)openSettings
{
  // Opening Settings from a link or the menu also closes the menu or layers sheet covering the map.
  MWMMapViewControlsManager * controlsManager = self.controlsManager;
  MWMBottomMenuState const menuState = controlsManager.menuState;
  if (menuState == MWMBottomMenuStateActive || menuState == MWMBottomMenuStateLayers)
    controlsManager.menuState = controlsManager.closedMenuState;
  // Switching apps to open a link outlasts the short menu/Layers animations,
  // so normal cross-app links need no modal-transition queue.
  [SettingsBuilder showRootIn:self.navigationController];
}

- (void)openMapsDownloader:(MWMMapDownloaderMode)mode
{
  [self performSegueWithIdentifier:kDownloaderSegue sender:@(mode)];
}

- (void)openEditor
{
  [self performSegueWithIdentifier:kEditorSegue sender:self.controlsManager.featureHolder];
}

- (void)openBookmarkEditor
{
  [self performSegueWithIdentifier:kPP2BookmarkEditingSegue sender:nil];
}

- (void)openFullPlaceDescriptionWithHtml:(NSString *)htmlString
{
  WebViewController * descriptionViewController =
      [[PlacePageDescriptionViewController alloc] initWithHtml:htmlString
                                                       baseUrl:nil
                                                         title:L(@"place_description_title")];
  descriptionViewController.openInSafari = YES;
  [self.navigationController pushViewController:descriptionViewController animated:YES];
}

- (void)openDrivingOptions
{
  [self.navigationController pushViewController:[SettingsBuilder buildRoutingOptionsWithRouterType:MWMRouter.renderType]
                                       animated:YES];
}

- (void)processMyPositionStateModeEvent:(MWMMyPositionMode)mode
{
  self.currentPositionMode = mode;
  [MWMLocationManager setMyPositionMode:mode];
  [[MWMSideButtons buttons] processMyPositionStateModeEvent:mode];
  NSArray<id<MWMLocationModeListener>> * objects = self.listeners.allObjects;
  for (id<MWMLocationModeListener> object in objects)
    [object processMyPositionStateModeEvent:mode];
  self.disableStandbyOnLocationStateMode = NO;
  switch (mode)
  {
  case MWMMyPositionModeNotFollowNoPosition: [MWMLocationManager stop]; break;
  case MWMMyPositionModePendingPosition:
    [MWMLocationManager start];
    if (![MWMLocationManager isStarted])
      [self processMyPositionStateModeEvent:MWMMyPositionModeNotFollowNoPosition];
    break;
  case MWMMyPositionModeNotFollow: break;
  case MWMMyPositionModeFollow:
  case MWMMyPositionModeFollowAndRotate: self.disableStandbyOnLocationStateMode = YES; break;
  }
}

#pragma mark - MWMFrameworkDrapeObserver

- (void)onVoiceOverStatusChanged
{
  [self scheduleMapAccessibilityRefresh];
}

- (void)searchManagerWithDidChangeState:(SearchOnMapState)state
{
  if (!UIAccessibilityIsVoiceOverRunning())
    return;

  __weak MapViewController * weakSelf = self;
  dispatch_async(dispatch_get_main_queue(), ^{ [weakSelf scheduleMapAccessibilityRefresh]; });
}

- (void)stopMapAccessibilityUpdates
{
  if (m_accessibilityRenderState)
  {
    m_accessibilityRenderState.reset();
    if (auto engine = GetFramework().GetDrapeEngine())
      engine->SetAccessibilityObjectsChangedListener({});
    GetFramework().GetBookmarkManager().SetBookmarksChangedCallback({});
  }
  [self.mapAccessibilityRefreshTimer invalidate];
  self.mapAccessibilityRefreshTimer = nil;
  self.mapView.accessibilityElements = nil;
  self.mapView.accessibilityElementsHidden = NO;
  self.lastActivatedMapAccessibilityElement = nil;
  ++self.mapAccessibilityRevision;
}

- (void)scheduleMapAccessibilityRefresh
{
  if (!UIAccessibilityIsVoiceOverRunning() || !self.isMapVisible || self.mapView.superview != self.view)
  {
    if (m_accessibilityRenderState)
      [self stopMapAccessibilityUpdates];
    return;
  }

  auto engine = GetFramework().GetDrapeEngine();
  if (!engine)
    return;

  __weak MapViewController * weakSelf = self;
  if (!m_accessibilityRenderState)
  {
    auto const state = std::make_shared<MapAccessibilityRenderState>();
    m_accessibilityRenderState = state;
    engine->SetAccessibilityObjectsChangedListener([weakSelf, state]
    {
      ++state->m_revision;
      if (state->m_notificationPending.exchange(true))
        return;
      dispatch_async(dispatch_get_main_queue(), ^{
        state->m_notificationPending = false;
        MapViewController * controller = weakSelf;
        if (controller && controller->m_accessibilityRenderState == state)
          [controller scheduleMapAccessibilityRefresh];
      });
    });
    GetFramework().GetBookmarkManager().SetBookmarksChangedCallback([weakSelf]
    { [weakSelf scheduleMapAccessibilityRefresh]; });
  }

  [self.mapAccessibilityRefreshTimer invalidate];
  self.mapAccessibilityRefreshTimer = nil;
  NSUInteger const revision = ++self.mapAccessibilityRevision;
  self.mapView.accessibilityElementsHidden = ![self isMapFullyVisible];
  if (self.mapView.accessibilityElementsHidden)
    return;

  // A renderer update can arrive during map movement or after a stationary viewport's tiles load.
  self.mapAccessibilityRefreshTimer =
      [NSTimer timerWithTimeInterval:0.2
                             repeats:NO
                               block:^(NSTimer * timer) { [weakSelf requestMapAccessibilitySnapshot:revision]; }];
  [NSRunLoop.mainRunLoop addTimer:self.mapAccessibilityRefreshTimer forMode:NSRunLoopCommonModes];
}

- (void)requestMapAccessibilitySnapshot:(NSUInteger)revision
{
  self.mapAccessibilityRefreshTimer = nil;
  auto const state = m_accessibilityRenderState;
  if (!state || revision != self.mapAccessibilityRevision || !UIAccessibilityIsVoiceOverRunning() ||
      ![self isMapFullyVisible] || self.mapView.superview != self.view)
    return;

  auto engine = GetFramework().GetDrapeEngine();
  if (!engine)
    return;

  __weak MapViewController * weakSelf = self;
  engine->RequestVisibleAccessibilityObjects([weakSelf, state, revision](df::AccessibleMapSnapshot && snapshot)
  {
    auto result = std::make_shared<df::AccessibleMapSnapshot>(std::move(snapshot));
    uint64_t const renderRevision = state->m_revision;
    dispatch_async(dispatch_get_main_queue(), ^{
      MapViewController * controller = weakSelf;
      if (!controller || controller->m_accessibilityRenderState != state || state->m_revision != renderRevision ||
          controller.mapAccessibilityRevision != revision || !UIAccessibilityIsVoiceOverRunning() ||
          ![controller isMapFullyVisible] || controller.mapView.superview != controller.view)
        return;

      auto tracks = GetFramework().GetAccessibleTrackObjects(*result);
      if (state->m_revision != renderRevision)
        return;
      auto & objects = result->m_objects;
      objects.insert(objects.end(), std::make_move_iterator(tracks.begin()), std::make_move_iterator(tracks.end()));
      [controller applyMapAccessibilitySnapshot:*result];
      controller->m_accessibilityAppliedRenderRevision = renderRevision;
      controller.mapAccessibilityAppliedRevision = revision;
    });
  });
}

- (void)applyMapAccessibilitySnapshot:(df::AccessibleMapSnapshot &)snapshot
{
  auto & objects = snapshot.m_objects;
  auto & framework = GetFramework();

  auto const identity = [](df::AccessibleMapObject const & object)
  { return MapAccessibilityID{object.m_featureId, object.m_markId, object.m_trackId}; };
  std::sort(objects.begin(), objects.end(), [&identity](auto const & lhs, auto const & rhs)
  {
    return std::tuple{lhs.m_pixelRect.minY(), lhs.m_pixelRect.minX(), identity(lhs)} <
           std::tuple{rhs.m_pixelRect.minY(), rhs.m_pixelRect.minX(), identity(rhs)};
  });

  std::map<MapAccessibilityID, MWMMapFeatureAccessibilityElement *> previous;
  for (MWMMapFeatureAccessibilityElement * element in self.mapView.accessibilityElements)
    previous.emplace([element mapIdentity], element);

  CGRect const viewport =
      snapshot.m_pixelViewport.IsValid()
          ? CGRectIntersection(MapAccessibilityFrame(snapshot.m_pixelViewport, self.mapView.contentScaleFactor),
                               self.mapView.bounds)
          : CGRectNull;
  NSMutableArray<MWMMapFeatureAccessibilityElement *> * elements = [NSMutableArray arrayWithCapacity:objects.size()];
  MWMMapFeatureAccessibilityElement * returnFocus = nil;
  for (auto & object : objects)
  {
    if (CGRectIsNull(viewport) ||
        !framework.IsAccessibleMapObjectCurrent(object.m_featureId, object.m_markId, object.m_trackId, snapshot.m_zoom))
      continue;
    auto const id = identity(object);
    auto const existing = previous.find(id);
    MWMMapFeatureAccessibilityElement * element =
        existing == previous.end()
            ? [[MWMMapFeatureAccessibilityElement alloc] initWithContainer:self.mapView controller:self identity:id]
            : existing->second;
    [element updateWithObject:object
                      mapView:self.mapView
                       screen:snapshot.m_screen
                     viewport:viewport
                     revision:self.mapAccessibilityRevision
                         zoom:snapshot.m_zoom];
    [elements addObject:element];
    if (element == self.lastActivatedMapAccessibilityElement)
      returnFocus = element;
  }
  if (![elements isEqualToArray:self.mapView.accessibilityElements])
    self.mapView.accessibilityElements = elements;

  if (returnFocus)
    UIAccessibilityPostNotification(UIAccessibilityLayoutChangedNotification, returnFocus);
  self.lastActivatedMapAccessibilityElement = nil;
}

- (BOOL)activateMapAccessibilityObject:(MapAccessibilityID const &)identity
                              mercator:(m2::PointD const &)mercator
                              revision:(NSUInteger)revision
                                  zoom:(int)zoom
{
  if (![self isMapFullyVisible] || !m_accessibilityRenderState ||
      m_accessibilityRenderState->m_revision != m_accessibilityAppliedRenderRevision ||
      revision != self.mapAccessibilityRevision || revision != self.mapAccessibilityAppliedRevision)
    return NO;

  auto & framework = GetFramework();
  auto const & [featureId, markId, trackId] = identity;
  if (!framework.IsAccessibleMapObjectCurrent(featureId, markId, trackId, zoom))
  {
    [self scheduleMapAccessibilityRefresh];
    return NO;
  }

  for (MWMMapFeatureAccessibilityElement * element in self.mapView.accessibilityElements)
  {
    if ([element mapIdentity] == identity)
    {
      self.lastActivatedMapAccessibilityElement = element;
      break;
    }
  }

  place_page::BuildInfo info;
  info.m_source = place_page::BuildInfo::Source::User;
  info.m_featureId = featureId;
  info.m_userMarkId = markId;
  info.m_trackId = trackId;
  info.m_mercator = mercator;
  if (trackId != kml::kInvalidTrackId)
  {
    info.m_match = place_page::BuildInfo::Match::TrackOnly;
    info.m_trackSelectionPoint = mercator;
  }
  else if (markId != kml::kInvalidMarkId)
  {
    info.m_match = place_page::BuildInfo::Match::UserMarkOnly;
  }
  else
  {
    info.m_match = place_page::BuildInfo::Match::FeatureOnly;
  }
  framework.BuildAndSetPlacePageInfo(info);
  return YES;
}

- (void)processViewportCountryEvent:(CountryId const &)countryId
{
  [self.downloadDialog processViewportCountryEvent:countryId];
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
    [self.alertController presentOsmAuthAlert];
  }
}

#pragma mark - 3d touch

- (void)performAction:(NSString *)action
{
  [self.navigationController popToRootViewControllerAnimated:NO];
  if (self.isViewLoaded)
  {
    [MWMRouter stopRouting];
    if ([action isEqualToString:@"app.organicmaps.3daction.bookmarks"])
      [self.bookmarksCoordinator open];
    else if ([action isEqualToString:@"app.organicmaps.3daction.search"])
      [self.searchManager startSearchingWithIsRouting:NO];
    else if ([action isEqualToString:@"app.organicmaps.3daction.route"])
      [self.controlsManager onRoutePrepare];
    else if ([action isEqualToString:@"app.organicmaps.3daction.report_bug"])
      [MailComposer sendBugReportWithTitle:@"Bug Report / Organic Maps"];
  }
  else
    dispatch_async(dispatch_get_main_queue(), ^{ [self performAction:action]; });
}

#pragma mark - ShowDialog callback

- (void)presentDisabledLocationAlert
{
  [self.alertController presentDisabledLocationAlert];
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
    [dvc setFeatureToEdit:static_cast<id<MWMFeatureHolder>>(sender).featureId];
  }
  else if ([segue.identifier isEqualToString:kDownloaderSegue])
  {
    MWMDownloadMapsViewController * dvc = segue.destinationViewController;
    NSNumber * mode = sender;
    dvc.mode = (MWMMapDownloaderMode)mode.integerValue;
  }
  else if ([segue.identifier isEqualToString:kCategorySelectorSegue])
  {
    MWMObjectsCategorySelectorController * dvc = segue.destinationViewController;
    m2::PointD position;
    [(NSValue *)sender getValue:&position];
    [dvc setCreatedPosition:position];
  }
}

#pragma mark - MWMKeyboard

- (void)onKeyboardWillAnimate
{
  [self.view setNeedsLayout];
}
- (void)onKeyboardAnimation
{
  auto const kbHeight = [MWMKeyboard keyboardHeight];
  self.sideButtonsAreaKeyboard.constant = kbHeight;
  if (IPAD)
  {
    self.visibleAreaKeyboard.constant = kbHeight;
    self.placePageAreaKeyboard.constant = kbHeight;
  }
}
#pragma mark - Properties

- (MWMMapViewControlsManager *)controlsManager
{
  if (!self.isViewLoaded)
  {
    // TODO: Returns nil when called from MapViewController.preferredStatusBarStyle.
    return nil;
  }
  if (!_controlsManager)
    _controlsManager = [[MWMMapViewControlsManager alloc] initWithParentController:self];
  return _controlsManager;
}

- (SearchOnMapManager *)searchManager
{
  if (!_searchManager)
    _searchManager = [[SearchOnMapManager alloc] init];
  return _searchManager;
}

- (TrackRecordingManager *)trackRecordingManager
{
  if (!_trackRecordingManager)
    _trackRecordingManager = TrackRecordingManager.shared;
  return _trackRecordingManager;
}

- (MWMNavigationDashboardManager *)navigationDashboardManager
{
  return MWMNavigationDashboardManager.sharedManager;
}

- (UIView * _Nullable)searchViewAvailableArea
{
  return self.searchManager.viewController.availableAreaView;
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

- (void)updateVisibleAreaInsetsFor:(NSObject * _Nonnull)object
                            insets:(UIEdgeInsets)insets
                  updatingViewport:(BOOL)updateViewport
{
  if (object == nil)
    return;
  [self.availableAreaInsetsMap setObject:[NSValue valueWithUIEdgeInsets:insets] forKey:object];

  UIEdgeInsets availableAreaInsets = [self availableAreaInsets];

  if (updateViewport)
  {
    self.visibleAreaBottom.constant = availableAreaInsets.bottom;
    self.visibleAreaLeading.constant = availableAreaInsets.left;
    self.visibleAreaTrailing.constant = availableAreaInsets.right;
  }
  self.sideButtonsAreaBottom.constant = availableAreaInsets.bottom;
  self.sideButtonsAreaCompactBottom.constant = availableAreaInsets.bottom;
}

- (UIEdgeInsets)availableAreaInsets
{
  CGRect bounds = self.view.bounds;

  CGFloat top = 0;
  CGFloat left = 0;
  CGFloat bottom = 0;
  CGFloat right = 0;

  for (id key in self.availableAreaInsetsMap)
  {
    NSValue * value = [self.availableAreaInsetsMap objectForKey:key];
    if (!value)
      continue;

    UIEdgeInsets areaInsets = [value UIEdgeInsetsValue];

    top = MAX(top, areaInsets.top);
    left = MAX(left, areaInsets.left);
    bottom = MAX(bottom, areaInsets.bottom);
    right = MAX(right, areaInsets.right);
  }

  if (top + bottom >= CGRectGetHeight(bounds) || left + right >= CGRectGetWidth(bounds))
    return UIEdgeInsetsZero;

  return UIEdgeInsetsMake(top, left, bottom, right);
}

+ (void)setViewport:(double)lat lon:(double)lon zoomLevel:(int)zoomLevel
{
  Framework & f = GetFramework();

  f.StopLocationFollow();

  auto const center = mercator::FromLatLon(lat, lon);
  f.SetViewportCenter(center, zoomLevel, false);
}

- (BookmarksCoordinator *)bookmarksCoordinator
{
  if (!_bookmarksCoordinator)
    _bookmarksCoordinator = [[BookmarksCoordinator alloc] initWithNavigationController:self.navigationController];
  return _bookmarksCoordinator;
}

#pragma mark - CarPlay map append/remove

- (void)disableCarPlayRepresentation
{
  self.carplayPlaceholderView.accessibilityElementsHidden = YES;
  self.carplayPlaceholderView.hidden = YES;
  self.mapView.frame = self.view.bounds;
  [self.view insertSubview:self.mapView atIndex:0];
  [[self.mapView.topAnchor constraintEqualToAnchor:self.view.topAnchor] setActive:YES];
  [[self.mapView.bottomAnchor constraintEqualToAnchor:self.view.bottomAnchor] setActive:YES];
  [[self.mapView.leadingAnchor constraintEqualToAnchor:self.view.leadingAnchor] setActive:YES];
  [[self.mapView.trailingAnchor constraintEqualToAnchor:self.view.trailingAnchor] setActive:YES];
  self.controlsView.hidden = NO;
  [MWMFrameworkHelper setVisibleViewport:self.view.bounds scaleFactor:self.view.contentScaleFactor];
  [self scheduleMapAccessibilityRefresh];
}

- (void)enableCarPlayRepresentation
{
  [_searchManager close];
  if (![_controlsManager dismissBottomMenuIfPresentedAnimated:NO])
    [self.presentedViewController dismissViewControllerAnimated:NO completion:nil];
  [[MapsAppDelegate theApp] showMap];
  [self loadViewIfNeeded];
  [self.mapView removeFromSuperview];
  [self stopMapAccessibilityUpdates];
  if (!self.controlsView.isHidden)
    self.controlsView.hidden = YES;
  self.carplayPlaceholderView.hidden = NO;
  self.carplayPlaceholderView.accessibilityElementsHidden = NO;
}

#pragma mark - MWMBookmarksObserver
- (void)onBookmarksFileLoadSuccess
{
  [[MWMAlertViewController activeAlertController] presentInfoAlert:L(@"load_kmz_title") text:L(@"load_kmz_successful")];
}

- (void)onBookmarksFileLoadError
{
  [[MWMAlertViewController activeAlertController] presentInfoAlert:L(@"load_kmz_title") text:L(@"load_kmz_failed")];
}

- (BOOL)canBecomeFirstResponder
{
  return YES;
}

- (NSArray<UIKeyCommand *> *)keyCommands
{
  UIKeyCommand * downArrow = [UIKeyCommand keyCommandWithInput:UIKeyInputDownArrow
                                                 modifierFlags:0
                                                        action:@selector(zoomOut)];

  UIKeyCommand * zoomOut = [UIKeyCommand keyCommandWithInput:@"-"
                                               modifierFlags:UIKeyModifierCommand
                                                      action:@selector(zoomOut)];
  zoomOut.discoverabilityTitle = @"Zoom Out";

  UIKeyCommand * upArrow = [UIKeyCommand keyCommandWithInput:UIKeyInputUpArrow
                                               modifierFlags:0
                                                      action:@selector(zoomIn)];

  UIKeyCommand * zoomInEquals = [UIKeyCommand keyCommandWithInput:@"="
                                                    modifierFlags:UIKeyModifierCommand
                                                           action:@selector(zoomIn)];

  UIKeyCommand * zoomInPlus = [UIKeyCommand keyCommandWithInput:@"+"
                                                  modifierFlags:UIKeyModifierCommand
                                                         action:@selector(zoomIn)];
  zoomInPlus.discoverabilityTitle = @"Zoom In";

  UIKeyCommand * escape = [UIKeyCommand keyCommandWithInput:UIKeyInputEscape modifierFlags:0 action:@selector(goBack)];
  escape.discoverabilityTitle = @"Go Back";

  UIKeyCommand * switchPositionMode = [UIKeyCommand keyCommandWithInput:@"0"
                                                          modifierFlags:UIKeyModifierCommand
                                                                 action:@selector(switchPositionMode)];
  switchPositionMode.discoverabilityTitle = @"Switch position mode";

  NSArray<UIKeyCommand *> * commands =
      @[downArrow, zoomOut, upArrow, zoomInEquals, zoomInPlus, escape, switchPositionMode];

  for (UIKeyCommand * command in commands)
    command.wantsPriorityOverSystemBehavior = YES;

  return commands;
}

- (void)zoomOut
{
  GetFramework().Scale(Framework::SCALE_MIN, true);
}

- (void)zoomIn
{
  GetFramework().Scale(Framework::SCALE_MAG, true);
}

- (void)switchPositionMode
{
  GetFramework().SwitchMyPositionNextMode();
}

- (void)goBack
{
  NSString * backURL = [DeepLinkHandler.shared getBackUrl];
  if (backURL != nil)
    [[UIApplication sharedApplication] openURL:[NSURL URLWithString:backURL] options:@{} completionHandler:nil];
}

- (void)closeCurrentView
{
  if ([_controlsManager dismissBottomMenuIfPresentedAnimated:YES])
    return;

  if (self.presentedViewController)
  {
    [self dismissViewControllerAnimated:YES completion:nil];
    return;
  }

  if (self.navigationController.viewControllers.count > 1)
  {
    [self.navigationController popViewControllerAnimated:YES];
    return;
  }
}

// MARK: - Track Recording Place Page

- (void)showTrackRecordingPlacePage
{
  if ([self.trackRecordingManager contains:self])
  {
    [self dismissPlacePage];
    return;
  }
  PlacePageData * placePageData =
      [[PlacePageData alloc] initWithTrackInfo:self.trackRecordingManager.trackRecordingInfo
                                 elevationInfo:self.trackRecordingManager.trackRecordingElevationProfileData];
  [self.controlsManager setTrackRecordingButtonState:TrackRecordingButtonStateHidden];
  [self showOrUpdatePlacePage:placePageData];
  [self startObservingTrackRecordingUpdatesForPlacePageData:placePageData];
}

- (void)startObservingTrackRecordingUpdatesForPlacePageData:(PlacePageData *)placePageData
{
  __weak __typeof(self) weakSelf = self;
  [self.trackRecordingManager addObserver:self
        recordingIsActiveDidChangeHandler:^(TrackRecordingState state, TrackInfo * _Nonnull trackInfo,
                                            ElevationProfileData * _Nullable (^elevationData)()) {
          __strong __typeof(weakSelf) self = weakSelf;
          if (!self)
            return;
          switch (state)
          {
          case TrackRecordingStateInactive: [self stopObservingTrackRecordingUpdates]; break;
          case TrackRecordingStateActive:
            if (UIApplication.sharedApplication.applicationState != UIApplicationStateActive)
              return;
            [self.controlsManager setTrackRecordingButtonState:TrackRecordingButtonStateHidden];
            [placePageData updateWithTrackInfo:trackInfo elevationInfo:elevationData()];
            break;
          }
        }];
}

- (void)stopObservingTrackRecordingUpdates
{
  [self.trackRecordingManager removeObserver:self];
  // Saving can open the completed track while the recording place page still owns the button.
  [self.controlsManager setTrackRecordingButtonState:self.trackRecordingManager.isActive
                                                         ? TrackRecordingButtonStateVisible
                                                         : TrackRecordingButtonStateClosed];
}

// MARK: - Handle macOS trackpad gestures

- (void)handlePan:(UIPanGestureRecognizer *)recognizer
{
  switch (recognizer.state)
  {
  case UIGestureRecognizerStateBegan:
  case UIGestureRecognizerStateChanged:
  {
    CGPoint translation = [recognizer translationInView:self.view];
    if (translation.x == 0 && CGPointEqualToPoint(translation, CGPointZero))
      return;
    self.userTouchesAction = UserTouchesActionScale;
    static CGFloat const kScaleFactor = 0.9;
    CGFloat const factor = translation.y > 0 ? 1 / kScaleFactor : kScaleFactor;
    GetFramework().Scale(factor, [self getZoomPoint], false);
    [recognizer setTranslation:CGPointZero inView:self.view];
    break;
  }
  case UIGestureRecognizerStateEnded: self.userTouchesAction = UserTouchesActionNone; break;
  default: break;
  }
}

- (void)handlePinch:(UIPinchGestureRecognizer *)recognizer
{
  switch (recognizer.state)
  {
  case UIGestureRecognizerStateBegan: self.currentScale = 1.0;
  case UIGestureRecognizerStateChanged:
  {
    CGFloat const scale = [recognizer scale];
    static CGFloat const kScaleDeltaMultiplier = 4.0;  // map trackpad scale to the map scale
    CGFloat const delta = scale - self.currentScale;
    CGFloat const scaleFactor = 1 + delta * kScaleDeltaMultiplier;
    GetFramework().Scale(scaleFactor, [self getZoomPoint], false);
    self.currentScale = scale;
    break;
  }
  case UIGestureRecognizerStateEnded: self.userTouchesAction = UserTouchesActionNone; break;
  default: break;
  }
}

- (void)handleRotation:(UIRotationGestureRecognizer *)recognizer
{
  switch (recognizer.state)
  {
  case UIGestureRecognizerStateBegan:
  case UIGestureRecognizerStateChanged:
  {
    self.userTouchesAction = UserTouchesActionDrag;
    GetFramework().Rotate(self.currentRotation == 0 ? recognizer.rotation : self.currentRotation + recognizer.rotation,
                          false);
    break;
  }
  case UIGestureRecognizerStateEnded:
    self.currentRotation += recognizer.rotation;
    self.userTouchesAction = UserTouchesActionNone;
    break;
  default: break;
  }
}

- (void)handlePointerHover:(UIHoverGestureRecognizer *)recognizer
{
  self.pointerLocation = [recognizer locationInView:self.view];
}

- (m2::PointD)getZoomPoint
{
  CGFloat const scale = [UIScreen mainScreen].scale;
  return m2::PointD(self.pointerLocation.x * scale, self.pointerLocation.y * scale);
}

// MARK: - UIGestureRecognizerDelegate

- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer
    shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer *)otherGestureRecognizer
{
  return YES;
}

@end
