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

  func testSameStopTransferIsNamedOnceBetweenTheRides() throws {
    let rows = TransitDetailsRows.make(steps: [ride("A", "B", color: red), ride("B", "C", color: blue)],
                                       destination: nil, waypointNames: [:])

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
    let rows = TransitDetailsRows.make(steps: [ride("A", "B"), ride("B", "C"), ride("C", "D")],
                                       destination: nil, waypointNames: [:])

    XCTAssertEqual(rows.count, 5)
    guard case .transfer(let first, _, _) = rows[1], case .transfer(let second, _, _) = rows[3] else {
      return XCTFail("\(rows)")
    }
    XCTAssertEqual(first, "B")
    XCTAssertEqual(second, "C")
    let middle = try rideRow(rows[2])
    XCTAssertTrue(middle.startsAtTransfer)
    XCTAssertTrue(middle.endsAtTransfer)
    XCTAssertNil(middle.boardStop)
    XCTAssertNil(middle.exitStop)
  }

  func testTransferBetweenDifferentlyNamedStopsKeepsBothNames() throws {
    let rows = TransitDetailsRows.make(steps: [ride("A", "Incoming"), ride("Connecting", "C")],
                                       destination: nil, waypointNames: [:])

    XCTAssertEqual(rows.count, 2)
    XCTAssertEqual(try rideRow(rows[0]).exitStop, "Incoming")
    XCTAssertFalse(try rideRow(rows[1]).startsAtTransfer)
    XCTAssertEqual(try rideRow(rows[1]).boardStop, "Connecting")
  }

  func testUnnamedStopsAreNotJoinedAndShowNoHeadings() throws {
    let rows = TransitDetailsRows.make(steps: [ride("A", ""), ride("", "C")], destination: nil, waypointNames: [:])

    XCTAssertEqual(rows.count, 2)
    XCTAssertNil(try rideRow(rows[0]).exitStop)
    XCTAssertFalse(try rideRow(rows[1]).startsAtTransfer)
    XCTAssertNil(try rideRow(rows[1]).boardStop)
    XCTAssertEqual(try rideRow(rows[1]).exitStop, "C")
  }

  func testWalkBetweenRidesIsNotATransfer() throws {
    let rows = TransitDetailsRows.make(steps: [ride("A", "B"), walk(), ride("B", "C")],
                                       destination: nil, waypointNames: [:])

    XCTAssertEqual(rows.count, 3)
    XCTAssertEqual(try rideRow(rows[0]).exitStop, "B")
    XCTAssertFalse(try rideRow(rows[2]).startsAtTransfer)
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
    let view = TransitDetailsTransferView(stop: "B", from: red, to: blue)
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

  func testLinesRunSquareIntoAndOutOfATransfer() throws {
    let rows = TransitDetailsRows.make(steps: [ride("A", "B", color: red), ride("B", "C", color: blue)],
                                       destination: nil, waypointNames: [:])
    let incoming = try TransitDetailsRideView(ride: rideRow(rows[0]))
    let outgoing = try TransitDetailsRideView(ride: rideRow(rows[2]))
    layOut(incoming)
    layOut(outgoing)

    let incomingBar = try XCTUnwrap(bars(in: incoming).first)
    XCTAssertEqual(incomingBar.convert(incomingBar.bounds, to: incoming).maxY, incoming.bounds.height, accuracy: 0.5)
    XCTAssertEqual(incomingBar.layer.maskedCorners, [.layerMinXMinYCorner, .layerMaxXMinYCorner])
    let outgoingBar = try XCTUnwrap(bars(in: outgoing).first)
    XCTAssertEqual(outgoingBar.convert(outgoingBar.bounds, to: outgoing).minY, 0, accuracy: 0.5)
    XCTAssertEqual(outgoingBar.layer.maskedCorners, [.layerMinXMaxYCorner, .layerMaxXMaxYCorner])
  }

  func testTimelineHasNoGapsAroundATransfer() throws {
    let controller = TransitDetailsViewController(steps: [walk(), ride("A", "B"), ride("B", "C"), walk()],
                                                  routePoints: [])
    controller.loadViewIfNeeded()
    let timeline = try XCTUnwrap(descendants(of: controller.view).compactMap { $0 as? UIStackView }
      .first { $0.axis == .vertical && $0.arrangedSubviews.count == 5 })
    let views = timeline.arrangedSubviews

    XCTAssertTrue(views[2] is TransitDetailsTransferView)
    XCTAssertEqual(timeline.customSpacing(after: views[0]), UIStackView.spacingUseDefault)
    XCTAssertEqual(timeline.customSpacing(after: views[1]), 0)
    XCTAssertEqual(timeline.customSpacing(after: views[2]), 0)
    XCTAssertEqual(timeline.customSpacing(after: views[3]), UIStackView.spacingUseDefault)
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
    let detailsTap = try XCTUnwrap(strip.gestureRecognizers?.first { $0 is UITapGestureRecognizer })
    let collection = try XCTUnwrap(strip.subviews.first { $0 is TransportTransitStepsCollectionView })

    strip.setSteps([walk(), ride("A", "B")], isRouteReady: false)
    XCTAssertFalse(detailsTap.isEnabled, "The steps belong to the previous route while building")

    strip.setSteps([walk(), ride("A", "B")], isRouteReady: true)
    XCTAssertTrue(detailsTap.isEnabled)
    XCTAssertEqual(strip.accessibilityLabel, L("transit_details_title"))

    let ruler = makeStep(type: .ruler)
    strip.setSteps([ruler, waypoint(0), ruler], isRouteReady: true)
    XCTAssertFalse(detailsTap.isEnabled)
    XCTAssertNil(strip.accessibilityLabel)
    // Without the chevron the strip gets its full width back.
    strip.layoutIfNeeded()
    XCTAssertEqual(collection.frame.maxX, strip.bounds.width, accuracy: 0.5)
  }

  func testStripStaysInteractiveWithAndWithoutDetails() throws {
    let strip = TransportTransitStepsView()
    let collection = try XCTUnwrap(strip.subviews.first { $0 is TransportTransitStepsCollectionView })
    let ruler = makeStep(type: .ruler)

    for (steps, isReady) in [([walk(), ride("A", "B")], true), ([walk(), ride("A", "B")], false), ([ruler], true)] {
      strip.setSteps(steps, isRouteReady: isReady)
      XCTAssertTrue(strip.isUserInteractionEnabled)
      XCTAssertTrue(collection.isUserInteractionEnabled)
    }
  }

  func testLongListsKeepTheirFullHeightWithAndWithoutTheChevron() throws {
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
    let collection = try XCTUnwrap(strip.subviews.first { $0 is TransportTransitStepsCollectionView } as? TransportTransitStepsCollectionView)
    let ruler = makeStep(type: .ruler)
    let longTransit = (0 ..< 40).flatMap { [ride("A\($0)", "B\($0)"), walk()] }
    let longRuler = (0 ..< 40).flatMap { [ruler, waypoint($0)] }

    for (steps, hasDetails, width) in [(longTransit, true, CGFloat(300)), (longRuler, false, CGFloat(320))] {
      strip.setSteps(steps, isRouteReady: true)
      // The width settles first and changes the height, so let the window run its layout as in the app.
      RunLoop.main.run(until: Date().addingTimeInterval(0.3))

      XCTAssertEqual(collection.bounds.width, width, accuracy: 0.5)
      XCTAssertGreaterThan(collection.contentHeight, 60)
      XCTAssertEqual(collection.bounds.height, collection.contentHeight, accuracy: 0.5)
      XCTAssertEqual(strip.gestureRecognizers?.first { $0 is UITapGestureRecognizer }?.isEnabled, hasDetails)
    }
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
