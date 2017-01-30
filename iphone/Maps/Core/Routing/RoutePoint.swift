import CoreLocation

@objc(MWMRoutePoint)
final class RoutePoint: NSObject {

  let point: MercatorCoordinate2D
  let name: String
  let isMyPosition: Bool
  var isValid: Bool { return self != RoutePoint() }
  var x: MercatorCoordinate { return point.x }
  var y: MercatorCoordinate { return point.y }

  init(point: MercatorCoordinate2D, name: String, isMyPosition: Bool = false) {
    self.point = point
    self.name = name
    self.isMyPosition = isMyPosition
    super.init()
  }
  convenience init(x: MercatorCoordinate, y: MercatorCoordinate, name: String, isMyPosition: Bool = false) {
    self.init(point: MercatorCoordinate2D(x: x, y: y), name: name, isMyPosition: isMyPosition)
  }

  convenience init(point: MercatorCoordinate2D) {
    self.init(point: point, name: L("p2p_your_location"), isMyPosition: true)
  }
  convenience init(x: MercatorCoordinate, y: MercatorCoordinate) {
    self.init(point: MercatorCoordinate2D(x: x, y: y))
  }

  convenience override init() {
    self.init(point: MercatorCoordinate2D(), name: "", isMyPosition: false)
  }

  override func isEqual(_ other: Any?) -> Bool {
    guard let other = other as? RoutePoint else { return false }
    return self == other
  }
}

func == (lhs: RoutePoint, rhs: RoutePoint) -> Bool {
  return lhs.isMyPosition == rhs.isMyPosition &&
    lhs.point == rhs.point &&
    lhs.name == rhs.name
}
