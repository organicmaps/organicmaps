@testable import Chart
@testable import Organic_Maps__Debug_
import XCTest

final class TrackNavigationFlowTests: XCTestCase {
  func testTrackAndOrdinaryRoutesUseTheirPlanningModels() throws {
    let controller = try XCTUnwrap(MapViewController.shared())
    controller.loadViewIfNeeded()
    let view = NavigationDashboardViewController()
    let presenter = NavigationDashboard.Presenter(view: view)
    let interactor = NavigationDashboard.Interactor(presenter: presenter, router: MockRouter.self)
    view.interactor = interactor
    view.loadViewIfNeeded()
    defer {
      MockRouter.trackMode = false
      MockRouter.corePoints = []
    }
    for mode in [NavigationDashboard.PlanningMode.navigateTrack, .default] {
      MockRouter.trackMode = mode == .navigateTrack
      for points in [suppliedPoints, [suppliedPoints[0], suppliedPoints[2]]] {
        MockRouter.corePoints = points
        interactor.process(.updateRoutePoints)
        let model = try XCTUnwrap(Mirror(reflecting: presenter).children
          .first { $0.label == "viewModel" }?.value as? NavigationDashboard.ViewModel)
        XCTAssertEqual(model.routePoints.points, mode == .default ? points : [])
        XCTAssertEqual(model.planningMode, mode)
        XCTAssertEqual(model.routerType, .bicycle)
        XCTAssertEqual(model.transportOptions, mode == .navigateTrack ? [.vehicle, .pedestrian, .bicycle] : MWMRouterType.allCases)
      }
    }
  }

  func testTrackProfileUsesCoreAPIWithoutOrdinaryRebuild() throws {
    let view = NavigationDashboardViewController()
    let presenter = NavigationDashboard.Presenter(view: view)
    let interactor = NavigationDashboard.Interactor(presenter: presenter, router: MockRouter.self)
    view.interactor = interactor
    view.loadViewIfNeeded()
    MockRouter.trackMode = true
    MockRouter.corePoints = suppliedPoints
    MockRouter.profile = .bicycle
    MockRouter.profileRequests = []
    MockRouter.globalTypeChanges = 0
    MockRouter.ordinaryRebuilds = 0
    defer {
      MockRouter.trackMode = false
      MockRouter.corePoints = []
      MockRouter.profile = .bicycle
      MockRouter.result = .ok
    }
    interactor.process(.updateRoutePoints)
    interactor.process(.selectRouterType(.pedestrian))
    XCTAssertEqual(MockRouter.profileRequests, [.pedestrian])
    XCTAssertEqual(MockRouter.globalTypeChanges, 0)
    XCTAssertEqual(MockRouter.ordinaryRebuilds, 0)
    interactor.process(.selectRouterType(.publicTransport))
    interactor.process(.selectRouterType(.ruler))
    XCTAssertEqual(MockRouter.profileRequests, [.pedestrian])
    MockRouter.result = .busy
    interactor.process(.selectRouterType(.vehicle))
    let model = try XCTUnwrap(Mirror(reflecting: presenter).children
      .first { $0.label == "viewModel" }?.value as? NavigationDashboard.ViewModel)
    XCTAssertEqual(model.routerType, .pedestrian)
  }

