@testable import Chart
import UIKit
import XCTest

@MainActor
final class ChartLineViewTests: XCTestCase {
  func testAreaFillsBelowSignedProfiles() throws {
    let profiles: [[CGFloat]] = [
      [-80, -50, -20],
      [20, 50, 80],
      [-50, 0, 50],
      [-0.2, 0, 0.2],
    ]
    for heights in profiles {
      for width: CGFloat in [320, 640] {
        let chart = makeChart(heights: heights, width: width)
        try assertFillBelowLine(chart)
      }
    }
  }

  func testFlatProfilesHaveDrawableStrokeAndFill() throws {
    for altitude: CGFloat in [-50, 0, 100] {
      let chart = makeChart(heights: [altitude, altitude, altitude])
      XCTAssertGreaterThan(chart.yAxisView.upperBound, chart.yAxisView.lowerBound)
      try assertFillBelowLine(chart)
    }
  }

  func testZoomedFillReachesBothBottomCorners() throws {
    let chart = makeChart(heights: [-80, -50, -20, -50, -80])
    chart.setViewport(lower: 1, upper: 3)
    try assertFillBelowLine(chart)
  }

  func testZoomingIntoFlatSectionKeepsPathsDrawable() throws {
    let chart = makeChart(heights: [-50, 0, 0, 0, 50])
    chart.setViewport(lower: 1, upper: 3)
    XCTAssertGreaterThan(chart.yAxisView.upperBound, chart.yAxisView.lowerBound)
    try assertFillBelowLine(chart)
  }

  func testSelectionFollowsZoomAndRecentersWithoutEchoingExternalChanges() {
    let chart = makeChart(heights: [100, 150, 200, 150, 100])
    chart.setSelectedPoint(1)
    var reportedDistances: [Double] = []
    chart.onSelectedPointChanged = { reportedDistances.append($0) }

    chart.setViewport(lower: 1, upper: 3)
    // Consecutive gesture updates can round to the same sample bounds.
    chart.setViewport(lower: 1, upper: 3)
    XCTAssertEqual(reportedDistances, [1.5])

    chart.setSelectedPoint(4)
    XCTAssertEqual(chart.xAxisView.lowerBound, 2)
    XCTAssertEqual(chart.xAxisView.upperBound, 4)
    XCTAssertEqual(chart.chartInfoView.infoX, 1)
    XCTAssertEqual(reportedDistances, [1.5])
  }

  func testFillStartsAtTheFirstDistanceAfterTransforming() throws {
    let values = [
      ChartValue(xValues: 10, y: -80),
      ChartValue(xValues: 20, y: -50),
      ChartValue(xValues: 30, y: -20),
    ]
    let line = ChartPresentationLine(TestLine(values: values, type: .lineArea))
    let originalPoints = points(in: line.path)
    let view = ChartLineView(frame: CGRect(x: 0, y: 0, width: 300, height: 150))
    view.chartLine = line
    view.setViewport(minX: 0, maxX: 2, minY: -100, maxY: 0)
    let fullPath = try XCTUnwrap(view.shapeLayer.path)
    XCTAssertEqual(fullPath.boundingBoxOfPath.minX, 100, accuracy: 0.001)

    view.setViewport(minX: 1, maxX: 2, minY: -100, maxY: 0)
    let zoomedPath = try XCTUnwrap(view.shapeLayer.path)
    let lastPoint = try XCTUnwrap(points(in: zoomedPath).last)
    XCTAssertEqual(lastPoint.x, -100, accuracy: 0.001)
    XCTAssertEqual(lastPoint.y, 0)
    XCTAssertEqual(points(in: line.path), originalPoints)
  }

