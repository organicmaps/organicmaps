@testable import Chart
import CoreApi
@testable import Organic_Maps__Debug_
import UIKit
import XCTest

final class ElevationXAxisTests: XCTestCase {
  private let locale = Locale(identifier: "en_US")

  func testZoomedTicksMatchTheirDistances() {
    for upper in [25500.0, 25510.0] {
      let ticks = formatter().xAxisLabels(lowerBound: 20500, upperBound: upper)
      XCTAssertEqual(ticks.map(\.value), [21000, 22000, 23000, 24000, 25000])
      XCTAssertEqual(ticks.map(\.text), (21 ... 25).map { "\($0)\u{202F}" + L("km") })
      let data = makeData(distances: stride(from: 0.0, through: 50000, by: 10).map { $0 })
      let axis = makeAxis(data: data, lower: 2050, upper: Int(upper / 10))
      for (label, tick) in zip(labels(in: axis), ticks) {
        XCTAssertEqual(label.frame.midX, CGFloat((tick.value - 20500) / (upper - 20500)) * 343, accuracy: 0.001)
      }
    }
  }

  func testSparseAndDenseProfilesHaveTheSameAxis() {
    let sparse = makeAxis(data: makeData(distances: [0, 510, 1200]), lower: 0, upper: 2)
    let dense = makeAxis(data: makeData(distances: stride(from: 0.0, through: 1200, by: 10).map { $0 }), lower: 0, upper: 120)
    XCTAssertEqual(labels(in: sparse).map(\.text), labels(in: dense).map(\.text))
    for (left, right) in zip(labels(in: sparse), labels(in: dense)) {
      XCTAssertEqual(left.frame.minX, right.frame.minX, accuracy: 0.001)
      XCTAssertEqual(left.frame.width, right.frame.width, accuracy: 0.001)
    }
    XCTAssertEqual(labels(in: sparse).count, 7)
    checkLayout(sparse)
  }

  func testElevenKilometerProfile() {
    let ticks = formatter().xAxisLabels(lowerBound: 0, upperBound: 11000)
    XCTAssertEqual(ticks.map(\.value), [0, 2000, 4000, 6000, 8000, 10000])
    checkFormatting(ticks, metersPerUnit: 1000)
  }

  func testZeroLengthAndSingleSample() {
    let ticks = formatter().xAxisLabels(lowerBound: 0, upperBound: 0)
    XCTAssertEqual(ticks.map(\.value), [0])
    XCTAssertEqual(ticks.map(\.text), ["0\u{202F}" + L("m")])
    for distances in [[0.0], [0.0, 0.0, 0.0]] {
      let axis = makeAxis(data: makeData(distances: distances), lower: 0, upper: distances.count - 1)
      XCTAssertEqual(labels(in: axis).count, 1)
      checkLayout(axis)
    }
  }

  func testZeroLengthProfileKeepsItsLabelAfterZoomAndPan() throws {
    for units in [Units.metric, .imperial] {
      let data = ChartPresentationData(Profile(distances: Array(repeating: 0, count: 501)), formatter: formatter(units))
      let axis = makeAxis(data: data, lower: 0, upper: 500)
      for lower in [0, 205, 449] {
        axis.setBounds(lower: lower, upper: lower + 51)
        axis.layoutIfNeeded()
        let labels = labels(in: axis)
        XCTAssertEqual(labels.count, 1)
        let label = try XCTUnwrap(labels.first)
        XCTAssertFalse(label.isHidden)
        XCTAssertEqual(label.text, "0\u{202F}" + L(units == .metric ? "m" : "ft"))
        XCTAssertEqual(label.frame.minX, 0)
        checkLayout(axis)
      }
    }
  }

  func testSubmeterWindowsRetainPrecision() {
    for units in [Units.metric, .imperial] {
      for range in [0.1, 0.001, 0.0000001] {
        let ticks = formatter(units).xAxisLabels(lowerBound: 0, upperBound: range)
        XCTAssertGreaterThan(ticks.count, 1)
        checkFormatting(ticks, metersPerUnit: units == .metric ? 1 : 0.3048)
      }
      let ticks = formatter(units).xAxisLabels(lowerBound: 20000.001, upperBound: 20000.011)
      checkFormatting(ticks, metersPerUnit: units == .metric ? 1000 : 1609.344)
    }
  }

  func testTenthsOfAMileStayInMiles() {
    let ticks = formatter(.imperial).xAxisLabels(lowerBound: 0, upperBound: 0.5 * 1609.344)
    XCTAssertEqual(ticks.count, 6)
    XCTAssertEqual(ticks[1].value, 0.1 * 1609.344, accuracy: 1e-9)
    XCTAssertEqual(ticks[1].text, "0.1\u{202F}" + L("mi"))
    checkFormatting(ticks, metersPerUnit: 1609.344)
  }

