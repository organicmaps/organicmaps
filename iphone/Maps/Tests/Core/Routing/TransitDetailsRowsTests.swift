@testable import Organic_Maps__Debug_
import XCTest

/// Verifies how the transit details timeline maps route steps onto rows: added stops get their own row, the
/// destination closes the timeline, and only rides sharing a named stop are joined as a transfer.
final class TransitDetailsRowsTests: XCTestCase {
  private let red = UIColor.red
  private let blue = UIColor.blue

  func testWaypointIsKeptAsItsOwnRowBetweenWalks() {
    let rows = TransitDetailsRows.make(steps: [walk(), waypoint(0), walk(), ride("A", "B")],
                                       destination: nil, waypointNames: [0: "Museum"])

    XCTAssertEqual(rows.count, 4)
    guard case .walk = rows[0], case .waypoint(let index, let name) = rows[1], case .walk = rows[2],
          case .ride = rows[3] else { return XCTFail("\(rows)") }
    XCTAssertEqual(index, 0)
    XCTAssertEqual(name, "Museum")
  }

  func testUnnamedWaypointHasNoName() {
    let rows = TransitDetailsRows.make(steps: [waypoint(1)], destination: nil, waypointNames: [:])

    guard case .waypoint(let index, let name) = rows[0] else { return XCTFail("\(rows)") }
    XCTAssertEqual(index, 1)
    XCTAssertNil(name)
  }

  func testDestinationIsAppendedAfterTheLegs() {
    let rows = TransitDetailsRows.make(steps: [walk(), ride("A", "B")], destination: "Hauptbahnhof",
                                       waypointNames: [:])

    XCTAssertEqual(rows.count, 3)
    guard case .destination(let name) = rows[2] else { return XCTFail("\(rows)") }
    XCTAssertEqual(name, "Hauptbahnhof")
  }

  func testEmptyDestinationAddsNoRow() {
    XCTAssertEqual(TransitDetailsRows.make(steps: [walk()], destination: "", waypointNames: [:]).count, 1)
  }

  func testSameStopTransferIsDrawnOnceAsTheNextRideHeading() throws {
    let rows = TransitDetailsRows.make(steps: [ride("A", "B", color: red), ride("B", "C", color: blue)],
                                       destination: nil, waypointNames: [:])

    let first = try rideRow(rows[0])
    let second = try rideRow(rows[1])
    XCTAssertEqual(first.boardStop, "A")
    XCTAssertNil(first.exitStop)
    XCTAssertTrue(first.continuesIntoTransfer)
    XCTAssertFalse(second.continuesIntoTransfer)
    XCTAssertTrue(second.isTransfer)
    XCTAssertEqual(second.transferFromColor, red)
    XCTAssertNil(second.boardStop)
    XCTAssertEqual(second.exitStop, "C")
  }

  func testTransferBetweenDifferentlyNamedStopsKeepsBothNames() throws {
    let rows = TransitDetailsRows.make(steps: [ride("A", "Incoming"), ride("Connecting", "C")],
                                       destination: nil, waypointNames: [:])

    XCTAssertEqual(try rideRow(rows[0]).exitStop, "Incoming")
    XCTAssertFalse(try rideRow(rows[1]).isTransfer)
    XCTAssertEqual(try rideRow(rows[1]).boardStop, "Connecting")
  }

  func testUnnamedStopsAreNotJoinedAndShowNoHeadings() throws {
    let rows = TransitDetailsRows.make(steps: [ride("A", ""), ride("", "C")], destination: nil, waypointNames: [:])

    XCTAssertNil(try rideRow(rows[0]).exitStop)
    XCTAssertFalse(try rideRow(rows[1]).isTransfer)
    XCTAssertNil(try rideRow(rows[1]).boardStop)
    XCTAssertEqual(try rideRow(rows[1]).exitStop, "C")
  }

  func testWalkBetweenRidesIsNotATransfer() throws {
    let rows = TransitDetailsRows.make(steps: [ride("A", "B"), walk(), ride("B", "C")],
                                       destination: nil, waypointNames: [:])

    XCTAssertEqual(try rideRow(rows[0]).exitStop, "B")
    XCTAssertFalse(try rideRow(rows[2]).isTransfer)
    XCTAssertEqual(try rideRow(rows[2]).boardStop, "B")
  }

  func testRideRowShowsSelectedLineAndExpandsToItsStops() throws {
    let step = ride("A", "D")
    step.number = "1, 2"
    step.lineNumber = "1"
    step.intermediateStopNames = ["B", "C"]
    step.stopCount = 3
    let rows = TransitDetailsRows.make(steps: [step], destination: nil, waypointNames: [:])
    let view = try TransitDetailsRideView(ride: rideRow(rows[0]))

    let badge = try XCTUnwrap(descendants(of: view).compactMap { $0 as? TransitLineBadgeView }.first)
    XCTAssertTrue(descendants(of: badge).compactMap { ($0 as? UILabel)?.text }.contains("1"))
    let stops = try XCTUnwrap(descendants(of: view).compactMap { $0 as? UILabel }.first { $0.text == "B\nC" })
    XCTAssertTrue(stops.isHidden)
    view.toggleIntermediates()
    XCTAssertTrue(view.isExpanded)
    view.toggleIntermediates()
    view.toggleIntermediates()
    XCTAssertTrue(view.isExpanded)
  }

