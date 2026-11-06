@testable import Organic_Maps__Debug_
import XCTest

final class NavigationDashboardLifecycleTests: XCTestCase {
  private var mapViewController: MapViewController!
  private var manager: MWMNavigationDashboardManager!
  private var previousRouterType: MWMRouterType!

  override func setUpWithError() throws {
    try super.setUpWithError()
    mapViewController = try XCTUnwrap(MapViewController.shared())
    mapViewController.loadViewIfNeeded()
    manager = MWMNavigationDashboardManager.shared()
    previousRouterType = MWMRouter.type()
    MWMRouter.stopRouting()
    MWMRouter.enableAutoAddLastLocation(false)
    MWMRouter.setType(.ruler)
    waitForDashboardsToClose()
    // Isolate the shared manager from dashboards retired by earlier tests.
    manager.setValue(nil, forKey: "navigationDashboardView")
  }

  override func tearDown() {
    MWMRouter.stopRouting()
    if let previousRouterType {
      MWMRouter.setType(previousRouterType)
    }
    waitForDashboardsToClose()
    manager.setValue(nil, forKey: "navigationDashboardView")
    manager = nil
    mapViewController = nil
    super.tearDown()
  }

  func testStoppingWithoutDashboardDoesNotCreateOne() {
    let manager = MockDashboardManager(parentViewController: mapViewController)

    manager.onRouteStop()

    XCTAssertEqual(manager.dashboardRequests, 0)
  }

  func testReplacementDashboardIgnoresTheRetiringDashboardsCommands() throws {
    buildRoute(prefix: "Old")
    waitUntilRouteReady()
    let oldDashboard = try activeDashboard()
    let oldInteractor = try XCTUnwrap(oldDashboard.interactor)

    MWMRouter.stopRouting()
    buildRoute(prefix: "Replacement")
    let replacement = try XCTUnwrap(manager.value(forKey: "_navigationDashboardView") as? NavigationDashboard.Interactor)
    XCTAssertFalse(replacement === oldInteractor)
    XCTAssertTrue(dashboards.contains { $0.interactor === replacement })
    oldInteractor.process(.close)

    XCTAssertEqual(MWMRouter.points().map(\.title), ["Replacement start", "Replacement finish"])
    waitUntilRouteReady()
    withExtendedLifetime(oldDashboard) {}
  }

  func testClosingDashboardIgnoresLateElevationSelection() {
    let (dashboard, interactor) = makeElevationDashboard()
    interactor.updateMapPoint(distance: 25)
    interactor.stateClosed()
    XCTAssertEqual(ElevationRouter.resetCount, 1)

    interactor.updateMapPoint(distance: 75)
    interactor.statePrepare()
    interactor.stateClosed()

    XCTAssertEqual(ElevationRouter.selectedDistances, [25])
    XCTAssertEqual(ElevationRouter.resetCount, 1)
    withExtendedLifetime(dashboard) {}
  }

  func testClosingDashboardIgnoresQueuedElevationRestore() {
    let (dashboard, interactor) = makeElevationDashboard()
    interactor.stateReady()
    interactor.updateMapPoint(distance: 25)
    interactor.stateClosed()
    XCTAssertEqual(ElevationRouter.resetCount, 1)
    interactor.stateClosed()
    interactor.stateReady()

    let drained = expectation(description: "Queued elevation restore completed")
    DispatchQueue.main.async { drained.fulfill() }
    wait(for: [drained], timeout: 5)

    XCTAssertEqual(ElevationRouter.selectedDistances, [25])
    XCTAssertEqual(ElevationRouter.resetCount, 1)
    withExtendedLifetime(dashboard) {}
  }

  private var dashboards: [NavigationDashboardViewController] {
    mapViewController.children.compactMap { $0 as? NavigationDashboardViewController }
  }

  private func activeDashboard() throws -> NavigationDashboardViewController {
    let interactor = try XCTUnwrap(manager.value(forKey: "_navigationDashboardView") as? NavigationDashboard.Interactor)
    return try XCTUnwrap(dashboards.first { $0.interactor === interactor })
  }

  private func buildRoute(prefix: String) {
    MWMRouter.addPoint(MWMRoutePoint(cgPoint: CGPoint(x: 10, y: 10), title: "\(prefix) start",
                                     subtitle: nil, type: .start, intermediateIndex: 0))
    MWMRouter.addPoint(MWMRoutePoint(cgPoint: CGPoint(x: 11, y: 11), title: "\(prefix) finish",
                                     subtitle: nil, type: .finish, intermediateIndex: 0))
    MWMRouter.rebuild(withBestRouter: false)
  }

  private func waitUntilRouteReady() {
    let ready = XCTNSPredicateExpectation(predicate: NSPredicate { [manager] _, _ in
      manager?.state == .ready
    }, object: nil)
    wait(for: [ready], timeout: 5)
    XCTAssertTrue(MWMRouter.isRoutingActive())
  }

  private func waitForDashboardsToClose() {
    let closed = XCTNSPredicateExpectation(predicate: NSPredicate { [weak self] _, _ in
      self?.dashboards.isEmpty == true
    }, object: nil)
    wait(for: [closed], timeout: 5)
  }

  private func makeElevationDashboard() -> (NavigationDashboardViewController, NavigationDashboard.Interactor) {
    ElevationRouter.selectedDistances = []
    ElevationRouter.resetCount = 0
    let dashboard = NavigationDashboardViewController()
    let interactor = NavigationDashboard.Interactor(presenter: NavigationDashboard.Presenter(view: dashboard),
                                                    router: ElevationRouter.self)
    dashboard.interactor = interactor
    dashboard.add(to: mapViewController)
    return (dashboard, interactor)
  }
}

private final class MockDashboardManager: MWMNavigationDashboardManager {
  var dashboardRequests = 0

  @objc(navigationDashboardView)
  func dashboardView() -> AnyObject? {
    dashboardRequests += 1
    return nil
  }
}

private final class ElevationRouter: MWMRouter {
  static var selectedDistances: [Double] = []
  static var resetCount = 0

  override class func setRouteElevationActivePointDistance(_ distance: Double) {
    selectedDistances.append(distance)
  }

  override class func resetRouteElevationActivePoint() {
    resetCount += 1
  }

  override class func routeElevationProfileData() -> RouteElevationPreviewData? {
    RouteElevationPreviewData(trackInfo: TrackInfo.empty(), elevationInfo: nil)
  }
}
