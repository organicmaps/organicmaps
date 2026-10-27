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
