@testable import Chart
@testable import Organic_Maps__Debug_
import XCTest

final class TrackNavigationFlowTests: XCTestCase {
  func testReadyUsesTheLatestAcceptedCoreEntry() throws {
    let renderer = DashboardRendererSpy()
    let interactor = NavigationDashboard.Interactor(presenter: .init(view: renderer), router: MockRouter.self)
    MockRouter.trackMode = true
    MockRouter.acceptedEntryDistance = 100
    MockRouter.elevationInfo = MockElevationProfileData()
    MockRouter.mapPointRequests = []
    MockRouter.entryRequests = []
    defer {
      interactor.invalidate()
      MockRouter.trackMode = false
      MockRouter.acceptedEntryDistance = nil
      MockRouter.elevationInfo = nil
      MockRouter.mapPointRequests = []
      MockRouter.entryRequests = []
    }
    interactor.statePlanning()
    interactor.updateMapPoint(distance: 250)
    interactor.didFinishSelectingPoint(distance: 250)
    interactor.stateReady()
    MockRouter.acceptedEntryDistance = 400
    interactor.stateReady()
    XCTAssertEqual(try XCTUnwrap(renderer.model).routeElevationActivePointDistance, 400)

    MockRouter.elevationInfo = nil
    interactor.stateReady()
    let restored = expectation(description: "Accepted entry restored without elevation data")
    DispatchQueue.main.async { restored.fulfill() }
    wait(for: [restored], timeout: 1)
    XCTAssertEqual(MockRouter.mapPointRequests.last, 400)
  }

  func testReadyKeepsPendingSelectionUntilCancelled() throws {
    let renderer = DashboardRendererSpy()
    let interactor = NavigationDashboard.Interactor(presenter: .init(view: renderer), router: MockRouter.self)
    MockRouter.trackMode = true
    MockRouter.acceptedEntryDistance = 100
    MockRouter.elevationInfo = MockElevationProfileData()
    MockRouter.mapPointRequests = []
    defer {
      interactor.invalidate()
      MockRouter.trackMode = false
      MockRouter.acceptedEntryDistance = nil
      MockRouter.elevationInfo = nil
      MockRouter.mapPointRequests = []
    }
    interactor.statePlanning()
    interactor.updateMapPoint(distance: 250)
    interactor.stateReady()
    let ready = expectation(description: "Pending selection survives ready")
    DispatchQueue.main.async { ready.fulfill() }
    wait(for: [ready], timeout: 1)
    XCTAssertEqual(try XCTUnwrap(renderer.model).routeElevationActivePointDistance, 250)
    XCTAssertEqual(MockRouter.acceptedEntryDistance, 100)

    interactor.didCancelSelectingPoint()
    XCTAssertEqual(try XCTUnwrap(renderer.model).routeElevationActivePointDistance, 100)
    XCTAssertEqual(MockRouter.mapPointRequests.last, 100)
  }

  func testWaterEstimateWaitsForReadyEvenAfterProgressReachesOne() {
    var model = NavigationDashboard.ViewModel.initial
    model.routePoints = .init(points: suppliedPoints)
    model.routerType = .water
    model.dashboardState = .planning
    model.progress = 1
    model.estimates = NSAttributedString(string: "250 m")
    XCTAssertEqual(model.estimatesState, .loading)
    XCTAssertTrue(model.isStartButtonHidden)

    model.dashboardState = .ready
    XCTAssertEqual(model.estimatesState, .estimates(model.estimates))
  }

  func testTrackBuildKeepsStartAvailableAndUsesTheLastEstimate() {
    var model = NavigationDashboard.ViewModel.initial
    model.dashboardState = .planning
    model.routerType = .bicycle
    model.routePoints = .init(track: MockTrackInfo())
    model.progress = 0
    let emptyEstimate = NSAttributedString()
    model.estimates = emptyEstimate

    XCTAssertEqual(model.startButtonState, .enabled)
    XCTAssertEqual(model.estimatesState, .estimates(emptyEstimate))

    let lastEstimate = NSAttributedString(string: "10 min • 2 km")
    model.estimates = lastEstimate
    model.progress = 0.5
    XCTAssertEqual(model.startButtonState, .enabled)
    XCTAssertEqual(model.estimatesState, .estimates(lastEstimate))

    model.routePoints = .init(points: suppliedPoints)
    XCTAssertEqual(model.startButtonState, .loading(progress: 0.5))
    XCTAssertEqual(model.estimatesState, .loading)
  }