  func testTrackEntryUpdatesOnlyWhenChartSelectionFinishes() {
    let view = NavigationDashboardViewController()
    let presenter = NavigationDashboard.Presenter(view: view)
    let interactor = NavigationDashboard.Interactor(presenter: presenter, router: MockRouter.self)
    view.interactor = interactor
    view.loadViewIfNeeded()
    MockRouter.trackMode = true
    MockRouter.profile = .bicycle
    MockRouter.profileRequests = []
    MockRouter.entryRequests = []
    MockRouter.mapPointRequests = []
    MockRouter.globalTypeChanges = 0
    MockRouter.ordinaryRebuilds = 0
    defer {
      MockRouter.trackMode = false
      MockRouter.onRoute = false
      MockRouter.profile = .bicycle
      MockRouter.result = .ok
      MockRouter.entryRequests = []
      MockRouter.mapPointRequests = []
    }
    interactor.process(.selectRouterType(.pedestrian))
    interactor.updateMapPoint(distance: 120)
    interactor.updateMapPoint(distance: 250)
    XCTAssertEqual(MockRouter.mapPointRequests, [120, 250])
    XCTAssertTrue(MockRouter.entryRequests.isEmpty)

    interactor.didFinishSelectingPoint(distance: 250)
    XCTAssertEqual(MockRouter.entryRequests, [250])
    XCTAssertEqual(MockRouter.profile, .pedestrian)
    XCTAssertEqual(MockRouter.profileRequests, [.pedestrian])
    XCTAssertEqual(MockRouter.globalTypeChanges, 0)
    XCTAssertEqual(MockRouter.ordinaryRebuilds, 0)

    MockRouter.onRoute = true
    interactor.didFinishSelectingPoint(distance: 300)
    MockRouter.onRoute = false
    MockRouter.trackMode = false
    interactor.didFinishSelectingPoint(distance: 350)
    MockRouter.trackMode = true
    interactor.invalidate()
    interactor.didFinishSelectingPoint(distance: 400)
    XCTAssertEqual(MockRouter.entryRequests, [250])
  }

  func testBusyEntryUpdatesRetryTheLatestSelectionOnceAndStopClearsIt() {
    let view = NavigationDashboardViewController()
    let presenter = NavigationDashboard.Presenter(view: view)
    let interactor = NavigationDashboard.Interactor(presenter: presenter, router: MockRouter.self)
    view.interactor = interactor
    view.loadViewIfNeeded()
    MockRouter.trackMode = true
    MockRouter.entryRequests = []
    MockRouter.result = .busy
    defer {
      MockRouter.trackMode = false
      MockRouter.result = .ok
      MockRouter.entryRequests = []
    }
    interactor.didFinishSelectingPoint(distance: 120)
    interactor.didFinishSelectingPoint(distance: 250)
    XCTAssertEqual(MockRouter.entryRequests, [120, 250])

    MockRouter.result = .ok
    interactor.stateReady()
    XCTAssertEqual(MockRouter.entryRequests, [120, 250, 250])
    interactor.stateReady()
    XCTAssertEqual(MockRouter.entryRequests, [120, 250, 250])

    MockRouter.result = .busy
    interactor.didFinishSelectingPoint(distance: 400)
    interactor.onRouteStop()
    MockRouter.result = .ok
    interactor.stateReady()
    XCTAssertEqual(MockRouter.entryRequests, [120, 250, 250, 400])
  }

  func testZoomDisabledChartFinishesTapAndScrubAtTheReleasedPoint() {
    let chart = ChartView(frame: CGRect(x: 0, y: 0, width: 350, height: 176))
    chart.isUserInteractionEnabled = true
    chart.layoutIfNeeded()
    let data = ChartPresentationData(GestureChartData(), formatter: ElevationProfileFormatter(units: .metric))
    chart.chartData = data
    chart.isZoomEnabled = false
    var movedDistances: [Double] = []
    var finishedDistances: [Double] = []
    chart.onSelectedPointChanged = { movedDistances.append($0) }
    chart.onSelectionFinished = { finishedDistances.append($0) }
    var cancellations = 0
    chart.onSelectionCancelled = { cancellations += 1 }
    chart.setSelectedPoint(200)
    chart.chartData = data
    XCTAssertTrue(movedDistances.isEmpty)
    XCTAssertTrue(finishedDistances.isEmpty)
    let zoomGestures = chart.chartsContainerView.gestureRecognizers?.filter {
      $0 is UIPinchGestureRecognizer || $0 is UIPanGestureRecognizer
    } ?? []
    XCTAssertFalse(zoomGestures.isEmpty)
    XCTAssertTrue(zoomGestures.allSatisfy { !$0.isEnabled })
    XCTAssertTrue(chart.chartInfoView.selectionGestureRecognizer.isEnabled)

    let tap = MockTapGestureRecognizer()
    tap.gestureState = .ended
    tap.point = CGPoint(x: 70, y: 30)
    chart.onTap(tap)
    XCTAssertEqual(finishedDistances.count, 1)
    XCTAssertEqual(finishedDistances.last, movedDistances.last)

    let scrub = MockScrubGestureRecognizer()
    scrub.gestureState = .began
    scrub.point = CGPoint(x: 100, y: 30)
    XCTAssertTrue(UIApplication.shared.sendAction(NSSelectorFromString("onScrub:"),
                                                  to: chart.chartInfoView, from: scrub, for: nil))
    XCTAssertEqual(finishedDistances.count, 1)
    scrub.gestureState = .ended
    scrub.point = CGPoint(x: 280, y: 30)
    XCTAssertTrue(UIApplication.shared.sendAction(NSSelectorFromString("onScrub:"),
                                                  to: chart.chartInfoView, from: scrub, for: nil))
    XCTAssertEqual(finishedDistances.count, 2)
    XCTAssertEqual(finishedDistances.last, movedDistances.last)
    XCTAssertNotEqual(finishedDistances.first, finishedDistances.last)
    for state in [UIGestureRecognizer.State.cancelled, .failed] {
      scrub.gestureState = .began
      XCTAssertTrue(UIApplication.shared.sendAction(NSSelectorFromString("onScrub:"),
                                                    to: chart.chartInfoView, from: scrub, for: nil))
      scrub.gestureState = state
      XCTAssertTrue(UIApplication.shared.sendAction(NSSelectorFromString("onScrub:"),
                                                    to: chart.chartInfoView, from: scrub, for: nil))
    }
    XCTAssertEqual(cancellations, 2)
    XCTAssertEqual(finishedDistances.count, 2)
  }

