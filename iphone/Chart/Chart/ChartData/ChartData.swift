import UIKit

public enum ChartType {
  case regular
  case percentage
}

public enum ChartLineType: String {
  case line
  case lineArea
}

public protocol ChartFormatter {
  func xAxisString(from value: Double) -> String
  func yAxisString(from value: Double) -> String
  func yAxisLowerBound(from value: CGFloat) -> CGFloat
  func yAxisUpperBound(from value: CGFloat) -> CGFloat
  func yAxisSteps(lowerBound: CGFloat, upperBound: CGFloat) -> [CGFloat]
}

public protocol ChartData {
  var xAxisValues: [Double] { get }
  var lines: [ChartLine] { get }
  var type: ChartType { get }
}

public protocol ChartLine {
  var values: [ChartValue] { get }
  var color: UIColor { get }
  var type: ChartLineType { get }
}

public struct ChartValue {
  let x: CGFloat
  let y: CGFloat

  public init(xValues: CGFloat, y: CGFloat) {
    x = xValues
    self.y = y
  }
}

public extension Array {
  func firstIndexAtOrAfter<Value: Comparable>(_ value: Value, by keyPath: KeyPath<Element, Value>) -> Int? {
    guard !isEmpty else { return nil }

    var lower = 0
    var upper = count
    while lower < upper {
      let mid = lower + (upper - lower) / 2
      if self[mid][keyPath: keyPath] < value {
        lower = mid + 1
      } else {
        upper = mid
      }
    }

    return lower < count ? lower : nil
  }
}

extension Array where Element == ChartValue {
  var maxDistance: CGFloat { map(\.x).max() ?? 0 }

  func interpolatedAltitude(at distance: CGFloat) -> CGFloat {
    guard let first, let last else { return 0 }
    if distance <= first.x {
      return first.y
    }
    if distance >= last.x {
      return last.y
    }

    guard let upperIndex = firstIndexAtOrAfter(distance, by: \.x) else {
      return last.y
    }
    let upper = self[upperIndex]
    if upper.x == distance || upperIndex == 0 {
      return upper.y
    }

    let lower = self[upperIndex - 1]
    let progress = (distance - lower.x) / (upper.x - lower.x)
    return lower.y + progress * (upper.y - lower.y)
  }
}
