import CarPlay
@testable import Organic_Maps__Debug_
import XCTest

@available(iOS 17.4, *)
final class CarPlayManeuverMapperTests: XCTestCase {
  func testManeuverTypeMapping() {
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

  func testRoundaboutExitManeuverTypeMapping() {
    XCTAssertEqual(CarPlayManeuverMapper.maneuverType(for: .leaveRoundabout), .exitRoundabout)
    XCTAssertEqual(CarPlayManeuverMapper.maneuverType(for: .leaveRoundabout, roundaboutExitNumber: 1),
                   .roundaboutExit1)
    XCTAssertEqual(CarPlayManeuverMapper.maneuverType(for: .leaveRoundabout, roundaboutExitNumber: 19),
                   .roundaboutExit19)
    XCTAssertEqual(CarPlayManeuverMapper.maneuverType(for: .leaveRoundabout, roundaboutExitNumber: 20),
                   .exitRoundabout)
  }

  func testManeuverMetadata() {
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

  func testManeuverStateDistanceThresholds() {
    XCTAssertEqual(CarPlayManeuverMapper.maneuverState(distanceToTurnMeters: 50), .execute)
    XCTAssertEqual(CarPlayManeuverMapper.maneuverState(distanceToTurnMeters: 50.1), .prepare)
    XCTAssertEqual(CarPlayManeuverMapper.maneuverState(distanceToTurnMeters: 500), .prepare)
    XCTAssertEqual(CarPlayManeuverMapper.maneuverState(distanceToTurnMeters: 500.1), .initial)
  }
}
