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
  }

  func testColdUniversalLinkIsConvertedAndQueued() throws {
    try handler.prepareForColdLaunch(
      universalLink: XCTUnwrap(URL(string: "https://omaps.app/AbCd/Test"))
    )

    XCTAssertTrue(handler.hasPendingColdLaunchDeepLink)
    XCTAssertEqual(handler.url?.absoluteString, "om://AbCd/Test")
  }

  func testLastLinkReceivedBeforeMapIsReadyWins() throws {
    let coldURL = try XCTUnwrap(URL(string: "om://map?ll=1,2&n=First"))
    let laterURL = try XCTUnwrap(URL(string: "om://map?ll=3,4&n=Second"))
    handler.prepareForColdLaunch(url: coldURL)

    XCTAssertTrue(handler.applicationDidOpenUrl(laterURL))
    XCTAssertEqual(handler.url, laterURL)
  }

  func testUniversalLinkReceivedBeforeMapIsReadyIsNotHandledTwice() throws {
    try handler.prepareForColdLaunch(url: XCTUnwrap(URL(string: "om://map?ll=1,2&n=First")))

    let universalLink = try XCTUnwrap(URL(string: "https://omaps.app/AbCd/Test"))
    XCTAssertTrue(handler.applicationDidReceiveUniversalLink(universalLink))
    XCTAssertTrue(handler.hasPendingColdLaunchDeepLink)
    XCTAssertEqual(handler.url?.absoluteString, "om://AbCd/Test")
  }

  func testResetClearsColdLaunchState() throws {
    try handler.prepareForColdLaunch(url: XCTUnwrap(URL(string: "om://map?ll=1,2")))
    handler.reset()

    XCTAssertFalse(handler.hasPendingColdLaunchDeepLink)
    XCTAssertNil(handler.url)
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
}
