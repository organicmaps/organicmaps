@testable import Organic_Maps__Debug_
import XCTest

final class PlacePageUserDescriptionWebViewTests: XCTestCase {
  private let width: CGFloat = 320

  func test_GivenUnmeasuredHTML_WhenHeightIsRequested_ThenReturnsNonZeroCollapsedHeight() {
    let webView = PlacePageUserDescriptionWebView(htmlString: "<b>HTML description</b>")

    let expandedHeight = webView.expandedHeight(for: width)
    XCTAssertGreaterThan(expandedHeight, 0)
    XCTAssertEqual(expandedHeight, webView.collapsedHeight(for: width))
  }

  func test_GivenFullDocumentWithoutViewport_WhenHeightIsMeasured_ThenUsesRenderedScale() {
    let html = """
      <!doctype html>
      <html>
      <head><style>html, body { margin: 0; padding: 0; }</style></head>
      <body><div style="height: 100px"></div></body>
      </html>
    """

    let (_, measuredHeights) = loadHTML(html) { $0 < 70 }

    XCTAssertTrue(measuredHeights.allSatisfy { $0 > 0 && $0 < 70 }, "Measured heights: \(measuredHeights)")
  }

  func test_GivenDifferentCreationAndHostAppearances_WhenHTMLIsLoaded_ThenMeasuresContent() {
    let (_, lightHeights) = loadHTML("<div style=\"height: 20px\"></div>",
                                     creationStyle: .dark,
                                     hostStyle: .light) { $0 < 70 }
    XCTAssertTrue(lightHeights.allSatisfy { $0 > 0 && $0 < 70 })

    let (_, darkHeights) = loadHTML("<div style=\"height: 20px\"></div>",
                                    creationStyle: .light,
                                    hostStyle: .dark) { $0 < 70 }
    XCTAssertTrue(darkHeights.allSatisfy { $0 > 0 && $0 < 70 })
  }

  func test_GivenExplicitAppearance_WhenFragmentIsBuilt_ThenUsesMatchingTextColor() {
    let builder = UserDescriptionHTMLDocumentBuilder()
    let lightTraits = UITraitCollection(userInterfaceStyle: .light)
    let darkTraits = UITraitCollection(userInterfaceStyle: .dark)

    let lightHTML = builder.buildHTML(with: "Text", compatibleWith: lightTraits)
    let darkHTML = builder.buildHTML(with: "Text", compatibleWith: darkTraits)
    let lightColor = UIColor.blackPrimaryText.resolvedColor(with: lightTraits).hexString
    let darkColor = UIColor.blackPrimaryText.resolvedColor(with: darkTraits).hexString

    XCTAssertNotEqual(lightColor, darkColor)
    XCTAssertTrue(lightHTML.contains("color: \(lightColor)"))
    XCTAssertTrue(darkHTML.contains("color: \(darkColor)"))
  }

  func test_GivenAlternatingPooledDocuments_WhenHeightIsMeasured_ThenIgnoresPreviousDocument() {
    var retainedWebView: PlacePageUserDescriptionWebView?
    var measuredHeights: [CGFloat]

    (retainedWebView, measuredHeights) = loadHTML("<div style=\"height: 240px\"></div>",
                                                  creationStyle: .light,
                                                  hostStyle: .dark) { $0 > 200 }
    XCTAssertTrue(measuredHeights.allSatisfy { $0 > 200 })
    retainedWebView = nil

    (retainedWebView, measuredHeights) = loadHTML("<div style=\"height: 20px\"></div>",
                                                  creationStyle: .dark,
                                                  hostStyle: .light) { $0 < 70 }
    XCTAssertTrue(measuredHeights.allSatisfy { $0 > 0 && $0 < 70 })
    retainedWebView = nil

    (retainedWebView, measuredHeights) = loadHTML("<div style=\"height: 240px\"></div>",
                                                  creationStyle: .light,
                                                  hostStyle: .dark) { $0 > 200 }
    XCTAssertTrue(measuredHeights.allSatisfy { $0 > 200 })
    withExtendedLifetime(retainedWebView) {}
  }

  private func loadHTML(_ html: String,
                        creationStyle: UIUserInterfaceStyle = .unspecified,
                        hostStyle: UIUserInterfaceStyle = .unspecified,
                        until isExpectedHeight: @escaping (CGFloat) -> Bool) ->
    (PlacePageUserDescriptionWebView, [CGFloat]) {
    let expectation = expectation(description: "HTML height measured")
    let creationTraits = UITraitCollection(userInterfaceStyle: creationStyle)
    var webView: PlacePageUserDescriptionWebView!
    creationTraits.performAsCurrent {
      webView = PlacePageUserDescriptionWebView(htmlString: html)
    }
    var measuredHeights = [CGFloat]()
    var isFulfilled = false
    webView.onContentHeightChanged = { [weak webView] in
      guard let webView else { return }
      let height = webView.expandedHeight(for: self.width)
      measuredHeights.append(height)
      guard !isFulfilled, isExpectedHeight(height) else { return }
      isFulfilled = true
      expectation.fulfill()
    }
    let viewController = UIViewController()
    viewController.overrideUserInterfaceStyle = hostStyle
    let window = UIWindow(frame: CGRect(x: 0, y: 0, width: width, height: 640))
    window.rootViewController = viewController
    window.isHidden = false
    webView.frame = CGRect(x: 0, y: 0, width: width, height: webView.collapsedHeight(for: width))
    viewController.view.addSubview(webView)
    webView.layoutIfNeeded()

    withExtendedLifetime(window) {
      wait(for: [expectation], timeout: 5)
    }
    webView.onContentHeightChanged = nil
    window.isHidden = true
    return (webView, measuredHeights)
  }
}