  func testCancelledTrackSelectionRestoresAcceptedEntryWithoutRebuilding() throws {
    let view = NavigationDashboardViewController()
    let presenter = NavigationDashboard.Presenter(view: view)
    let interactor = NavigationDashboard.Interactor(presenter: presenter, router: MockRouter.self)
    view.interactor = interactor
    view.loadViewIfNeeded()
    MockRouter.trackMode = true
    MockRouter.acceptedEntryDistance = 250
    MockRouter.elevationInfo = MockElevationProfileData()
    MockRouter.entryRequests = []
    MockRouter.mapPointRequests = []
    MockRouter.profile = .bicycle
    defer {
      interactor.invalidate()
      MockRouter.trackMode = false
      MockRouter.acceptedEntryDistance = nil
      MockRouter.elevationInfo = nil
      MockRouter.entryRequests = []
      MockRouter.mapPointRequests = []
      MockRouter.result = .ok
    }
    interactor.stateReady()
    interactor.updateMapPoint(distance: 700)
    interactor.didCancelSelectingPoint()
    var model = try XCTUnwrap(Mirror(reflecting: presenter).children
      .first { $0.label == "viewModel" }?.value as? NavigationDashboard.ViewModel)
    XCTAssertEqual(model.routeElevationActivePointDistance, 250)
    XCTAssertTrue(MockRouter.entryRequests.isEmpty)
    XCTAssertEqual(MockRouter.profile, .bicycle)

    MockRouter.result = .busy
    interactor.didFinishSelectingPoint(distance: 400)
    interactor.updateMapPoint(distance: 800)
    interactor.didCancelSelectingPoint()
    model = try XCTUnwrap(Mirror(reflecting: presenter).children
      .first { $0.label == "viewModel" }?.value as? NavigationDashboard.ViewModel)
    XCTAssertEqual(model.routeElevationActivePointDistance, 400)
    XCTAssertEqual(MockRouter.entryRequests, [400])
    let restored = expectation(description: "Selection restored after cancellation")
    DispatchQueue.main.async { restored.fulfill() }
    wait(for: [restored], timeout: 1)
    XCTAssertEqual(MockRouter.mapPointRequests.last, 400)
    MockRouter.result = .ok
    interactor.stateReady()
    XCTAssertEqual(MockRouter.entryRequests, [400, 400])
  }

