@testable import Organic_Maps__Debug_
import XCTest

final class TransportTransitWrappingTests: XCTestCase {
  func testWrappedLineListsStayWithinThePanel() throws {
    for width: CGFloat in [200, 300, 430] {
      let collection = makeCollection(width: width)
      collection.layoutIfNeeded()
      for item in collection.steps.indices {
        let attributes = try XCTUnwrap(collection.collectionViewLayout.layoutAttributesForItem(at: IndexPath(item: item, section: 0)))
        XCTAssertGreaterThanOrEqual(attributes.frame.minX, 0)
        XCTAssertLessThanOrEqual(attributes.frame.maxX, width)
      }
      let last = try XCTUnwrap(collection.collectionViewLayout.layoutAttributesForItem(at: IndexPath(item: 2, section: 0)))
      XCTAssertGreaterThan(last.frame.minY, 0)
    }
  }

  func testNarrowPanelsKeepNonnegativeCellWidths() throws {
    for width: CGFloat in [0, 8, 20, 55] {
      let collection = makeCollection(width: width)
      collection.collectionViewLayout.prepare()
      for item in collection.steps.indices {
        let attributes = try XCTUnwrap(collection.collectionViewLayout.layoutAttributesForItem(at: IndexPath(item: item, section: 0)))
        XCTAssertGreaterThanOrEqual(attributes.frame.width, 0)
        XCTAssertLessThanOrEqual(attributes.frame.maxX, width)
      }
    }
  }

  private func makeCollection(width: CGFloat) -> TransportTransitStepsCollectionView {
    let collection = TransportTransitStepsCollectionView(layout: TransportTransitFlowLayout())
    collection.frame = CGRect(x: 0, y: 0, width: width, height: 200)
    let longNumber = (1 ... 30).map(String.init).joined(separator: ", ")
    collection.steps = ["12", longNumber, longNumber].map { number in
      let step = MWMRouterTransitStepInfo()
      step.type = .bus
      step.number = number
      step.distance = "42"
      step.distanceUnits = "m"
      step.color = .blue
      step.intermediateIndex = 0
      return step
    }
    return collection
  }
}
