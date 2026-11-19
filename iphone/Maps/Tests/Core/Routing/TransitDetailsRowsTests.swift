@testable import Organic_Maps__Debug_
import XCTest

/// Verifies how the transit details timeline maps route steps onto rows: added stops get their own row, the
/// destination closes the timeline, and only rides sharing a named stop are joined as a transfer.
final class TransitDetailsRowsTests: XCTestCase {
  private let red = UIColor.red
  private let blue = UIColor.blue

  func testWaypointIsKeptAsItsOwnRowBetweenWalks() {
    let rows = make([walk(), waypoint(0), walk(), ride("A", "B")], points: [point(.intermediate, "Museum", index: 0)])

    XCTAssertEqual(rows.count, 4)
    guard case .walk = rows[0], case .waypoint(let number, let title) = rows[1], case .walk = rows[2],
          case .ride = rows[3] else { return XCTFail("\(rows)") }
    XCTAssertEqual(number, 1)
    XCTAssertEqual(title, "Museum")
  }

  func testUnnamedWaypointIsShownByItsNumber() {
    // Without a name the title holds the coordinates, which would hide the "Stop N" fallback.
    let unnamed = point(.intermediate, nil, index: 1)
    XCTAssertEqual(unnamed.title, unnamed.latLonString)

    let rows = make([waypoint(1)], points: [unnamed])

    guard case .waypoint(let number, let title) = rows[0] else { return XCTFail("\(rows)") }
    XCTAssertEqual(number, 2)
    XCTAssertEqual(title, String(format: L("transit_waypoint_label"), 2))
  }

  func testDestinationIsAppendedAfterTheLegs() {
    let rows = make([walk(), ride("A", "B")], points: [point(.start, "Home"), point(.finish, "Hauptbahnhof")])

    XCTAssertEqual(rows.count, 3)
    guard case .destination(let name) = rows[2] else { return XCTFail("\(rows)") }
    XCTAssertEqual(name, "Hauptbahnhof")
  }

  func testWalkCarriesItsDistanceAndTime() {
    guard case .walk(let walk) = make([walk()])[0] else { return XCTFail() }
    XCTAssertEqual(walk.distance, "240 m")
    XCTAssertEqual(walk.timeInSec, 180)
  }

  func testSameStopTransferIsNamedOnceBetweenTheRides() throws {
    let rows = make([ride("A", "B", color: red), ride("B", "C", color: blue)])

    XCTAssertEqual(rows.count, 3)
    let first = try rideRow(rows[0])
    let second = try rideRow(rows[2])
    guard case .transfer(let stop, let from, let to) = rows[1] else { return XCTFail("\(rows)") }
    XCTAssertEqual(stop, "B")
    XCTAssertEqual(from, red)
    XCTAssertEqual(to, blue)
    XCTAssertEqual(first.boardStop, "A")
    XCTAssertNil(first.exitStop)
    XCTAssertFalse(first.startsAtTransfer)
    XCTAssertTrue(first.endsAtTransfer)
    XCTAssertTrue(second.startsAtTransfer)
    XCTAssertFalse(second.endsAtTransfer)
    XCTAssertNil(second.boardStop)
    XCTAssertEqual(second.exitStop, "C")
  }

  func testConsecutiveTransfersEachGetTheirRow() throws {
    let rows = make([ride("A", "B"), ride("B", "C"), ride("C", "D")])

    XCTAssertEqual(rows.count, 5)
    guard case .transfer(let first, _, _) = rows[1], case .transfer(let second, _, _) = rows[3] else {
      return XCTFail("\(rows)")
    }
    XCTAssertEqual(first, "B")
    XCTAssertEqual(second, "C")
    let middle = try rideRow(rows[2])
    XCTAssertNil(middle.boardStop)
    XCTAssertNil(middle.exitStop)
  }

  func testTransferBetweenDifferentlyNamedStopsKeepsBothNames() throws {
    let rows = make([ride("A", "Incoming"), ride("Connecting", "C")])

    XCTAssertEqual(rows.count, 2)
    XCTAssertEqual(try rideRow(rows[0]).exitStop, "Incoming")
    XCTAssertEqual(try rideRow(rows[1]).boardStop, "Connecting")
  }

  func testUnnamedStopsAreNotJoinedAndShowNoHeadings() throws {
    let rows = make([ride("A", ""), ride("", "C")])

    XCTAssertEqual(rows.count, 2)
    XCTAssertNil(try rideRow(rows[0]).exitStop)
    XCTAssertNil(try rideRow(rows[1]).boardStop)
    XCTAssertEqual(try rideRow(rows[1]).exitStop, "C")
  }

  func testWalkBetweenRidesIsNotATransfer() throws {
    let rows = make([ride("A", "B"), walk(), ride("B", "C")])

    XCTAssertEqual(rows.count, 3)
    XCTAssertEqual(try rideRow(rows[0]).exitStop, "B")
    XCTAssertEqual(try rideRow(rows[2]).boardStop, "B")
  }

  func testRideShowsTheSelectedLineAndItsStops() throws {
    let step = ride("A", "D", color: red)
    step.number = "1, 2"
    step.lineNumber = "1"
    step.intermediateStopNames = ["B", "C"]
    step.stopCount = 3

    let ride = try rideRow(make([step])[0])

    XCTAssertEqual(ride.line.number, "1")
    XCTAssertEqual(ride.line.type, .bus)
    XCTAssertEqual(ride.line.color, red)
    XCTAssertEqual(ride.intermediateStops, ["B", "C"])
    XCTAssertEqual(ride.stopCount, 3)
    XCTAssertTrue(ride.canExpand)
  }

