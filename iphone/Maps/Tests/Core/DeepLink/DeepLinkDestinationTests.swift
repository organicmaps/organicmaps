@testable import Organic_Maps__Debug_
import XCTest

final class DeepLinkDestinationTests: XCTestCase {
  private let handler = DeepLinkHandler.shared
  private var controlsManager: MWMMapViewControlsManager!
  private var navigationController: UINavigationController!

  override func setUpWithError() throws {
    try super.setUpWithError()
    handler.reset()
    let mapViewController = try XCTUnwrap(MapsAppDelegate.theApp().mapViewController)
    controlsManager = mapViewController.controlsManager
    navigationController = try XCTUnwrap(mapViewController.navigationController)
  }

  override func tearDown() {
    navigationController.popToRootViewController(animated: false)
    controlsManager.menuState = .inactive
    controlsManager.menuRestoreState = .inactive
    handler.reset()
    super.tearDown()
  }

  func testMenuLinkWhileMenuIsHiddenDropsHighlight() throws {
    controlsManager.menuState = .hidden

    try openLink("om://menu?highlight=track-recorder")

    XCTAssertEqual(controlsManager.menuState, .hidden)
    XCTAssertNil(handler.getInAppFeatureHighlightData())
  }

  func testMenuLinkReturnsToMapFromSettings() throws {
    try openLink("om://settings")
    _ = try waitForSettingsOnTop()
    // The test host can't build the menu, so the map handles the link with a hidden menu.
    controlsManager.menuState = .hidden
    controlsManager.menuRestoreState = .hidden

    try openLink("om://menu?highlight=track-recorder")

    waitUntil { [navigationController] in
      navigationController?.topViewController is MapViewController
        && navigationController?.transitionCoordinator == nil
    }
    XCTAssertNil(handler.getInAppFeatureHighlightData())
  }

  private func openLink(_ link: String) throws {
    XCTAssertTrue(try handler.applicationDidOpenUrl(XCTUnwrap(URL(string: link))))
  }

  private func waitForSettingsOnTop() throws -> RootSettingsViewController {
    waitUntil { [navigationController] in
      navigationController?.topViewController is RootSettingsViewController
        && navigationController?.transitionCoordinator == nil
    }
    return try XCTUnwrap(navigationController.topViewController as? RootSettingsViewController)
  }

  private func waitUntil(_ condition: @escaping () -> Bool) {
    let expectation = XCTNSPredicateExpectation(predicate: NSPredicate { _, _ in condition() }, object: nil)
    wait(for: [expectation], timeout: 5)
  }
}
