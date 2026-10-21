@testable import Organic_Maps__Debug_
import XCTest

final class ActionBarViewControllerTests: XCTestCase {
  private struct RouteState {
    var canAddStop = false
    var canReplaceStop = false
    var shouldAppendNewPoints = false
  }

  func test_GivenRouteState_WhenActionBarIsConfigured_ThenShowsMatchingRouteButtons() {
    let full = RouteState()
    let fullReplacing = RouteState(canReplaceStop: true)
    let appendingWithoutRoute = RouteState(shouldAppendNewPoints: true)
    let spare = RouteState(canAddStop: true)
    let spareReplacing = RouteState(canAddStop: true, canReplaceStop: true)
    let spareAppending = RouteState(canAddStop: true, shouldAppendNewPoints: true)

    let cases: [(PlacePageObjectType, RouteState, [ActionBarButtonType])] = [
      (.POI, fullReplacing, [.routeReplaceStop, .bookmark]),
      (.bookmark, fullReplacing, [.routeReplaceStop, .bookmark]),
      (.track, fullReplacing, [.routeReplaceStop, .bookmark, .track]),
      (.relationTrack, fullReplacing, [.routeReplaceStop, .bookmark]),
      (.track, spareReplacing, [.routeReplaceStop, .bookmark, .track]),
      (.relationTrack, spareReplacing, [.routeReplaceStop, .bookmark]),
      (.POI, spare, [.routeFrom, .routeAddStop, .bookmark, .routeTo]),
      (.track, spareAppending, [.routeAddDestination, .bookmark, .track]),
      (.POI, full, [.routeFrom, .bookmark, .routeTo]),
      (.track, full, [.routeFrom, .bookmark, .routeTo, .track]),
      (.bookmark, appendingWithoutRoute, [.routeAddDestination, .bookmark]),
      (.relationTrack, appendingWithoutRoute, [.routeAddDestination, .bookmark]),
      (.trackRecording, spareReplacing, [.deleteTrackRecording, .saveTrackRecording]),
    ]

    for (index, (objectType, routeState, expectedButtons)) in cases.enumerated() {
      XCTAssertEqual(buttons(for: objectType, routeState), expectedButtons, "Case \(index)")
    }
  }

  private func buttons(for objectType: PlacePageObjectType, _ routeState: RouteState) -> [ActionBarButtonType] {
    let actionBar = UIStoryboard.instance(.placePage).instantiateViewController(ofType: ActionBarViewController.self)
    actionBar.placePageData = PlacePageDataStub(objectType: objectType)
    actionBar.canAddStop = routeState.canAddStop
    actionBar.canReplaceStop = routeState.canReplaceStop
    actionBar.shouldAppendNewPoints = routeState.shouldAppendNewPoints
    actionBar.loadViewIfNeeded()
    return actionBar.stackView.arrangedSubviews.compactMap { ($0 as? ActionBarButton)?.type }
  }
}

private final class PlacePageDataStub: PlacePageData {
  private let stubObjectType: PlacePageObjectType

  init(objectType: PlacePageObjectType) {
    stubObjectType = objectType
    super.init(trackInfo: TrackInfo.empty(), elevationInfo: nil)
  }

  override var objectType: PlacePageObjectType { stubObjectType }
}
