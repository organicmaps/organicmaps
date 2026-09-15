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
    XCTAssertTrue(handler.isLaunchedByDeepLink)
    XCTAssertEqual(handler.url, url)
  }

  func testColdUniversalLinkIsConvertedAndQueued() throws {
    try handler.prepareForColdLaunch(
      universalLink: XCTUnwrap(URL(string: "https://omaps.app/AbCd/Test"))
    )

    XCTAssertTrue(handler.hasPendingColdLaunchDeepLink)
    XCTAssertTrue(handler.isLaunchedByDeepLink)
    XCTAssertEqual(handler.url?.absoluteString, "om://AbCd/Test")
  }

  func testLastLinkReceivedBeforeMapIsReadyWins() throws {
    let coldURL = try XCTUnwrap(URL(string: "om://map?ll=1,2&n=First"))
    let laterURL = try XCTUnwrap(URL(string: "om://map?ll=3,4&n=Second"))
    handler.prepareForColdLaunch(url: coldURL)

    XCTAssertTrue(handler.applicationDidOpenUrl(laterURL))
    XCTAssertEqual(handler.url, laterURL)
    XCTAssertTrue(handler.isLaunchedByDeepLink)
  }

  func testUniversalLinkReceivedBeforeMapIsReadyIsNotHandledTwice() throws {
    try handler.prepareForColdLaunch(url: XCTUnwrap(URL(string: "om://map?ll=1,2&n=First")))

    let universalLink = try XCTUnwrap(URL(string: "https://omaps.app/AbCd/Test"))
    XCTAssertTrue(handler.applicationDidReceiveUniversalLink(universalLink))
    XCTAssertTrue(handler.hasPendingColdLaunchDeepLink)
    XCTAssertTrue(handler.isLaunchedByDeepLink)
    XCTAssertEqual(handler.url?.absoluteString, "om://AbCd/Test")
  }

  func testResetClearsColdLaunchState() throws {
    try handler.prepareForColdLaunch(url: XCTUnwrap(URL(string: "om://map?ll=1,2")))
    handler.reset()

    XCTAssertFalse(handler.hasPendingColdLaunchDeepLink)
    XCTAssertFalse(handler.isLaunchedByDeepLink)
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
}