  func testUnitTransitionsUseOneUnitAndDistinctText() {
    for (units, threshold, lowUnit, highUnit, lowScale, highScale) in [
      (Units.metric, 1000.0, "m", "km", 1.0, 1000.0),
      (Units.imperial, 304.8, "ft", "mi", 0.3048, 1609.344),
    ] {
      for upper in [threshold - 0.01, threshold, threshold + 0.01] {
        let ticks = formatter(units).xAxisLabels(lowerBound: threshold - 0.1, upperBound: upper)
        let high = upper >= threshold
        XCTAssertTrue(ticks.allSatisfy { $0.text.hasSuffix(L(high ? highUnit : lowUnit)) })
        checkFormatting(ticks, metersPerUnit: high ? highScale : lowScale)
      }
    }
  }

  func testTickInvariantsAcrossScales() {
    for units in [Units.metric, .imperial] {
      let f = formatter(units)
      for exponent in -8 ... 7 {
        let range = pow(10.0, Double(exponent))
        for lower in [0, range * 0.13, range * 20.5, 20000.001] {
          let upper = lower + range
          let ticks = f.xAxisLabels(lowerBound: lower, upperBound: upper)
          XCTAssertFalse(ticks.isEmpty)
          XCTAssertLessThanOrEqual(ticks.count, 9)
          let scale = units == .metric ? (upper >= 1000 ? 1000.0 : 1.0) : (upper >= 304.8 ? 1609.344 : 0.3048)
          checkFormatting(ticks, metersPerUnit: scale)
          for tick in ticks {
            XCTAssertGreaterThanOrEqual(tick.value, lower - range * 1e-8)
            XCTAssertLessThanOrEqual(tick.value, upper + range * 1e-8)
          }
          for (left, right) in zip(ticks, ticks.dropFirst()) {
            XCTAssertLessThan(left.value, right.value)
          }
        }
      }
    }
  }

  func testLocalizedDecimalSeparator() {
    let ticks = ElevationProfileFormatter(unitsProvider: { .metric }, locale: Locale(identifier: "fr_FR"))
      .xAxisLabels(lowerBound: 0, upperBound: 0.1)
    XCTAssertTrue(ticks.contains { $0.text.hasPrefix("0,02") })
  }

  func testVisibleLabelsKeepAUniformDistanceStep() {
    let data = makeData(distances: [0, 700, 1410])
    let axis = makeAxis(data: data, lower: 0, upper: 2)
    let ticks = formatter().xAxisLabels(lowerBound: 0, upperBound: 1410)
    for width in [288.0, 343] {
      axis.frame.size.width = width
      axis.layoutIfNeeded()
      let visible = zip(labels(in: axis), ticks).filter { !$0.0.isHidden }.map(\.1.value)
      XCTAssertGreaterThan(visible.count, 1)
      let step = visible[1] - visible[0]
      for (left, right) in zip(visible, visible.dropFirst()) {
        XCTAssertEqual(right - left, step, accuracy: 1e-9)
      }
      checkLayout(axis)
    }
    axis.frame.size.width = 343
    axis.chartData = makeData(distances: stride(from: 0.0, through: 50000, by: 10).map { $0 })
    axis.setBounds(lower: 2600, upper: 3250)
    axis.layoutIfNeeded()
    XCTAssertEqual(labels(in: axis).last(where: { !$0.isHidden })?.text, "32\u{202F}" + L("km"))
    checkLayout(axis)
  }

  func testLatinDigitsMatchTheCoreFormatter() {
    for localeID in ["fa_IR", "ar_SA", "ar_EG"] {
      let f = ElevationProfileFormatter(unitsProvider: { .metric }, locale: Locale(identifier: localeID))
      let ticks = f.xAxisLabels(lowerBound: 0, upperBound: 0.1)
      XCTAssertTrue(ticks.contains { $0.text.hasPrefix("0٫02") }, localeID)
    }
  }

  func testCollisionsEdgesResizingAndFontChanges() {
    let axis = makeAxis(data: makeData(distances: [0, 510, 1200]), lower: 0, upper: 2)
    for width in [20.0, 60, 100, 180, 343, 700] {
      for size in [12.0, 24, 40] {
        axis.frame.size.width = width
        axis.font = .systemFont(ofSize: size)
        axis.layoutIfNeeded()
        checkLayout(axis)
      }
    }
    axis.frame.size.width = 700
    axis.font = .systemFont(ofSize: 12)
    axis.layoutIfNeeded()
    XCTAssertTrue(labels(in: axis).allSatisfy { !$0.isHidden })
  }