  func testStateClosedClearsSelectionBeforeInvalidatingAndCancelsDeferredRestore() throws {
    let view = NavigationDashboardViewController()
    let presenter = NavigationDashboard.Presenter(view: view)
    let interactor = NavigationDashboard.Interactor(presenter: presenter, router: MockRouter.self)
    view.interactor = interactor
    view.loadViewIfNeeded()
    MockRouter.trackMode = true
    MockRouter.acceptedEntryDistance = 250
    MockRouter.elevationInfo = MockElevationProfileData()
    MockRouter.mapPointRequests = []
    MockRouter.selectionResets = 0
    defer {
      interactor.invalidate()
      MockRouter.trackMode = false
      MockRouter.acceptedEntryDistance = nil
      MockRouter.elevationInfo = nil
      MockRouter.mapPointRequests = []
      MockRouter.selectionResets = 0
    }
    interactor.stateReady()
    let drawnBeforeClose = MockRouter.mapPointRequests
    interactor.stateClosed()
    XCTAssertEqual(MockRouter.selectionResets, 1)
    let closed = try XCTUnwrap(Mirror(reflecting: presenter).children
      .first { $0.label == "viewModel" }?.value as? NavigationDashboard.ViewModel)
    XCTAssertEqual(closed.dashboardState, .closed)
    interactor.stateReady()
    interactor.stateClosed()
    let drained = expectation(description: "Deferred selection restore skipped")
    DispatchQueue.main.async { drained.fulfill() }
    wait(for: [drained], timeout: 1)
    XCTAssertEqual(MockRouter.mapPointRequests, drawnBeforeClose)
    XCTAssertEqual(MockRouter.selectionResets, 1)
  }

  func testPrepareKeepsElevationForRebuildButClearsItWithoutAnEndpoint() throws {
    let view = NavigationDashboardViewController()
    let presenter = NavigationDashboard.Presenter(view: view)
    view.loadViewIfNeeded()
    let data = RouteElevationPreviewData(trackInfo: TrackInfo.empty(), elevationInfo: MockElevationProfileData())
    presenter.process(.show(points: .init(points: suppliedPoints), routerType: .pedestrian))
    presenter.process(.updateState(.ready))
    presenter.process(.updateElevationInfo(data, activePointDistance: 250))
    presenter.process(.show(points: .init(points: suppliedPoints), routerType: .bicycle))
    presenter.process(.updateState(.prepare))
    var model = try XCTUnwrap(Mirror(reflecting: presenter).children
      .first { $0.label == "viewModel" }?.value as? NavigationDashboard.ViewModel)
    XCTAssertTrue(model.routeElevationPreviewData === data)
    presenter.process(.show(points: .init(points: [suppliedPoints[0]]), routerType: .bicycle))
    model = try XCTUnwrap(Mirror(reflecting: presenter).children
      .first { $0.label == "viewModel" }?.value as? NavigationDashboard.ViewModel)
    XCTAssertNil(model.routeElevationPreviewData)
    XCTAssertNil(model.routeElevationActivePointDistance)
  }

  func testRejectedTrackEntryRestoresTheAcceptedChartAndMapSelection() throws {
    let view = NavigationDashboardViewController()
    let presenter = NavigationDashboard.Presenter(view: view)
    let interactor = NavigationDashboard.Interactor(presenter: presenter, router: MockRouter.self)
    view.interactor = interactor
    view.loadViewIfNeeded()
    MockRouter.trackMode = true
    MockRouter.acceptedEntryDistance = 120
    MockRouter.elevationInfo = MockElevationProfileData()
    MockRouter.entryRequests = []
    MockRouter.mapPointRequests = []
    MockRouter.result = .ok
    defer {
      interactor.invalidate()
      MockRouter.trackMode = false
      MockRouter.acceptedEntryDistance = nil
      MockRouter.elevationInfo = nil
      MockRouter.entryRequests = []
      MockRouter.mapPointRequests = []
      MockRouter.result = .ok
    }
    interactor.updateMapPoint(distance: 250)
    interactor.didFinishSelectingPoint(distance: 250)
    interactor.stateReady()

    MockRouter.result = .noRemainingGeometry
    interactor.updateMapPoint(distance: 1000)
    interactor.didFinishSelectingPoint(distance: 1000)
    let model = try XCTUnwrap(Mirror(reflecting: presenter).children
      .first { $0.label == "viewModel" }?.value as? NavigationDashboard.ViewModel)
    XCTAssertEqual(model.routeElevationActivePointDistance, 250)
    XCTAssertEqual(MockRouter.acceptedEntryDistance, 250)
    XCTAssertEqual(MockRouter.entryRequests, [250, 1000])
    let restored = expectation(description: "Accepted map selection restored")
    DispatchQueue.main.async { restored.fulfill() }
    wait(for: [restored], timeout: 1)
    XCTAssertEqual(MockRouter.mapPointRequests.last, 250)
  }

