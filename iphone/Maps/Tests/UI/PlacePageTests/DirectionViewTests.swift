@testable import Organic_Maps__Debug_
import XCTest

final class DirectionViewTests: XCTestCase {
  func testMissingHeadingHidesArrowWithoutHidingMeasurements() throws {
    let direction: DirectionView = try XCTUnwrap(Bundle.main.load(viewClass: DirectionView.self))
    direction.updateDistance("12.3 km")
    direction.updateBearing(123)
    direction.updateHeading(nil)
    let arrow = try XCTUnwrap(direction.directionArrow)
    let distance = try XCTUnwrap(direction.distanceLabel)
    let bearing = try XCTUnwrap(direction.bearingLabel)
    XCTAssertTrue(arrow.isHidden)
    XCTAssertEqual(distance.text, "12.3 km")
    XCTAssertEqual(bearing.text, "123°")
    XCTAssertFalse(distance.isHidden)
    XCTAssertFalse(bearing.isHidden)
    direction.updateHeading(0)
    XCTAssertFalse(arrow.isHidden)
  }

  func testMeasurementsFitPhoneAndTabletLayouts() throws {
    let scene = try XCTUnwrap(UIApplication.shared.connectedScenes.compactMap { $0 as? UIWindowScene }.first)
    let window = UIWindow(windowScene: scene)
    let parent = UIViewController()
    window.rootViewController = parent
    window.isHidden = false
    defer { window.isHidden = true }
    window.layoutIfNeeded()
    let child = UIViewController()
    parent.addChild(child)
    parent.view.addSubview(child.view)
    child.didMove(toParent: parent)
    let direction: DirectionView = try XCTUnwrap(Bundle.main.load(viewClass: DirectionView.self))
    child.view.addSubview(direction)
    defer { direction.removeFromSuperview() }
    direction.alignToSuperview()
    direction.updateTitle("A long destination name across several lines", subtitle: "A long place type")
    direction.updateDistance("20 000 km")
    direction.updateBearing(359)
    direction.updateHeading(.pi / 2 - 359 * .pi / 180)

    let cases: [(CGSize, UIUserInterfaceSizeClass, UIUserInterfaceSizeClass, Bool)] = [
      (CGSize(width: 1024, height: 768), .regular, .regular, false),
      (CGSize(width: 320, height: 568), .compact, .regular, false),
      (CGSize(width: 568, height: 320), .compact, .compact, false),
      (CGSize(width: 874, height: 402), .regular, .compact, false),
      (CGSize(width: 568, height: 320), .compact, .compact, true),
    ]
    for (size, horizontal, vertical, rtl) in cases {
      parent.setOverrideTraitCollection(UITraitCollection(traitsFrom: [
        UITraitCollection(horizontalSizeClass: horizontal),
        UITraitCollection(verticalSizeClass: vertical),
      ]), forChild: child)
      child.view.frame = CGRect(origin: .zero, size: size)
      direction.semanticContentAttribute = rtl ? .forceRightToLeft : .forceLeftToRight
      direction.setNeedsLayout()
      child.view.layoutIfNeeded()
      direction.layoutIfNeeded()
      try assertLayout(direction, size: size, horizontal: horizontal, vertical: vertical)
    }
  }

  private func assertLayout(_ direction: DirectionView, size: CGSize,
                            horizontal: UIUserInterfaceSizeClass, vertical: UIUserInterfaceSizeClass) throws {
    XCTAssertEqual(direction.traitCollection.horizontalSizeClass, horizontal)
    XCTAssertEqual(direction.traitCollection.verticalSizeClass, vertical)

    let stack = try XCTUnwrap(direction.measurementsStack)
    let distance = try XCTUnwrap(direction.distanceLabel)
    let bearing = try XCTUnwrap(direction.bearingLabel)
    let horizontalMeasurements = vertical == .compact || horizontal == .regular
    XCTAssertEqual(stack.axis, horizontalMeasurements ? .horizontal : .vertical)
    XCTAssertFalse(direction.hasAmbiguousLayout)
    for label in [distance, bearing] {
      XCTAssertFalse(label.hasAmbiguousLayout, "\(size) \(label.text ?? "")")
      let frame = label.convert(label.bounds, to: direction)
      XCTAssertTrue(direction.bounds.insetBy(dx: -0.5, dy: -0.5).contains(frame), "\(size) \(frame)")
      XCTAssertGreaterThanOrEqual(frame.height, label.font.lineHeight - 1)
      XCTAssertGreaterThan(frame.width, 0)
    }
    let distanceFrame = distance.convert(distance.bounds, to: direction)
    let bearingFrame = bearing.convert(bearing.bounds, to: direction)
    XCTAssertFalse(distanceFrame.intersects(bearingFrame))
    if !horizontalMeasurements {
      XCTAssertGreaterThan(bearingFrame.minY, distanceFrame.maxY)
    }
  }
}
