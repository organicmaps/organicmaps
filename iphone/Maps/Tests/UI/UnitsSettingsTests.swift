import CoreApi
@testable import Organic_Maps__Debug_
import XCTest

final class UnitsSettingsTests: XCTestCase {
  override func setUp() {
    super.setUp()
    MockUnitsSettings.distanceUnits = .metric
    MockUnitsSettings.elevationUnits = .metric
  }

  func testChangingElevationKeepsDistanceUnits() {
    let interactor = UnitsSettingsInteractor(settings: MockUnitsSettings.self)
    interactor.handle(.didSelect(.altitude(.imperial)))

    XCTAssertEqual(MockUnitsSettings.altitudeUnits(), .imperial)
    XCTAssertEqual(MockUnitsSettings.measurementUnits(), .metric)
  }

  func testChangingDistanceKeepsElevationUnits() {
    let interactor = UnitsSettingsInteractor(settings: MockUnitsSettings.self)
    interactor.handle(.didSelect(.distance(.imperial)))

    XCTAssertEqual(MockUnitsSettings.measurementUnits(), .imperial)
    XCTAssertEqual(MockUnitsSettings.altitudeUnits(), .metric)
  }

  func testMetersChartWithMilesDistances() {
    MockUnitsSettings.distanceUnits = .imperial
    let formatter = ElevationProfileFormatter(settings: MockUnitsSettings.self)

    XCTAssertEqual(formatter.yAxisLowerBound(from: 51), 50)
    XCTAssertEqual(formatter.yAxisUpperBound(from: 51), 100)
    XCTAssertEqual(formatter.yAxisSteps(lowerBound: 0, upperBound: 200), [0, 50, 100, 150, 200])
  }

  func testFeetChartWithKilometerDistances() {
    MockUnitsSettings.elevationUnits = .imperial
    let formatter = ElevationProfileFormatter(settings: MockUnitsSettings.self)

    XCTAssertEqual(formatter.yAxisLowerBound(from: 100), 91.44, accuracy: 0.0001)
    XCTAssertEqual(formatter.yAxisUpperBound(from: 100), 121.92, accuracy: 0.0001)
    let steps = formatter.yAxisSteps(lowerBound: 0, upperBound: 100)
    XCTAssertEqual(steps.count, 5)
    let expected: [CGFloat] = [0, 30.48, 60.96, 91.44, 121.92]
    for (actual, expectedValue) in zip(steps, expected) {
      XCTAssertEqual(actual, expectedValue, accuracy: 0.0001)
    }
  }

  func testFeetChartBelowSeaLevel() {
    MockUnitsSettings.elevationUnits = .imperial
    let formatter = ElevationProfileFormatter(settings: MockUnitsSettings.self)

    XCTAssertEqual(formatter.yAxisLowerBound(from: -10), -30.48, accuracy: 0.0001)
    XCTAssertEqual(formatter.yAxisUpperBound(from: -10), 0, accuracy: 0.0001)
  }

  func testExistingChartUsesChangedElevationUnits() {
    let formatter = ElevationProfileFormatter(settings: MockUnitsSettings.self)
    XCTAssertEqual(formatter.yAxisLowerBound(from: 100), 100)

    MockUnitsSettings.elevationUnits = .imperial
    XCTAssertEqual(formatter.yAxisLowerBound(from: 100), 91.44, accuracy: 0.0001)
  }
}

private final class MockUnitsSettings: Settings {
  static var distanceUnits: Units = .metric
  static var elevationUnits: Units = .metric

  override class func measurementUnits() -> Units { distanceUnits }
  override class func setMeasurementUnits(_ units: Units) { distanceUnits = units }
  override class func altitudeUnits() -> Units { elevationUnits }
  override class func setAltitudeUnits(_ units: Units) { elevationUnits = units }
}
