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

  func test_GivenDifferentCreationAndHostAppearances_WhenHTMLIsLoaded_ThenUsesHostAppearance() {
    let lightTraits = UITraitCollection(userInterfaceStyle: .light)
    let (lightWebView, lightHeights) = loadHTML("<div style=\"height: 20px\"></div>",
                                                creationStyle: .dark,
                                                hostStyle: .light) { $0 < 70 }
    XCTAssertFalse(lightHeights.isEmpty)
    XCTAssertTrue(lightHeights.allSatisfy { $0 > 0 && $0 < 70 })
    assertRenderedBodyColor(in: lightWebView, compatibleWith: lightTraits)

    let darkTraits = UITraitCollection(userInterfaceStyle: .dark)
    let (darkWebView, darkHeights) = loadHTML("<div style=\"height: 20px\"></div>",
                                              creationStyle: .light,
                                              hostStyle: .dark) { $0 < 70 }
    XCTAssertFalse(darkHeights.isEmpty)
    XCTAssertTrue(darkHeights.allSatisfy { $0 > 0 && $0 < 70 })
    assertRenderedBodyColor(in: darkWebView, compatibleWith: darkTraits)
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
    XCTAssertTrue(lightHTML.contains("--om-text-color: \(lightColor)"))
    XCTAssertTrue(darkHTML.contains("--om-text-color: \(darkColor)"))
  }

  func test_GivenFullDocument_WhenBuilt_ThenEnablesAdaptiveColorSchemeBeforeDocumentStyles() throws {
    let authorStyle = "<style>body { color: red; }</style>"
    let html = "<!doctype html><html><head>\(authorStyle)</head><body>Text</body></html>"

    let builtHTML = UserDescriptionHTMLDocumentBuilder().buildHTML(
      with: html,
      compatibleWith: UITraitCollection(userInterfaceStyle: .dark)
    )

    let colorSchemeRange = try XCTUnwrap(builtHTML.range(of: ":root { color-scheme: light dark; }"))
    let authorStyleRange = try XCTUnwrap(builtHTML.range(of: authorStyle))
    XCTAssertLessThan(colorSchemeRange.lowerBound, authorStyleRange.lowerBound)
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

  func test_GivenLoadedFragment_WhenAppearanceChanges_ThenUpdatesColorWithoutReloading() {
    var loadCount = 0
    let descriptionView = PlacePageUserDescriptionWebView(htmlString: "Text") { webView, html, baseURL in
      loadCount += 1
      return webView.loadHTMLString(html, baseURL: baseURL)
    }
    let loadExpectation = expectation(description: "Initial HTML height measured")
    var didLoad = false
    descriptionView.onContentHeightChanged = {
      guard !didLoad else { return }
      didLoad = true
      loadExpectation.fulfill()
    }
    let viewController = UIViewController()
    viewController.overrideUserInterfaceStyle = .light
    let window = UIWindow(frame: CGRect(x: 0, y: 0, width: width, height: 640))
    window.rootViewController = viewController
    window.isHidden = false
    descriptionView.frame = CGRect(x: 0, y: 0, width: width, height: descriptionView.collapsedHeight(for: width))
    viewController.view.addSubview(descriptionView)
    wait(for: [loadExpectation], timeout: 5)

    XCTAssertEqual(loadCount, 1)
    assertRenderedBodyColor(in: descriptionView, compatibleWith: UITraitCollection(userInterfaceStyle: .light))

    let darkTraits = UITraitCollection(userInterfaceStyle: .dark)
    descriptionView.applyAppearance(compatibleWith: darkTraits)
    assertRenderedBodyColor(in: descriptionView, compatibleWith: darkTraits)
    XCTAssertEqual(loadCount, 1)

    descriptionView.onContentHeightChanged = nil
    withExtendedLifetime(window) {}
    window.isHidden = true
  }

  func test_GivenWebContentProcessTerminates_WhenViewIsVisible_ThenReloadsCurrentHTML() throws {
    var loadedHTML = [String]()
    let currentHTML = "<div style=\"height: 20px\">Current</div>"
    let descriptionView = PlacePageUserDescriptionWebView(htmlString: currentHTML) {
      webView, html, baseURL in
      loadedHTML.append(html)
      return webView.loadHTMLString(html, baseURL: baseURL)
    }
    let viewController = UIViewController()
    let window = UIWindow(frame: CGRect(x: 0, y: 0, width: width, height: 640))
    window.rootViewController = viewController
    window.isHidden = false
    descriptionView.frame = CGRect(x: 0, y: 0, width: width, height: descriptionView.collapsedHeight(for: width))
    viewController.view.addSubview(descriptionView)
    XCTAssertEqual(loadedHTML.count, 1)

    let webView = try XCTUnwrap(embeddedWebView(in: descriptionView))
    descriptionView.webViewWebContentProcessDidTerminate(webView)
    XCTAssertEqual(loadedHTML.count, 2)
    XCTAssertEqual(loadedHTML.last?.contains(currentHTML), true)

    withExtendedLifetime(window) {}
    window.isHidden = true
  }

  func test_GivenNilFailureNavigation_WhenViewReattaches_ThenRetriesHTMLLoad() throws {
    var loadCount = 0
    let descriptionView = PlacePageUserDescriptionWebView(htmlString: "Text") { webView, html, baseURL in
      loadCount += 1
      return webView.loadHTMLString(html, baseURL: baseURL)
    }
    let viewController = UIViewController()
    let window = UIWindow(frame: CGRect(x: 0, y: 0, width: width, height: 640))
    window.rootViewController = viewController
    window.isHidden = false
    viewController.view.addSubview(descriptionView)
    XCTAssertEqual(loadCount, 1)

    let webView = try XCTUnwrap(embeddedWebView(in: descriptionView))
    descriptionView.webView(webView, didFail: nil, withError: NSError(domain: "Test", code: 1))
    XCTAssertEqual(loadCount, 1)

    descriptionView.removeFromSuperview()
    viewController.view.addSubview(descriptionView)
    XCTAssertEqual(loadCount, 2)

    withExtendedLifetime(window) {}
    window.isHidden = true
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

  private func assertRenderedBodyColor(in descriptionView: PlacePageUserDescriptionWebView,
                                       compatibleWith traitCollection: UITraitCollection,
                                       file: StaticString = #filePath,
                                       line: UInt = #line) {
    let expectation = expectation(description: "Rendered body color read")
    guard let webView = embeddedWebView(in: descriptionView) else {
      XCTFail("Missing embedded WKWebView", file: file, line: line)
      return
    }

    var red: CGFloat = 0
    var green: CGFloat = 0
    var blue: CGFloat = 0
    var alpha: CGFloat = 0
    let color = UIColor.blackPrimaryText.resolvedColor(with: traitCollection).sRGBColor
    XCTAssertTrue(color.getRed(&red, green: &green, blue: &blue, alpha: &alpha), file: file, line: line)

    let script = "getComputedStyle(document.body).color.match(/[\\d.]+/g).map(Number)"
    webView.evaluateJavaScript(script) { result, error in
      defer { expectation.fulfill() }
      XCTAssertNil(error, file: file, line: line)
      guard let components = result as? [NSNumber], components.count == 4 else {
        XCTFail("Unexpected computed color: \(String(describing: result))", file: file, line: line)
        return
      }
      XCTAssertEqual(components[0].doubleValue / 255, Double(red), accuracy: 1 / 255, file: file, line: line)
      XCTAssertEqual(components[1].doubleValue / 255, Double(green), accuracy: 1 / 255, file: file, line: line)
      XCTAssertEqual(components[2].doubleValue / 255, Double(blue), accuracy: 1 / 255, file: file, line: line)
      XCTAssertEqual(components[3].doubleValue, Double(alpha), accuracy: 1 / 255, file: file, line: line)
    }
    wait(for: [expectation], timeout: 5)
  }

  private func embeddedWebView(in descriptionView: PlacePageUserDescriptionWebView?) -> WKWebView? {
    descriptionView?.subviews.compactMap { $0 as? WKWebView }.first
  }
}
