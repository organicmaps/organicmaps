@testable import Organic_Maps__Debug_
import UIKit
import XCTest

private final class RouteValidationObserver: NSObject, RoutingManagerListener {
  var onEvent: ((RouterResultCode) -> Void)?

  func processRouteBuilderEvent(with code: RouterResultCode, countries _: [String]) {
    onEvent?(code)
  }

  func didLocationUpdate(_: [String]) {}
  func updateCameraInfo(isCameraOnRoute _: Bool, speedLimitMps _: Double) {}
}

final class DeepLinkHandlerTests: XCTestCase {
  private let handler = DeepLinkHandler.shared

  override func setUp() {
    super.setUp()
    handler.reset()
  }

  override func tearDown() {
    handler.reset()
    super.tearDown()
  }

  func testColdCustomSchemeLinkIsQueuedUntilMapIsReady() throws {
    let url = try XCTUnwrap(URL(string: "om://map?ll=52.52,13.405&n=Test"))
    handler.prepareForColdLaunch(url: url)

    XCTAssertTrue(handler.hasPendingColdLaunchDeepLink)
    XCTAssertEqual(handler.url, url)
    XCTAssertNil(handler.getInAppFeatureHighlightData())
  }

  func testColdUniversalLinkIsConvertedAndQueued() throws {
    try handler.prepareForColdLaunch(
      universalLink: XCTUnwrap(URL(string: "https://omaps.app/AbCd/Test"))
    )

    XCTAssertTrue(handler.hasPendingColdLaunchDeepLink)
    XCTAssertEqual(handler.url?.absoluteString, "om://AbCd/Test")
    XCTAssertNil(handler.getInAppFeatureHighlightData())
  }

  func testLastLinkReceivedBeforeMapIsReadyWins() throws {
    let coldURL = try XCTUnwrap(URL(string: "om://map?ll=1,2&n=First"))
    let laterURL = try XCTUnwrap(URL(string: "om://map?ll=3,4&n=Second"))
    handler.prepareForColdLaunch(url: coldURL)

    XCTAssertTrue(handler.applicationDidOpenUrl(laterURL))
    XCTAssertEqual(handler.url, laterURL)
    XCTAssertNil(handler.getInAppFeatureHighlightData())
  }

  func testUniversalLinkReceivedBeforeMapIsReadyIsNotHandledTwice() throws {
    try handler.prepareForColdLaunch(url: XCTUnwrap(URL(string: "om://map?ll=1,2&n=First")))

    let universalLink = try XCTUnwrap(URL(string: "https://omaps.app/AbCd/Test"))
    XCTAssertTrue(handler.applicationDidReceiveUniversalLink(universalLink))
    XCTAssertTrue(handler.hasPendingColdLaunchDeepLink)
    XCTAssertEqual(handler.url?.absoluteString, "om://AbCd/Test")
    XCTAssertNil(handler.getInAppFeatureHighlightData())
  }

  func testHandlingPendingLinkRetainsBackURL() throws {
    let url = try XCTUnwrap(URL(string: "om://invalid?backurl=testapp%3A%2F%2F"))
    handler.prepareForColdLaunch(url: url)

    XCTAssertFalse(handler.handlePendingDeepLink())
    XCTAssertFalse(handler.hasPendingColdLaunchDeepLink)
    XCTAssertEqual(handler.getBackUrl(), "testapp://")
    XCTAssertEqual(handler.url, url)
  }

  func testResetClearsColdLaunchState() throws {
    try handler.prepareForColdLaunch(url: XCTUnwrap(URL(string: "om://map?ll=1,2")))
    handler.reset()

    XCTAssertFalse(handler.hasPendingColdLaunchDeepLink)
    XCTAssertNil(handler.url)
    XCTAssertNil(handler.getInAppFeatureHighlightData())
  }

  func testRouteValidationIsDeliveredAfterReturningToTheMap() throws {
    let map = try prepareMapViewController()
    let navigation = try XCTUnwrap(map.navigationController)
    MWMRouter.subscribeToEvents()
    let previousRouter = MWMRouter.type()
    let routing = RoutingManager.routingManager
    let observer = RouteValidationObserver()
    var validations = 0
    observer.onEvent = { code in
      XCTAssertEqual(code, .cancelled)
      XCTAssertTrue(navigation.topViewController === map)
      validations += 1
    }
    routing.add(observer)
    defer {
      routing.remove(observer)
      MWMRouter.stopRouting()
      MWMRouter.setType(previousRouter)
      navigation.popToRootViewController(animated: false)
    }
    navigation.pushViewController(UIViewController(), animated: false)
    let url = try XCTUnwrap(URL(string: "om://route?sll=1,1&saddr=Start&dll=1,1&daddr=Finish&type=vehicle"))

    XCTAssertTrue(handler.applicationDidOpenUrl(url))

    XCTAssertTrue(navigation.topViewController === map)
    XCTAssertEqual(validations, 1)
    XCTAssertEqual(MWMNavigationDashboardManager.shared().state, .prepare)
    XCTAssertEqual(MWMRouter.points().map(\.title), ["Start", "Finish"])
  }

