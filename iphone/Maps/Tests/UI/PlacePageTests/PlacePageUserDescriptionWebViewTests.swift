@testable import Organic_Maps__Debug_
import XCTest

final class PlacePageUserDescriptionWebViewTests: XCTestCase {
  func test_GivenUnmeasuredHTML_WhenExpandedHeightIsRequested_ThenReturnsPreviewHeight() {
    let webView = PlacePageUserDescriptionWebView(htmlString: "<b>HTML description</b>")

    XCTAssertEqual(webView.expandedHeight(for: 320), 70)
  }
}