  func testTransferDrawsTheIncomingLineAboveABreakBesideTheSharedStop() throws {
    let rows = TransitDetailsRows.make(steps: [ride("A", "B", color: red), ride("B", "C", color: blue)],
                                       destination: nil, waypointNames: [:])
    let view = try TransitDetailsRideView(ride: rideRow(rows[1]))
    layOut(view)

    let incoming = try XCTUnwrap(bars(in: view).first { $0.backgroundColor == red })
    let outgoing = try XCTUnwrap(bars(in: view).first { $0.backgroundColor == blue })
    let heading = try XCTUnwrap(descendants(of: view).compactMap { $0 as? UILabel }.first { $0.text == "B" })
    let headingCenter = heading.convert(CGPoint(x: 0, y: heading.bounds.midY), to: view).y
    let incomingFrame = incoming.convert(incoming.bounds, to: view)
    let outgoingFrame = outgoing.convert(outgoing.bounds, to: view)

    XCTAssertEqual(incomingFrame.minY, 0, accuracy: 0.5)
    XCTAssertLessThan(incomingFrame.maxY, headingCenter)
    XCTAssertGreaterThan(outgoingFrame.minY, headingCenter)
    XCTAssertEqual(outgoingFrame.maxY, view.bounds.height, accuracy: 0.5)
  }

  func testLineRunningIntoATransferEndsSquareAtTheRowBottom() throws {
    let rows = TransitDetailsRows.make(steps: [ride("A", "B", color: red), ride("B", "C", color: blue)],
                                       destination: nil, waypointNames: [:])
    let view = try TransitDetailsRideView(ride: rideRow(rows[0]))
    layOut(view)

    let bar = try XCTUnwrap(bars(in: view).first { $0.backgroundColor == red })
    XCTAssertEqual(bar.convert(bar.bounds, to: view).maxY, view.bounds.height, accuracy: 0.5)
    XCTAssertEqual(bar.layer.maskedCorners, [.layerMinXMinYCorner, .layerMaxXMinYCorner])
  }

  func testUnnamedRoutePointIsShownByItsNumber() {
    let unnamed = MWMRoutePoint(cgPoint: .zero, title: nil, subtitle: nil, type: .intermediate, intermediateIndex: 0)
    let named = MWMRoutePoint(cgPoint: .zero, title: "Museum", subtitle: nil, type: .intermediate,
                              intermediateIndex: 1)

    // Without a name the title holds the coordinates, which would hide the "Stop N" fallback.
    XCTAssertEqual(unnamed.title, unnamed.latLonString)
    XCTAssertFalse(TransitDetailsViewController.hasName(unnamed))
    XCTAssertTrue(TransitDetailsViewController.hasName(named))
  }

  func testStripOpensDetailsOnlyForAReadyPublicTransportRoute() throws {
    let strip = TransportTransitStepsView()
    strip.frame = CGRect(x: 0, y: 0, width: 320, height: 100)

    strip.setSteps([walk(), ride("A", "B")], isRouteReady: false)
    XCTAssertFalse(strip.isUserInteractionEnabled, "The steps belong to the previous route while building")

    strip.setSteps([walk(), ride("A", "B")], isRouteReady: true)
    XCTAssertTrue(strip.isUserInteractionEnabled)
    XCTAssertEqual(strip.accessibilityLabel, L("transit_details_title"))

    let ruler = makeStep(type: .ruler)
    strip.setSteps([ruler, waypoint(0), ruler], isRouteReady: true)
    XCTAssertFalse(strip.isUserInteractionEnabled)
    XCTAssertNil(strip.accessibilityLabel)
    // Without the chevron the strip gets its full width back.
    strip.layoutIfNeeded()
    let collection = try XCTUnwrap(strip.subviews.first { $0 is TransportTransitStepsCollectionView })
    XCTAssertEqual(collection.frame.maxX, strip.bounds.width, accuracy: 0.5)
  }

  // MARK: - Helpers

  private func layOut(_ view: UIView) {
    let size = view.systemLayoutSizeFitting(CGSize(width: 375, height: UIView.layoutFittingCompressedSize.height),
                                            withHorizontalFittingPriority: .required,
                                            verticalFittingPriority: .fittingSizeLevel)
    view.frame = CGRect(origin: .zero, size: size)
    view.layoutIfNeeded()
  }

  /// The line bars: plain views drawn in a line color, as opposed to labels, badges and icons.
  private func bars(in view: UIView) -> [UIView] {
    descendants(of: view).filter { type(of: $0) == UIView.self && ($0.backgroundColor == red || $0.backgroundColor == blue) }
  }

  private struct NotARide: Error {}

  private func rideRow(_ row: TransitDetailsRow) throws -> TransitDetailsRide {
    guard case .ride(let ride) = row else {
      XCTFail("Not a ride: \(row)")
      throw NotARide()
    }
    return ride
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
    step.number = ""
    step.distance = "240"
    step.distanceUnits = "m"
    step.color = .red
    step.timeInSec = 180
    step.lineNumber = ""
    step.startStopName = ""
    step.endStopName = ""
    step.intermediateStopNames = []
    return step
  }

  private func descendants(of view: UIView) -> [UIView] {
    view.subviews + view.subviews.flatMap { descendants(of: $0) }
  }
}
