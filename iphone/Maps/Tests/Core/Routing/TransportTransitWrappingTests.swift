@testable import Organic_Maps__Debug_
import XCTest

final class TransportTransitWrappingTests: XCTestCase {
  func testWrappedLineListsAndSeparatorsStayWithinThePanel() throws {
    for width: CGFloat in [200, 300, 430] {
      let frames = TransportTransitFlowLayout.frames(cellSizes: sizes([40, 1000, 1000]), width: width)
      XCTAssertTrue((frames.cells + frames.separators).allSatisfy { $0.minX >= 0 && $0.maxX <= width })
      XCTAssertGreaterThan(try XCTUnwrap(frames.cells.last).minY, 0)
    }
  }

  func testNarrowPanelsKeepNonnegativeCellWidths() {
    for width: CGFloat in [0, 8, 20, 55] {
      let frames = TransportTransitFlowLayout.frames(cellSizes: sizes([40, 1000, 1000]), width: width)
      for frame in frames.cells {
        XCTAssertGreaterThanOrEqual(frame.size.width, 0)
        XCTAssertGreaterThanOrEqual(frame.minX, 0)
        XCTAssertLessThanOrEqual(frame.maxX, width)
      }
    }
  }

  func testFinalLongBadgeUsesTheFullRemainingWidth() {
    for width: CGFloat in [200, 300, 430] {
      for prefix: [CGFloat] in [[], [40], [40, 1000]] {
        let frames = TransportTransitFlowLayout.frames(cellSizes: sizes(prefix + [1000]), width: width)
        XCTAssertEqual(frames.cells.last?.maxX, width)
        if prefix.isEmpty {
          XCTAssertEqual(frames.cells.last?.minY, 0)
        }
      }
    }
  }

  func testFinalBadgeStaysOnTheRowWhenItFitsWithoutAFollowingSeparator() {
    let frames = TransportTransitFlowLayout.frames(cellSizes: sizes([40, 40]), width: 100)
    XCTAssertEqual(frames.cells.last?.minY, 0)
  }

  private func sizes(_ widths: [CGFloat]) -> [CGSize] {
    widths.map { CGSize(width: $0, height: 20) }
  }
}
