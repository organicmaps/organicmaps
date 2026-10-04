import Chart
import CoreApi

final class ElevationProfileFormatter {
  private enum Constants {
    static let metricToImperialMultiplier: CGFloat = 0.3048
    static var metricAltitudeStep: CGFloat = 50
    static var imperialAltitudeStep: CGFloat = 100
  }

  private let distanceFormatter: DistanceFormatter.Type
  private let altitudeFormatter: AltitudeFormatter.Type
  private let settings: Settings.Type

  private var unitSystemMultiplier: CGFloat {
    settings.altitudeUnits() == .imperial ? Constants.metricToImperialMultiplier : 1
  }

  private var altitudeStep: CGFloat {
    settings.altitudeUnits() == .imperial ? Constants.imperialAltitudeStep : Constants.metricAltitudeStep
  }

  init(settings: Settings.Type = Settings.self) {
    distanceFormatter = DistanceFormatter.self
    altitudeFormatter = AltitudeFormatter.self
    self.settings = settings
  }
}

extension ElevationProfileFormatter: ChartFormatter {
  func xAxisString(from value: Double) -> String {
    distanceFormatter.distanceString(fromMeters: value)
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
    var stepSize = altitudeStep * unitSystemMultiplier
    var stepsCount = Int((range / stepSize).rounded(.up))

    while stepsCount > 6 {
      stepSize *= 2 // Double the step size to reduce the step count
      stepsCount = Int((range / stepSize).rounded(.up))
    }

    let steps = stride(from: lower, through: upper, by: stepSize)
    return Array(steps)
  }
}
