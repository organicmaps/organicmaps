import UIKit

public enum ChartType {
  case regular
  case percentage
}

public enum ChartLineType: String {
  case line = "line"
  case lineArea = "lineArea"
}

public protocol IFormatter {
  func distanceString(from value: Double) -> String
  func altitudeString(from value: Double) -> String
}

public protocol IChartData {
  var xAxisValues: [Double] { get }
  var lines: [IChartLine] { get }
  var type: ChartType { get }
}

public protocol IChartLine {
  var values: [Int] { get }
  var name: String { get }
  var color: UIColor { get }
  var type: ChartLineType { get }
}
