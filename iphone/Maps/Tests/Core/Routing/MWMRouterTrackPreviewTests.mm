#import <XCTest/XCTest.h>

#import "MWMNavigationDashboardManager.h"
#import "MWMRouter.h"
#import "MapViewController.h"

#include <CoreApi/Framework.h>
#include <CoreApi/MWMBookmarksManager.h>

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
  auto & bookmarks = GetFramework().GetBookmarkManager();
  auto const start = mercator::FromLatLon(45.0, 19.0);
  kml::TrackData data;
  data.m_layers.emplace_back();
  data.m_geometry.AddLine({{start, 100}, {mercator::FromLatLon(45.01, 19.0), 110}});
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

  auto const * route = GetFramework().GetRoutingManager().RoutingSession().GetRoute();
  XCTAssertEqual([MWMRouter updateFollowedTrackEntryAtDistance:bookmarks.GetTrack(id)->GetLengthMeters()],
                 MWMFollowTrackResultNoRemainingGeometry);
  XCTAssertEqual(GetFramework().GetRoutingManager().RoutingSession().GetRoute(), route);
  XCTAssertEqual(MWMNavigationDashboardManager.sharedManager.state, MWMNavigationDashboardStateReady);
  XCTAssertEqual(bookmarks.GetElevationActivePoint(id), distance);
}

- (void)testDeletionCancelsPreviewWhileItsBuildIsPending
{
  auto const id = [self createPreview];
  [MWMBookmarksManager.sharedManager deleteTrack:id];
  XCTAssertTrue([MWMRouter isFollowingTrack], @"Do not tear down routing inside bookmark publication");
  [self drainMainQueue];
  XCTAssertFalse([MWMRouter isFollowingTrack]);
  XCTAssertEqual(MWMNavigationDashboardManager.sharedManager.state, MWMNavigationDashboardStateClosed);
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
