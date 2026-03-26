import UIKit

public class ChartPresentationData {
  private let chartData: ChartData
  private var presentationLines: [ChartPresentationLine]
  private let pathBuilder = DefaultChartPathBuilder()

  let formatter: ChartFormatter
  var linesCount: Int { chartData.lines.count }
  var pointsCount: Int { chartData.xAxisValues.count }
  var xAxisValues: [Double] { chartData.xAxisValues }
  var segmentDistances: [Double] { chartData.segmentDistances }
  var type: ChartType { chartData.type }
  var labels: [String]
  var lower = CGFloat(Int.max)
  var upper = CGFloat(Int.min)

  public init(_ chartData: ChartData, formatter: ChartFormatter) {
    self.chartData = chartData
    self.formatter = formatter
    presentationLines = chartData.lines.map { ChartPresentationLine($0) }
    labels = chartData.xAxisValues.map { formatter.xAxisString(from: $0) }
    recalculateBounds()
  }

  func labelAt(_ point: CGFloat) -> String {
    formatter.xAxisString(from: xAxisValueAt(point))
  }

  func xAxisValueAt(_ point: CGFloat) -> Double {
    distance(forChartX: point)
  }

  func chartX(forDistance distance: Double) -> CGFloat {
    guard pointsCount > 1, let maxDistance = chartData.xAxisValues.last, maxDistance > 0 else { return 0 }
    return CGFloat(distance / maxDistance) * CGFloat(pointsCount - 1)
  }

  func distance(forChartX point: CGFloat) -> Double {
    guard pointsCount > 1, let maxDistance = chartData.xAxisValues.last else {
      return chartData.xAxisValues.last ?? 0
    }
    return Double(point) / Double(pointsCount - 1) * maxDistance
  }

  func lineAt(_ index: Int) -> ChartPresentationLine {
    presentationLines[index]
  }

  private func recalculateBounds() {
    presentationLines.forEach { $0.aggregatedValues = [] }
    pathBuilder.build(presentationLines, type: type)

    var l = CGFloat(Int.max)
    var u = CGFloat(Int.min)
    for presentationLine in presentationLines {
      l = min(presentationLine.minY, l)
      u = max(presentationLine.maxY, u)
    }
    lower = l
    upper = u
  }
}
