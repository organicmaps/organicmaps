@objc(MWMRouteInfo)
class RouteInfo: NSObject {
  let timeToTarget: TimeInterval
  let targetDistance: String
  let targetUnits: UnitLength
  let distanceToTurn: String
  let turnUnits: UnitLength
  let streetName: String
  let turnImageName: String?
  let nextTurnImageName: String?
  let speedMps: Double
  let speedLimitMps: Double?
  let roundExitNumber: Int

  @objc init(timeToTarget: TimeInterval,
             targetDistance: String,
             targetUnitsIndex: UInt8,
             distanceToTurn: String,
             turnUnitsIndex: UInt8,
             streetName: String,
             turnImageName: String?,
             nextTurnImageName: String?,
             speedMps: Double,
             speedLimitMps: Double,
             roundExitNumber: Int) {
    self.timeToTarget = timeToTarget
    self.targetDistance = targetDistance
    self.targetUnits = RouteInfo.unitLength(for: targetUnitsIndex)
    self.distanceToTurn = distanceToTurn
    self.turnUnits = RouteInfo.unitLength(for: turnUnitsIndex)
    self.streetName = streetName;
    self.turnImageName = turnImageName
    self.nextTurnImageName = nextTurnImageName
    self.speedMps = speedMps
    // speedLimitMps >= 0 means known limited speed.
    self.speedLimitMps = speedLimitMps < 0 ? nil : speedLimitMps
    self.roundExitNumber = roundExitNumber
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
