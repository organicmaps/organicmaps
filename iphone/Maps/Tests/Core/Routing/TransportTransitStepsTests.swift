@testable import Organic_Maps__Debug_
import XCTest

final class TransportTransitStepsTests: XCTestCase {
  private let transportCases: [(type: MWMRouterTransitType, imageName: String)] = [
    (.subway, "ic_20px_route_planning_metro"),
    (.train, "ic_20px_route_planning_train"),
    (.lightRail, "ic_20px_route_planning_lightrail"),
    (.monorail, "ic_20px_route_planning_monorail"),
    (.tram, "ic_20px_route_planning_tram"),
    (.bus, "ic_20px_route_planning_bus"),
    (.ferry, "ic_20px_route_planning_ferry"),
    (.cableTram, "ic_20px_route_planning_tram"),
    (.aerialLift, "ic_20px_route_planning_aerial_lift"),
    (.funicular, "ic_20px_route_planning_funicular"),
    (.airService, "ic_20px_route_planning_air_service"),
    (.waterService, "ic_20px_route_planning_ferry"),
  ]

  func testTransportCellsKeepIconsAndNumbers() throws {
    let collection = TransportTransitStepsCollectionView(layout: TransportTransitFlowLayout())
    collection.frame = CGRect(x: 0, y: 0, width: 1000, height: 200)
    collection.steps = transportCases.map { makeStep(type: $0.type, number: "12") }
    collection.layoutIfNeeded()

    for (index, test) in transportCases.enumerated() {
      let type = test.type
      let cell = try XCTUnwrap(collection.cellForItem(at: IndexPath(item: index, section: 0)))
      XCTAssertTrue(cell is TransportTransitTrain, "\(type)")
      let views = descendants(of: cell.contentView)
      let icon = try XCTUnwrap(views.compactMap { $0 as? UIImageView }.first)
      let label = try XCTUnwrap(views.compactMap { $0 as? UILabel }.first)
      let expected = try XCTUnwrap(UIImage(named: test.imageName))
      XCTAssertEqual(icon.image?.pngData(), try XCTUnwrap(expected.pngData()), "\(type)")
      XCTAssertEqual(icon.image?.renderingMode, .alwaysTemplate, "\(type)")
      XCTAssertEqual(icon.image?.size, CGSize(width: 20, height: 20), "\(type)")
      XCTAssertEqual(label.text, "12")
      XCTAssertFalse(label.isHidden)
    }
  }

  func testTransportCellRecoversWhenLineNumberChanges() throws {
    let cell = try XCTUnwrap(UINib(nibName: "TransportTransitTrain", bundle: .main)
      .instantiate(withOwner: nil).first as? TransportTransitTrain)
    let views = descendants(of: cell.contentView)
    let label = try XCTUnwrap(views.compactMap { $0 as? UILabel }.first)

    for test in transportCases {
      cell.config(step: makeStep(type: test.type, number: ""))
      XCTAssertTrue(label.isHidden)
      cell.config(step: makeStep(type: test.type, number: "F1"))
      XCTAssertFalse(label.isHidden)
      XCTAssertEqual(label.text, "F1")
    }
  }

  func testWalkingWaypointsAndRulerKeepTheirCells() throws {
    let types: [MWMRouterTransitType] = [.pedestrian, .intermediatePoint, .ruler]
    let collection = TransportTransitStepsCollectionView(layout: TransportTransitFlowLayout())
    collection.frame = CGRect(x: 0, y: 0, width: 300, height: 100)
    collection.steps = types.map { makeStep(type: $0, number: "") }
    collection.layoutIfNeeded()
    let walking = try XCTUnwrap(collection.cellForItem(at: IndexPath(item: 0, section: 0)))
    let waypoint = try XCTUnwrap(collection.cellForItem(at: IndexPath(item: 1, section: 0)))
    let ruler = try XCTUnwrap(collection.cellForItem(at: IndexPath(item: 2, section: 0)))
    XCTAssertTrue(walking is TransportTransitPedestrian)
    XCTAssertTrue(waypoint is TransportTransitIntermediatePoint)
    XCTAssertTrue(ruler is TransportRuler)
  }

