@testable import Organic_Maps__Debug_
import XCTest

final class MWMRoutePointTests: XCTestCase {
  func test_GivenMissingRoutePointMetadata_WhenInitialized_ThenUsesCoordinatesAsTitle() {
    for title in [nil, ""] as [String?] {
      let routePoint = MWMRoutePoint(cgPoint: .zero,
                                     title: title,
                                     subtitle: nil,
                                     type: .intermediate,
                                     intermediateIndex: 0)

      XCTAssertEqual(routePoint.title, routePoint.latLonString)
      XCTAssertEqual(routePoint.subtitle, "")
    }
  }

  func test_GivenNamedRoutePoint_WhenInitialized_ThenKeepsName() {
    let routePoint = MWMRoutePoint(cgPoint: .zero,
                                   title: "Some place",
                                   subtitle: nil,
                                   type: .intermediate,
                                   intermediateIndex: 0)

    XCTAssertEqual(routePoint.title, "Some place")
    XCTAssertEqual(routePoint.subtitle, "")
  }

  func test_GivenNoNearbyAddress_WhenInitializedFromMapPoint_ThenUsesCoordinatesAsTitle() {
    let point = CGPoint.zero
    let address = FrameworkHelper.address(at: point)
    XCTAssertNil(address)

    let routePoint = MWMRoutePoint(cgPoint: point,
                                   title: address,
                                   subtitle: nil,
                                   type: .intermediate,
                                   intermediateIndex: 0)

    XCTAssertEqual(routePoint.title, routePoint.latLonString)
  }
}
