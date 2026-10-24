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

  func testSettingsLinkHighlightsInOpenSettings() throws {
    try openLink("om://settings")
    let settings = try waitForSettingsOnTop()

    try openLink("om://settings?highlight=icloud")

    XCTAssertTrue(navigationController.topViewController === settings)
    XCTAssertEqual(settingsCount, 1)
    XCTAssertNil(handler.getInAppFeatureHighlightData())
    waitForHighlight(of: .iCloud, in: settings)
  }

  func testSettingsLinkReturnsFromSettingsSubscreen() throws {
    try openLink("om://settings")
    let settings = try waitForSettingsOnTop()
    navigationController.pushViewController(SettingsBuilder.build(.units), animated: false)

    try openLink("om://settings?highlight=icloud")

    XCTAssertTrue(try waitForSettingsOnTop() === settings)
    XCTAssertEqual(settingsCount, 1)
    waitForHighlight(of: .iCloud, in: settings)
  }

  func testSettingsLinkScrollsToOffscreenSetting() throws {
    try openLink("om://settings?highlight=icloud")
    let settings = try waitForSettingsOnTop()

    waitForHighlight(of: .iCloud, in: settings)
  }

  private var settingsCount: Int {
    navigationController.viewControllers.filter { $0 is RootSettingsViewController }.count
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

  /// `UIView.highlight()` flashes a row with a tinted overlay that it adds to the row's cell.
  private func waitForHighlight(of setting: RootSettings, in settings: RootSettingsViewController) {
    let overlayColor = UIColor.linkBlueHighlighted.withAlphaComponent(0.2)
    waitUntil {
      settings.tableView.visibleCells.contains { cell in
        let color = overlayColor.resolvedColor(with: cell.traitCollection)
        return cell.textLabel?.text == setting.title
          && cell.subviews.contains { $0.backgroundColor?.resolvedColor(with: cell.traitCollection) == color }
      }
    }
  }

  private func waitUntil(_ condition: @escaping () -> Bool) {
    let expectation = XCTNSPredicateExpectation(predicate: NSPredicate { _, _ in condition() }, object: nil)
    wait(for: [expectation], timeout: 5)
  }
}
