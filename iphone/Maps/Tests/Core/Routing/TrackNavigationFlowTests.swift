@testable import Organic_Maps__Debug_
import XCTest

final class TrackNavigationFlowTests: XCTestCase {
  func testCorePointsAndProfileArePassedToThePlanningModel() throws {
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
        XCTAssertEqual(model.routePoints.points, points)
        XCTAssertEqual(model.planningMode, mode)
        XCTAssertEqual(model.routerType, .bicycle)
        XCTAssertEqual(model.transportOptions, mode == .navigateTrack ? [.vehicle, .pedestrian, .bicycle] : MWMRouterType.allCases)
      }
    }
  }

  func testMovingTrackRowsKeepsTrackPlanningMode() {
    var points = NavigationDashboard.RoutePoints(points: suppliedPoints, planningMode: .navigateTrack)
    points.movePoint(from: 1, to: 2)
    XCTAssertEqual(points.planningMode, .navigateTrack)
    XCTAssertEqual(points.title(for: 1), "Lake pier")
    XCTAssertEqual(points.title(for: 2), "Forest entrance")
  }

  func testNativeTrackDropSwapsVisibleRowsAndReversesOnlyOnAcceptedDrop() throws {
    let map = try XCTUnwrap(MapViewController.shared())
    map.loadViewIfNeeded()
    let window = try XCTUnwrap(MapsAppDelegate.theApp().window)
    let dashboard = NavigationDashboardViewController()
    dashboard.loadViewIfNeeded()
    let presenter = NavigationDashboard.Presenter(view: dashboard)
    let interactor = NavigationDashboard.Interactor(presenter: presenter, router: MockRouter.self)
    let view = RoutePointsView()
    view.interactor = interactor
    view.frame = CGRect(x: 20, y: 180, width: window.bounds.width - 40, height: 240)
    window.addSubview(view)
    MockRouter.trackMode = true
    MockRouter.corePoints = suppliedPoints
    MockRouter.reverseCount = 0
    defer {
      view.removeFromSuperview()
      MockRouter.trackMode = false
      MockRouter.corePoints = []
      MockRouter.reverseCount = 0
    }
    let collection = try XCTUnwrap(view.subviews.first as? UICollectionView)
    for source in [1, 2] {
      view.setRoutePoints(.init(points: MockRouter.corePoints, planningMode: .navigateTrack))
      view.layoutIfNeeded()
      collection.layoutIfNeeded()
      XCTAssertTrue(collection.dragInteractionEnabled)
      let session = MockDragSession()
      XCTAssertTrue(view.collectionView(collection, itemsForBeginning: session,
                                        at: IndexPath(item: 0, section: 0)).isEmpty)
      let sourcePath = IndexPath(item: source, section: 0)
      let destinationPath = IndexPath(item: source == 1 ? 2 : 1, section: 0)
      let sourceCell = try XCTUnwrap(collection.cellForItem(at: sourcePath))
      let handle = sourceCell.hitTest(CGPoint(x: sourceCell.bounds.maxX - 24, y: sourceCell.bounds.midY), with: nil)
      XCTAssertFalse(handle is UIControl, "The decorative handle must not consume the native drag gesture")
      let item = try XCTUnwrap(view.collectionView(collection, itemsForBeginning: session, at: sourcePath).first)
      session.items = [item]
      let preview = try XCTUnwrap(view.collectionView(collection, dragPreviewParametersForItemAt: sourcePath))
      XCTAssertNil(preview.visiblePath, "Use the same full-cell drag preview as ordinary routes")
      XCTAssertEqual(view.collectionView(collection, dropSessionDidUpdate: session,
                                         withDestinationIndexPath: IndexPath(item: 0, section: 0)).operation, .forbidden)
      XCTAssertEqual(view.collectionView(collection, dropSessionDidUpdate: session,
                                         withDestinationIndexPath: sourcePath).operation, .forbidden)
      XCTAssertEqual(MockRouter.reverseCount, source - 1, "Hovering must not reverse the route")
      let stationaryDrop = MockDropCoordinator(item: item, source: sourcePath, destination: sourcePath, session: session)
      view.collectionView(collection, performDropWith: stationaryDrop)
      stationaryDrop.finish()
      XCTAssertNil(stationaryDrop.droppedAt)
      XCTAssertEqual(MockRouter.reverseCount, source - 1, "Dropping in the same slot must not reverse")
      let original = MockRouter.corePoints.map(\.title)
      let coordinator = MockDropCoordinator(item: item, source: sourcePath, destination: destinationPath, session: session)
      view.collectionView(collection, performDropWith: coordinator)
      XCTAssertEqual(MockRouter.reverseCount, source, "Reverse only after a changed drop is accepted")
      XCTAssertEqual(coordinator.droppedAt, destinationPath)
      XCTAssertEqual(collection.numberOfItems(inSection: 0), 3, "Track mode must not gain Add destination")
      let local = try XCTUnwrap(Mirror(reflecting: view).children.first { $0.label == "routePoints" }?.value as? NavigationDashboard.RoutePoints)
      XCTAssertEqual(local.points.map(\.title), [original[0], original[2], original[1]])
      XCTAssertEqual(collection.cellForItem(at: IndexPath(item: 1, section: 0))?.accessibilityLabel, original[2])
      XCTAssertEqual(collection.cellForItem(at: IndexPath(item: 2, section: 0))?.accessibilityLabel, original[1])
      coordinator.finish()
      XCTAssertEqual(MockRouter.reverseCount, source)
      XCTAssertEqual(MockRouter.corePoints.map(\.title), local.points.map(\.title))
      view.setRoutePoints(.init(points: MockRouter.corePoints, planningMode: .navigateTrack))
      XCTAssertEqual(collection.numberOfItems(inSection: 0), 3)
    }
    let session = MockDragSession()
    let source = IndexPath(item: 1, section: 0)
    let item = try XCTUnwrap(view.collectionView(collection, itemsForBeginning: session, at: source).first)
    let drop = MockDropCoordinator(item: item, source: source, destination: IndexPath(item: 2, section: 0), session: session)
    view.collectionView(collection, performDropWith: drop)
    view.setRoutePoints(.init(points: [suppliedPoints[0], suppliedPoints[2]], planningMode: .navigateTrack))
    XCTAssertEqual(collection.numberOfItems(inSection: 0), 3, "Do not replace the drop target mid-animation")
    drop.finish()
    XCTAssertEqual(collection.numberOfItems(inSection: 0), 2, "Apply the latest core points when the drop finishes")
    XCTAssertEqual(MockRouter.reverseCount, 3)
    XCTAssertFalse(collection.dragInteractionEnabled)
    view.setRoutePoints(.init(points: suppliedPoints, planningMode: .default))
    XCTAssertTrue(collection.dragInteractionEnabled)
  }

  func testTrackReverseRemainsAvailableAfterBuildFailureOrCancellation() {
    var model = NavigationDashboard.ViewModel.initial
    model.routePoints = .init(points: [], planningMode: .navigateTrack)
    model.dashboardState = .ready
    XCTAssertFalse(model.canReverseRoutePoints)
    model.routePoints = .init(points: [suppliedPoints[0], suppliedPoints[2]], planningMode: .navigateTrack)
    for state in [MWMNavigationDashboardState.prepare, .ready, .error] {
      model.dashboardState = state
      XCTAssertTrue(model.canReverseRoutePoints)
    }
    for state in [MWMNavigationDashboardState.planning, .hidden, .navigation, .closed] {
      model.dashboardState = state
      XCTAssertFalse(model.canReverseRoutePoints)
    }
  }

  private var suppliedPoints: [MWMRoutePoint] {
    [
      MWMRoutePoint(cgPoint: .zero, title: "Your Location", subtitle: nil, type: .start, intermediateIndex: 0),
      MWMRoutePoint(cgPoint: .zero, title: "Forest entrance", subtitle: nil, type: .intermediate, intermediateIndex: 0),
      MWMRoutePoint(cgPoint: .zero, title: "Lake pier", subtitle: nil, type: .finish, intermediateIndex: 0),
    ]
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

  private final class MockDropItem: NSObject, UICollectionViewDropItem {
    let dragItem: UIDragItem
    let sourceIndexPath: IndexPath?
    var previewSize: CGSize { .zero }
    init(item: UIDragItem, source: IndexPath) {
      dragItem = item
      sourceIndexPath = source
    }
  }

  private final class MockDropCoordinator: NSObject, UICollectionViewDropCoordinator, UIDragAnimating {
    let items: [UICollectionViewDropItem]
    let destinationIndexPath: IndexPath?
    let session: UIDropSession
    let proposal = UICollectionViewDropProposal(operation: .move, intent: .insertAtDestinationIndexPath)
    private(set) var droppedAt: IndexPath?
    private var completions: [(UIViewAnimatingPosition) -> Void] = []
    init(item: UIDragItem, source: IndexPath, destination: IndexPath, session: UIDropSession) {
      items = [MockDropItem(item: item, source: source)]
      destinationIndexPath = destination
      self.session = session
    }

    func drop(_: UIDragItem, toItemAt indexPath: IndexPath) -> UIDragAnimating {
      droppedAt = indexPath
      return self
    }

    func drop(_: UIDragItem, intoItemAt _: IndexPath, rect _: CGRect) -> UIDragAnimating {
      XCTFail("Use the ordinary full-cell drop animation")
      return self
    }

    func drop(_: UIDragItem, to _: UIDragPreviewTarget) -> UIDragAnimating { self }
    func drop(_: UIDragItem, to _: UICollectionViewDropPlaceholder) -> UICollectionViewDropPlaceholderContext {
      fatalError("Placeholders are not used for local reordering")
    }

    func addAnimations(_ animations: @escaping () -> Void) { animations() }
    func addCompletion(_ completion: @escaping (UIViewAnimatingPosition) -> Void) { completions.append(completion) }
    func finish() {
      let callbacks = completions
      completions.removeAll()
      callbacks.forEach { $0(.end) }
    }
  }

  private final class MockRouter: MWMRouter {
    static var reverseCount = 0
    override class func reverseRoute() {
      reverseCount += 1
      corePoints.swapAt(1, 2)
    }

    static var trackMode = false
    static var corePoints: [MWMRoutePoint] = []
    override class func isTrackRoute() -> Bool { trackMode }
    override class func points() -> [MWMRoutePoint] { corePoints }
    override class func type() -> MWMRouterType { .bicycle }
  }
}
