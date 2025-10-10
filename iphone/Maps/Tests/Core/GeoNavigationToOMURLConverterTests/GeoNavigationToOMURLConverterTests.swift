import XCTest
@testable import Organic_Maps__Debug_

@available(iOS 18.4, *)
final class GeoNavigationToOMURLConverterTests: XCTestCase {

  private let converter = GeoNavigationToOMURLConverter.self

  private let coordinate1 = "1.1,2.2"
  private let coordinate2 = "3.3,4.4"

  // MARK: - Directions tests

  func testDirections_GivenSourceAndDestination_WhenNoWaypoints_ThenBuildRoute() throws {
    let geoUrl = geoURLStub("geo-navigation:///directions?source=\(coordinate1)&destination=\(coordinate2)")
    let omUrl = try XCTUnwrap(converter.convert(geoUrl))
    let routingType = MWMRouter.type()
    XCTAssertEqual(omUrl.absoluteString, "om://route?sll=\(coordinate1)&saddr=&dll=\(coordinate2)&daddr=&type=\(MWMRouter.string(from: routingType)!)")
    let urlType = DeepLinkParser.parseAndSetApiURL(omUrl)
    switch urlType {
    case .route:
      let adapter = try XCTUnwrap(DeepLinkRouteStrategyAdapter(omUrl))
      CheckAlmostEqual(adapter.p1.latitude, 1.1)
      CheckAlmostEqual(adapter.p1.longitude, 2.2)
      XCTAssertEqual(adapter.p1.type, .start)
      XCTAssertFalse(adapter.p1.isMyPosition)
      CheckAlmostEqual(adapter.p2.latitude, 3.3)
      CheckAlmostEqual(adapter.p2.longitude, 4.4)
      XCTAssertFalse(adapter.p2.isMyPosition)
      XCTAssertEqual(adapter.type, routingType)
    default:
      XCTFail("Unexpected url type")
    }
  }

  func test_GivenDirections_WhenSourceDestinationAnd2Waypoints_ThenBuildRouteWithoutWaypoints() throws {
    let geoUrl = geoURLStub("geo-navigation:///directions?source=\(coordinate1)&destination=\(coordinate2)&waypoint=48.0,8.0&waypoint=47.0,8.0")
    let omUrl = try XCTUnwrap(converter.convert(geoUrl))
    let routingType = MWMRouter.type()
    XCTAssertEqual(omUrl.absoluteString, "om://route?sll=\(coordinate1)&saddr=&dll=\(coordinate2)&daddr=&type=vehicle")
    let urlType = DeepLinkParser.parseAndSetApiURL(omUrl)
    switch urlType {
    case .route:
      let adapter = try XCTUnwrap(DeepLinkRouteStrategyAdapter(omUrl))
      CheckAlmostEqual(adapter.p1.latitude, 1.1)
      CheckAlmostEqual(adapter.p1.longitude, 2.2)
      XCTAssertEqual(adapter.p1.type, .start)
      XCTAssertFalse(adapter.p1.isMyPosition)
      CheckAlmostEqual(adapter.p2.latitude, 3.3)
      CheckAlmostEqual(adapter.p2.longitude, 4.4)
      XCTAssertFalse(adapter.p2.isMyPosition)
      XCTAssertEqual(adapter.type, routingType)
    default:
      XCTFail("Unexpected url type")
    }
  }

  func test_GivenDirections_WhenSourceWithoutDestination_ThenFail() {
    let geoUrl = geoURLStub("geo-navigation:///directions?source=\(coordinate1)")
    XCTAssertNil(converter.convert(geoUrl))
  }

  func test_GivenDirections_WhenDestinationIsCoordinatesWithoutSource_ThenShowPlace() throws {
    let geoUrl = geoURLStub("geo-navigation:///directions?destination=\(coordinate1)")
    let omUrl = try XCTUnwrap(converter.convert(geoUrl))
    XCTAssertEqual(omUrl.absoluteString, "om://map?v=1&ll=\(coordinate1)")
    XCTAssertEqual(DeepLinkParser.parseAndSetApiURL(omUrl), .map)
  }

