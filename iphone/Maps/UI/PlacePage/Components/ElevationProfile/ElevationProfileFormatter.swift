import Chart
import CoreApi
import Foundation

final class ElevationProfileFormatter {
  private enum Constants {
    static let metersPerFoot = 0.3048
    static let metersPerMile = 1609.344
    static var metricAltitudeStep: CGFloat = 50
    static var imperialAltitudeStep: CGFloat = 100
  }

  private let distanceFormatter: DistanceFormatter.Type
  private let altitudeFormatter: AltitudeFormatter.Type
  private let axisNumberFormatter: NumberFormatter
  // Presenters keep the formatter across chart updates, so read the current units on each use.
  private let unitsProvider: () -> Units
  private var unitSystemMultiplier: CGFloat { unitsProvider() == .metric ? 1 : CGFloat(Constants.metersPerFoot) }
  private var altitudeStep: CGFloat {
    unitsProvider() == .metric ? Constants.metricAltitudeStep : Constants.imperialAltitudeStep
  }

  init(unitsProvider: @escaping () -> Units = Settings.measurementUnits, locale: Locale = .current) {
    self.unitsProvider = unitsProvider
    axisNumberFormatter = NumberFormatter()
    // Match the core formatter: Latin digits with the locale's decimal separator.
    axisNumberFormatter.locale = Locale(identifier: "en_US_POSIX")
    axisNumberFormatter.decimalSeparator = locale.decimalSeparator ?? "."
    axisNumberFormatter.numberStyle = .decimal
    axisNumberFormatter.usesGroupingSeparator = false
    distanceFormatter = DistanceFormatter.self
    altitudeFormatter = AltitudeFormatter.self
  }
}

extension ElevationProfileFormatter: ChartFormatter {
  func xAxisString(from value: Double) -> String {
    distanceFormatter.distanceString(fromMeters: value)
  }

  func xAxisLabels(lowerBound: Double, upperBound: Double) -> [(value: Double, text: String)] {
    // Use one unit for the whole axis; navigation formatting rounds and switches units per value.
    let (metersPerUnit, unit): (Double, String)
    if unitsProvider() == .metric {
      (metersPerUnit, unit) = upperBound >= 1000 ? (1000, "km") : (1, "m")
    } else {
      (metersPerUnit, unit) = upperBound >= 1000 * Constants.metersPerFoot
        ? (Constants.metersPerMile, "mi") : (Constants.metersPerFoot, "ft")
    }
    let suffix = "\u{202F}" + L(unit)
    let lower = lowerBound / metersPerUnit
    let upper = upperBound / metersPerUnit
    let range = upper - lower
    guard range > 0 else {
      axisNumberFormatter.maximumFractionDigits = 15
      return [(lowerBound, axisNumberFormatter.string(from: NSNumber(value: lower))! + suffix)]
    }

    let roughStep = range / 5
    let magnitude = pow(10, floor(log10(roughStep)))
    let multiplier: Double
    switch roughStep / magnitude {
    case ..<1.5: multiplier = 1
    case ..<3.5: multiplier = 2
    case ..<7.5: multiplier = 5
    default: multiplier = 10
    }
    let step = multiplier * magnitude
    axisNumberFormatter.maximumFractionDigits = max(0, -Int(floor(log10(step))))

    // A tiny tolerance retains boundary ticks after converting feet/miles to meters and back.
    let tolerance = step * 1e-9
    let first = ceil((lower - tolerance) / step)
    let last = floor((upper + tolerance) / step)
    return (0 ... Int(last - first)).map { index in
      let value = (first + Double(index)) * step
      let text = axisNumberFormatter.string(from: NSNumber(value: value))! + suffix
      return (value * metersPerUnit, text)
    }
  }

  func yAxisString(from value: Double) -> String {
    altitudeFormatter.altitudeString(fromMeters: value)
  }

  func yAxisLowerBound(from value: CGFloat) -> CGFloat {
    floor((value / unitSystemMultiplier) / altitudeStep) * altitudeStep * unitSystemMultiplier
  }

  func yAxisUpperBound(from value: CGFloat) -> CGFloat {
    ceil((value / unitSystemMultiplier) / altitudeStep) * altitudeStep * unitSystemMultiplier
  }

  func yAxisSteps(lowerBound: CGFloat, upperBound: CGFloat) -> [CGFloat] {
    let lower = yAxisLowerBound(from: lowerBound)
    let upper = yAxisUpperBound(from: upperBound)
    let range = upper - lower
    var stepSize = altitudeStep
    var stepsCount = Int((range / stepSize).rounded(.up))

    while stepsCount > 6 {
      stepSize *= 2 // Double the step size to reduce the step count
      stepsCount = Int((range / stepSize).rounded(.up))
    }

    let steps = stride(from: lower, through: upper, by: stepSize)
    return Array(steps)
  }
}
