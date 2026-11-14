import CoreApi
import CoreLocation
import XCTest

final class GeoUtilTests: XCTestCase {
  func testAnglesUseNearestLongitude() {
    let cases: [(Double, Double, Float)] = [
      (0, 0.2, 0), (0.2, 0, .pi),
      (179.9, -179.9, 0), (-179.9, 179.9, .pi),
    ]
    for (from, to, expected) in cases {
      let angle = GeoUtil.angle(atPoint: CLLocationCoordinate2D(latitude: 0, longitude: from),
                                toPoint: CLLocationCoordinate2D(latitude: 0, longitude: to))
      XCTAssertEqual(angle, expected, accuracy: 0.000001)
    }
  }
}
