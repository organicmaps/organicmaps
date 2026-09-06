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
    // The search engine is a process-wide singleton; leave it as the tests found it.
    Search.clear()
    Search.setSearchMode(.everywhere)
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
                              turnDirection: .left,
                              nextTurnDirection: .right,
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

  func testEmptySearchCompletesImmediately() {
    let searchService = CarPlaySearchService()
    var completionCount = 0

    searchService.searchText("", forInputLocale: "en") { results in
      XCTAssertEqual(results?.isEmpty, true)
      completionCount += 1
    }

    XCTAssertEqual(completionCount, 1)
  }

  func testSupersededSearchIsCompletedWithoutResults() {
    let searchService = CarPlaySearchService()
    var completionCount = 0

    searchService.searchText("query", forInputLocale: "en") { results in
      XCTAssertNil(results, "A superseded request must be distinguishable from an empty result set.")
      completionCount += 1
    }
    searchService.searchText("", forInputLocale: "en") { _ in }

    XCTAssertEqual(completionCount, 1, "A superseded request must still be completed.")
  }

  /// MWMSearch is shared with the phone UI: its newer query cancels a CarPlay request and completes with
  /// results of its own.
  func testSearchReplacedByAnotherQueryIsCompletedWithoutResults() {
    let searchService = CarPlaySearchService()
    var completionCount = 0

    searchService.searchText("query", forInputLocale: "en") { results in
      XCTAssertNil(results, "Results of another query must not be reported.")
      completionCount += 1
    }
    Search.searchQuery(SearchQuery("another query", source: .typedText))
    (searchService as? MWMSearchObserver)?.onSearchCompleted?()

    XCTAssertEqual(completionCount, 1)
  }

  /// `MWMSearch` reports completion only for everywhere-searches, so a CarPlay request must switch
  /// the shared mode; the phone UI leaves it at viewport, where the handler would never run.
  func testSearchSwitchesSharedModeToEverywhere() {
    Search.setSearchMode(.viewport)
    let searchService = CarPlaySearchService()

    searchService.searchText("query", forInputLocale: "en") { _ in }

    XCTAssertEqual(Search.searchMode(), .everywhere)
  }

  /// A pan button moves the viewport, so the map moves the opposite way.
  /// FrameworkHelper.moveMap uses an upward-positive vertical axis, unlike UIKit.
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
      XCTAssertEqual(direction.offset(step: step),
                     UIOffset(horizontal: horizontal * step, vertical: vertical * step),
                     "direction \(direction.rawValue)")
    }
  }
}