  func testTrackPlannerHidesActionsAndRestoresOrdinaryLayoutAcrossSizesAndThemes() throws {
    let controller = NavigationDashboardViewController()
    controller.loadViewIfNeeded()
    var model = NavigationDashboard.ViewModel.initial
    model.dashboardState = .ready
    model.progress = 1
    for size in [CGSize(width: 375, height: 812), CGSize(width: 1024, height: 1366)] {
      for appearance in [UIUserInterfaceStyle.light, .dark] {
        controller.overrideUserInterfaceStyle = appearance
        controller.view.frame = CGRect(origin: .zero, size: size)
        for mode in [NavigationDashboard.PlanningMode.navigateTrack, .default] {
          model.routePoints = mode == .navigateTrack
            ? .init(trackName: "Forest entrance") : .init(points: suppliedPoints)
          controller.render(model)
          controller.view.layoutIfNeeded()
          let start = try XCTUnwrap(Mirror(reflecting: controller).children.first { $0.label == "startRouteButton" }?.value as? StartRouteButton)
          XCTAssertEqual(start.bounds.height, 44, accuracy: 0.5)
          let reverse = try XCTUnwrap(Mirror(reflecting: controller).children
            .first { $0.label == "reverseRoutePointsButton" }?.value as? UIButton)
          XCTAssertEqual(reverse.isHidden, mode == .navigateTrack)
          for name in ["searchButton", "bookmarksButton", "saveRouteAsTrackButton", "settingsButton"] {
            let button = try XCTUnwrap(Mirror(reflecting: controller).children.first { $0.label == name }?.value as? UIButton)
            XCTAssertEqual(button.isHidden, mode == .navigateTrack)
          }
        }
      }
    }
  }

  func testTrackRowsCannotBeReordered() {
    var points = NavigationDashboard.RoutePoints(trackName: "Forest entrance")
    let original = (0 ..< points.count).map { points.title(for: $0) }
    for index in 0 ..< points.count {
      XCTAssertFalse(points.canMovePoint(at: index))
    }
    points.movePoint(from: 1, to: 2)
    XCTAssertEqual((0 ..< points.count).map { points.title(for: $0) }, original)
  }

  func testTrackRowsDisableDragAndRestoreOrdinaryRouting() throws {
    let map = try XCTUnwrap(MapViewController.shared())
    map.loadViewIfNeeded()
    let window = try XCTUnwrap(MapsAppDelegate.theApp().window)
    let view = RoutePointsView()
    view.frame = CGRect(x: 20, y: 180, width: window.bounds.width - 40, height: 240)
    window.addSubview(view)
    defer { view.removeFromSuperview() }
    view.setRoutePoints(.init(trackName: "Forest entrance"))
    view.layoutIfNeeded()
    let collection = try XCTUnwrap(view.subviews.first as? UICollectionView)
    collection.layoutIfNeeded()
    let session = MockDragSession()
    XCTAssertFalse(collection.dragInteractionEnabled)
    XCTAssertFalse(view.collectionView(collection, canHandle: session))
    for index in 0 ..< 3 {
      let path = IndexPath(item: index, section: 0)
      XCTAssertTrue(view.collectionView(collection, itemsForBeginning: session, at: path).isEmpty)
      XCTAssertEqual(view.collectionView(collection, dropSessionDidUpdate: session,
                                         withDestinationIndexPath: path).operation, .forbidden)
      let cell = try XCTUnwrap(collection.cellForItem(at: path))
      let background = try XCTUnwrap(cell.contentView.subviews.first)
      XCTAssertTrue(background.subviews.compactMap { $0 as? UIButton }.allSatisfy(\.isHidden))
    }
    view.setRoutePoints(.init(points: suppliedPoints))
    XCTAssertTrue(collection.dragInteractionEnabled)
    XCTAssertTrue(view.collectionView(collection, canHandle: session))
    XCTAssertFalse(view.collectionView(collection, itemsForBeginning: session,
                                       at: IndexPath(item: 1, section: 0)).isEmpty)
  }

