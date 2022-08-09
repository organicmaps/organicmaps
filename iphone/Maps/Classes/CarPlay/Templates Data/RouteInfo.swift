@objc(MWMRouteInfo)
class RouteInfo: NSObject {
  let timeToTarget: TimeInterval
  let targetDistance: String
  let targetUnits: UnitLength
  let distanceToTurn: String
  let streetName: String
  let turnUnits: UnitLength
  let turnImageName: String?
  let nextTurnImageName: String?
  let speedMps: Double
  let speedLimitMps: Double?
  let roundExitNumber: Int

  @objc init(timeToTarget: TimeInterval,
             targetDistance: String,
             targetUnits: String,
             distanceToTurn: String,
             streetName: String,
             turnUnits: String,
             turnImageName: String?,
             nextTurnImageName: String?,
             speedMps: Double,
             speedLimitMps: Double,
             roundExitNumber: Int) {
    self.timeToTarget = timeToTarget
    self.targetDistance = targetDistance
    self.targetUnits = RouteInfo.unitLength(for: targetUnits)
    self.distanceToTurn = distanceToTurn
    self.streetName = streetName;
    self.turnUnits = RouteInfo.unitLength(for: turnUnits)
    self.turnImageName = turnImageName
    self.nextTurnImageName = nextTurnImageName
    self.speedMps = speedMps
    // speedLimitMps >= 0 means known limited speed.
    self.speedLimitMps = speedLimitMps < 0 ? nil : speedLimitMps
    self.roundExitNumber = roundExitNumber
  }

  class func unitLength(for targetUnits: String) -> UnitLength {
    switch targetUnits {
    case "mi":
      return .miles
    case "ft":
      return .feet
    case "km":
      return .kilometers
    case "m":
      return .meters
    default:
      return .meters
    }
  }
}
