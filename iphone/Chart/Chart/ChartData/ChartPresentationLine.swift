import UIKit

final class ChartPresentationLine {
  struct HighlightedPath {
    let path: CGPath
    let entry: CGPoint
  }

  private let chartLine: ChartLine

  let minY: CGFloat
  let maxY: CGFloat
  let path: CGPath
  let highlighted: HighlightedPath?
  let pathStartX: CGFloat
  /// Maps a value's distance to the X coordinate of `path`.
  let xScale: CGFloat

  var values: [ChartValue] { chartLine.values }
  var color: UIColor { chartLine.color }
  var type: ChartLineType { chartLine.type }

  init(_ chartLine: ChartLine, section: (entry: Double, finish: Int)? = nil) {
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
    if let section {
      let lower = Int(section.entry.rounded(.down))
      let upper = Int(section.entry.rounded(.up))
      let fraction = CGFloat(section.entry) - CGFloat(lower)
      let a = values[lower], b = values[upper]
      let entry = CGPoint(x: (a.x + (b.x - a.x) * fraction) * xScale,
                          y: a.y + (b.y - a.y) * fraction - minY)
      let selected = CGMutablePath()
      selected.move(to: entry)
      let step = Double(section.finish) > section.entry ? 1 : -1
      let first = step > 0 ? lower + 1 : upper - 1
      for i in stride(from: first, through: section.finish, by: step) {
        selected.addLine(to: CGPoint(x: values[i].x * xScale, y: values[i].y - minY))
      }
      highlighted = HighlightedPath(path: selected, entry: entry)
      if chartLine.type == .lineArea {
        path = selected
        return
      }
    } else {
      highlighted = nil
    }

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