  func testRepeatedPlanningReplacesDashboardWhilePreviousDismissalIsPending() throws {
    let map = try XCTUnwrap(MapViewController.shared())
    map.loadViewIfNeeded()
    let originalChildren = Set(map.children.map(ObjectIdentifier.init))
    let manager = MWMNavigationDashboardManager(parentViewController: map)
    var retainedDashboards: [NavigationDashboardViewController] = []
    defer {
      manager.onRouteStop()
      for child in map.children where !originalChildren.contains(ObjectIdentifier(child)) {
        child.willMove(toParent: nil)
        child.view.removeFromSuperview()
        child.removeFromParent()
      }
    }
    for attempt in 0 ..< 3 {
      manager.onRoutePlanning()
      manager.onRouteReady(false)
      let interactor = try XCTUnwrap(manager.value(forKey: "navigationDashboardView") as? NavigationDashboard.Interactor)
      let dashboard = try XCTUnwrap(map.children.compactMap { $0 as? NavigationDashboardViewController }
        .first { $0.interactor === interactor })
      if let previous = retainedDashboards.last {
        XCTAssertFalse(dashboard === previous, "A new route must not reuse the closing dashboard")
      }
      retainedDashboards.append(dashboard)
      let presenter = try XCTUnwrap(Mirror(reflecting: interactor).children
        .first { $0.label == "presenter" }?.value as? NavigationDashboard.Presenter)
      let model = try XCTUnwrap(Mirror(reflecting: presenter).children
        .first { $0.label == "viewModel" }?.value as? NavigationDashboard.ViewModel)
      XCTAssertEqual(model.dashboardState, .ready)
      XCTAssertNotEqual(model.presentationStep, .hidden)
      XCTAssertFalse(model.isBottomActionsMenuHidden)
      XCTAssertTrue(dashboard.view.superview === map.view)
      XCTAssertTrue(manager.availableAreaView === dashboard.availableAreaView)
      if attempt < 2 {
        manager.onSelectPlacePage(true)
        manager.onSelectPlacePage(false)
        manager.onRouteStop()
        XCTAssertNil(manager.availableAreaView)
      }
    }
    let animationsFinished = expectation(description: "Previous dismissals finished")
    DispatchQueue.main.asyncAfter(deadline: .now() + AppConstants.defaultAnimationDuration + 0.1) {
      animationsFinished.fulfill()
    }
    wait(for: [animationsFinished], timeout: 2)
    let visible = try XCTUnwrap(retainedDashboards.last)
    XCTAssertTrue(visible.parent === map)
    XCTAssertTrue(visible.view.window === map.view.window)
    XCTAssertTrue(visible.availableAreaView.frame.intersects(visible.view.bounds))
  }

  func testStoppingWithoutDashboardDoesNotCreateOne() throws {
    let map = try XCTUnwrap(MapViewController.shared())
    map.loadViewIfNeeded()
    let originalChildren = Set(map.children.map(ObjectIdentifier.init))
    let manager = MWMNavigationDashboardManager(parentViewController: map)
    defer {
      for child in map.children where !originalChildren.contains(ObjectIdentifier(child)) {
        child.willMove(toParent: nil)
        child.view.removeFromSuperview()
        child.removeFromParent()
      }
    }
    manager.onRouteStop()
    XCTAssertEqual(Set(map.children.map(ObjectIdentifier.init)), originalChildren)
    XCTAssertNil(manager.availableAreaView)
  }

  private var suppliedPoints: [MWMRoutePoint] {
    [
      MWMRoutePoint(cgPoint: .zero, title: "Your Location", subtitle: nil, type: .start, intermediateIndex: 0),
      MWMRoutePoint(cgPoint: .zero, title: "Forest entrance", subtitle: nil, type: .intermediate, intermediateIndex: 0),
      MWMRoutePoint(cgPoint: .zero, title: "Lake pier", subtitle: nil, type: .finish, intermediateIndex: 0),
    ]
  }

  private struct GestureChartData: ChartData {
    let xAxisValues = Array(stride(from: 0.0, through: 1000.0, by: 100.0))
    var type: ChartType { .regular }
    var lines: [ChartLine] {
      let values = xAxisValues.map { ChartValue(xValues: CGFloat($0), y: CGFloat(100 + $0 / 10)) }
      return [ElevationProfileChartData.Line(values: values, color: .blue, type: .line)]
    }
  }