  func testContinuationsPreserveNumbersAndRegroupWhenTheWidthChanges() throws {
    let collection = TransportTransitStepsCollectionView(layout: TransportTransitFlowLayout())
    let numbers = (101 ... 160).map(String.init).joined(separator: ", ")
    let original = makeStep(type: .bus, number: numbers)
    collection.steps = [original, makeStep(type: .ferry, number: "F1")]
    var itemCounts = [Int]()
    for width: CGFloat in [200, 430, 200] {
      collection.frame = CGRect(x: 0, y: 0, width: width, height: 10000)
      collection.layoutIfNeeded()
      let count = collection.numberOfItems(inSection: 0)
      itemCounts.append(count)
      var texts = [String]()
      for item in 0 ..< count {
        let cell = try XCTUnwrap(collection.cellForItem(at: IndexPath(item: item, section: 0)))
        let views = descendants(of: cell.contentView)
        let label = try XCTUnwrap(views.compactMap { $0 as? UILabel }.first)
        let icon = try XCTUnwrap(views.compactMap { $0 as? UIImageView }.first)
        XCTAssertEqual(icon.isHidden, item > 0 && item < count - 1)
        XCTAssertFalse(label.text?.contains("…") ?? true)
        try texts.append(XCTUnwrap(label.text))
        let frame = try XCTUnwrap(collection.collectionViewLayout.layoutAttributesForItem(at: IndexPath(item: item, section: 0))).frame
        XCTAssertGreaterThanOrEqual(frame.minX, 0)
        XCTAssertLessThanOrEqual(frame.maxX, width)
      }
      XCTAssertEqual(texts.dropLast().joined(separator: " "), numbers)
      XCTAssertEqual(texts.last, "F1")
      let attributes = try XCTUnwrap(collection.collectionViewLayout.layoutAttributesForElements(in: collection.bounds))
      XCTAssertEqual(attributes.filter { $0.representedElementCategory == .decorationView }.count, 1)
      XCTAssertEqual(original.number, numbers)
    }
    XCTAssertGreaterThan(itemCounts[0], itemCounts[1])
    XCTAssertEqual(itemCounts[0], itemCounts[2])
  }

  func testContinuationCellRestoresItsIconAndOriginalNumberWhenReused() throws {
    let cell = try XCTUnwrap(UINib(nibName: "TransportTransitTrain", bundle: .main)
      .instantiate(withOwner: nil).first as? TransportTransitTrain)
    let views = descendants(of: cell.contentView)
    let label = try XCTUnwrap(views.compactMap { $0 as? UILabel }.first)
    let icon = try XCTUnwrap(views.compactMap { $0 as? UIImageView }.first)
    let step = makeStep(type: .bus, number: "101, 102, 103")
    cell.frame.size = TransportTransitTrain.estimatedCellSize(number: "103", isContinuation: true)
    cell.config(step: step, number: "103", isContinuation: true)
    cell.layoutIfNeeded()
    XCTAssertTrue(icon.isHidden)
    XCTAssertEqual(icon.bounds.width, 0)
    XCTAssertEqual(label.text, "103")
    XCTAssertEqual(step.number, "101, 102, 103")

    cell.frame.size = TransportTransitTrain.estimatedCellSize(step: step)
    cell.config(step: step)
    cell.layoutIfNeeded()
    XCTAssertFalse(icon.isHidden)
    XCTAssertGreaterThan(icon.bounds.width, 0)
    XCTAssertEqual(label.text, step.number)
  }

  func testScrollingDoesNotInvalidateLayoutButResizingDoes() {
    let layout = CountingLayout()
    let collection = TransportTransitStepsCollectionView(layout: layout)
    collection.frame = CGRect(x: 0, y: 0, width: 200, height: 100)
    collection.contentSize = CGSize(width: 200, height: 1000)
    collection.layoutIfNeeded()
    layout.invalidations = 0
    collection.contentOffset.y = 30
    collection.frame.origin.y = 10
    XCTAssertEqual(layout.invalidations, 0)
    collection.bounds.size.width = 300
    XCTAssertGreaterThan(layout.invalidations, 0)
  }

