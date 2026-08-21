@objc(MWMRouteInfo)
class RouteInfo: NSObject {
  let timeToTarget: TimeInterval
  let targetDistance: Double
  let targetUnits: UnitLength
  let distanceToTurn: Double
  let turnUnits: UnitLength
  let currentStreetName: String
  let streetName: String
  let nextStreetName: String
  let turnDirection: RouteTurnDirection
  let nextTurnDirection: RouteTurnDirection
  let turnImageName: String?
  let nextTurnImageName: String?
  let speedMps: Double
  let speedLimitMps: Double?
  let roundExitNumber: Int
  let isLeftHandTraffic: Bool

  @objc init(timeToTarget: TimeInterval,
             targetDistance: Double,
             targetUnitsIndex: UInt8,
             distanceToTurn: Double,
             turnUnitsIndex: UInt8,
             currentStreetName: String,
             streetName: String,
             nextStreetName: String,
             turnDirectionRawValue: Int,
             nextTurnDirectionRawValue: Int,
             turnImageName: String?,
             nextTurnImageName: String?,
             speedMps: Double,
             speedLimitMps: Double,
             roundExitNumber: Int,
             isLeftHandTraffic: Bool) {
    self.timeToTarget = timeToTarget
    self.targetDistance = targetDistance
    targetUnits = RouteInfo.unitLength(for: targetUnitsIndex)
    self.distanceToTurn = distanceToTurn
    turnUnits = RouteInfo.unitLength(for: turnUnitsIndex)
    self.currentStreetName = currentStreetName
    self.streetName = streetName
    self.nextStreetName = nextStreetName
    guard let turnDirection = RouteTurnDirection(rawValue: turnDirectionRawValue),
          let nextTurnDirection = RouteTurnDirection(rawValue: nextTurnDirectionRawValue)
    else {
      preconditionFailure("Unknown route turn direction")
    }
    self.turnDirection = turnDirection
    self.nextTurnDirection = nextTurnDirection
    self.turnImageName = turnImageName
    self.nextTurnImageName = nextTurnImageName
    self.speedMps = speedMps
    // speedLimitMps >= 0 means known limited speed.
    self.speedLimitMps = speedLimitMps < 0 ? nil : speedLimitMps
    self.roundExitNumber = roundExitNumber
    self.isLeftHandTraffic = isLeftHandTraffic
  }

  /// > Warning: Order of enum values MUST BE the same with
  /// > native ``Distance::Units`` enum (see platform/distance.hpp for details).
  class func unitLength(for targetUnitsIndex: UInt8) -> UnitLength {
    switch targetUnitsIndex {
    case 0:
      return .meters
    case 1:
      return .kilometers
    case 2:
      return .feet
    case 3:
      return .miles
    default:
      return .meters
    }
  }
}

@objc(MWMRouteTurnDirection)
enum RouteTurnDirection: Int {
  /// Raw values must match `routing::turns::CarDirection` in `routing/turns.hpp`.
  case none = 0
  case straight = 1
  case right = 2
  case sharpRight = 3
  case slightRight = 4
  case left = 5
  case sharpLeft = 6
  case slightLeft = 7
  case uTurnLeft = 8
  case uTurnRight = 9
  case enterRoundabout = 10
  case leaveRoundabout = 11
  case stayOnRoundabout = 12
  case startAtEndOfStreet = 13
  case destination = 14
  case exitHighwayLeft = 15
  case exitHighwayRight = 16
}
