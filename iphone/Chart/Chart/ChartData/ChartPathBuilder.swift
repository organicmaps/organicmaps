import UIKit

protocol ChartPathBuilder {
  func build(_ lines: [ChartPresentationLine])
  func makeLinePath(line: ChartPresentationLine) -> UIBezierPath
  func makePercentLinePath(line: ChartPresentationLine, bottomLine: ChartPresentationLine?) -> UIBezierPath
}

extension ChartPathBuilder {
  func makeLinePreviewPath(line: ChartPresentationLine) -> UIBezierPath {
    let path = UIBezierPath()
    let values = line.values
    let xScale = CGFloat(values.count) / values.maxDistance
    for i in 0 ..< values.count {
      let x = values[i].x * xScale
      let y = values[i].y - line.minY
      if i == 0 {
        path.move(to: CGPoint(x: x, y: y))
      } else {
        path.addLine(to: CGPoint(x: x, y: y))
      }
    }
    return path
  }

  func makeLinePath(line: ChartPresentationLine) -> UIBezierPath {
    let path = UIBezierPath()
    let values = line.values
    let xScale = CGFloat(values.count) / values.maxDistance
    for i in 0 ..< values.count {
      let x = values[i].x * xScale
      let y = values[i].y - line.minY
      if i == 0 {
        path.move(to: CGPoint(x: x, y: y))
      } else {
        path.addLine(to: CGPoint(x: x, y: y))
      }
    }
    return path
  }

  func makePercentLinePreviewPath(line: ChartPresentationLine, bottomLine _: ChartPresentationLine?) -> UIBezierPath {
    let path = UIBezierPath()
    path.move(to: CGPoint(x: 0, y: 0))
    let aggregatedValues = line.aggregatedValues
    let xScale = CGFloat(aggregatedValues.count) / aggregatedValues.maxDistance
    for i in 0 ..< aggregatedValues.count {
      let x = aggregatedValues[i].x * xScale
      let y = aggregatedValues[i].y - CGFloat(line.minY)
      path.addLine(to: CGPoint(x: x, y: y))
    }
    path.addLine(to: CGPoint(x: aggregatedValues.maxDistance, y: 0))
    path.close()
    return path
  }

  func makePercentLinePath(line: ChartPresentationLine, bottomLine _: ChartPresentationLine?) -> UIBezierPath {
    let path = UIBezierPath()
    path.move(to: CGPoint(x: 0, y: 0))
    let aggregatedValues = line.aggregatedValues
    let xScale = CGFloat(aggregatedValues.count) / aggregatedValues.maxDistance
    for i in 0 ..< aggregatedValues.count {
      let x = aggregatedValues[i].x * xScale
      let y = aggregatedValues[i].y - CGFloat(line.minY)
      path.addLine(to: CGPoint(x: x, y: y))
    }
    path.addLine(to: CGPoint(x: aggregatedValues.maxDistance, y: 0))
    path.close()
    return path
  }
}

final class DefaultChartPathBuilder {
  private let builders: [ChartType: ChartPathBuilder] = [
    .regular: LinePathBuilder(),
    .percentage: PercentagePathBuilder(),
  ]

  func build(_ lines: [ChartPresentationLine], type: ChartType) {
    builders[type]?.build(lines)
  }
}

final class LinePathBuilder: ChartPathBuilder {
  func build(_ lines: [ChartPresentationLine]) {
    for item in lines {
      item.aggregatedValues = item.values
      if item.type == .lineArea {
        item.minY = 0
        for val in item.values {
          item.maxY = max(val.y, item.maxY)
        }
        item.path = makePercentLinePath(line: item, bottomLine: nil)
        item.previewPath = makePercentLinePreviewPath(line: item, bottomLine: nil)
      } else {
        for val in item.values {
          item.minY = min(val.y, item.minY)
          item.maxY = max(val.y, item.maxY)
        }
        item.path = makeLinePath(line: item)
        item.previewPath = makeLinePreviewPath(line: item)
      }
    }
  }
}

final class PercentagePathBuilder: ChartPathBuilder {
  func build(_ lines: [ChartPresentationLine]) {
    for line in lines {
      line.minY = 0
      line.maxY = CGFloat(Int.min)
    }

    for i in 0 ..< lines[0].values.count {
      let sum = CGFloat(lines.reduce(0) { r, l in r + l.values[i].y })
      var aggrPercentage: CGFloat = 0
      for line in lines {
        aggrPercentage += CGFloat(line.values[i].y) / sum * 100
        line.aggregatedValues.append(ChartValue(xValues: lines[0].values[i].x, y: aggrPercentage))
        line.maxY = max(round(aggrPercentage), CGFloat(line.maxY))
      }
    }

    var prevLine: ChartPresentationLine? = nil
    for item in lines {
      item.path = makePercentLinePath(line: item, bottomLine: prevLine)
      item.previewPath = makePercentLinePreviewPath(line: item, bottomLine: prevLine)
      prevLine = item
    }
  }
}
