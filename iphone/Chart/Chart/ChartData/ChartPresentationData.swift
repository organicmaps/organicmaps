import UIKit

public class ChartPresentationData {
  private let chartData: ChartData
  private let presentationLines: [ChartPresentationLine]

  let formatter: ChartFormatter
  var linesCount: Int { chartData.lines.count }
  var pointsCount: Int { chartData.xAxisValues.count }
  var segmentDistances: [Double] { chartData.segmentDistances }

  public init(_ chartData: ChartData, formatter: ChartFormatter) {
    self.chartData = chartData
    self.formatter = formatter
    presentationLines = chartData.lines.map { ChartPresentationLine($0, section: chartData.highlightedSection) }
  }

  func setHighlightedSection(entry: Double, finish: Int) {
    for line in presentationLines {
      line.setHighlightedSection(entry: entry, finish: finish)
    }
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
}
