import UIKit

final class ChartPresentationLine {
  private let chartLine: ChartLine

  let minY: CGFloat
  let maxY: CGFloat
  let path: CGPath
  let pathStartX: CGFloat
  /// Maps a value's distance to the X coordinate of `path`.
  let xScale: CGFloat

  var values: [ChartValue] { chartLine.values }
  var color: UIColor { chartLine.color }
  var type: ChartLineType { chartLine.type }

  init(_ chartLine: ChartLine) {
    self.chartLine = chartLine

    let values = chartLine.values
    var lower = CGFloat(Int.max)
    var upper = CGFloat(Int.min)
    var maxDistance: CGFloat = 0
    for value in values {
      lower = min(value.y, lower)
      upper = max(value.y, upper)
      maxDistance = max(value.x, maxDistance)
    }
    minY = lower
    maxY = upper

    let xScale = maxDistance > 0 ? CGFloat(max(values.count - 1, 1)) / maxDistance : 0
    self.xScale = xScale
    pathStartX = (values.first?.x ?? 0) * xScale
    let path = CGMutablePath()
    for (index, value) in values.enumerated() {
      let point = CGPoint(x: value.x * xScale, y: value.y - minY)
      if index == 0 {
        path.move(to: point)
      } else {
        path.addLine(to: point)
      }
    }
    self.path = path
  }
}
