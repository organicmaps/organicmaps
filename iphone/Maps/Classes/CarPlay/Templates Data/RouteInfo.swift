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
  let speed: Int
  let roundExitNumber: Int
  
  @objc init(timeToTarget: TimeInterval,
             targetDistance: String,
             targetUnits: String,
             distanceToTurn: String,
             streetName: String,
             turnUnits: String,
             turnImageName: String?,
             nextTurnImageName: String?,
             speed: Int,
             roundExitNumber: Int) {
    self.timeToTarget = timeToTarget
    self.targetDistance = targetDistance
    self.targetUnits = RouteInfo.unitLength(for: targetUnits)
    self.distanceToTurn = distanceToTurn
    self.streetName = streetName;
    self.turnUnits = RouteInfo.unitLength(for: turnUnits)
    self.turnImageName = turnImageName
    self.nextTurnImageName = nextTurnImageName
    self.speed = speed
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
