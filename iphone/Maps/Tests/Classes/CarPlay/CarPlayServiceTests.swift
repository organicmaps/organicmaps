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
                              currentStreetName: "Bahdanoviča Street",
                              streetName: "Niamiha",
                              nextStreetName: "Internacyjanalnaja Street",
                              turnDirectionRawValue: RouteTurnDirection.left.rawValue,
                              nextTurnDirectionRawValue: RouteTurnDirection.right.rawValue,
                              turnImageName: nil,
                              nextTurnImageName: nil,
                              speedMps: 40.5,
                              speedLimitMps: 60,
                              roundExitNumber: 0,
                              isLeftHandTraffic: false)
    let estimates = carPlayService.createEstimates(routeInfo: routeInfo)

    guard let estimates else {
      XCTFail("Estimates should not be nil.")
      return
    }

    XCTAssertEqual(estimates.distanceRemaining, Measurement<UnitLength>(value: 25.2, unit: .kilometers))
    XCTAssertEqual(estimates.timeRemaining, 100)
  }

  @available(iOS 17.4, *)
  func testCarPlayManeuverTypeMapping() {
    let expectedTypes: [(RouteTurnDirection, CPManeuverType)] = [
      (.none, .noTurn),
      (.straight, .straightAhead),
      (.right, .rightTurn),
      (.sharpRight, .sharpRightTurn),
      (.slightRight, .slightRightTurn),
      (.left, .leftTurn),
      (.sharpLeft, .sharpLeftTurn),
      (.slightLeft, .slightLeftTurn),
      (.uTurnLeft, .uTurn),
      (.uTurnRight, .uTurn),
      (.enterRoundabout, .enterRoundabout),
      (.stayOnRoundabout, .followRoad),
      (.startAtEndOfStreet, .startRoute),
      (.destination, .arriveAtDestination),
      (.exitHighwayLeft, .highwayOffRampLeft),
      (.exitHighwayRight, .highwayOffRampRight),
    ]

    for (direction, expectedType) in expectedTypes {
      XCTAssertEqual(CarPlayManeuverMapper.maneuverType(for: direction), expectedType)
    }
  }

  @available(iOS 17.4, *)
  func testRoundaboutExitManeuverTypeMapping() {
    XCTAssertEqual(CarPlayManeuverMapper.maneuverType(for: .leaveRoundabout), .exitRoundabout)
    XCTAssertEqual(CarPlayManeuverMapper.maneuverType(for: .leaveRoundabout, roundaboutExitNumber: 1),
                   .roundaboutExit1)
    XCTAssertEqual(CarPlayManeuverMapper.maneuverType(for: .leaveRoundabout, roundaboutExitNumber: 19),
                   .roundaboutExit19)
    XCTAssertEqual(CarPlayManeuverMapper.maneuverType(for: .leaveRoundabout, roundaboutExitNumber: 20),
                   .exitRoundabout)
  }

  @available(iOS 17.4, *)
  func testCarPlayManeuverMetadata() {
    let maneuver = CPManeuver()

    CarPlayManeuverMapper.configure(maneuver, direction: .left, roadName: "Main Street")

    XCTAssertEqual(maneuver.maneuverType, .leftTurn)
    XCTAssertEqual(maneuver.roadFollowingManeuverVariants, ["Main Street"])
    XCTAssertEqual(maneuver.junctionType, .intersection)

    CarPlayManeuverMapper.configure(maneuver,
                                    direction: .leaveRoundabout,
                                    roadName: "Roundabout Road",
                                    roundaboutExitNumber: 3)

    XCTAssertEqual(maneuver.maneuverType, .roundaboutExit3)
    XCTAssertEqual(maneuver.junctionType, .roundabout)
  }
}