  private final class MockTapGestureRecognizer: UITapGestureRecognizer {
    var gestureState: UIGestureRecognizer.State = .possible
    var point: CGPoint = .zero
    override var state: UIGestureRecognizer.State {
      get { gestureState }
      set { gestureState = newValue }
    }

    override func location(in _: UIView?) -> CGPoint { point }
  }

  private final class MockScrubGestureRecognizer: UILongPressGestureRecognizer {
    var gestureState: UIGestureRecognizer.State = .possible
    var point: CGPoint = .zero
    override var state: UIGestureRecognizer.State {
      get { gestureState }
      set { gestureState = newValue }
    }

    override func location(in _: UIView?) -> CGPoint { point }
  }

  private final class MockDragSession: NSObject, UIDragSession, UIDropSession {
    var items: [UIDragItem] = []
    var localContext: Any?
    var localDragSession: UIDragSession? { self }
    var allowsMoveOperation: Bool { true }
    var isRestrictedToDraggingApplication: Bool { true }
    var progressIndicatorStyle: UIDropSessionProgressIndicatorStyle = .none
    let progress = Progress(totalUnitCount: 1)
    func location(in _: UIView) -> CGPoint { .zero }
    func hasItemsConforming(toTypeIdentifiers _: [String]) -> Bool { true }
    func canLoadObjects(ofClass _: NSItemProviderReading.Type) -> Bool { true }
    func loadObjects(ofClass _: NSItemProviderReading.Type,
                     completion: @escaping ([NSItemProviderReading]) -> Void) -> Progress {
      completion([])
      return progress
    }
  }

  private final class MockTrackInfo: MWMFollowedTrackInfo {
    override var name: String { "Forest entrance" }
  }

  private final class MockElevationProfileData: ElevationProfileData {
    override var points: [ElevationHeightPoint] {
      [ElevationHeightPoint(distance: 0, altitude: 100), ElevationHeightPoint(distance: 1000, altitude: 200)]
    }

    override var segmentDistances: [NSNumber] { [] }
  }

  private final class MockRouter: MWMRouter {
    static var profile: MWMRouterType = .bicycle
    static var result: MWMFollowTrackResult = .ok
    static var profileRequests: [MWMRouterType] = []
    static var globalTypeChanges = 0
    static var ordinaryRebuilds = 0
    static var entryRequests: [Double] = []
    static var mapPointRequests: [Double] = []
    static var acceptedEntryDistance: Double?
    static var elevationInfo: ElevationProfileData?
    static var selectionResets = 0
    static var onRoute = false
    override class func updateFollowedTrackEntry(atDistance distance: Double) -> MWMFollowTrackResult {
      entryRequests.append(distance)
      if result == .ok, acceptedEntryDistance != nil {
        acceptedEntryDistance = distance
      }
      return result
    }

    override class func setRouteElevationActivePointDistance(_ distance: Double) {
      mapPointRequests.append(distance)
    }

    override class func isOnRoute() -> Bool { onRoute }
    override class func routeElevationProfileData() -> RouteElevationPreviewData? {
      guard let elevationInfo else { return nil }
      return RouteElevationPreviewData(trackInfo: TrackInfo.empty(), elevationInfo: elevationInfo)
    }

    override class func routeElevationActivePointDistance() -> NSNumber? {
      acceptedEntryDistance.map { NSNumber(value: $0) }
    }

    override class func resetRouteElevationActivePoint() { selectionResets += 1 }
    override class func setFollowTrackProfile(_ type: MWMRouterType) -> MWMFollowTrackResult {
      profileRequests.append(type)
      if result == .ok {
        profile = type
      }
      return result
    }

    override class func setType(_: MWMRouterType) { globalTypeChanges += 1 }
    override class func rebuild(withBestRouter _: Bool) { ordinaryRebuilds += 1 }
    static var trackMode = false
    static var corePoints: [MWMRoutePoint] = []
    override class func isFollowingTrack() -> Bool { trackMode }
    override class func points() -> [MWMRoutePoint] { trackMode ? [] : corePoints }
    override class func renderType() -> MWMRouterType { profile }
    override class func followedTrackInfo() -> MWMFollowedTrackInfo? { trackMode ? MockTrackInfo() : nil }
  }
}
