#import <XCTest/XCTest.h>

#import "MWMAlertViewController+CPP.h"
#import "MWMLocationManager.h"
#import "MWMMapViewControlsManager.h"
#import "MWMNavigationDashboardManager.h"
#import "MWMRoutePoint+CPP.h"
#import "MWMRouter.h"
#import "MapViewController.h"
#include "base/scope_guard.hpp"

#import <objc/runtime.h>

#include <CoreApi/ElevationProfileData.h>
#include <CoreApi/Framework.h>
#include <CoreApi/MWMBookmarksManager.h>
#include <CoreApi/RouteElevationPreviewData.h>

#include "geometry/mercator.hpp"

@interface MWMRouter (TrackPreviewTests)
+ (MWMRouter *)router;
- (void)onRouteReady:(BOOL)hasWarnings;
@end

@interface MWMRouterTrackPreviewTests : XCTestCase
{
  kml::MarkGroupId m_categoryId;
  MWMRouterType m_previousRouter;
  m2::PointD m_previousPosition;
  bool m_hadPosition;
}
@end

@implementation MWMRouterTrackPreviewTests

- (void)setUp
{
  [super setUp];
  [[MapViewController sharedController] loadViewIfNeeded];
  m_previousRouter = [MWMRouter type];
  auto const & position = GetFramework().GetBookmarkManager().MyPositionMark();
  m_previousPosition = position.GetPivot();
  m_hadPosition = position.HasPosition();
  [MWMRouter stopRouting];
  [MWMRouter subscribeToEvents];
  m_categoryId = GetFramework().GetBookmarkManager().CreateBookmarkCategory("Track preview tests", false);
}

- (void)tearDown
{
  [MWMRouter stopRouting];
  auto & bookmarks = GetFramework().GetBookmarkManager();
  bookmarks.GetEditSession().DeleteBmCategory(m_categoryId, true);
  bookmarks.MyPositionMark().SetUserPosition(m_previousPosition, m_hadPosition);
  [MWMRouter setType:m_previousRouter];
  [super tearDown];
}

- (kml::TrackId)createPreview
{
  return [self createPreviewWithFinishAltitude:110];
}

- (kml::TrackId)createPreviewWithFinishAltitude:(geometry::Altitude)finishAltitude
{
  auto const id = [self selectTrackWithProfile:"pedestrian" finishAltitude:finishAltitude];
  XCTAssertEqual([MWMRouter navigateTrack:id], MWMFollowTrackResultOk);
  return id;
}

- (kml::TrackId)selectTrackWithProfile:(char const *)profile finishAltitude:(geometry::Altitude)finishAltitude
{
  auto & bookmarks = GetFramework().GetBookmarkManager();
  auto const start = mercator::FromLatLon(45.0, 19.0);
  kml::TrackData data;
  data.m_layers.emplace_back();
  data.m_geometry.AddLine({{start, 100}, {mercator::FromLatLon(45.01, 19.0), finishAltitude}});
  data.m_geometry.AddTimestamps({});
  data.m_routerType = profile;
  auto const id = bookmarks.GetEditSession().CreateTrack(std::move(data))->GetId();
  bookmarks.GetEditSession().AttachTrack(id, m_categoryId);
  bookmarks.SetTrackSelectionInfo({id, start, 0.0}, false);
  bookmarks.MyPositionMark().SetUserPosition(start, true);
  return id;
}

- (void)testSavedWaterTrackRejectsNavigationAndOrdinaryTrackCanFollowWaterSelection
{
  auto const water = [self selectTrackWithProfile:"water" finishAltitude:110];
  XCTAssertEqual([MWMRouter navigateTrack:water], MWMFollowTrackResultUnsupportedProfile);
  XCTAssertFalse([MWMRouter hasTrackNavigationSession]);
  [MWMRouter setType:MWMRouterTypeWater];
  [self createPreview];
  [self waitForPreview];
  XCTAssertEqual([MWMRouter renderType], MWMRouterTypePedestrian);
}

