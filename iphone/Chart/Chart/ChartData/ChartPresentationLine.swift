import UIKit

final class ChartPresentationLine {
  struct HighlightedPath {
    let path: CGPath
    let entry: CGPoint
  }

  private let chartLine: ChartLine

  let minY: CGFloat
  private(set) var path: CGPath = CGMutablePath()
  private(set) var highlighted: HighlightedPath?
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
    var maxDistance: CGFloat = 0
    for value in values {
      lower = min(value.y, lower)
      maxDistance = max(value.x, maxDistance)
    }
    minY = lower

    let xScale = maxDistance > 0 ? CGFloat(max(values.count - 1, 1)) / maxDistance : 0
    self.xScale = xScale
    pathStartX = (values.first?.x ?? 0) * xScale
    if let section {
      setHighlightedSection(entry: section.entry, finish: section.finish)
      if chartLine.type == .lineArea {
        return
      }
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

  func setHighlightedSection(entry: Double, finish: Int) {
    let values = chartLine.values
    let lower = Int(entry.rounded(.down))
    let upper = Int(entry.rounded(.up))
    let fraction = CGFloat(entry) - CGFloat(lower)
    let a = values[lower], b = values[upper]
    let point = CGPoint(x: (a.x + (b.x - a.x) * fraction) * xScale,
                        y: a.y + (b.y - a.y) * fraction - minY)
    let selected = CGMutablePath()
    selected.move(to: point)
    let step = Double(finish) > entry ? 1 : -1
    let first = step > 0 ? lower + 1 : upper - 1
    for i in stride(from: first, through: finish, by: step) {
      selected.addLine(to: CGPoint(x: values[i].x * xScale, y: values[i].y - minY))
    }
    highlighted = HighlightedPath(path: selected, entry: point)
    if chartLine.type == .lineArea {
      path = selected
    }
  }
}
