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
             turnDirection: RouteTurnDirection,
             nextTurnDirection: RouteTurnDirection,
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
  case none
  case straight
  case right
  case sharpRight
  case slightRight
  case left
  case sharpLeft
  case slightLeft
  case uTurnLeft
  case uTurnRight
  case enterRoundabout
  case leaveRoundabout
  case stayOnRoundabout
  case startAtEndOfStreet
  case destination
  case exitHighwayLeft
  case exitHighwayRight
}