- (void)waitForPreview
{
  auto ready =
      [[XCTNSPredicateExpectation alloc] initWithPredicate:[NSPredicate predicateWithBlock:^BOOL(id, NSDictionary *) {
                                           return [MWMRouter hasTrackNavigationSession] && [MWMRouter isRouteBuilt];
                                         }]
                                                    object:nil];
  [self waitForExpectations:@[ready] timeout:5];
}

- (void)drainMainQueue
{
  auto drained = [self expectationWithDescription:@"Deferred bookmark changes finished"];
  dispatch_async(dispatch_get_main_queue(), ^{ [drained fulfill]; });
  [self waitForExpectations:@[drained] timeout:5];
}

- (void)testFromAnActiveTrackReleasesNavigationControls
{
  [self createPreview];
  [self waitForPreview];
  XCTAssertTrue(GetFramework().GetRoutingManager().RoutingSession().EnableFollowMode());
  [MWMRouter showNavigationMapControls];
  auto controls = [MWMMapViewControlsManager manager];
  XCTAssertTrue([[controls valueForKey:@"disableStandbyOnRouteFollowing"] boolValue]);
  XCTAssertTrue(controls.trafficButtonHidden);
  auto point = [[MWMRoutePoint alloc] initWithPoint:mercator::FromLatLon(45.1, 19.0)
                                              title:@"New start"
                                           subtitle:nil
                                               type:MWMRoutePointTypeStart
                                  intermediateIndex:0];
  [MWMRouter buildFromPoint:point bestRouter:NO];
  XCTAssertFalse([MWMRouter hasTrackNavigationSession]);
  XCTAssertFalse([MWMRouter isOnRoute]);
  XCTAssertEqual([MWMRouter pointsCount], 1);
  XCTAssertEqual(MWMNavigationDashboardManager.sharedManager.state, MWMNavigationDashboardStatePrepare);
  XCTAssertFalse([[controls valueForKey:@"disableStandbyOnRouteFollowing"] boolValue]);
  XCTAssertFalse(controls.trafficButtonHidden);
}

- (void)testStartWithoutGpsExplainsTheMissingLocation
{
  [self createPreview];
  [self waitForPreview];
  Method location = class_getClassMethod(MWMLocationManager.class, @selector(lastLocation));
  Method disclaimer =
      class_getClassMethod(NSClassFromString(@"MWMSettings"), NSSelectorFromString(@"routingDisclaimerApproved"));
  Method alert = class_getInstanceMethod(MWMAlertViewController.class, @selector(presentAlert:));
  IMP noLocation = imp_implementationWithBlock(^CLLocation *(id self) { return nil; });
  IMP approved = imp_implementationWithBlock(^BOOL(id self) { return YES; });
  __block routing::RouterResultCode shown = routing::RouterResultCode::NoError;
  IMP captureAlert = imp_implementationWithBlock(^(id self, routing::RouterResultCode code) { shown = code; });
  IMP oldLocation = method_setImplementation(location, noLocation);
  IMP oldDisclaimer = method_setImplementation(disclaimer, approved);
  IMP oldAlert = method_setImplementation(alert, captureAlert);
  SCOPE_GUARD(restore, [&]
  {
    method_setImplementation(location, oldLocation);
    method_setImplementation(disclaimer, oldDisclaimer);
    method_setImplementation(alert, oldAlert);
    imp_removeBlock(noLocation);
    imp_removeBlock(approved);
    imp_removeBlock(captureAlert);
  });
  [MWMRouter startRouting];
  XCTAssertEqual(shown, routing::RouterResultCode::NoCurrentPosition);
  XCTAssertFalse([MWMRouter isOnRoute]);
  XCTAssertEqual(MWMNavigationDashboardManager.sharedManager.state, MWMNavigationDashboardStateReady);
}