  func testFollowedFillUsesTheConnectedSectionInRecordedCoordinates() throws {
    let values = zip([0, 100, 100, 300], [100, 130, 210, 190]).map {
      ChartValue(xValues: CGFloat($0.0), y: CGFloat($0.1))
    }
    let view = ChartLineView(frame: CGRect(x: 0, y: 0, width: 300, height: 150))
    let line = TestLine(values: values, type: .lineArea)
    for (entry, finish, expectedX, expectedHeight) in [(2.5, 2, 200.0, 200.0), (2.0, 3, 100.0, 210.0), (1.0, 0, 100.0, 130.0)] {
      view.chartLine = ChartPresentationLine(line, section: (entry, finish))
      view.setViewport(minX: 0, maxX: 3, minY: 50, maxY: 250)
      let path = try XCTUnwrap(view.shapeLayer.path)
      let rendered = points(in: path)
      XCTAssertEqual(rendered[0].x, expectedX, accuracy: 0.001)
      XCTAssertEqual(rendered[0].y, (expectedHeight - 50) / 200 * 149 + 0.5, accuracy: 0.001)
      XCTAssertEqual(rendered[1].x, values[finish].x, accuracy: 0.001)
      XCTAssertEqual(path.boundingBoxOfPath.width, abs(expectedX - values[finish].x), accuracy: 0.001)
    }
  }

  func testFollowedSectionKeepsTheFullProfileStrokeWidth() throws {
    let heights: [CGFloat] = [100, 150, 200, 150, 100]
    let chart = makeChart(heights: heights)
    let fullWidth = chart.lineViews[0].shapeLayer.lineWidth
    chart.chartData = ChartPresentationData(TestData(heights: heights, highlightedSection: (1, 3)), formatter: TestFormatter())
    let profile = chart.lineViews[0].shapeLayer
    let followed = try XCTUnwrap(profile.sublayers?.first as? CAShapeLayer)
    XCTAssertEqual(followed.lineWidth, fullWidth)
    XCTAssertGreaterThan(followed.lineWidth, profile.lineWidth)
    XCTAssertFalse(try XCTUnwrap(followed.path).isEmpty)
  }

  func testEntryAfterGapUsesItsOwnHeightAndKeepsOtherPointsSelectable() throws {
    // UIKit can round the selection overlay and its parent to different widths.
    let chart = ChartView(frame: CGRect(x: 0, y: 0, width: 370.5, height: 180))
    let data = TestData(heights: [100, 130, 210, 190], distances: [0, 1000, 1000, 5000], highlightedSection: (2, 3))
    chart.chartData = ChartPresentationData(data, formatter: TestFormatter())
    chart.setNeedsLayout()
    chart.layoutIfNeeded()
    let overlay = chart.chartInfoView
    chart.setSelectedPoint(1000)
    let entryX = overlay.infoX * overlay.bounds.width
    let entry = try XCTUnwrap(chart.chartInfoView(overlay, infoAtPointX: entryX))
    XCTAssertEqual(entry.1.first?.formattedValue, "210.0")
    var mapDistance: Double?
    chart.onSelectedPointChanged = { mapDistance = $0 }
    chart.chartInfoView(overlay, didMoveToPoint: entryX)
    XCTAssertEqual(try XCTUnwrap(mapDistance), 1000, accuracy: 1e-9)
    let outside = try XCTUnwrap(chart.chartInfoView(overlay, infoAtPointX: entryX / 2))
    XCTAssertEqual(outside.1.first?.formattedValue, "115.0")
  }

  func testViewportEdgeOnSegmentBreakKeepsBothHeights() {
    // 13 samples over 108 m draw at chart X = distance / 9, so both vertices of the break at 63 m are at X 7.
    let distances: [CGFloat] = [0, 9, 18, 27, 36, 45, 54, 63, 63, 81, 90, 99, 108]
    let heights: [CGFloat] = [100, 100, 100, 100, 100, 100, 100, 100, 0, 0, 0, 0, 0]
    let chart = ChartView(frame: CGRect(x: 0, y: 0, width: 320, height: 180))
    chart.chartData = ChartPresentationData(TestData(heights: heights, distances: distances), formatter: TestFormatter())
    chart.layoutIfNeeded()
    chart.setViewport(lower: 7, upper: 11)
    chart.layoutIfNeeded()

    XCTAssertEqual(chart.yAxisView.lowerBound, 0)
    XCTAssertGreaterThanOrEqual(chart.yAxisView.upperBound, 100)
  }