  func testColdSettingsHighlightSurvivesUntilPresentation() throws {
    let mapViewController = try XCTUnwrap(MapsAppDelegate.theApp().mapViewController)
    defer { mapViewController.navigationController?.popToRootViewController(animated: false) }
    let url = try XCTUnwrap(URL(string: "om://settings?highlight=icloud"))
    handler.prepareForColdLaunch(url: url)

    XCTAssertTrue(handler.handlePendingDeepLink())
    XCTAssertFalse(handler.hasPendingColdLaunchDeepLink)
    let highlight = try XCTUnwrap(handler.getInAppFeatureHighlightData())
    XCTAssertEqual(highlight.urlType, .settings)
    XCTAssertEqual(highlight.feature, .iCloud)
    XCTAssertNil(handler.getInAppFeatureHighlightData())
  }

  func testUnrelatedHotLinkLeavesNoHighlight() throws {
    let url = try XCTUnwrap(URL(string: "om://crosshair"))

    XCTAssertFalse(handler.applicationDidOpenUrl(url))
    XCTAssertNil(handler.getInAppFeatureHighlightData())
  }

  func testOpeningMapLinkWithPlacePageOpenKeepsNewPlacePageVisible() throws {
    let mapViewController = try prepareMapViewController()
    try openMapPoint(named: "First")
    waitForPlacePageAnimations()

    try openMapPoint(named: "Second")
    waitForPlacePageAnimations()

    assertSinglePlacePageIsVisible(in: mapViewController)
    let data = PlacePageData(localizationProvider: OpeinigHoursLocalization())
    XCTAssertEqual(data.previewData.title, "Second")
  }

  func testOpeningMapLinkAfterRepeatedDismissalKeepsNewPlacePageVisible() throws {
    let mapViewController = try prepareMapViewController()
    try openMapPoint(named: "First")
    waitForPlacePageAnimations()

    mapViewController.dismissPlacePage()
    mapViewController.dismissPlacePage()
    try openMapPoint(named: "Second")
    waitForPlacePageAnimations()

    assertSinglePlacePageIsVisible(in: mapViewController)
  }

  func testDismissingMapLinkPlacePageHidesIt() throws {
    let mapViewController = try prepareMapViewController()
    try openMapPoint(named: "First")
    waitForPlacePageAnimations()

    mapViewController.dismissPlacePage()
    waitForPlacePageAnimations()

    XCTAssertFalse(PlacePageData.hasData)
    XCTAssertTrue(mapViewController.placePageContainer.isHidden)
    XCTAssertTrue(placePages(in: mapViewController).isEmpty)
  }

  private func prepareMapViewController() throws -> MapViewController {
    let mapViewController = try XCTUnwrap(MapViewController.shared())
    mapViewController.loadViewIfNeeded()
    addTeardownBlock {
      mapViewController.dismissPlacePage()
      self.waitForPlacePageAnimations()
    }
    return mapViewController
  }

  private func placePages(in mapViewController: MapViewController) -> [PlacePageViewController] {
    mapViewController.children.compactMap { $0 as? PlacePageViewController }
  }

  private func assertSinglePlacePageIsVisible(in mapViewController: MapViewController,
                                              file: StaticString = #filePath,
                                              line: UInt = #line) {
    XCTAssertTrue(PlacePageData.hasData, "The API point should remain selected", file: file, line: line)
    XCTAssertFalse(mapViewController.placePageContainer.isHidden, file: file, line: line)
    let pages = placePages(in: mapViewController)
    XCTAssertEqual(pages.count, 1, file: file, line: line)
    XCTAssertNotNil(pages.first?.view.window, file: file, line: line)
    XCTAssertEqual(pages.first?.view.isUserInteractionEnabled, true, file: file, line: line)
  }

  private func openMapPoint(named name: String) throws {
    let url = try XCTUnwrap(URL(string: "om://map?ll=52.521,13.406&n=\(name)"))
    XCTAssertTrue(handler.applicationDidOpenUrl(url))
  }

  private func waitForPlacePageAnimations() {
    let settled = expectation(description: "Place Page animations finished")
    DispatchQueue.main.asyncAfter(deadline: .now() + AppConstants.defaultAnimationDuration * 2) {
      settled.fulfill()
    }
    wait(for: [settled], timeout: 2)
  }
}