- (void)testFlatTrackKeepsTheSourceElevationSelector
{
  [self createPreviewWithFinishAltitude:100];
  [self waitForPreview];
  auto const profile = [MWMRouter routeElevationProfileData];
  XCTAssertNotNil(profile);
  XCTAssertEqual(profile.elevationProfileData.points.count, 2);
  XCTAssertEqual(profile.elevationProfileData.points.firstObject.altitude, 100.0);
  XCTAssertEqual(profile.elevationProfileData.points.lastObject.altitude, 100.0);
}

- (void)testRoadRebuildDoesNotReplaceCapturedPreview
{
  auto const id = [self createPreview];
  [self waitForPreview];
  auto const * route = GetFramework().GetRoutingManager().RoutingSession().GetRoute();
  [MWMRouter rebuildWithBestRouter:NO];
  XCTAssertTrue([MWMRouter hasTrackNavigationSession]);
  XCTAssertEqual([MWMRouter followedTrackInfo].trackId, id);
  XCTAssertEqual(GetFramework().GetRoutingManager().RoutingSession().GetRoute(), route);
}

- (void)testTrackBuildEntersNavigationOnlyAfterFollowingStarts
{
  [self createPreview];
  [self waitForPreview];
  [MWMNavigationDashboardManager.sharedManager onRoutePlanning];
  [[MWMRouter router] onRouteReady:NO];
  XCTAssertEqual(MWMNavigationDashboardManager.sharedManager.state, MWMNavigationDashboardStateReady);

  // The test host has no renderer, so enter following before delivering its build-ready event.
  [MWMNavigationDashboardManager.sharedManager onRoutePlanning];
  XCTAssertTrue(GetFramework().GetRoutingManager().RoutingSession().EnableFollowMode());
  [[MWMRouter router] onRouteReady:NO];
  XCTAssertEqual(MWMNavigationDashboardManager.sharedManager.state, MWMNavigationDashboardStateNavigation);
}

- (void)testChartEntryUpdateKeepsTheChosenProfileAndPreservesRefusals
{
  auto const id = [self createPreview];
  [self waitForPreview];
  XCTAssertEqual([MWMRouter setFollowTrackProfile:MWMRouterTypeVehicle], MWMFollowTrackResultOk);
  [self waitForPreview];
  auto & bookmarks = GetFramework().GetBookmarkManager();
  auto const * track = bookmarks.GetTrack(id);
  auto const & line = track->GetData().m_geometry.m_lines.front();
  auto const entry = (line.front().GetPoint() + line.back().GetPoint()) * 0.5;
  double const distance = track->GetLengthMeters() * 0.5;
  double const mapSelection = bookmarks.GetElevationActivePoint(id);
  XCTAssertEqual([MWMRouter updateFollowedTrackEntryAtDistance:distance], MWMFollowTrackResultOk);
  XCTAssertEqual(MWMNavigationDashboardManager.sharedManager.state, MWMNavigationDashboardStatePlanning);
  [self waitForPreview];
  XCTAssertEqual([MWMRouter renderType], MWMRouterTypeVehicle);
  XCTAssertTrue(bookmarks.GetTrack(id)->GetData().m_routerType == "pedestrian");
  auto const & point = GetFramework().GetRoutingManager().RoutingSession().GetRoute()->GetPoly().GetPoint(1);
  XCTAssertTrue(point.EqualDxDy(entry, mercator::kPointEqualityEps));
  XCTAssertEqual(bookmarks.GetElevationActivePoint(id), mapSelection);
  XCTAssertEqual([MWMRouter followedTrackInfo].entryDistanceMeters, distance);

  bookmarks.SetElevationActivePoint(id, track->GetLengthMeters() * 0.8);

  XCTAssertEqual([MWMRouter routeElevationActivePointDistance].doubleValue, distance);
  auto const selectionBeforeRefusal = bookmarks.GetTrackSelectionInfo(id);
  auto const * route = GetFramework().GetRoutingManager().RoutingSession().GetRoute();
  XCTAssertEqual([MWMRouter updateFollowedTrackEntryAtDistance:bookmarks.GetTrack(id)->GetLengthMeters()],
                 MWMFollowTrackResultNoRemainingGeometry);
  XCTAssertEqual(GetFramework().GetRoutingManager().RoutingSession().GetRoute(), route);
  XCTAssertEqual(MWMNavigationDashboardManager.sharedManager.state, MWMNavigationDashboardStateReady);
  XCTAssertEqual([MWMRouter renderType], MWMRouterTypeVehicle);
  XCTAssertEqual([MWMRouter followedTrackInfo].entryDistanceMeters, distance);
  auto const selectionAfterRefusal = bookmarks.GetTrackSelectionInfo(id);
  XCTAssertTrue(
      selectionAfterRefusal.m_trackPoint.EqualDxDy(selectionBeforeRefusal.m_trackPoint, mercator::kPointEqualityEps));
  XCTAssertEqual(selectionAfterRefusal.m_distFromBegM, selectionBeforeRefusal.m_distFromBegM);
}

