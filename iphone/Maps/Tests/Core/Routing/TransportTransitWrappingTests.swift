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

  func testContinuationsOnlySeparateDifferentLegs() throws {
    let frames = TransportTransitFlowLayout.frames(cellSizes: sizes([180, 180, 40]), width: 200,
                                                   continuationItems: [1])
    XCTAssertEqual(frames.separators.count, 1)
    XCTAssertEqual(frames.cells[1].minX, 0)
    XCTAssertGreaterThan(frames.cells[1].minY, frames.cells[0].minY)
    let separator = try XCTUnwrap(frames.separators.first)
    XCTAssertEqual(separator.minY, frames.cells[1].minY)
    XCTAssertGreaterThanOrEqual(separator.minX, frames.cells[1].maxX)
    XCTAssertTrue((frames.cells + frames.separators).allSatisfy { $0.minX >= 0 && $0.maxX <= 200 })
  }

  func testFirstIdentifierMovesToTheNextRowWhenItsCommaDoesNotFit() {
    let number = "101, 102"
    let firstRowWidth = TransportTransitTrain.estimatedCellSize(number: "101").width
    let groups = TransportTransitTrain.numberGroups(number, maximumWidth: 200,
                                                    firstRowWidth: firstRowWidth, hasFollowingStep: false)
    XCTAssertEqual(groups, [number])
  }

  func testLongListsKeepEveryIdentifierAndOnlySeparateTheNextLeg() {
    let number = (1 ... 1000).map(String.init).joined(separator: ", ")
    for width: CGFloat in [200, 430] {
      let groups = TransportTransitTrain.numberGroups(number, maximumWidth: width,
                                                      firstRowWidth: 20, hasFollowingStep: true)
      XCTAssertEqual(groups.joined(separator: " "), number)
      XCTAssertTrue(groups.dropLast().allSatisfy { $0.hasSuffix(",") })
      let cellSizes = groups.enumerated().map { index, text in
        TransportTransitTrain.estimatedCellSize(number: text, isContinuation: index > 0)
      }
      let frames = TransportTransitFlowLayout.frames(cellSizes: cellSizes + sizes([40]), width: width,
                                                     continuationItems: Set(groups.indices.dropFirst()))
      XCTAssertEqual(frames.separators.count, 1)
      XCTAssertTrue((frames.cells + frames.separators).allSatisfy { $0.minX >= 0 && $0.maxX <= width })
    }
  }

  private func sizes(_ widths: [CGFloat]) -> [CGSize] {
    widths.map { CGSize(width: $0, height: 20) }
  }
}
