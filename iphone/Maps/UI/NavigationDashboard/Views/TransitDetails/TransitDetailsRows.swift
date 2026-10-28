/// A row on the transit details timeline.
enum TransitDetailsRow {
  case walk(MWMRouterTransitStepInfo)
  case ride(TransitDetailsRide)
  /// A user-added intermediate point (Add Stop); the name is nil when the point has none.
  case waypoint(index: Int, name: String?)
  case destination(String)
}

struct TransitDetailsRide {
  let step: MWMRouterTransitStepInfo
  /// The previous ride's line color when this ride starts at the stop where that ride ended. The shared
  /// stop is then drawn once, as this ride's heading.
  let transferFromColor: UIColor?
  /// The next ride starts at this ride's exit, so this ride's line runs on into that ride's transfer heading.
  let continuesIntoTransfer: Bool
  /// Hidden when the next ride starts at this exit and names it, or when the stop is unnamed.
  var exitStop: String? { continuesIntoTransfer ? nil : step.endStopName.nonEmpty }

  /// Hidden on a transfer, which names the stop as a transfer heading instead, or when the stop is unnamed.
  var boardStop: String? { transferFromColor == nil ? step.startStopName.nonEmpty : nil }
  var isTransfer: Bool { transferFromColor != nil }
}

enum TransitDetailsRows {
  /// Maps the route steps onto timeline rows, closed by the destination when it has a name.
  static func make(steps: [MWMRouterTransitStepInfo], destination: String?,
                   waypointNames: [Int: String]) -> [TransitDetailsRow] {
    var rows = steps.indices.compactMap { index -> TransitDetailsRow? in
      let step = steps[index]
      switch step.type {
      case .pedestrian:
        return .walk(step)
      case .intermediatePoint:
        return .waypoint(index: step.intermediateIndex, name: waypointNames[step.intermediateIndex])
      case .ruler:
        // Only straight-line routes have rulers; they have no per-leg details.
        return nil
      default:
        let transferFromColor = isSameStopTransfer(steps, at: index) ? steps[index - 1].color : nil
        return .ride(TransitDetailsRide(step: step, transferFromColor: transferFromColor,
                                        continuesIntoTransfer: isSameStopTransfer(steps, at: index + 1)))
      }
    }
    if let destination = destination?.nonEmpty {
      rows.append(.destination(destination))
    }
    return rows
  }

  /// Rides joined by a transfer edge may use differently named stops; those keep both the exit and the boarding
  /// stop. Unnamed stops can't be matched, so they never join.
  static func isSameStopTransfer(_ steps: [MWMRouterTransitStepInfo], at index: Int) -> Bool {
    guard index > 0, index < steps.count, isRide(steps[index - 1]), isRide(steps[index]) else { return false }
    let stop = steps[index].startStopName
    return !stop.isEmpty && steps[index - 1].endStopName == stop
  }

  private static func isRide(_ step: MWMRouterTransitStepInfo) -> Bool {
    switch step.type {
    case .pedestrian, .intermediatePoint, .ruler: false
    default: true
    }
  }
}

private extension String {
  var nonEmpty: String? { isEmpty ? nil : self }
}
