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

  func testListTemplateKeepsTheTypeItWasBuiltFor() {
    let template = ListTemplateBuilder.buildListTemplate(for: .searchResults(results: []))

    guard let type = template.userInfo as? ListTemplateBuilder.ListTemplateType,
          case .searchResults = type
    else {
      XCTFail("The template should keep the type it was built for.")
      return
    }
  }

  func testRefreshKeepsRowsOfTemplatesThatDoNotShowBookmarks() {
    let template = ListTemplateBuilder.buildListTemplate(for: .searchResults(results: []))
    template.updateSections([CPListSection(items: [CPListItem(text: "Result", detailText: nil)])])

    ListTemplateBuilder.refreshBookmarks(in: template)

    XCTAssertEqual(template.sections.first?.items.count, 1)
  }

  func testRefreshKeepsRowsOfTemplatesBuiltByAnybodyElse() {
    let section = CPListSection(items: [CPListItem(text: "Row", detailText: nil)])
    let template = CPListTemplate(title: "Any", sections: [section])

    ListTemplateBuilder.refreshBookmarks(in: template)

    XCTAssertEqual(template.sections.first?.items.count, 1)
  }
}
