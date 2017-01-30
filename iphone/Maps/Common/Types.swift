typealias MercatorCoordinate = Double

struct MercatorCoordinate2D {
  var x: MercatorCoordinate
  var y: MercatorCoordinate

  init() {
    x = 0
    y = 0
  }

  init(x: MercatorCoordinate, y: MercatorCoordinate) {
    self.x = x
    self.y = y
  }
}

extension MercatorCoordinate2D: Equatable {
  static func == (lhs: MercatorCoordinate2D, rhs: MercatorCoordinate2D) -> Bool {
    let eps = 1e-8
    return abs(lhs.x - rhs.x) < eps && abs(lhs.y - rhs.y) < eps
  }
}
