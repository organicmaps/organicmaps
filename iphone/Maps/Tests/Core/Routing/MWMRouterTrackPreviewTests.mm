#import <XCTest/XCTest.h>

#import "MWMNavigationDashboardManager.h"
#import "MWMRouter.h"
#import "MapViewController.h"

#import <objc/runtime.h>

#include <CoreApi/ElevationProfileData.h>
#include <CoreApi/Framework.h>
#include <CoreApi/MWMBookmarksManager.h>
#include <CoreApi/RouteElevationPreviewData.h>

#include "geometry/mercator.hpp"

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
  auto & bookmarks = GetFramework().GetBookmarkManager();
  auto const start = mercator::FromLatLon(45.0, 19.0);
  kml::TrackData data;
  data.m_layers.emplace_back();
  data.m_geometry.AddLine({{start, 100}, {mercator::FromLatLon(45.01, 19.0), finishAltitude}});
  data.m_geometry.AddTimestamps({});
  data.m_routerType = "pedestrian";
  auto const id = bookmarks.GetEditSession().CreateTrack(std::move(data))->GetId();
  bookmarks.GetEditSession().AttachTrack(id, m_categoryId);
  bookmarks.SetTrackSelectionInfo({id, start, 0.0}, false);
  bookmarks.MyPositionMark().SetUserPosition(start, true);
  XCTAssertEqual([MWMRouter navigateTrack:id], MWMFollowTrackResultOk);
  return id;
}