  func testEdgeLabelsFitWithoutMovingTheirTicks() throws {
    let data = makeData(distances: stride(from: 0.0, through: 50000, by: 10).map { $0 })
    let axis = makeAxis(data: data, lower: 2099, upper: 2599)
    let first = labels(in: axis)[0]
    XCTAssertFalse(first.isHidden)
    XCTAssertEqual(first.frame.minX, 0)
    XCTAssertEqual(try XCTUnwrap(tickPositions(in: axis).first), 343 * 10 / 5000, accuracy: 0.001)
    checkLayout(axis)
    axis.setBounds(lower: 2100, upper: 2600)
    axis.layoutIfNeeded()
    XCTAssertEqual(labels(in: axis).first?.frame.minX, 0)
    XCTAssertEqual(labels(in: axis).last?.frame.maxX, 343)
    checkLayout(axis)
  }

  func testLastMileLabelStaysVisibleAtItsActualDistance() throws {
    // The imported test track is slightly longer than half a mile.
    let length = 805.554985973293
    let data = ChartPresentationData(Profile(distances: [0, 510, length]), formatter: formatter(.imperial))
    let axis = makeAxis(data: data, lower: 0, upper: 2)
    let last = try XCTUnwrap(labels(in: axis).last)
    XCTAssertEqual(last.text, "0.5\u{202F}" + L("mi"))
    for width in [100.0, 180, 250, 288, 343, 358, 700] {
      for size in [12.0, 24] {
        axis.frame.size.width = width
        axis.font = .systemFont(ofSize: size)
        axis.layoutIfNeeded()
        XCTAssertFalse(last.isHidden)
        XCTAssertEqual(last.frame.maxX, width, accuracy: 0.001)
        XCTAssertEqual(try XCTUnwrap(tickPositions(in: axis).last), width * 0.5 * 1609.344 / length, accuracy: 0.001)
        checkLayout(axis)
      }
    }
  }

  func testLongProfilesKeepTheirLastRoundLabel() throws {
    for (length, lastDistance, text) in [(25370.0, 25000.0, "25"), (100_500.0, 100_000.0, "100")] {
      let axis = makeAxis(data: makeData(distances: [0, length / 2, length]), lower: 0, upper: 2)
      let last = try XCTUnwrap(labels(in: axis).last)
      XCTAssertEqual(last.text, text + "\u{202F}" + L("km"))
      XCTAssertFalse(last.isHidden)
      XCTAssertEqual(last.frame.maxX, 343, accuracy: 0.001)
      XCTAssertEqual(try XCTUnwrap(tickPositions(in: axis).last), 343 * lastDistance / length, accuracy: 0.001)
      checkLayout(axis)
    }
  }

  func testLastLabelMovesContinuouslyAtTheEdge() throws {
    let length = 100_000.0 / 0.9
    let axis = makeAxis(data: makeData(distances: [0, length / 2, length]), lower: 0, upper: 2)
    let last = try XCTUnwrap(labels(in: axis).last)
    let transitionWidth = last.intrinsicContentSize.width / (2 * (1 - 0.9))
    axis.frame.size.width = transitionWidth - 0.01
    axis.layoutIfNeeded()
    let x = last.frame.minX
    axis.frame.size.width = transitionWidth + 0.01
    axis.layoutIfNeeded()
    XCTAssertFalse(last.isHidden)
    XCTAssertLessThanOrEqual(abs(last.frame.minX - x), 0.021)
    checkLayout(axis)
  }

  func testPanAndReconfiguration() {
    let axis = makeAxis(data: makeData(distances: [0, 20000, 50000]), lower: 0, upper: 2)
    axis.chartData = makeData(distances: stride(from: 0.0, through: 50000, by: 10).map { $0 })
    for lower in stride(from: 0, through: 4490, by: 37) {
      axis.setBounds(lower: lower, upper: lower + 501)
      axis.layoutIfNeeded()
      checkLayout(axis)
      XCTAssertTrue(labels(in: axis).allSatisfy { $0.text != nil })
    }
  }

  func testSelectingATickReturnsItsDistance() throws {
    let chart = ChartView(frame: CGRect(x: 0, y: 0, width: 343, height: 200))
    chart.chartData = makeData(distances: stride(from: 0.0, through: 50000, by: 10).map { $0 })
    chart.layoutIfNeeded()
    chart.chartPreviewView.setX(min: 2050, max: 2551)
    chart.xAxisView.layoutIfNeeded()
    let tickX = try XCTUnwrap(tickPositions(in: chart.xAxisView).first)
    var selectedDistance: Double?
    chart.onSelectedPointChanged = { selectedDistance = $0 }
    chart.chartInfoView(chart.chartInfoView, didMoveToPoint: tickX)
    XCTAssertEqual(try XCTUnwrap(selectedDistance), 21000, accuracy: 0.001)
  }

