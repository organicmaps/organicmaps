@testable import Organic_Maps__Debug_
import XCTest

final class TransportTransitWrappingTests: XCTestCase {
  func testWrappedCellsAndSeparatorsStayWithinThePanel() throws {
    for width: CGFloat in [200, 300, 430] {
      var cursor = TransportTransitFlowLayout.Cursor(width: width)
      cursor.append(size: size(40), hasFollowingStep: true)
      cursor.append(size: size(1000), hasFollowingStep: true)
      cursor.append(size: size(1000), hasFollowingStep: false)
      let frames = cursor.frames
      XCTAssertTrue((frames.cells + frames.separators).allSatisfy { $0.minX >= 0 && $0.maxX <= width })
      XCTAssertEqual(frames.cells.last?.maxX, width)
      XCTAssertGreaterThan(try XCTUnwrap(frames.cells.last).minY, 0)
    }
  }

  func testNarrowPanelsKeepNonnegativeCellWidths() {
    for width: CGFloat in [0, 8, 20, 55] {
      var cursor = TransportTransitFlowLayout.Cursor(width: width)
      cursor.append(size: size(40), hasFollowingStep: true)
      cursor.append(size: size(1000), hasFollowingStep: false)
      for frame in cursor.frames.cells {
        XCTAssertGreaterThanOrEqual(frame.size.width, 0)
        XCTAssertGreaterThanOrEqual(frame.minX, 0)
        XCTAssertLessThanOrEqual(frame.maxX, width)
      }
    }
  }

  func testFirstBadgeUsesTheFullRowWithoutLeavingAnEmptyRow() {
    var cursor = TransportTransitFlowLayout.Cursor(width: 200)
    cursor.append(size: size(1000), hasFollowingStep: false)
    XCTAssertEqual(cursor.frames.cells, [CGRect(x: 0, y: 0, width: 200, height: 20)])
  }

  func testBadgesReserveSpaceOnlyForFollowingArrows() {
    var cursor = TransportTransitFlowLayout.Cursor(width: 100)
    cursor.append(size: size(40), hasFollowingStep: true)
    cursor.append(size: size(40), hasFollowingStep: false)
    XCTAssertEqual(cursor.frames.cells.last?.minY, 0)

    cursor = TransportTransitFlowLayout.Cursor(width: 200)
    cursor.append(size: size(70), hasFollowingStep: true)
    cursor.append(size: size(100), hasFollowingStep: true)
    cursor.append(size: size(30), hasFollowingStep: false)
    XCTAssertGreaterThan(cursor.frames.cells[1].minY, cursor.frames.cells[0].maxY)
    XCTAssertEqual(cursor.frames.cells[2].minY, cursor.frames.cells[1].minY)
  }

  func testContinuationsOnlySeparateDifferentLegs() throws {
    var cursor = TransportTransitFlowLayout.Cursor(width: 200)
    cursor.append(size: size(180), hasFollowingStep: false)
    cursor.append(size: size(180), hasFollowingStep: true, isContinuation: true)
    cursor.append(size: size(40), hasFollowingStep: false)
    let frames = cursor.frames
    XCTAssertEqual(frames.separators.count, 1)
    XCTAssertEqual(frames.cells[1].minX, 0)
    XCTAssertGreaterThan(frames.cells[1].minY, frames.cells[0].minY)
    let separator = try XCTUnwrap(frames.separators.first)
    XCTAssertEqual(separator.minY, frames.cells[1].minY)
    XCTAssertGreaterThanOrEqual(separator.minX, frames.cells[1].maxX)
    XCTAssertTrue((frames.cells + frames.separators).allSatisfy { $0.minX >= 0 && $0.maxX <= 200 })
  }

  func testFullWidthIdentifierMovesTheFollowingArrowToTheNextRow() throws {
    var cursor = TransportTransitFlowLayout.Cursor(width: 200)
    cursor.append(size: size(200), hasFollowingStep: true)
    cursor.append(size: size(40), hasFollowingStep: false)
    let frames = cursor.frames
    let separator = try XCTUnwrap(frames.separators.first)
    XCTAssertEqual(frames.cells[0].width, 200)
    XCTAssertGreaterThan(separator.minY, frames.cells[0].maxY)
    XCTAssertEqual(separator.minY, frames.cells[1].minY)
    XCTAssertTrue((frames.cells + frames.separators).allSatisfy { $0.minX >= 0 && $0.maxX <= 200 })
  }

  func testFirstIdentifierMovesToTheNextRowWhenItsCommaDoesNotFit() {
    let number = "101, 102"
    let firstRowWidth = TransportTransitTrain.estimatedCellSize(number: "101").width
    let groups = TransportTransitTrain.numberGroups(number, maximumWidth: 200,
                                                    firstRowWidth: firstRowWidth, hasFollowingStep: false)
    XCTAssertEqual(groups, [number])
  }

  func testLongListsKeepEveryIdentifierAndWrapComma() {
    let number = (1 ... 1000).map(String.init).joined(separator: ", ")
    for width: CGFloat in [200, 430] {
      let groups = TransportTransitTrain.numberGroups(number, maximumWidth: width,
                                                      firstRowWidth: 20, hasFollowingStep: true)
      XCTAssertEqual(groups.joined(separator: " "), number)
      XCTAssertTrue(groups.dropLast().allSatisfy { $0.hasSuffix(",") })
      for (index, text) in groups.enumerated() {
        XCTAssertLessThanOrEqual(TransportTransitTrain.estimatedCellSize(number: text, isContinuation: index > 0).width, width)
      }
    }
  }

  private func size(_ width: CGFloat) -> CGSize {
    CGSize(width: width, height: 20)
  }
}