- (void)waitForPreview
{
  auto ready =
      [[XCTNSPredicateExpectation alloc] initWithPredicate:[NSPredicate predicateWithBlock:^BOOL(id, NSDictionary *) {
                                           return [MWMRouter isFollowingTrack] && [MWMRouter isRouteBuilt];
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
  XCTAssertTrue([MWMRouter isFollowingTrack]);
  XCTAssertEqual([MWMRouter followedTrackInfo].trackId, id);
  XCTAssertEqual(GetFramework().GetRoutingManager().RoutingSession().GetRoute(), route);
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
  XCTAssertEqual([MWMRouter updateFollowedTrackEntryAtDistance:distance], MWMFollowTrackResultOk);
  XCTAssertEqual(MWMNavigationDashboardManager.sharedManager.state, MWMNavigationDashboardStatePlanning);
  [self waitForPreview];
  XCTAssertEqual([MWMRouter renderType], MWMRouterTypeVehicle);
  XCTAssertTrue(bookmarks.GetTrack(id)->GetData().m_routerType == "pedestrian");
  auto const & point = GetFramework().GetRoutingManager().RoutingSession().GetRoute()->GetPoly().GetPoint(1);
  XCTAssertTrue(point.EqualDxDy(entry, mercator::kPointEqualityEps));
  XCTAssertEqual(bookmarks.GetElevationActivePoint(id), distance);
  XCTAssertEqual([MWMRouter followedTrackInfo].entryDistanceMeters, distance);

  bookmarks.SetElevationActivePoint(id, track->GetLengthMeters() * 0.8);

  XCTAssertEqual([MWMRouter routeElevationActivePointDistance].doubleValue, distance);
  auto const * route = GetFramework().GetRoutingManager().RoutingSession().GetRoute();
  XCTAssertEqual([MWMRouter updateFollowedTrackEntryAtDistance:bookmarks.GetTrack(id)->GetLengthMeters()],
                 MWMFollowTrackResultNoRemainingGeometry);
  XCTAssertEqual(GetFramework().GetRoutingManager().RoutingSession().GetRoute(), route);
  XCTAssertEqual(MWMNavigationDashboardManager.sharedManager.state, MWMNavigationDashboardStateReady);
  XCTAssertEqual([MWMRouter followedTrackInfo].entryDistanceMeters, distance);
}

- (void)testPreviewEditsKeepPhoneRoutingOwnership
{
  Class const serviceClass = NSClassFromString(@"MWMCarPlayService");
  SEL const selector = NSSelectorFromString(@"preservePhoneTrackSession");
  Method method = class_getInstanceMethod(serviceClass, selector);
  XCTAssertTrue(method != nullptr);
  if (method == nullptr)
    return;
  IMP const original = method_getImplementation(method);
  __block NSUInteger handoffs = 0;
  IMP replacement = imp_implementationWithBlock(^(id service) {
    ++handoffs;
    reinterpret_cast<void (*)(id, SEL)>(original)(service, selector);
  });
  method_setImplementation(method, replacement);
  @try
  {
    auto const id = [self createPreview];
    [self waitForPreview];
    XCTAssertEqual(handoffs, 1);
    XCTAssertEqual([MWMRouter updateFollowedTrackEntryAtDistance:100.0], MWMFollowTrackResultOk);
    [self waitForPreview];
    XCTAssertEqual([MWMRouter setFollowTrackProfile:MWMRouterTypeVehicle], MWMFollowTrackResultOk);
    [self waitForPreview];
    XCTAssertEqual(handoffs, 1);
    XCTAssertEqual([MWMRouter followedTrackInfo].trackId, id);
  }
  @finally
  {
    method_setImplementation(method, original);
    imp_removeBlock(replacement);
  }
}

- (void)testStoppingTrackNotifiesRoutingOwnershipOnce
{
  Class const serviceClass = NSClassFromString(@"MWMCarPlayService");
  SEL const selector = NSSelectorFromString(@"phoneTrackSessionDidEnd");
  Method method = class_getInstanceMethod(serviceClass, selector);
  XCTAssertTrue(method != nullptr);
  if (method == nullptr)
    return;
  IMP const original = method_getImplementation(method);
  __block NSUInteger ended = 0;
  IMP replacement = imp_implementationWithBlock(^(id service) {
    ++ended;
    XCTAssertFalse([MWMRouter isFollowingTrack]);
    reinterpret_cast<void (*)(id, SEL)>(original)(service, selector);
  });
  method_setImplementation(method, replacement);
  @try
  {
    [self createPreview];
    [self waitForPreview];
    [MWMRouter stopRouting];
    [MWMRouter stopRouting];
    XCTAssertEqual(ended, 1);
  }
  @finally
  {
    method_setImplementation(method, original);
    imp_removeBlock(replacement);
  }
}

- (void)testDeletionCancelsPreviewWhileItsBuildIsPending
{
  auto const id = [self createPreview];
  [MWMBookmarksManager.sharedManager deleteTrack:id];
  XCTAssertTrue([MWMRouter isFollowingTrack], @"Do not tear down routing inside bookmark publication");
  XCTAssertEqual([MWMRouter routeElevationActivePointDistance].doubleValue, 0.0);
  [self drainMainQueue];
  XCTAssertFalse([MWMRouter isFollowingTrack]);
  XCTAssertEqual(MWMNavigationDashboardManager.sharedManager.state, MWMNavigationDashboardStateClosed);
}

- (void)testRejectedEndpointKeepsTheAcceptedEntryAndPreview
{
  auto const id = [self createPreview];
  [self waitForPreview];
  auto & framework = GetFramework();
  auto & bookmarks = framework.GetBookmarkManager();
  double const finishDistance = bookmarks.GetTrack(id)->GetLengthMeters();
  double const entryDistance = finishDistance / 2.0;
  XCTAssertEqual([MWMRouter updateFollowedTrackEntryAtDistance:entryDistance], MWMFollowTrackResultOk);
  [self waitForPreview];
  auto const entry = bookmarks.GetTrackSelectionInfo(id);
  auto const * route = framework.GetRoutingManager().RoutingSession().GetRoute();
  auto const profile = [MWMRouter renderType];

  XCTAssertEqual([MWMRouter updateFollowedTrackEntryAtDistance:finishDistance],
                 MWMFollowTrackResultNoRemainingGeometry);
  XCTAssertEqual(framework.GetRoutingManager().RoutingSession().GetRoute(), route);
  XCTAssertEqual([MWMRouter renderType], profile);
  auto const selection = bookmarks.GetTrackSelectionInfo(id);
  XCTAssertTrue(selection.m_trackPoint.EqualDxDy(entry.m_trackPoint, mercator::kPointEqualityEps));
  XCTAssertEqual(selection.m_distFromBegM, entry.m_distFromBegM);
  XCTAssertEqual([MWMRouter routeElevationActivePointDistance].doubleValue, entryDistance);
}

- (void)testBusyDistanceUpdateKeepsTheSelection
{
  auto const trackId = [self createPreview];
  auto & bookmarks = GetFramework().GetBookmarkManager();
  auto const selection = bookmarks.GetTrackSelectionInfo(trackId);
  auto const distance = [MWMRouter routeElevationActivePointDistance];
  XCTAssertEqual([MWMRouter updateFollowedTrackEntryAtDistance:250.0], MWMFollowTrackResultBusy);
  XCTAssertTrue(bookmarks.GetTrackSelectionInfo(trackId).m_trackPoint.EqualDxDy(selection.m_trackPoint,
                                                                                mercator::kPointEqualityEps));
  XCTAssertEqual(bookmarks.GetTrackSelectionInfo(trackId).m_distFromBegM, selection.m_distFromBegM);
  XCTAssertEqualObjects([MWMRouter routeElevationActivePointDistance], distance);
  [self waitForPreview];
  XCTAssertTrue(GetFramework().GetRoutingManager().RoutingSession().EnableFollowMode());
  auto const * route = GetFramework().GetRoutingManager().RoutingSession().GetRoute();
  XCTAssertEqual([MWMRouter updateFollowedTrackEntryAtDistance:250.0], MWMFollowTrackResultBusy);
  XCTAssertEqual(GetFramework().GetRoutingManager().RoutingSession().GetRoute(), route);
  XCTAssertEqual(bookmarks.GetTrackSelectionInfo(trackId).m_distFromBegM, selection.m_distFromBegM);
  XCTAssertEqual(bookmarks.GetElevationActivePoint(trackId), distance.doubleValue);
}

- (void)testDistanceUpdateKeepsTheReversedDirectionAndRejectsTheStart
{
  auto const id = [self createPreview];
  [self waitForPreview];
  auto & framework = GetFramework();
  auto & bookmarks = framework.GetBookmarkManager();
  double const distance = bookmarks.GetTrack(id)->GetLengthMeters() / 2.0;
  XCTAssertEqual([MWMRouter updateFollowedTrackEntryAtDistance:distance], MWMFollowTrackResultOk);
  [self waitForPreview];
  XCTAssertEqual([MWMRouter reverseFollowedTrack], MWMFollowTrackResultOk);
  [self waitForPreview];
  XCTAssertEqual([MWMRouter updateFollowedTrackEntryAtDistance:distance / 2.0], MWMFollowTrackResultOk);
  [self waitForPreview];
  auto const selection = bookmarks.GetTrackSelectionInfo(id);
  auto const * route = framework.GetRoutingManager().RoutingSession().GetRoute();
  XCTAssertTrue([MWMRouter followedTrackInfo].isReversed);
  XCTAssertEqual([MWMRouter renderType], MWMRouterTypePedestrian);
  XCTAssertTrue(route->GetPoly().GetPoint(1).EqualDxDy(selection.m_trackPoint, mercator::kPointEqualityEps));
  XCTAssertTrue(route->GetPoly().Back().EqualDxDy(mercator::FromLatLon(45.0, 19.0), mercator::kPointEqualityEps));
  XCTAssertEqual([MWMRouter updateFollowedTrackEntryAtDistance:0.0], MWMFollowTrackResultNoRemainingGeometry);
  XCTAssertEqual(framework.GetRoutingManager().RoutingSession().GetRoute(), route);
  XCTAssertTrue(
      bookmarks.GetTrackSelectionInfo(id).m_trackPoint.EqualDxDy(selection.m_trackPoint, mercator::kPointEqualityEps));
  XCTAssertEqual(bookmarks.GetTrackSelectionInfo(id).m_distFromBegM, selection.m_distFromBegM);
  XCTAssertEqual([MWMRouter routeElevationActivePointDistance].doubleValue, distance / 2.0);
}

- (void)testReverseKeepsTheCapturedEntryAndProfile
{
  auto const id = [self createPreview];
  [self waitForPreview];
  XCTAssertEqual([MWMRouter setFollowTrackProfile:MWMRouterTypeVehicle], MWMFollowTrackResultOk);
  [self waitForPreview];
  auto & framework = GetFramework();
  auto & bookmarks = framework.GetBookmarkManager();
  auto const * track = bookmarks.GetTrack(id);
  auto const & line = track->GetData().m_geometry.m_lines.front();
  auto const start = line.front().GetPoint();
  auto const entry = (start + line.back().GetPoint()) * 0.5;
  XCTAssertEqual([MWMRouter updateFollowedTrackEntryAtDistance:track->GetLengthMeters() * 0.5], MWMFollowTrackResultOk);
  [self waitForPreview];
  XCTAssertEqual([MWMRouter reverseFollowedTrack], MWMFollowTrackResultOk);
  XCTAssertEqual([MWMRouter reverseFollowedTrack], MWMFollowTrackResultBusy);
  [self waitForPreview];
  XCTAssertTrue([MWMRouter followedTrackInfo].isReversed);
  XCTAssertEqual([MWMRouter renderType], MWMRouterTypeVehicle);
  auto const * route = framework.GetRoutingManager().RoutingSession().GetRoute();
  XCTAssertTrue(route->GetPoly().GetPoint(1).EqualDxDy(entry, mercator::kPointEqualityEps));
  XCTAssertTrue(route->GetPoly().Back().EqualDxDy(start, mercator::kPointEqualityEps));
  XCTAssertTrue(bookmarks.GetTrack(id)->GetData().m_routerType == "pedestrian");
  // The XCTest host omits the renderer; enter the session directly to test the active-navigation guard.
  XCTAssertTrue(framework.GetRoutingManager().RoutingSession().EnableFollowMode());
  XCTAssertTrue([MWMRouter isOnRoute]);
  XCTAssertEqual([MWMRouter reverseFollowedTrack], MWMFollowTrackResultBusy);
  XCTAssertEqual(framework.GetRoutingManager().RoutingSession().GetRoute(), route);
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
  XCTAssertTrue([MWMRouter isFollowingTrack]);
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
  XCTAssertTrue([MWMRouter isFollowingTrack]);
  XCTAssertEqual([MWMRouter followedTrackInfo].trackId, replacement);
}

@end
