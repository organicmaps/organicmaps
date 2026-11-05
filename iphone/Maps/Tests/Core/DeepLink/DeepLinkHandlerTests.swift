@testable import Organic_Maps__Debug_
import XCTest

final class DeepLinkHandlerTests: XCTestCase {
  private let handler = DeepLinkHandler.shared

  override func setUp() {
    super.setUp()
    handler.reset()
  }

  override func tearDown() {
    handler.reset()
    FrameworkHelper.clearParsedBackUrl()
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
    _ = try prepareMapViewController()
    let url = try XCTUnwrap(URL(string: "om://map?ll=1,2&backurl=testapp%3A%2F%2F"))
    handler.prepareForColdLaunch(url: url)

    XCTAssertTrue(handler.handlePendingDeepLink())
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

  func testHighlightLookupDoesNotReparseRouteOrRearmReturn() throws {
    let url = try XCTUnwrap(URL(string: "om://v2/dir?destination=1,2&callback=app%3A%2F%2Fback"))
    handler.prepareForColdLaunch(url: url)
    XCTAssertEqual(DeepLinkParser.parseAndSetApiURL(url), .route)
    FrameworkHelper.clearParsedBackUrl()

    XCTAssertNil(handler.getInAppFeatureHighlightData())
    XCTAssertNil(handler.getBackUrl())
  }

  func testCallbackNormalizationPreservesNestedEscapes() throws {
    let url = try XCTUnwrap(MWMRouter.callbackURL(from: "app://done?next=a%2Fb|c&progress=100%"))
    XCTAssertEqual(url.absoluteString, "app://done?next=a%2Fb%7Cc&progress=100%25")
  }

  func testCallbackNormalizationRejectsMissingText() {
    XCTAssertNil(MWMRouter.callbackURL(from: nil))
    XCTAssertNil(MWMRouter.callbackURL(from: ""))
  }

  func testLegacyRouteKeepsItsReturnURL() throws {
    let url = try XCTUnwrap(URL(string: "om://route?sll=1,1&saddr=A&dll=2,2&daddr=B&type=vehicle&backurl=testapp%3A%2F%2Fback"))
    XCTAssertEqual(DeepLinkParser.parseAndSetApiURL(url), .route)
    XCTAssertEqual(handler.getBackUrl(), "testapp://back")
  }

  func testReturnCompletionDoesNotConsumeANewerRequestsCallback() throws {
    // Identical links are separate requests too, even when both use the same callback URL.
    for callback in ["app%3A%2F%2Fnew", "app%3A%2F%2Fold"] {
      try withDelayedURLOpen {
        let first = try XCTUnwrap(URL(string: "om://v2/dir?destination=1,2&callback=app%3A%2F%2Fold"))
        XCTAssertEqual(DeepLinkParser.parseAndSetApiURL(first), .route)
        try XCTUnwrap(MapViewController.shared()).goBack()
        let completion = try XCTUnwrap(URLOpenMock.completion)

        let second = try XCTUnwrap(URL(string: "om://v2/dir?destination=1,2&callback=\(callback)"))
        XCTAssertEqual(DeepLinkParser.parseAndSetApiURL(second), .route)
        let expected = try XCTUnwrap(handler.getBackUrl())
        completion(true)

        XCTAssertEqual(handler.getBackUrl(), expected)
      }
    }
  }

  func testReturnURLIsConsumedOnlyOnSuccessfulLaunch() throws {
    for success in [false, true] {
      try withDelayedURLOpen {
        let url = try XCTUnwrap(URL(string: "om://v2/dir?destination=1,2&callback=app%3A%2F%2Fback"))
        XCTAssertEqual(DeepLinkParser.parseAndSetApiURL(url), .route)
        try XCTUnwrap(MapViewController.shared()).goBack()
        let completion = try XCTUnwrap(URLOpenMock.completion)
        completion(success)

        XCTAssertEqual(handler.getBackUrl(), success ? nil : "app://back")
      }
    }
  }

  private func withDelayedURLOpen(_ body: () throws -> Void) throws {
    let original = try XCTUnwrap(class_getInstanceMethod(UIApplication.self, #selector(UIApplication.open(_:options:completionHandler:))))
    let replacement = try XCTUnwrap(class_getInstanceMethod(URLOpenMock.self, #selector(URLOpenMock.open(_:options:completionHandler:))))
    method_exchangeImplementations(original, replacement)
    defer {
      method_exchangeImplementations(original, replacement)
      URLOpenMock.completion = nil
    }
    try body()
  }

  private final class URLOpenMock: NSObject {
    static var completion: ((Bool) -> Void)?

    @objc func open(_: URL, options _: [UIApplication.OpenExternalURLOptionsKey: Any], completionHandler: ((Bool) -> Void)?) {
      URLOpenMock.completion = completionHandler
    }
  }
}