  func testTrackAndOrdinaryRoutesUseTheirPlanningModels() throws {
    let renderer = DashboardRendererSpy()
    let presenter = NavigationDashboard.Presenter(view: renderer)
    let interactor = NavigationDashboard.Interactor(presenter: presenter, router: MockRouter.self)
    defer {
      MockRouter.trackMode = false
      MockRouter.corePoints = []
    }

    MockRouter.trackMode = true
    interactor.process(.updateRoutePoints)
    let track = try XCTUnwrap(renderer.model)
    XCTAssertEqual(track.planningMode, .navigateTrack)
    XCTAssertTrue(track.routePoints.points.isEmpty)
    XCTAssertEqual(track.routePoints.title(for: 1), L("navigate_track_entry"))
    XCTAssertEqual(track.routerType, .bicycle)
    XCTAssertEqual(track.transportOptions, [.vehicle, .pedestrian, .bicycle])

    MockRouter.trackMode = false
    for points in [suppliedPoints, [suppliedPoints[0], suppliedPoints[2]]] {
      MockRouter.corePoints = points
      interactor.process(.updateRoutePoints)
      let road = try XCTUnwrap(renderer.model)
      XCTAssertEqual(road.routePoints.points, points)
      XCTAssertEqual(road.planningMode, .default)
      XCTAssertEqual(road.routerType, .bicycle)
      XCTAssertEqual(road.transportOptions, MWMRouterType.allCases)
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

  func testAcceptedTrackEntryDoesNotRebuildOnAnUnchangedCompletion() {
    let view = NavigationDashboardViewController()
    let interactor = NavigationDashboard.Interactor(presenter: .init(view: view), router: MockRouter.self)
    MockRouter.trackMode = true
    MockRouter.acceptedEntryDistance = 250
    MockRouter.entryRequests = []
    defer {
      interactor.invalidate()
      MockRouter.trackMode = false
      MockRouter.acceptedEntryDistance = nil
      MockRouter.entryRequests = []
    }
    interactor.updateMapPoint(distance: 250)
    interactor.didFinishSelectingPoint(distance: 250)
    XCTAssertTrue(MockRouter.entryRequests.isEmpty)
    interactor.updateMapPoint(distance: 350)
    interactor.didFinishSelectingPoint(distance: 350)
    XCTAssertEqual(MockRouter.entryRequests, [350])
    interactor.didFinishSelectingPoint(distance: 350)
    XCTAssertEqual(MockRouter.entryRequests, [350])
  }

  func testEmptyPlannerRejectsDraggingButSingleEndpointCanSwap() {
    let view = RoutePointsView()
    let collection = UICollectionView(frame: .zero, collectionViewLayout: UICollectionViewFlowLayout())
    let session = MockDragSession()
    for index in 0 ..< 2 {
      let path = IndexPath(item: index, section: 0)
      XCTAssertTrue(view.collectionView(collection, itemsForBeginning: session, at: path).isEmpty)
      XCTAssertEqual(view.collectionView(collection, dropSessionDidUpdate: session,
                                         withDestinationIndexPath: path).operation, .forbidden)
    }
    var points = NavigationDashboard.RoutePoints(points: [suppliedPoints[0]])
    XCTAssertTrue(points.canMovePoint(from: 0, to: 1))
    points.movePoint(from: 0, to: 1)
    XCTAssertNil(points.start)
    XCTAssertNotNil(points.finish)
  }

  func testZoomDisabledChartFinishesTapAndScrubAtTheReleasedPoint() {
    let chart = ChartView(frame: CGRect(x: 0, y: 0, width: 350, height: 176))
    chart.isUserInteractionEnabled = true
    chart.layoutIfNeeded()
    let data = ChartPresentationData(GestureChartData(), formatter: ElevationProfileFormatter(unitsProvider: { .metric }))
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
    let model = try XCTUnwrap(Mirror(reflecting: presenter).children
      .first { $0.label == "viewModel" }?.value as? NavigationDashboard.ViewModel)
    XCTAssertEqual(model.routeElevationActivePointDistance, 250)
    XCTAssertTrue(MockRouter.entryRequests.isEmpty)
    XCTAssertEqual(MockRouter.profile, .bicycle)

    let restored = expectation(description: "Selection restored after cancellation")
    DispatchQueue.main.async { restored.fulfill() }
    wait(for: [restored], timeout: 1)
    XCTAssertEqual(MockRouter.mapPointRequests.last, 250)
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

  func testReplacingTheDestinationKeepsTheOldElevationUntilReady() throws {
    let renderer = DashboardRendererSpy()
    let presenter = NavigationDashboard.Presenter(view: renderer)
    let data = RouteElevationPreviewData(trackInfo: TrackInfo.empty(), elevationInfo: MockElevationProfileData())
    let originalPoints = suppliedPoints
    presenter.process(.show(points: .init(points: originalPoints), routerType: .pedestrian))
    presenter.process(.updateState(.ready))
    presenter.process(.updateElevationInfo(data, activePointDistance: 250))
    XCTAssertTrue(renderer.model?.routeElevationPreviewData === data)

    let replacement = MWMRoutePoint(cgPoint: CGPoint(x: 1, y: 1), title: "New destination",
                                    subtitle: nil, type: .finish, intermediateIndex: 0)
    presenter.process(.updateState(.prepare))
    XCTAssertTrue(renderer.model?.routeElevationPreviewData === data)
    presenter.process(.updateState(.planning))
    presenter.process(.show(points: .init(points: Array(originalPoints.dropLast()) + [replacement]), routerType: .pedestrian))
    let model = try XCTUnwrap(renderer.model)
    XCTAssertTrue(model.routePoints.hasStartAndFinish)
    XCTAssertEqual(model.routePoints.finish?.title, "New destination")
    XCTAssertTrue(model.routeElevationPreviewData === data)

    let newData = RouteElevationPreviewData(trackInfo: TrackInfo.empty(), elevationInfo: MockElevationProfileData())
    presenter.process(.updateState(.ready))
    presenter.process(.updateElevationInfo(newData, activePointDistance: nil))
    XCTAssertTrue(renderer.model?.routeElevationPreviewData === newData)
    XCTAssertNil(renderer.model?.routeElevationActivePointDistance)
  }

  func testCancelledRoadRebuildClearsTheOldElevationWithBothEndpoints() throws {
    let map = try XCTUnwrap(MapViewController.shared())
    let manager = MWMNavigationDashboardManager(parentViewController: map)
    let renderer = DashboardRendererSpy()
    let presenter = NavigationDashboard.Presenter(view: renderer)
    let interactor = NavigationDashboard.Interactor(presenter: presenter, router: MockRouter.self)
    manager.setValue(interactor, forKey: "navigationDashboardView")
    let previousTrackMode = MockRouter.trackMode
    MockRouter.trackMode = false
    defer { MockRouter.trackMode = previousTrackMode }
    let data = RouteElevationPreviewData(trackInfo: TrackInfo.empty(), elevationInfo: MockElevationProfileData())
    presenter.process(.show(points: .init(points: suppliedPoints), routerType: .pedestrian))
    presenter.process(.updateState(.ready))
    presenter.process(.updateElevationInfo(data, activePointDistance: 250))
    presenter.process(.updateState(.planning))
    XCTAssertTrue(renderer.model?.routeElevationPreviewData === data)

    manager.onRoutePrepare()
    let model = try XCTUnwrap(renderer.model)
    XCTAssertTrue(model.routePoints.hasStartAndFinish)
    XCTAssertNil(model.routeElevationPreviewData)
    XCTAssertNil(model.routeElevationActivePointDistance)
  }

  func testRemovingTheDestinationClearsTheOldElevation() throws {
    let renderer = DashboardRendererSpy()
    let presenter = NavigationDashboard.Presenter(view: renderer)
    let data = RouteElevationPreviewData(trackInfo: TrackInfo.empty(), elevationInfo: MockElevationProfileData())
    let originalPoints = suppliedPoints
    presenter.process(.show(points: .init(points: [originalPoints[0], originalPoints[2]]), routerType: .pedestrian))
    presenter.process(.updateState(.ready))
    presenter.process(.updateElevationInfo(data, activePointDistance: 250))
    XCTAssertTrue(renderer.model?.routeElevationPreviewData === data)

    presenter.process(.updateState(.prepare))
    presenter.process(.show(points: .init(points: [originalPoints[0]]), routerType: .pedestrian))
    let model = try XCTUnwrap(renderer.model)
    XCTAssertFalse(model.routePoints.hasStartAndFinish)
    XCTAssertNil(model.routeElevationPreviewData)
    XCTAssertNil(model.routeElevationActivePointDistance)
  }

  func testSwitchingBetweenBicycleAndPedestrianKeepsTheChartUntilReady() throws {
    MockRouter.trackMode = false
    MockRouter.corePoints = suppliedPoints
    let previousProfile = MockRouter.profile
    defer {
      MockRouter.profile = previousProfile
      MockRouter.corePoints = []
      MockRouter.onOrdinaryRebuild = nil
      MockRouter.elevationInfo = nil
    }
    for (from, to): (MWMRouterType, MWMRouterType) in [(.pedestrian, .bicycle), (.bicycle, .pedestrian)] {
      let renderer = DashboardRendererSpy()
      let presenter = NavigationDashboard.Presenter(view: renderer)
      let interactor = NavigationDashboard.Interactor(presenter: presenter, router: MockRouter.self)
      MockRouter.profile = from
      MockRouter.elevationInfo = MockElevationProfileData()
      interactor.onRoutePointsUpdated()
      interactor.stateReady()
      let oldChart = try XCTUnwrap(renderer.model?.routeElevationPreviewData)

      MockRouter.onOrdinaryRebuild = {
        MockRouter.profile = to
        interactor.statePrepare()
        XCTAssertTrue(renderer.model?.routeElevationPreviewData === oldChart)
        interactor.statePlanning()
        XCTAssertTrue(renderer.model?.routeElevationPreviewData === oldChart)
      }
      interactor.process(.selectRouterType(to))
      XCTAssertEqual(renderer.model?.routerType, to)
      XCTAssertTrue(renderer.model?.routeElevationPreviewData === oldChart)

      MockRouter.elevationInfo = MockElevationProfileData()
      interactor.stateReady()
      let newChart = try XCTUnwrap(renderer.model?.routeElevationPreviewData)
      XCTAssertFalse(newChart === oldChart)
    }
  }

  func testCancellingSearchKeepsTheReadyRoadElevation() throws {
    let renderer = DashboardRendererSpy()
    let presenter = NavigationDashboard.Presenter(view: renderer)
    let data = RouteElevationPreviewData(trackInfo: TrackInfo.empty(), elevationInfo: MockElevationProfileData())
    presenter.process(.show(points: .init(points: suppliedPoints), routerType: .pedestrian))
    presenter.process(.updateState(.ready))
    presenter.process(.updateElevationInfo(data, activePointDistance: 250))

    presenter.process(.updateSearchState(.searching))
    presenter.process(.updateSearchState(.closed))

    let model = try XCTUnwrap(renderer.model)
    XCTAssertTrue(model.routeElevationPreviewData === data)
    XCTAssertEqual(model.routeElevationActivePointDistance, 250)
  }

  func testEditingTrackEntryKeepsTheTrackElevationWhilePlanning() throws {
    let renderer = DashboardRendererSpy()
    let presenter = NavigationDashboard.Presenter(view: renderer)
    let data = RouteElevationPreviewData(trackInfo: TrackInfo.empty(), elevationInfo: MockElevationProfileData())
    presenter.process(.show(points: .init(track: MockTrackInfo()), routerType: .pedestrian))
    presenter.process(.updateState(.ready))
    presenter.process(.updateElevationInfo(data, activePointDistance: 250))

    presenter.process(.updateState(.planning))

    let model = try XCTUnwrap(renderer.model)
    XCTAssertEqual(model.dashboardState, .planning)
    XCTAssertTrue(model.routeElevationPreviewData === data)
    XCTAssertEqual(model.routeElevationActivePointDistance, 250)
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

    MockRouter.result = .busy
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
    model.routerType = .pedestrian
    model.dashboardState = .ready
    model.progress = 1
    for size in [CGSize(width: 375, height: 812), CGSize(width: 1024, height: 1366)] {
      for appearance in [UIUserInterfaceStyle.light, .dark] {
        controller.overrideUserInterfaceStyle = appearance
        controller.view.frame = CGRect(origin: .zero, size: size)
        for mode in [NavigationDashboard.PlanningMode.navigateTrack, .default] {
          model.routePoints = mode == .navigateTrack
            ? .init(track: MockTrackInfo()) : .init(points: suppliedPoints)
          controller.render(model)
          controller.view.layoutIfNeeded()
          let start = try XCTUnwrap(Mirror(reflecting: controller).children.first { $0.label == "startRouteButton" }?.value as? StartRouteButton)
          XCTAssertFalse(start.isHidden)
          XCTAssertEqual(start.bounds.height, 44, accuracy: 0.5)
          let reverse = try XCTUnwrap(Mirror(reflecting: controller).children
            .first { $0.label == "reverseRoutePointsButton" }?.value as? UIButton)
          XCTAssertFalse(reverse.isHidden)
          if mode == .navigateTrack {
            XCTAssertTrue(reverse.isEnabled)
          }
          for name in ["searchButton", "bookmarksButton", "saveRouteAsTrackButton", "settingsButton"] {
            let button = try XCTUnwrap(Mirror(reflecting: controller).children.first { $0.label == name }?.value as? UIButton)
            XCTAssertEqual(button.isHidden, mode == .navigateTrack)
          }
        }
      }
    }
  }

  func testTrackRowsDisableDragAndRestoreOrdinaryRouting() throws {
    let map = try XCTUnwrap(MapViewController.shared())
    map.loadViewIfNeeded()
    let window = try XCTUnwrap(MapsAppDelegate.theApp().window)
    let view = RoutePointsView()
    view.frame = CGRect(x: 20, y: 180, width: window.bounds.width - 40, height: 240)
    window.addSubview(view)
    defer { view.removeFromSuperview() }
    view.setRoutePoints(.init(track: MockTrackInfo()))
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

  private var suppliedPoints: [MWMRoutePoint] {
    [
      MWMRoutePoint(cgPoint: .zero, title: "Your Location", subtitle: nil, type: .start, intermediateIndex: 0),
      MWMRoutePoint(cgPoint: .zero, title: "Forest entrance", subtitle: nil, type: .intermediate, intermediateIndex: 0),
      MWMRoutePoint(cgPoint: .zero, title: "Lake pier", subtitle: nil, type: .finish, intermediateIndex: 0),
    ]
  }

  private struct GestureChartData: ChartData {
    let xAxisValues = Array(stride(from: 0.0, through: 1000.0, by: 100.0))
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

  private final class DashboardRendererSpy: NavigationDashboard.Rendering {
    private(set) var model: NavigationDashboard.ViewModel?

    func render(_ viewModel: NavigationDashboard.ViewModel) {
      model = viewModel
    }

    func updateElevationActivePoint(_ distance: Double?) {
      model?.routeElevationActivePointDistance = distance
    }
  }

  private final class MockTrackInfo: MWMFollowedTrackInfo {
    override var name: String { "" }
    override var canReverse: Bool { true }
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
    static var onOrdinaryRebuild: (() -> Void)?
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
    override class func rebuild(withBestRouter _: Bool) {
      ordinaryRebuilds += 1
      onOrdinaryRebuild?()
    }

    static var trackMode = false
    static var corePoints: [MWMRoutePoint] = []
    override class func hasTrackNavigationSession() -> Bool { trackMode }
    override class func points() -> [MWMRoutePoint] { trackMode ? [] : corePoints }
    override class func renderType() -> MWMRouterType { profile }
    override class func followedTrackInfo() -> MWMFollowedTrackInfo? { trackMode ? MockTrackInfo() : nil }
  }
}
