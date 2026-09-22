@testable import Organic_Maps__Debug_
import XCTest

final class MWMRoutePointSelectionTests: XCTestCase {
  private var manager: MWMNavigationDashboardManager!
  private var delegate: MWMRoutePreviewDelegate!
  private var previousRouterType: MWMRouterType!

  override func setUpWithError() throws {
    try super.setUpWithError()
    let mapViewController = try XCTUnwrap(MapViewController.shared())
    mapViewController.loadViewIfNeeded()
    manager = MWMNavigationDashboardManager.shared()
    delegate = try XCTUnwrap(manager as? MWMRoutePreviewDelegate)
    previousRouterType = MWMRouter.type()
    MWMRouter.stopRouting()
    MWMRouter.enableAutoAddLastLocation(false)
    // Ruler routes exercise the real core without downloaded map data or a GPS fix.
    MWMRouter.setType(.ruler)
  }

  override func tearDown() {
    manager?.cancelRoutePointSelection()
    MWMRouter.stopRouting()
    if let previousRouterType {
      MWMRouter.setType(previousRouterType)
    }
    manager = nil
    delegate = nil
    super.tearDown()
  }

  func test_GivenEmptyRoute_WhenStartAndFinishAreSelected_ThenBuildsRoute() {
    select(type: .start, title: "start", coordinate: 10)
    XCTAssertEqual(MWMRouter.points().map(\.title), ["start"])
    XCTAssertEqual(MWMRouter.startPoint()?.type, .start)

    select(type: .finish, title: "finish", coordinate: 13)

    XCTAssertEqual(MWMRouter.points().map(\.title), ["start", "finish"])
    XCTAssertEqual(MWMRouter.finishPoint()?.type, .finish)
    waitUntilRouteReady()
  }

  func test_GivenRoute_WhenPointIsAppended_ThenKeepsOldFinishAsLastStop() {
    seedRoute()

    select(type: .intermediate, title: "new finish", coordinate: 14, shouldAppend: true)

    let points = MWMRouter.points()
    XCTAssertEqual(points.map(\.title), ["start", "first stop", "second stop", "finish", "new finish"])
    XCTAssertEqual(points.map(\.type), [.start, .intermediate, .intermediate, .intermediate, .finish])
    XCTAssertEqual(points.filter { $0.type == .intermediate }.map(\.intermediateIndex), [0, 1, 2])
    waitUntilRouteReady()
  }

  func test_GivenRoute_WhenIntermediatePointIsSelected_ThenAddsItBetweenEndpoints() {
    seedEndpoints()

    select(type: .intermediate, title: "stop", coordinate: 11)

    let points = MWMRouter.points()
    XCTAssertEqual(points.map(\.title), ["start", "stop", "finish"])
    XCTAssertEqual(points.map(\.type), [.start, .intermediate, .finish])
    XCTAssertEqual(points[1].intermediateIndex, 0)
    waitUntilRouteReady()
  }

  func test_GivenIntermediatePoint_WhenReplaced_ThenPreservesItsIndexAndOtherPoints() throws {
    seedRoute()
    let secondStop = try XCTUnwrap(MWMRouter.points().first { $0.title == "second stop" })
    XCTAssertEqual(secondStop.intermediateIndex, 1)

    select(type: .intermediate, title: "replacement", coordinate: 9, replacing: secondStop)

    let points = MWMRouter.points()
    XCTAssertEqual(points.map(\.title), ["start", "first stop", "replacement", "finish"])
    XCTAssertEqual(points.filter { $0.type == .intermediate }.map(\.intermediateIndex), [0, 1])
    waitUntilRouteReady()
  }

  func test_GivenEndpoints_WhenReplaced_ThenKeepsStopsAndPointCount() throws {
    seedRoute()
    let start = try XCTUnwrap(MWMRouter.startPoint())
    select(type: .start, title: "new start", coordinate: 9, replacing: start)
    let finish = try XCTUnwrap(MWMRouter.finishPoint())
    select(type: .finish, title: "new finish", coordinate: 14, replacing: finish)

    XCTAssertEqual(MWMRouter.points().map(\.title), ["new start", "first stop", "second stop", "new finish"])
    waitUntilRouteReady()
  }