  private func makeChart(heights: [CGFloat], width: CGFloat = 320) -> ChartView {
    let chart = ChartView(frame: CGRect(x: 0, y: 0, width: width, height: 180))
    chart.chartData = ChartPresentationData(TestData(heights: heights), formatter: TestFormatter())
    chart.setNeedsLayout()
    chart.layoutIfNeeded()
    chart.lineViews.forEach { $0.layoutIfNeeded() }
    return chart
  }

  private func assertFillBelowLine(_ chart: ChartView, file: StaticString = #filePath, line: UInt = #line) throws {
    let stroke = try XCTUnwrap(chart.lineViews[0].shapeLayer.path, file: file, line: line)
    let fill = try XCTUnwrap(chart.lineViews[1].shapeLayer.path, file: file, line: line)
    XCTAssertFalse(stroke.isEmpty, file: file, line: line)
    XCTAssertFalse(fill.isEmpty, file: file, line: line)
    let fillPoints = points(in: fill)
    for corner in fillPoints.suffix(2) {
      XCTAssertEqual(corner.y, 0, file: file, line: line)
    }
    for point in points(in: stroke) + fillPoints {
      XCTAssertTrue(point.x.isFinite && point.y.isFinite, file: file, line: line)
    }

    let strokePoints = points(in: stroke)
    for (start, end) in zip(strokePoints, strokePoints.dropFirst()) {
      let x = (start.x + end.x) / 2
      guard x > 0, x < chart.chartsContainerView.bounds.width else { continue }
      let y = (start.y + end.y) / 2
      XCTAssertTrue(fill.contains(CGPoint(x: x, y: y / 2)), file: file, line: line)
      XCTAssertFalse(fill.contains(CGPoint(x: x, y: y + 1)), file: file, line: line)
    }
  }

  private func points(in path: CGPath) -> [CGPoint] {
    var result: [CGPoint] = []
    path.applyWithBlock { element in
      switch element.pointee.type {
      case .moveToPoint, .addLineToPoint:
        result.append(element.pointee.points[0])
      case .closeSubpath:
        break
      default:
        XCTFail("Expected a polyline")
      }
    }
    return result
  }
}

private struct TestLine: ChartLine {
  let values: [ChartValue]
  let type: ChartLineType
  var color: UIColor { .red }
}

private struct TestData: ChartData {
  let heights: [CGFloat]
  var distances: [CGFloat]?
  var highlightedSection: (entry: Double, finish: Int)?
  var xAxisValues: [Double] { (distances ?? heights.indices.map { CGFloat($0) }).map(Double.init) }
  var lines: [ChartLine] {
    let values = zip(xAxisValues, heights).map { ChartValue(xValues: CGFloat($0), y: $1) }
    return [TestLine(values: values, type: .line), TestLine(values: values, type: .lineArea)]
  }
}

private struct TestFormatter: ChartFormatter {
  func xAxisString(from value: Double) -> String { String(value) }
  func xAxisLabels(lowerBound: Double, upperBound: Double) -> [(value: Double, text: String)] {
    [(lowerBound, String(lowerBound)), (upperBound, String(upperBound))]
  }

  func yAxisString(from value: Double) -> String { String(value) }
  func yAxisLowerBound(from value: CGFloat) -> CGFloat { floor(value / 50) * 50 }
  func yAxisUpperBound(from value: CGFloat) -> CGFloat { ceil(value / 50) * 50 }
  func yAxisSteps(lowerBound: CGFloat, upperBound: CGFloat) -> [CGFloat] {
    Array(stride(from: lowerBound, through: upperBound, by: 50))
  }
}
