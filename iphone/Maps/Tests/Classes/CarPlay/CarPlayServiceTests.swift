import CarPlay
@testable import Organic_Maps__Debug_
import XCTest

final class CarPlayServiceTests: XCTestCase {
  var carPlayService: CarPlayService!

  override func setUp() {
    super.setUp()
    carPlayService = CarPlayService()
  }

  override func tearDown() {
    carPlayService = nil
    super.tearDown()
  }

  func testCreateEstimates() {
    let routeInfo = RouteInfo(timeToTarget: 100,
                              targetDistance: 25.2,
                              targetUnitsIndex: 1, // km
                              distanceToTurn: 0.5,
                              turnUnitsIndex: 0, // m
                              streetName: "Niamiha",
                              turnImageName: nil,
                              nextTurnImageName: nil,
                              speedMps: 40.5,
                              speedLimitMps: 60,
                              roundExitNumber: 0)
    let estimates = carPlayService.createEstimates(routeInfo: routeInfo)

    guard let estimates else {
      XCTFail("Estimates should not be nil.")
      return
    }

    XCTAssertEqual(estimates.distanceRemaining, Measurement<UnitLength>(value: 25.2, unit: .kilometers))
    XCTAssertEqual(estimates.timeRemaining, 100)
  }

  /// The offsets are in mixed axes, so pin the signs for every direction combination.
  func testPanDirectionOffset() {
    let step: CGFloat = 0.25
    // Direction, and the expected offset in `step` units.
    let expected: [(CPMapTemplate.PanDirection, CGFloat, CGFloat)] = [
      ([], 0, 0),
      ([.left], 1, 0),
      ([.right], -1, 0),
      ([.up], 0, -1),
      ([.down], 0, 1),
      ([.left, .right], 0, 0),
      ([.up, .down], 0, 0),
      ([.left, .up], 1, -1),
      ([.left, .down], 1, 1),
      ([.right, .up], -1, -1),
      ([.right, .down], -1, 1),
      ([.left, .right, .up], 0, -1),
      ([.left, .right, .down], 0, 1),
      ([.left, .up, .down], 1, 0),
      ([.right, .up, .down], -1, 0),
      ([.left, .right, .up, .down], 0, 0),
    ]

    for (direction, horizontal, vertical) in expected {
      let offset = direction.offset(step: step)
      XCTAssertEqual(offset.horizontal, horizontal * step, "direction \(direction.rawValue)")
      XCTAssertEqual(offset.vertical, vertical * step, "direction \(direction.rawValue)")
    }
  }
}