- (void)testDeletionCancelsPreviewWhileItsBuildIsPending
{
  auto const trackId = [self createPreview];
  [MWMBookmarksManager.sharedManager deleteTrack:trackId];
  XCTAssertTrue([MWMRouter hasTrackNavigationSession], @"Do not tear down routing inside bookmark publication");
  XCTAssertEqualObjects([MWMRouter routeElevationActivePointDistance], @0.0);
  [self drainMainQueue];
  XCTAssertFalse([MWMRouter hasTrackNavigationSession]);
  XCTAssertEqual(MWMNavigationDashboardManager.sharedManager.state, MWMNavigationDashboardStateClosed);
}

- (void)testDistanceUpdateReplacesPendingPreviewButKeepsActiveRoute
{
  [self createPreview];
  double const distance = 250.0;
  XCTAssertEqual([MWMRouter updateFollowedTrackEntryAtDistance:distance], MWMFollowTrackResultOk);
  XCTAssertEqual([MWMRouter routeElevationActivePointDistance].doubleValue, distance);
  [self waitForPreview];
  XCTAssertTrue(GetFramework().GetRoutingManager().RoutingSession().EnableFollowMode());
  auto const * route = GetFramework().GetRoutingManager().RoutingSession().GetRoute();
  XCTAssertEqual([MWMRouter updateFollowedTrackEntryAtDistance:500.0], MWMFollowTrackResultBusy);
  XCTAssertEqual(GetFramework().GetRoutingManager().RoutingSession().GetRoute(), route);
  XCTAssertEqual([MWMRouter followedTrackInfo].entryDistanceMeters, distance);
}

- (void)testDeletionKeepsAnActiveJourneyOnItsFrozenGeometry
{
  auto const id = [self createPreview];
  [self waitForPreview];
  auto & manager = GetFramework().GetRoutingManager();
  // The test host omits the renderer; this exercises deletion during an active session, not camera startup.
  XCTAssertTrue(manager.RoutingSession().EnableFollowMode());
  [MWMRouter showNavigationMapControls];
  XCTAssertTrue([MWMRouter isOnRoute]);
  auto const * route = manager.RoutingSession().GetRoute();
  double const length = route->GetTotalDistanceMeters();
  [MWMBookmarksManager.sharedManager deleteTrack:id];
  [self drainMainQueue];
  XCTAssertTrue([MWMRouter hasTrackNavigationSession]);
  XCTAssertTrue([MWMRouter isOnRoute]);
  XCTAssertEqual(manager.RoutingSession().GetRoute(), route);
  XCTAssertEqual(route->GetTotalDistanceMeters(), length);
}

- (void)testDeferredDeletionDoesNotCancelAReplacementPreview
{
  auto const deleted = [self createPreview];
  [MWMBookmarksManager.sharedManager deleteTrack:deleted];
  auto const replacement = [self createPreview];
  [self drainMainQueue];
  [self waitForPreview];
  XCTAssertTrue([MWMRouter hasTrackNavigationSession]);
  XCTAssertEqual([MWMRouter followedTrackInfo].trackId, replacement);
}

@end
