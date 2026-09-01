@testable import Organic_Maps__Debug_
import XCTest

final class SearchOnMapHeaderViewTests: XCTestCase {
  private var header: SearchOnMapHeaderView!
  private var searchBar: UISearchBar!
  private var actions: UIStackView!

  override func setUpWithError() throws {
    try super.setUpWithError()
    header = SearchOnMapHeaderView(frame: CGRect(x: 0, y: 0, width: 400, height: 60))
    searchBar = try XCTUnwrap(header.subviews.compactMap { $0 as? UISearchBar }.first)
    actions = try XCTUnwrap(header.subviews.compactMap { $0 as? UIStackView }.first)
  }

  override func tearDown() {
    header = nil
    searchBar = nil
    actions = nil
    super.tearDown()
  }

  func test_GivenRoutePointActions_WhenTypingAndClearing_ThenVisibilityFollowsActualText() {
    header.setRoutePointActions(.init(title: "Choose destination", canSelectCurrentLocation: true))
    assertActionsHidden(false)

    searchBar.text = "cafe"
    searchBar.searchTextField.sendActions(for: .editingChanged)
    assertActionsHidden(true)

    searchBar.text = ""
    searchBar.searchTextField.sendActions(for: .editingChanged)
    assertActionsHidden(false)
  }

  func test_GivenProgrammaticQuery_WhenLocationAvailabilityChanges_ThenActionsStayHiddenUntilCleared() {
    header.setRoutePointActions(.init(title: "Choose destination", canSelectCurrentLocation: true))
    header.setSearchText("cafe")
    assertActionsHidden(true)

    header.setRoutePointActions(.init(title: "Choose destination", canSelectCurrentLocation: false))
    assertActionsHidden(true)
    XCTAssertEqual(header.searchQuery.text, "cafe")

    header.setSearchText("")
    assertActionsHidden(false)
    XCTAssertTrue(actions.arrangedSubviews[0].isHidden)
    XCTAssertFalse(actions.arrangedSubviews[1].isHidden)
  }

  func test_GivenOrdinarySearch_WhenQueryIsCleared_ThenRoutePointActionsStayHidden() {
    header.setRoutePointActions(nil)
    header.setSearchText("cafe")
    header.setSearchText("")

    assertActionsHidden(true)
  }

  private func assertActionsHidden(_ hidden: Bool, file: StaticString = #filePath, line: UInt = #line) {
    let settled = XCTNSPredicateExpectation(predicate: NSPredicate { [actions] _, _ in
      actions?.isHidden == hidden && actions?.alpha == (hidden ? 0 : 1)
    }, object: nil)
    XCTAssertEqual(XCTWaiter.wait(for: [settled], timeout: 2), .completed, file: file, line: line)
  }
}

final class SearchOnMapManagerTests: XCTestCase {
  func test_GivenClosingSearch_WhenReopenedBeforeAnimationEnds_ThenCreatesANewController() throws {
    let previousSearchMode = Search.searchMode()
    defer { Search.setSearchMode(previousSearchMode) }
    let mapViewController = try XCTUnwrap(MapViewController.shared())
    mapViewController.loadViewIfNeeded()
    let manager = SearchOnMapManager()
    let observer = SearchClosureObserver(manager: manager)
    manager.addObserver(observer)
    manager.startSearching(isRouting: false)
    let closingController = try XCTUnwrap(manager.viewController)

    manager.close()

    XCTAssertFalse(manager.isSearching)
    XCTAssertEqual(observer.wasSearchingWhenClosed, false)
    manager.startSearching(isRouting: false)
    let reopenedController = try XCTUnwrap(manager.viewController)
    defer {
      manager.close()
      waitUntilDetached(reopenedController)
    }
    XCTAssertFalse(closingController === reopenedController)

    waitUntilDetached(closingController)
    XCTAssertTrue(manager.viewController === reopenedController)
    XCTAssertTrue(reopenedController.parent === mapViewController)
  }

  private func waitUntilDetached(_ controller: UIViewController) {
    let detached = XCTNSPredicateExpectation(predicate: NSPredicate { _, _ in
      controller.parent == nil
    }, object: nil)
    wait(for: [detached], timeout: 3)
  }
}

private final class SearchClosureObserver: NSObject, SearchOnMapManagerObserver {
  private let manager: SearchOnMapManager
  var wasSearchingWhenClosed: Bool?

  init(manager: SearchOnMapManager) {
    self.manager = manager
  }

  func searchManager(didChangeState state: SearchOnMapState) {
    if state == .closed {
      wasSearchingWhenClosed = manager.isSearching
    }
  }
}
