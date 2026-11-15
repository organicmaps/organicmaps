/// A row on the transit details timeline.
enum TransitDetailsRow {
  case walk(MWMRouterTransitStepInfo)
  case ride(TransitDetailsRide)
  /// Two rides joined at a stop they share, named once between the incoming and the outgoing line.
  case transfer(stop: String, from: UIColor, to: UIColor)
  /// A user-added intermediate point (Add Stop); the name is nil when the point has none.
  case waypoint(index: Int, name: String?)
  case destination(String)
}

struct TransitDetailsRide {
  let step: MWMRouterTransitStepInfo
  /// The ride starts or ends at a transfer row, which names that stop and joins the lines.
  let startsAtTransfer: Bool
  let endsAtTransfer: Bool

  /// Nil when a transfer row names the stop, or when the stop is unnamed.
  var boardStop: String? { startsAtTransfer ? nil : step.startStopName.nonEmpty }
  var exitStop: String? { endsAtTransfer ? nil : step.endStopName.nonEmpty }
}

enum TransitDetailsRows {
  /// Maps the route steps onto timeline rows, closed by the destination when it has a name.
  static func make(steps: [MWMRouterTransitStepInfo], destination: String?,
                   waypointNames: [Int: String]) -> [TransitDetailsRow] {
    var rows: [TransitDetailsRow] = []
    var endsAtTransfer = false
    for (index, step) in steps.enumerated() {
      let startsAtTransfer = endsAtTransfer
      endsAtTransfer = false
      switch step.type {
      case .pedestrian:
        rows.append(.walk(step))
      case .intermediatePoint:
        rows.append(.waypoint(index: step.intermediateIndex, name: waypointNames[step.intermediateIndex]))
      case .ruler:
        // Only straight-line routes have rulers; they have no per-leg details.
        break
      default:
        endsAtTransfer = index + 1 < steps.count && isSameStopTransfer(from: step, to: steps[index + 1])
        rows.append(.ride(TransitDetailsRide(step: step, startsAtTransfer: startsAtTransfer,
                                             endsAtTransfer: endsAtTransfer)))
        if endsAtTransfer {
          rows.append(.transfer(stop: step.endStopName, from: step.color, to: steps[index + 1].color))
        }
      }
    }
    if let destination = destination?.nonEmpty {
      rows.append(.destination(destination))
    }
    return rows
  }

  /// Rides joined by a transfer edge may use differently named stops; those keep both the exit and the boarding
  /// stop. Unnamed stops can't be matched, so they never join.
  private static func isSameStopTransfer(from step: MWMRouterTransitStepInfo,
                                         to next: MWMRouterTransitStepInfo) -> Bool {
    isRide(next) && !next.startStopName.isEmpty && step.endStopName == next.startStopName
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