  func testUnitsAreReadOnEachUpdate() {
    var units = Units.metric
    let f = ElevationProfileFormatter(unitsProvider: { units }, locale: locale)
    XCTAssertEqual(f.xAxisLabels(lowerBound: 0, upperBound: 2000).last?.text, "2\u{202F}" + L("km"))
    XCTAssertEqual(f.yAxisLowerBound(from: 120), 100)
    units = .imperial
    XCTAssertEqual(f.xAxisLabels(lowerBound: 0, upperBound: 2000).last?.text, "1.2\u{202F}" + L("mi"))
    XCTAssertEqual(f.yAxisLowerBound(from: 120), 300 * 0.3048, accuracy: 1e-9)
  }

  private func formatter(_ units: Units = .metric) -> ElevationProfileFormatter {
    ElevationProfileFormatter(unitsProvider: { units }, locale: locale)
  }

  private func checkFormatting(_ ticks: [(value: Double, text: String)], metersPerUnit: Double,
                               file: StaticString = #filePath, line: UInt = #line) {
    XCTAssertEqual(Set(ticks.map(\.text)).count, ticks.count, file: file, line: line)
    let parser = NumberFormatter()
    parser.locale = locale
    parser.numberStyle = .decimal
    for tick in ticks {
      let number = String(tick.text.split(separator: "\u{202F}")[0])
      guard let value = parser.number(from: number)?.doubleValue else {
        XCTFail("Cannot parse \(tick.text)", file: file, line: line)
        continue
      }
      XCTAssertEqual(value * metersPerUnit, tick.value, accuracy: max(1e-14, abs(tick.value) * 1e-12), file: file, line: line)
    }
  }

  private func checkLayout(_ axis: ChartXAxisView, file: StaticString = #filePath, line: UInt = #line) {
    let visible = labels(in: axis).filter { !$0.isHidden }
    for label in visible {
      XCTAssertGreaterThanOrEqual(label.frame.minX, -0.001, file: file, line: line)
      XCTAssertLessThanOrEqual(label.frame.maxX, axis.bounds.width + 0.001, file: file, line: line)
      XCTAssertGreaterThanOrEqual(label.frame.width, label.intrinsicContentSize.width, file: file, line: line)
    }
    for (left, right) in zip(visible, visible.dropFirst()) {
      XCTAssertGreaterThanOrEqual(right.frame.minX - left.frame.maxX, 4 - 0.001, file: file, line: line)
    }

    let tickXs = tickPositions(in: axis)
    let lower = axis.chartData.distance(forChartX: CGFloat(axis.lowerBound))
    let upper = axis.chartData.distance(forChartX: CGFloat(axis.upperBound))
    let ticks = axis.chartData.formatter.xAxisLabels(lowerBound: lower, upperBound: upper)
    let visibleTicks = zip(labels(in: axis), ticks).filter { !$0.0.isHidden }.map(\.1.value)
    XCTAssertEqual(tickXs.count, visibleTicks.count, file: file, line: line)
    for (x, distance) in zip(tickXs, visibleTicks) {
      let fraction = upper > lower ? (distance - lower) / (upper - lower) : 0
      XCTAssertEqual(x, axis.bounds.width * CGFloat(fraction), accuracy: 0.001, file: file, line: line)
    }
  }

  private func tickPositions(in axis: ChartXAxisView) -> [CGFloat] {
    var positions: [CGFloat] = []
    (axis.layer as? CAShapeLayer)?.path?.applyWithBlock { element in
      if element.pointee.type == .moveToPoint {
        positions.append(element.pointee.points[0].x)
      }
    }
    return positions
  }

  private func makeAxis(data: ChartPresentationData, lower: Int, upper: Int) -> ChartXAxisView {
    let axis = ChartXAxisView(frame: CGRect(x: 0, y: 0, width: 343, height: 26))
    axis.chartData = data
    axis.setBounds(lower: lower, upper: upper)
    axis.layoutIfNeeded()
    return axis
  }

  private func labels(in axis: ChartXAxisView) -> [UILabel] {
    axis.subviews.compactMap { $0 as? UILabel }
  }

  private func makeData(distances: [Double]) -> ChartPresentationData {
    ChartPresentationData(Profile(distances: distances), formatter: formatter())
  }

  private struct Profile: ChartData {
    let distances: [Double]
    var xAxisValues: [Double] { distances }
    var lines: [ChartLine] { [Line(distances: distances)] }
    var type: ChartType { .regular }
  }

  private struct Line: ChartLine {
    let distances: [Double]
    var values: [ChartValue] { distances.enumerated().map { ChartValue(xValues: CGFloat($0.element), y: 100 + CGFloat($0.offset % 50)) } }
    var color: UIColor { .blue }
    var type: ChartLineType { .line }
  }
}
