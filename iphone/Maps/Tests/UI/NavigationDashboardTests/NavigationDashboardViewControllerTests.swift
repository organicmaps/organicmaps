@testable import Organic_Maps__Debug_
import XCTest

final class NavigationDashboardViewControllerTests: XCTestCase {
  // The route preview is rendered at every position: 5 times a second with the sensor box of the navigation without
  // GPS. A view takes no touches while it is animated, so the Start button in the menu did nothing.
  func test_GivenShownBottomMenu_WhenPositionsCome_ThenItIsNotAnimated() throws {
    let mapViewController = try XCTUnwrap(MapViewController.shared())
    mapViewController.loadViewIfNeeded()
    let manager = MWMNavigationDashboardManager.shared()
    let delegate = try XCTUnwrap(manager as? MWMRoutePreviewDelegate & RouteNavigationControlsDelegate)
    let controller = NavigationDashboardBuilder.build(delegate: delegate)
    controller.add(to: mapViewController)
    let interactor = try XCTUnwrap(controller.interactor)
    defer {
      // Gives the map its visible area back.
      interactor.stateClosed()
      waitUntilDetached(controller)
    }
    let menu = try XCTUnwrap(findSubview(RouteActionsBottomMenuView.self, in: controller.view))

    interactor.stateReady()
    interactor.onNavigationInfoUpdated(MWMNavigationDashboardEntity())
    XCTAssertFalse(menu.isHidden)
    menu.layer.removeAllAnimations()

    interactor.onNavigationInfoUpdated(MWMNavigationDashboardEntity())
    XCTAssertNil(menu.layer.animationKeys())

    interactor.setHidden(true)
    XCTAssertTrue(menu.isHidden)
    interactor.setHidden(false)
    XCTAssertFalse(menu.isHidden)
    XCTAssertEqual(menu.alpha, 1)
  }

  private func findSubview<T: UIView>(_ type: T.Type, in view: UIView) -> T? {
    if let view = view as? T {
      return view
    }
    for child in view.subviews {
      if let found = findSubview(type, in: child) {
        return found
      }
    }
    return nil
  }

  private func waitUntilDetached(_ controller: UIViewController) {
    let detached = XCTNSPredicateExpectation(predicate: NSPredicate { _, _ in
      controller.parent == nil
    }, object: nil)
    wait(for: [detached], timeout: 3)
  }
}
