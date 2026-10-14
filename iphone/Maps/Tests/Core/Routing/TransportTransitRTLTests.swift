@testable import Organic_Maps__Debug_
import XCTest

final class TransportTransitRTLTests: XCTestCase {
  func testRTLShowsTheFirstStepAtTheRightEdge() throws {
    try checkOrder(direction: .forceRightToLeft)
  }

  func testLTRShowsTheFirstStepAtTheLeftEdge() throws {
    try checkOrder(direction: .forceLeftToRight)
  }

  private func checkOrder(direction: UISemanticContentAttribute) throws {
    let collection = TransportTransitStepsCollectionView(layout: TransportTransitFlowLayout())
    collection.frame = CGRect(x: 0, y: 0, width: 300, height: 100)
    collection.semanticContentAttribute = direction
    collection.steps = ["12", "34"].map { number in
      let step = MWMRouterTransitStepInfo()
      step.type = .bus
      step.number = number
      step.distance = "42"
      step.distanceUnits = "m"
      step.color = .blue
      step.intermediateIndex = 0
      return step
    }
    let controller = UIViewController()
    controller.view.semanticContentAttribute = direction
    controller.view.addSubview(collection)
    let window = UIWindow(frame: collection.frame)
    window.rootViewController = controller
    window.isHidden = false
    defer { window.isHidden = true }
    collection.layoutIfNeeded()
    let first = try XCTUnwrap(collection.cellForItem(at: IndexPath(item: 0, section: 0)))
    let last = try XCTUnwrap(collection.cellForItem(at: IndexPath(item: 1, section: 0)))
    let firstFrame = first.convert(first.bounds, to: window)
    let lastFrame = last.convert(last.bounds, to: window)
    let separator = try XCTUnwrap(collection.collectionViewLayout.layoutAttributesForDecorationView(
      ofKind: TransportTransitFlowLayout.Config.separatorKind, at: IndexPath(item: 1, section: 0)
    ))
    let arrowStart = CGPoint.zero.applying(separator.transform)
    let arrowEnd = CGPoint(x: 16, y: 0).applying(separator.transform)
    if direction == .forceRightToLeft {
      XCTAssertGreaterThan(firstFrame.midX, lastFrame.midX)
      XCTAssertLessThan(arrowEnd.x, arrowStart.x)
    } else {
      XCTAssertLessThan(firstFrame.midX, lastFrame.midX)
      XCTAssertGreaterThan(arrowEnd.x, arrowStart.x)
    }
    for (index, cell) in [first, last].enumerated() {
      let label = try XCTUnwrap(descendants(of: cell).compactMap { $0 as? UILabel }.first)
      XCTAssertEqual(label.text, collection.steps[index].number)
      XCTAssertGreaterThanOrEqual(cell.frame.minX, 0)
      XCTAssertLessThanOrEqual(cell.frame.maxX, collection.bounds.width)
    }
  }

  private func descendants(of view: UIView) -> [UIView] {
    view.subviews.flatMap { [$0] + descendants(of: $0) }
  }
}
