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
  func test_GivenSearchModes_WhenStartedRepeatedly_ThenReusesOnlyMatchingMode() throws {
    let previousSearchMode = Search.searchMode()
    defer { Search.setSearchMode(previousSearchMode) }
    let mapViewController = try XCTUnwrap(MapViewController.shared())
    mapViewController.loadViewIfNeeded()
    let navigationManager = MWMNavigationDashboardManager.shared()
    let routePreviewDelegate = try XCTUnwrap(navigationManager as? MWMRoutePreviewDelegate)
    let manager = SearchOnMapManager()
    var controllers: [SearchOnMapViewController] = []
    defer {
      navigationManager.cancelRoutePointSelection()
      manager.close()
      controllers.forEach(waitUntilDetached)
    }

    manager.startSearching(isRouting: false)
    let normalController = try XCTUnwrap(manager.viewController)
    controllers.append(normalController)
    manager.startSearching(isRouting: false)
    XCTAssertTrue(manager.viewController === normalController)

    routePreviewDelegate.routePreviewDidSelect(
      MWMRoutePointSelection(point: nil, type: .finish, shouldAppend: false)
    )
    manager.startSearching(isRouting: true)
    let routePointController = try XCTUnwrap(manager.viewController)
    controllers.append(routePointController)
    XCTAssertFalse(routePointController === normalController)
    manager.startSearching(isRouting: true)
    XCTAssertTrue(manager.viewController === routePointController)

    navigationManager.cancelRoutePointSelection()
    manager.startSearching(isRouting: true)
    let routingController = try XCTUnwrap(manager.viewController)
    controllers.append(routingController)
    XCTAssertFalse(routingController === routePointController)
    manager.startSearching(isRouting: true)
    XCTAssertTrue(manager.viewController === routingController)

    waitUntilDetached(normalController)
    waitUntilDetached(routePointController)
    XCTAssertTrue(manager.viewController === routingController)
  }

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

  func test_GivenRoutePointSearch_WhenRouteStopsExternally_ThenClosesPickerAndSearch() throws {
    let mapViewController = try XCTUnwrap(MapViewController.shared())
    mapViewController.loadViewIfNeeded()
    let manager = mapViewController.searchManager
    let navigationManager = MWMNavigationDashboardManager.shared()
    let routePreviewDelegate = try XCTUnwrap(navigationManager as? MWMRoutePreviewDelegate)
    routePreviewDelegate.routePreviewDidSelect(
      MWMRoutePointSelection(point: nil, type: .finish, shouldAppend: false)
    )
    manager.startSearching(isRouting: true)
    let searchController = try XCTUnwrap(manager.viewController)
    searchController.interactor?.handle(.chooseOnMapButtonDidTap)
    let picker = try XCTUnwrap(mapViewController.children.compactMap { $0 as? MapPointPickerViewController }.first)

    MWMRouter.stopRouting()

    XCTAssertFalse(navigationManager.isRoutePointSelectionActive)
    XCTAssertFalse(manager.isSearching)
    waitUntilDetached(searchController)
    waitUntilDetached(picker)
  }

  func test_GivenOrdinarySearch_WhenRouteStopsExternally_ThenKeepsSearchOpen() throws {
    let mapViewController = try XCTUnwrap(MapViewController.shared())
    mapViewController.loadViewIfNeeded()
    let manager = mapViewController.searchManager
    manager.startSearching(isRouting: false)
    let searchController = try XCTUnwrap(manager.viewController)
    defer {
      manager.close()
      waitUntilDetached(searchController)
    }

    MWMRouter.stopRouting()

    XCTAssertTrue(manager.viewController === searchController)
  }

  private func waitUntilDetached(_ controller: UIViewController) {
    let detached = XCTNSPredicateExpectation(predicate: NSPredicate { _, _ in
      controller.parent == nil
    }, object: nil)
    wait(for: [detached], timeout: 3)
  }
}

final class MapPointPickerViewControllerTests: XCTestCase {
  func test_GivenVisibleMapControls_WhenPickerIsPresentedAndClosed_ThenKeepsControlsVisible() throws {
    let mapViewController = try XCTUnwrap(MapViewController.shared())
    mapViewController.loadViewIfNeeded()
    let controlsManager = try XCTUnwrap(MWMMapViewControlsManager.manager())
    let previousHidden = controlsManager.hidden
    let previousSideButtonsHidden = controlsManager.sideButtonsHidden
    let previousZoomHidden = controlsManager.zoomHidden
    controlsManager.hidden = false
    controlsManager.sideButtonsHidden = false
    controlsManager.zoomHidden = false
    defer {
      controlsManager.hidden = previousHidden
      controlsManager.sideButtonsHidden = previousSideButtonsHidden
      controlsManager.zoomHidden = previousZoomHidden
    }
    let picker = MapPointPickerViewController(title: "Choose point",
                                              hint: "Move the map",
                                              enableBounds: false,
                                              initialMercatorPosition: nil,
                                              shouldChangeViewport: false)

    picker.present(in: mapViewController)

    XCTAssertTrue(picker.parent === mapViewController)
    XCTAssertFalse(controlsManager.hidden)
    XCTAssertFalse(controlsManager.sideButtonsHidden)
    XCTAssertFalse(controlsManager.zoomHidden)

    picker.close()
    let detached = XCTNSPredicateExpectation(predicate: NSPredicate { _, _ in
      picker.parent == nil
    }, object: nil)
    wait(for: [detached], timeout: 3)
    XCTAssertFalse(controlsManager.hidden)
    XCTAssertFalse(controlsManager.sideButtonsHidden)
    XCTAssertFalse(controlsManager.zoomHidden)
  }

  func test_GivenButtonDismissalInProgress_WhenClosedExternally_ThenDoesNotDeliverResult() throws {
    let mapViewController = try XCTUnwrap(MapViewController.shared())
    mapViewController.loadViewIfNeeded()
    let picker = MapPointPickerViewController(title: "Choose point",
                                              hint: "Move the map",
                                              enableBounds: false,
                                              initialMercatorPosition: nil,
                                              shouldChangeViewport: false)
    var didCancel = false
    picker.cancelHandler = { didCancel = true }
    picker.present(in: mapViewController)
    let cancelButton = try XCTUnwrap(picker.view.firstDescendant {
      ($0 as? UIButton)?.title(for: .normal) == L("cancel")
    } as? UIButton)

    cancelButton.sendActions(for: .touchUpInside)
    picker.close()

    let detached = XCTNSPredicateExpectation(predicate: NSPredicate { _, _ in
      picker.parent == nil
    }, object: nil)
    wait(for: [detached], timeout: 3)
    XCTAssertFalse(didCancel)
  }
}

private extension UIView {
  func firstDescendant(where predicate: (UIView) -> Bool) -> UIView? {
    for subview in subviews {
      if predicate(subview) {
        return subview
      }
      if let descendant = subview.firstDescendant(where: predicate) {
        return descendant
      }
    }
    return nil
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