  func testDashboardKeepsTransitHeightWhenRebuiltInCompactOrReopened() throws {
    let map = try XCTUnwrap(MapViewController.shared())
    map.loadViewIfNeeded()
    let controller = NavigationDashboardViewController()
    controller.loadViewIfNeeded()
    controller.view.frame = CGRect(x: 0, y: 0, width: 390, height: 844)
    let stepsView = try XCTUnwrap(descendants(of: controller.view).compactMap { $0 as? TransportTransitStepsView }.first)
    var model = NavigationDashboard.ViewModel.initial
    model.dashboardState = .prepare
    model.routerType = .publicTransport
    model.routingOptions = RoutingOptions(routerType: .publicTransport)
    model.presentationStep = .compact
    let entity = TransitEntity()
    entity.steps = [makeStep(type: .bus, number: "12")]
    model.entity = entity
    controller.render(model)
    settle(controller)
    let shortHeight = stepsView.bounds.height
    XCTAssertGreaterThan(shortHeight, 0)

    entity.steps = [makeStep(type: .bus, number: (101 ... 200).map(String.init).joined(separator: ", "))]
    controller.render(model)
    settle(controller)
    let longHeight = stepsView.bounds.height
    XCTAssertGreaterThan(longHeight, shortHeight)
    XCTAssertLessThanOrEqual(longHeight, controller.view.safeAreaLayoutGuide.layoutFrame.height / 2)
    for step in [NavigationDashboardModalPresentationStep.hidden, .compact, .regular, .compact] {
      model.presentationStep = step
      controller.render(model)
      settle(controller)
      XCTAssertEqual(stepsView.bounds.height, longHeight, accuracy: 0.5, "\(step)")
    }

    let scroll = DraggingScrollView()
    scroll.contentSize = CGSize(width: 100, height: 1000)
    scroll.frame.size = CGSize(width: 100, height: 100)
    scroll.contentOffset.y = 80
    controller.scrollViewWillBeginDragging(scroll)
    scroll.contentOffset.y = 60
    controller.scrollViewDidScroll(scroll)
    XCTAssertEqual(scroll.contentOffset.y, 60)
    let panelPan = UIPanGestureRecognizer()
    XCTAssertFalse(controller.gestureRecognizer(panelPan, shouldRecognizeSimultaneouslyWith: scroll.panGestureRecognizer))
    controller.scrollViewDidEndDragging(scroll, willDecelerate: false)
    scroll.contentOffset.y = 0
    controller.scrollViewWillBeginDragging(scroll)
    scroll.contentOffset.y = 10
    controller.scrollViewDidScroll(scroll)
    XCTAssertEqual(scroll.contentOffset.y, 0)
    XCTAssertTrue(controller.gestureRecognizer(panelPan, shouldRecognizeSimultaneouslyWith: scroll.panGestureRecognizer))
    controller.scrollViewDidEndDragging(scroll, willDecelerate: false)
  }

  func testReplacingStepsClearsOldFragmentsAndTransfers() throws {
    let collection = TransportTransitStepsCollectionView(layout: TransportTransitFlowLayout())
    collection.frame = CGRect(x: 0, y: 0, width: 200, height: 10000)
    collection.steps = [makeStep(type: .bus, number: (101 ... 160).map(String.init).joined(separator: ", ")),
                        makeStep(type: .ferry, number: "F1")]
    collection.layoutIfNeeded()
    XCTAssertGreaterThan(collection.numberOfItems(inSection: 0), 2)

    collection.steps = [makeStep(type: .bus, number: "7")]
    collection.layoutIfNeeded()
    XCTAssertEqual(collection.numberOfItems(inSection: 0), 1)
    let attributes = try XCTUnwrap(collection.collectionViewLayout.layoutAttributesForElements(in: collection.bounds))
    XCTAssertEqual(attributes.filter { $0.representedElementCategory == .cell }.count, 1)
    XCTAssertFalse(attributes.contains { $0.representedElementCategory == .decorationView })
    let cell = try XCTUnwrap(collection.cellForItem(at: IndexPath(item: 0, section: 0)))
    let label = try XCTUnwrap(descendants(of: cell.contentView).compactMap { $0 as? UILabel }.first)
    XCTAssertEqual(label.text, "7")
  }

  private func settle(_ controller: UIViewController) {
    RunLoop.main.run(until: Date().addingTimeInterval(0.35))
    controller.view.layoutIfNeeded()
  }

  private final class TransitEntity: MWMNavigationDashboardEntity {
    var steps = [MWMRouterTransitStepInfo]()
    override var transitSteps: [MWMRouterTransitStepInfo]! { steps }
  }

  private final class DraggingScrollView: UIScrollView {
    override var isDragging: Bool { true }
  }

  private final class CountingLayout: UICollectionViewLayout {
    var invalidations = 0
    override func invalidateLayout() {
      invalidations += 1
      super.invalidateLayout()
    }
  }

  private func makeStep(type: MWMRouterTransitType, number: String) -> MWMRouterTransitStepInfo {
    let step = MWMRouterTransitStepInfo()
    step.type = type
    step.number = number
    step.distance = "42"
    step.distanceUnits = "m"
    step.color = UIColor(red: 47 / 255, green: 100 / 255, blue: 153 / 255, alpha: 1)
    step.intermediateIndex = 2
    return step
  }

  private func descendants(of view: UIView) -> [UIView] {
    view.subviews.flatMap { [$0] + descendants(of: $0) }
  }
}