  func test_GivenPendingReplacement_WhenCancelled_ThenKeepsRouteUnchanged() throws {
    seedRoute()
    let finish = try XCTUnwrap(MWMRouter.finishPoint())
    delegate.routePreviewDidSelect(MWMRoutePointSelection(point: finish, type: .finish, shouldAppend: false))
    XCTAssertTrue(manager.isRoutePointSelectionActive)

    manager.cancelRoutePointSelection()

    XCTAssertFalse(manager.isRoutePointSelectionActive)
    XCTAssertNil(manager.selectedRoutePoint)
    XCTAssertEqual(MWMRouter.points().map(\.title), ["start", "first stop", "second stop", "finish"])
  }

  func test_GivenRoutePoints_WhenResolvingTypes_ThenUsesTheirPositions() {
    let endpoints = NavigationDashboard.RoutePoints(points: [
      makePoint(type: .start, title: "start", coordinate: 10),
      makePoint(type: .finish, title: "finish", coordinate: 13),
    ])
    XCTAssertEqual((0 ..< endpoints.count).map(endpoints.type(for:)), [.start, .finish])

    let route = NavigationDashboard.RoutePoints(points: [
      makePoint(type: .start, title: "start", coordinate: 10),
      makePoint(type: .intermediate, title: "first stop", coordinate: 11),
      makePoint(type: .intermediate, title: "second stop", coordinate: 12),
      makePoint(type: .finish, title: "finish", coordinate: 13),
    ])
    XCTAssertEqual((0 ..< route.count).map(route.type(for:)), [.start, .intermediate, .intermediate, .finish])
  }

  private func seedEndpoints() {
    MWMRouter.addPoint(makePoint(type: .start, title: "start", coordinate: 10))
    MWMRouter.addPoint(makePoint(type: .finish, title: "finish", coordinate: 13))
  }

  private func seedRoute() {
    for (title, coordinate, type, index) in [
      ("start", 10.0, MWMRoutePointType.start, 0),
      ("finish", 13.0, .finish, 0),
      ("first stop", 11.0, .intermediate, 0),
      ("second stop", 12.0, .intermediate, 1),
    ] {
      MWMRouter.addPoint(MWMRoutePoint(cgPoint: CGPoint(x: coordinate, y: coordinate),
                                       title: title, subtitle: nil, type: type, intermediateIndex: index))
    }
  }

  private func makePoint(type: MWMRoutePointType, title: String, coordinate: Double,
                         intermediateIndex: Int = 0) -> MWMRoutePoint {
    MWMRoutePoint(cgPoint: CGPoint(x: coordinate, y: coordinate),
                  title: title, subtitle: nil, type: type, intermediateIndex: intermediateIndex)
  }

  private func select(type: MWMRoutePointType, title: String, coordinate: Double,
                      replacing point: MWMRoutePoint? = nil, shouldAppend: Bool = false,
                      file: StaticString = #filePath, line: UInt = #line) {
    delegate.routePreviewDidSelect(MWMRoutePointSelection(point: point, type: type, shouldAppend: shouldAppend))

    XCTAssertTrue(manager.selectRoutePoint(at: CGPoint(x: coordinate, y: coordinate), title: title, subtitle: nil),
                  file: file, line: line)
    XCTAssertFalse(manager.isRoutePointSelectionActive, file: file, line: line)
    XCTAssertNil(manager.selectedRoutePoint, file: file, line: line)
  }

  private func waitUntilRouteReady() {
    // A fixed start disables follow mode after building, making isRouteBuilt() transient.
    let ready = XCTNSPredicateExpectation(predicate: NSPredicate { [manager] _, _ in
      manager?.state == .ready
    }, object: nil)
    wait(for: [ready], timeout: 5)
    XCTAssertTrue(MWMRouter.isRoutingActive())
  }
}
