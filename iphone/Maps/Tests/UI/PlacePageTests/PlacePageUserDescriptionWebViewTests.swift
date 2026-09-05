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

  func test_GivenAlternatingPooledDocuments_WhenHeightIsMeasured_ThenIgnoresPreviousDocument() {
    var retainedWebView: PlacePageUserDescriptionWebView?
    var measuredHeights: [CGFloat]

    (retainedWebView, measuredHeights) = loadHTML("<div style=\"height: 240px\"></div>") { $0 > 200 }
    XCTAssertTrue(measuredHeights.allSatisfy { $0 > 200 })
    retainedWebView = nil

    (retainedWebView, measuredHeights) = loadHTML("<div style=\"height: 20px\"></div>") { $0 < 70 }
    XCTAssertTrue(measuredHeights.allSatisfy { $0 > 0 && $0 < 70 })
    retainedWebView = nil

    (retainedWebView, measuredHeights) = loadHTML("<div style=\"height: 240px\"></div>") { $0 > 200 }
    XCTAssertTrue(measuredHeights.allSatisfy { $0 > 200 })
    withExtendedLifetime(retainedWebView) {}
  }

  func test_GivenDifferentCreationAndHostAppearances_WhenHTMLIsLoaded_ThenUsesHostAppearance() {
    for hostStyle in [UIUserInterfaceStyle.light, .dark] {
      let (_, heights) = loadHTML("Text<br>More text",
                                  creationStyle: hostStyle == .light ? .dark : .light,
                                  hostStyle: hostStyle,
                                  afterLoad: { descriptionView, _ in
                                    self.assertRenderedBodyColor(in: descriptionView, style: hostStyle)
                                  }) { $0 > 0 }
      XCTAssertFalse(heights.isEmpty)
    }
  }

  func test_GivenLoadedFragment_WhenHostAppearanceChanges_ThenUpdatesColorWithoutReloading() {
    for initialStyle in [UIUserInterfaceStyle.light, .dark] {
      let (_, heights) = loadHTML("Text<br>More text", hostStyle: initialStyle, afterLoad: { descriptionView, host in
        self.assertRenderedBodyColor(in: descriptionView, style: initialStyle)
        self.evaluateJavaScript("document.body.dataset.token = 'original'", in: descriptionView)
        let style: UIUserInterfaceStyle = initialStyle == .light ? .dark : .light
        host.overrideUserInterfaceStyle = style
        host.view.layoutIfNeeded()
        XCTAssertEqual(descriptionView.traitCollection.userInterfaceStyle, style)
        self.assertRenderedBodyColor(in: descriptionView, style: style)
        XCTAssertEqual(self.evaluateJavaScript("document.body.dataset.token", in: descriptionView) as? String, "original")
      }) { $0 > 0 }
      XCTAssertFalse(heights.isEmpty)
    }
  }

  func test_GivenHTMLLoadInFlight_WhenDescriptionChanges_ThenDisplaysLatestHTML() {
    let (_, heights) = loadHTML("First description", afterAttach: { descriptionView in
      descriptionView.configure(with: "Second description")
      descriptionView.configure(with: "Latest description")
    }, afterLoad: { descriptionView, _ in
      XCTAssertEqual(self.evaluateJavaScript("document.body.textContent.trim()", in: descriptionView) as? String,
                     "Latest description")
    }) { $0 > 0 }
    XCTAssertFalse(heights.isEmpty)
  }

  func test_GivenQueuedDescription_WhenNavigationFails_ThenLoadsLatestHTML() {
    let failingDelegate = FailingNavigationDelegate()
    let (_, heights) = loadHTML("First description", afterAttach: { descriptionView in
      let webView = descriptionView.subviews.compactMap { $0 as? WKWebView }.first!
      failingDelegate.originalDelegate = webView.navigationDelegate
      webView.navigationDelegate = failingDelegate
      descriptionView.configure(with: "Latest description")
    }, afterLoad: { descriptionView, _ in
      XCTAssertTrue(failingDelegate.didFail)
      XCTAssertEqual(self.evaluateJavaScript("document.body.textContent.trim()", in: descriptionView) as? String,
                     "Latest description")
    }) { $0 > 0 }
    XCTAssertFalse(heights.isEmpty)
  }

  func test_GivenFailedNavigation_WhenViewReattaches_ThenRetriesHTMLLoad() {
    let failingDelegate = FailingNavigationDelegate()
    let (_, heights) = loadHTML("Text", afterAttach: { descriptionView in
      let webView = descriptionView.subviews.compactMap { $0 as? WKWebView }.first!
      let container = descriptionView.superview!
      failingDelegate.originalDelegate = webView.navigationDelegate
      failingDelegate.onFailure = {
        descriptionView.removeFromSuperview()
        container.addSubview(descriptionView)
      }
      webView.navigationDelegate = failingDelegate
    }, afterLoad: { descriptionView, _ in
      XCTAssertTrue(failingDelegate.didFail)
      XCTAssertEqual(self.evaluateJavaScript("document.body.textContent.trim()", in: descriptionView) as? String, "Text")
    }) { $0 > 0 }
    XCTAssertFalse(heights.isEmpty)
  }

  func test_GivenFinishedDocument_WhenNilFailureArrives_ThenKeepsMeasuringContent() {
    _ = loadHTML("Text", afterLoad: { descriptionView, _ in
      let webView = descriptionView.subviews.compactMap { $0 as? WKWebView }.first!
      descriptionView.webView(webView, didFailProvisionalNavigation: nil, withError: NSError(domain: "Test", code: 1))
      self.changeRenderedHeight(in: descriptionView)
    }) { $0 > 0 }
  }

  func test_GivenWebContentProcessTerminates_WhenViewIsVisible_ThenReloadsCurrentHTML() {
    _ = loadHTML("Text", afterLoad: { descriptionView, _ in
      self.evaluateJavaScript("document.body.dataset.token = 'original'", in: descriptionView)
      self.changeRenderedHeight(in: descriptionView)
      let reloaded = self.expectation(description: "Original HTML height restored")
      descriptionView.onContentHeightChanged = { reloaded.fulfill() }
      defer { descriptionView.onContentHeightChanged = nil }
      let webView = descriptionView.subviews.compactMap { $0 as? WKWebView }.first!
      descriptionView.webViewWebContentProcessDidTerminate(webView)
      self.wait(for: [reloaded], timeout: 5)
      XCTAssertLessThan(descriptionView.expandedHeight(for: self.width), 70)
      XCTAssertEqual(self.evaluateJavaScript("document.body.dataset.token === undefined", in: descriptionView) as? Bool, true)
      XCTAssertEqual(self.evaluateJavaScript("document.body.textContent.trim()", in: descriptionView) as? String, "Text")
    }) { $0 > 0 }
  }

  private func changeRenderedHeight(in descriptionView: PlacePageUserDescriptionWebView) {
    let measured = expectation(description: "Changed HTML height measured")
    descriptionView.onContentHeightChanged = { measured.fulfill() }
    defer { descriptionView.onContentHeightChanged = nil }
    evaluateJavaScript("document.body.style.height = '240px'", in: descriptionView)
    wait(for: [measured], timeout: 5)
    XCTAssertEqual(descriptionView.expandedHeight(for: width), 240)
  }

  private func loadHTML(_ html: String,
                        creationStyle: UIUserInterfaceStyle = .unspecified,
                        hostStyle: UIUserInterfaceStyle = .unspecified,
                        afterAttach: (PlacePageUserDescriptionWebView) -> Void = { _ in },
                        afterLoad: (PlacePageUserDescriptionWebView, UIViewController) -> Void = { _, _ in },
                        until isExpectedHeight: @escaping (CGFloat) -> Bool) ->
    (PlacePageUserDescriptionWebView, [CGFloat]) {
    let expectation = expectation(description: "HTML height measured")
    var webView: PlacePageUserDescriptionWebView!
    UITraitCollection(userInterfaceStyle: creationStyle).performAsCurrent {
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
    let host = UIViewController()
    host.overrideUserInterfaceStyle = hostStyle
    let window = UIWindow(frame: CGRect(x: 0, y: 0, width: width, height: 640))
    window.rootViewController = host
    window.isHidden = false
    defer { window.isHidden = true }
    webView.frame = CGRect(x: 0, y: 0, width: width, height: webView.collapsedHeight(for: width))
    host.view.addSubview(webView)
    afterAttach(webView)
    webView.layoutIfNeeded()

    // WebKit's first process launch can be slow in a cold simulator.
    wait(for: [expectation], timeout: 30)
    webView.onContentHeightChanged = nil
    afterLoad(webView, host)
    return (webView, measuredHeights)
  }

  private func assertRenderedBodyColor(in descriptionView: PlacePageUserDescriptionWebView,
                                       style: UIUserInterfaceStyle,
                                       file: StaticString = #filePath,
                                       line: UInt = #line) {
    let color = UIColor.blackPrimaryText.resolvedColor(with: UITraitCollection(userInterfaceStyle: style)).hexString
    let script = """
    (async () => {
      const media = matchMedia('(prefers-color-scheme: dark)');
      if (media.matches !== \(style == .dark)) {
        await new Promise(resolve => media.addEventListener('change', resolve, { once: true }));
      }
      const expected = document.createElement('span');
      expected.style.color = '\(color)';
      return [getComputedStyle(document.body).color, expected.style.color];
    })()
    """
    let colors = evaluateJavaScript(script, in: descriptionView, file: file, line: line) as? [String]
    XCTAssertEqual(colors?.count, 2, file: file, line: line)
    XCTAssertEqual(colors?.first, colors?.last, file: file, line: line)
  }

  @discardableResult
  private func evaluateJavaScript(_ script: String,
                                  in descriptionView: PlacePageUserDescriptionWebView,
                                  file: StaticString = #filePath,
                                  line: UInt = #line) -> Any? {
    guard let webView = descriptionView.subviews.compactMap({ $0 as? WKWebView }).first else {
      XCTFail("Missing embedded WKWebView", file: file, line: line)
      return nil
    }
    let expectation = expectation(description: "JavaScript evaluated")
    var value: Any?
    webView.callAsyncJavaScript("return \(script)", in: nil, in: .page) { result in
      switch result {
      case .success(let result): value = result
      case .failure(let error): XCTFail("JavaScript failed: \(error)", file: file, line: line)
      }
      expectation.fulfill()
    }
    wait(for: [expectation], timeout: 5)
    return value
  }
}

private final class FailingNavigationDelegate: NSObject, WKNavigationDelegate {
  weak var originalDelegate: WKNavigationDelegate?
  var didFail = false
  var onFailure: (() -> Void)?

  func webView(_ webView: WKWebView,
               decidePolicyFor _: WKNavigationAction,
               preferences: WKWebpagePreferences,
               decisionHandler: @escaping (WKNavigationActionPolicy, WKWebpagePreferences) -> Void) {
    didFail = true
    webView.navigationDelegate = originalDelegate
    decisionHandler(.cancel, preferences)
    // Cancelling the policy alone does not reliably deliver a failure callback.
    originalDelegate?.webView?(webView, didFailProvisionalNavigation: nil, withError: NSError(domain: "Test", code: 1))
    onFailure?()
  }
}