  func test_GivenDirections_WhenDestinationIsNameWithoutSource_ThenSearch() throws {
    let address = "PlaceName"
    let geoUrl = geoURLStub("geo-navigation:///directions?destination=\(address)")
    let omUrl = try XCTUnwrap(converter.convert(geoUrl))
    XCTAssertEqual(DeepLinkParser.parseAndSetApiURL(omUrl), .search)
    let sd = DeepLinkSearchData()
    XCTAssertEqual(sd.query.trimmingCharacters(in: .whitespacesAndNewlines), address)
    XCTAssertEqual(sd.locale, AppInfo.shared().twoLetterLanguageId)
  }

  func test_GivenDirections_WhenSourceAndDestinationUseNamesInsteadOfCoordinates_ThenFail() {
    let geoUrl = geoURLStub("geo-navigation:///directions?source=StartPoint&destination=EndPoint")
    XCTAssertNil(converter.convert(geoUrl))
  }

  // MARK: - Place tests

  func test_GivenPlace_WhenHasCoordinatesWithoutAddress_ThenShowPlace() throws {
    let geoUrl = geoURLStub("geo-navigation:///place?coordinate=\(coordinate1)")
    let omUrl = try XCTUnwrap(converter.convert(geoUrl))
    XCTAssertEqual(omUrl.absoluteString, "om://map?v=1&ll=\(coordinate1)")
    XCTAssertEqual(DeepLinkParser.parseAndSetApiURL(omUrl), .map)
  }

  func test_GivenPlace_WhenHasCoordinatesAndAddress_ThenShowPlace() throws {
    let address = "Address"
    let geoUrl = geoURLStub("geo-navigation:///place?coordinate=\(coordinate1)&address=\(address)")
    let omUrl = try XCTUnwrap(converter.convert(geoUrl))
    XCTAssertEqual(omUrl.absoluteString, "om://map?v=1&ll=\(coordinate1)&n=\(address)")
  }

  func test_GivenPlace_WhenNoAddressNoCoordinates_ThenFail() throws {
    let geoUrl = geoURLStub("geo-navigation:///place?coordinate=&address=")
    XCTAssertNil(converter.convert(geoUrl))
  }

  func test_GivenPlace_WhenBrokenCoordinates_ThenFail() throws {
    let geoUrl = geoURLStub("geo-navigation:///place?coordinate=1.1")
    XCTAssertNil(converter.convert(geoUrl))
  }

  func test_GivenPlace_WhenHasAddressWithoutCoordinates_ThenSearch() throws {
    let address = "Address"
    let geoUrl = geoURLStub("geo-navigation:///place?address=\(address)")
    let omUrl = try XCTUnwrap(converter.convert(geoUrl))
    let locale = AppInfo.shared().twoLetterLanguageId
    XCTAssertEqual(omUrl.absoluteString, "om://search?locale=\(locale)&query=\(address)")
    let urlType = DeepLinkParser.parseAndSetApiURL(omUrl)
    switch urlType {
    case .search:
      let sd = DeepLinkSearchData()
      XCTAssertEqual(sd.query.trimmingCharacters(in: .whitespacesAndNewlines), address)
      XCTAssertEqual(sd.locale, AppInfo.shared().twoLetterLanguageId)
    default:
      XCTFail("Unexpected url type")
    }
  }

  private func geoURLStub(_ s: String) -> URL {
    guard let url = URL(string: s) else {
      XCTFail("Bad URL: \(s)")
      fatalError()
    }
    return url
  }

  private func CheckAlmostEqual(_ lhs: Double, _ rhs: Double, tolerance: Double = 1e-6) {
    XCTAssertTrue(lhs - rhs < tolerance)
  }
}
