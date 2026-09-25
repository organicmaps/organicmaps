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
}
