import CarPlay

@available(iOS 17.4, *)
enum CarPlayManeuverMapper {
  private static let executeDistanceMeters = 50.0
  private static let prepareDistanceMeters = 500.0

  static func configure(_ maneuver: CPManeuver,
                        direction: RouteTurnDirection,
                        roadName: String,
                        roundaboutExitNumber: Int = 0) {
    maneuver.maneuverType = maneuverType(for: direction, roundaboutExitNumber: roundaboutExitNumber)
    maneuver.roadFollowingManeuverVariants = roadName.isEmpty ? nil : [roadName]
    maneuver.junctionType = isRoundabout(direction) ? .roundabout : .intersection
  }

  static func maneuverType(for direction: RouteTurnDirection,
                           roundaboutExitNumber: Int = 0) -> CPManeuverType {
    switch direction {
    case .none:
      return .noTurn
    case .straight:
      return .straightAhead
    case .right:
      return .rightTurn
    case .sharpRight:
      return .sharpRightTurn
    case .slightRight:
      return .slightRightTurn
    case .left:
      return .leftTurn
    case .sharpLeft:
      return .sharpLeftTurn
    case .slightLeft:
      return .slightLeftTurn
    case .uTurnLeft, .uTurnRight:
      return .uTurn
    case .enterRoundabout:
      return .enterRoundabout
    case .leaveRoundabout:
      guard (1 ... 19).contains(roundaboutExitNumber),
            let type = CPManeuverType(
              rawValue: CPManeuverType.roundaboutExit1.rawValue + UInt(roundaboutExitNumber - 1)
            )
      else {
        return .exitRoundabout
      }
      return type
    case .stayOnRoundabout:
      return .followRoad
    case .startAtEndOfStreet:
      return .startRoute
    case .destination:
      return .arriveAtDestination
    case .exitHighwayLeft:
      return .highwayOffRampLeft
    case .exitHighwayRight:
      return .highwayOffRampRight
    }
  }

  static func maneuverState(distanceToTurnMeters: Double) -> CPManeuverState {
    switch distanceToTurnMeters {
    case ...executeDistanceMeters:
      return .execute
    case ...prepareDistanceMeters:
      return .prepare
    default:
      return .initial
    }
  }

  private static func isRoundabout(_ direction: RouteTurnDirection) -> Bool {
    switch direction {
    case .enterRoundabout, .leaveRoundabout, .stayOnRoundabout:
      return true
    default:
      return false
    }
  }
}