  func testLineWithoutNumberDoesNotBorrowParallelLines() throws {
    let step = ride("A", "B")
    step.number = "7, 8"

    let ride = try rideRow(make([step])[0])

    XCTAssertEqual(ride.line.number, "")
    XCTAssertFalse(ride.canExpand)
    XCTAssertEqual(TransitLineBadgeView(line: ride.line).accessibilityLabel, L("transit_type_bus"))
  }

  func testTappingTheSummaryOpensAndClosesTheStopList() throws {
    let step = ride("A", "C")
    step.intermediateStopNames = ["B"]
    let view = try TransitDetailsRideView(ride: rideRow(make([step])[0]))
    let stops = try XCTUnwrap(descendants(of: view).compactMap { $0 as? UILabel }.first { $0.text == "B" })
    XCTAssertTrue(stops.isHidden)

    view.toggleIntermediates()
    XCTAssertTrue(view.isExpanded)
    XCTAssertFalse(stops.isHidden)

    view.toggleIntermediates()
    XCTAssertFalse(view.isExpanded)
    XCTAssertTrue(stops.isHidden)
  }

  // MARK: - Summary strip

  func testStripOffersDetailsOnlyWhenTheDashboardAllowsIt() throws {
    let strip = TransportTransitStepsView()
    var taps = 0
    strip.onTap = { taps += 1 }
    let detailsButton = try XCTUnwrap(descendants(of: strip).compactMap { $0 as? UIButton }
      .first { $0.accessibilityLabel == L("transit_details_title") })

    strip.setSteps([walk(), ride("A", "B")], canOpenDetails: false)
    XCTAssertTrue(detailsButton.isHidden)

    strip.setSteps([walk(), ride("A", "B")], canOpenDetails: true)
    XCTAssertFalse(detailsButton.isHidden)
    XCTAssertTrue(strip.isUserInteractionEnabled)
    detailsButton.sendActions(for: .touchUpInside)
    XCTAssertEqual(taps, 1)
  }

  func testLongListsKeepTheirFullHeightWithAndWithoutDetails() throws {
    // The dashboard's scroll view relies on the strip's own height to reach every line.
    let scene = try XCTUnwrap(UIApplication.shared.connectedScenes.compactMap { $0 as? UIWindowScene }.first)
    let container = UIWindow(windowScene: scene)
    container.frame = CGRect(x: 0, y: 0, width: 320, height: 400)
    container.isHidden = false
    let strip = TransportTransitStepsView()
    strip.translatesAutoresizingMaskIntoConstraints = false
    container.addSubview(strip)
    NSLayoutConstraint.activate([
      strip.topAnchor.constraint(equalTo: container.topAnchor),
      strip.leadingAnchor.constraint(equalTo: container.leadingAnchor),
      strip.trailingAnchor.constraint(equalTo: container.trailingAnchor),
    ])
    let ruler = makeStep(type: .ruler)

    func height(of steps: [MWMRouterTransitStepInfo], canOpenDetails: Bool) -> CGFloat {
      strip.setSteps(steps, canOpenDetails: canOpenDetails)
      // The width settles first and changes the content height, which the next pass applies.
      for _ in 0 ..< 3 {
        container.layoutIfNeeded()
      }
      return strip.frame.height
    }

    let oneRow = height(of: [ride("A", "B")], canOpenDetails: true)
    let longTransit = height(of: (0 ..< 40).flatMap { [ride("A\($0)", "B\($0)"), walk()] }, canOpenDetails: true)
    let longRuler = height(of: (0 ..< 40).flatMap { [ruler, waypoint($0)] }, canOpenDetails: false)

    XCTAssertGreaterThan(oneRow, 0)
    XCTAssertGreaterThan(longTransit, oneRow * 8)
    XCTAssertGreaterThan(longRuler, oneRow * 8)
  }

  // MARK: - Helpers

  private struct NotARide: Error {}

  private func make(_ steps: [MWMRouterTransitStepInfo], points: [MWMRoutePoint] = []) -> [TransitDetailsRow] {
    TransitDetailsRows.make(steps: steps, routePoints: points)
  }

  private func rideRow(_ row: TransitDetailsRow) throws -> TransitDetailsRide {
    guard case .ride(let ride) = row else {
      XCTFail("Not a ride: \(row)")
      throw NotARide()
    }
    return ride
  }

  private func point(_ type: MWMRoutePointType, _ title: String?, index: Int = 0) -> MWMRoutePoint {
    MWMRoutePoint(cgPoint: .zero, title: title, subtitle: nil, type: type, intermediateIndex: index)
  }

  private func walk() -> MWMRouterTransitStepInfo {
    makeStep(type: .pedestrian)
  }

  private func waypoint(_ index: Int) -> MWMRouterTransitStepInfo {
    let step = makeStep(type: .intermediatePoint)
    step.intermediateIndex = index
    return step
  }

  private func ride(_ start: String, _ end: String, color: UIColor = .red) -> MWMRouterTransitStepInfo {
    let step = makeStep(type: .bus)
    step.number = "5"
    step.color = color
    step.startStopName = start
    step.endStopName = end
    step.stopCount = 1
    return step
  }

  private func makeStep(type: MWMRouterTransitType) -> MWMRouterTransitStepInfo {
    let step = MWMRouterTransitStepInfo()
    step.type = type
    step.distance = "240"
    step.distanceUnits = "m"
    step.color = .red
    step.timeInSec = 180
    return step
  }

  private func descendants(of view: UIView) -> [UIView] {
    view.subviews + view.subviews.flatMap { descendants(of: $0) }
  }
}
