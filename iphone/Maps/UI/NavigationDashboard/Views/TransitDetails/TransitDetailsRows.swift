/// A row on the transit details timeline, carrying only what its view displays.
enum TransitDetailsRow {
  case walk(TransitDetailsWalk)
  case ride(TransitDetailsRide)
  /// Two rides joined at a stop they share, named once between the incoming and the outgoing line.
  case transfer(stop: String, from: UIColor, to: UIColor)
  /// A user-added intermediate point (Add Stop), shown by its marker number.
  case waypoint(number: Int, title: String)
  case destination(String)
}

struct TransitDetailsWalk {
  /// Empty when the core has no distance for the walk.
  let distance: String
  let timeInSec: Int
}

/// The routed line. Unlike the summary strip's number, which may list parallel lines, the stops of a ride
/// belong to this line only.
struct TransitDetailsLine {
  let type: MWMRouterTransitType
  let color: UIColor
  /// Empty when the line has no number; the badge then shows only the transport icon.
  let number: String
}

struct TransitDetailsRide {
  let line: TransitDetailsLine
  /// Nil when a transfer row names the stop, or when the stop is unnamed.
  let boardStop: String?
  let exitStop: String?
  let intermediateStops: [String]
  let stopCount: Int
  let timeInSec: Int
  /// The ride starts or ends at a transfer row, which joins the lines.
  let startsAtTransfer: Bool
  let endsAtTransfer: Bool

  var canExpand: Bool { !intermediateStops.isEmpty }
}

enum TransitDetailsRows {
  /// Maps the route steps onto timeline rows, closed by the destination when it has a name.
  static func make(steps: [MWMRouterTransitStepInfo], routePoints: [MWMRoutePoint]) -> [TransitDetailsRow] {
    var waypointNames: [Int: String] = [:]
    for point in routePoints where point.type == .intermediate && hasName(point) {
      waypointNames[point.intermediateIndex] = point.title
    }

    var rows: [TransitDetailsRow] = []
    var endsAtTransfer = false
    for (index, step) in steps.enumerated() {
      let startsAtTransfer = endsAtTransfer
      endsAtTransfer = false
      switch step.type {
      case .pedestrian:
        let distance = step.distance.isEmpty ? "" : "\(step.distance) \(step.distanceUnits)"
        rows.append(.walk(TransitDetailsWalk(distance: distance, timeInSec: step.timeInSec)))
      case .intermediatePoint:
        let number = step.intermediateIndex + 1
        let title = waypointNames[step.intermediateIndex] ?? String(format: L("transit_waypoint_label"), number)
        rows.append(.waypoint(number: number, title: title))
      case .ruler:
        // Only straight-line routes have rulers; they have no per-leg details.
        break
      default:
        endsAtTransfer = index + 1 < steps.count && isSameStopTransfer(from: step, to: steps[index + 1])
        rows.append(.ride(ride(step, startsAtTransfer: startsAtTransfer, endsAtTransfer: endsAtTransfer)))
        if endsAtTransfer {
          rows.append(.transfer(stop: step.endStopName, from: step.color, to: steps[index + 1].color))
        }
      }
    }
    if let destination = routePoints.last(where: { $0.type == .finish })?.title.nonEmpty {
      rows.append(.destination(destination))
    }
    return rows
  }

  /// An unnamed point's title is its formatted coordinates; such a stop is shown by its number instead.
  static func hasName(_ point: MWMRoutePoint) -> Bool {
    point.isMyPosition || (!point.title.isEmpty && point.title != point.latLonString)
  }

  private static func ride(_ step: MWMRouterTransitStepInfo, startsAtTransfer: Bool,
                           endsAtTransfer: Bool) -> TransitDetailsRide {
    TransitDetailsRide(line: TransitDetailsLine(type: step.type, color: step.color, number: step.lineNumber),
                       boardStop: startsAtTransfer ? nil : step.startStopName.nonEmpty,
                       exitStop: endsAtTransfer ? nil : step.endStopName.nonEmpty,
                       intermediateStops: step.intermediateStopNames,
                       stopCount: step.stopCount,
                       timeInSec: step.timeInSec,
                       startsAtTransfer: startsAtTransfer,
                       endsAtTransfer: endsAtTransfer)
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
