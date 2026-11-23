@testable import Organic_Maps__Debug_
import XCTest

final class PlacePageUserDescriptionWebViewTests: XCTestCase {
  private let width: CGFloat = 320
  private var window: UIWindow!
  private var host: UIViewController { window.rootViewController! }

  override func setUpWithError() throws {
    try super.setUpWithError()
    let scene = try XCTUnwrap(MapsAppDelegate.theApp().window?.windowScene)
    window = UIWindow(windowScene: scene)
    window.rootViewController = UIViewController()
    window.isHidden = false
  }

  override func tearDown() {
    window?.isHidden = true
    window = nil
    super.tearDown()
  }

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

    let measuredHeights = loadHTML(html) { $0 < 70 }

    XCTAssertTrue(measuredHeights.allSatisfy { $0 > 0 && $0 < 70 }, "Measured heights: \(measuredHeights)")
  }

  func test_GivenAlternatingPooledDocuments_WhenHeightIsMeasured_ThenIgnoresPreviousDocument() {
    var firstWebView: WKWebView?
    for height in [240, 20, 240] {
      let expectedHeight: (CGFloat) -> Bool = { height > 70 ? $0 > 200 : $0 > 0 && $0 < 70 }
      let heights = loadHTML("<div style=\"height: \(height)px\"></div>", afterAttach: { description in
        let embedded = description.subviews.compactMap { $0 as? WKWebView }.first!
        if let firstWebView {
          XCTAssertIdentical(embedded, firstWebView)
        } else {
          firstWebView = embedded
        }
      }, until: expectedHeight)
      XCTAssertTrue(heights.allSatisfy(expectedHeight), "Measured heights: \(heights)")
    }
  }

  func test_GivenDifferentCreationAndHostAppearances_WhenHTMLIsLoaded_ThenUsesHostAppearance() {
    for hostStyle in [UIUserInterfaceStyle.light, .dark] {
      withLoadedFragment(creationStyle: hostStyle == .light ? .dark : .light, hostStyle: hostStyle) { description in
        self.assertRenderedBodyColor(in: description, style: hostStyle)
      }
    }
  }

  func test_GivenLoadedFragment_WhenHostAppearanceChanges_ThenUpdatesColorWithoutReloading() {
    for initialStyle in [UIUserInterfaceStyle.light, .dark] {
      withLoadedFragment(hostStyle: initialStyle) { description in
        self.assertRenderedBodyColor(in: description, style: initialStyle)
        self.evaluateJavaScript("document.body.dataset.token = 'original'", in: description)
        let style: UIUserInterfaceStyle = initialStyle == .light ? .dark : .light
        self.host.overrideUserInterfaceStyle = style
        self.host.view.layoutIfNeeded()
        XCTAssertEqual(description.traitCollection.userInterfaceStyle, style)
        self.assertRenderedBodyColor(in: description, style: style)
        XCTAssertEqual(self.evaluateJavaScript("document.body.dataset.token", in: description) as? String, "original")
      }
    }
  }

  func test_GivenHTMLLoadInFlight_WhenDescriptionChanges_ThenDisplaysLatestHTML() {
    loadHTML("First description", afterAttach: { descriptionView in
      descriptionView.configure(with: "Second description")
      descriptionView.configure(with: "Latest description")
    }, afterLoad: { descriptionView in
      XCTAssertEqual(self.evaluateJavaScript("document.body.textContent.trim()", in: descriptionView) as? String,
                     "Latest description")
    }) { $0 > 0 }
  }

  func test_GivenQueuedDescription_WhenNavigationFails_ThenLoadsLatestHTML() {
    let failingDelegate = FailingNavigationDelegate()
    loadHTML("First description", afterAttach: { descriptionView in
      let webView = descriptionView.subviews.compactMap { $0 as? WKWebView }.first!
      failingDelegate.originalDelegate = webView.navigationDelegate
      webView.navigationDelegate = failingDelegate
      descriptionView.configure(with: "Latest description")
    }, afterLoad: { descriptionView in
      XCTAssertTrue(failingDelegate.didFail)
      XCTAssertEqual(self.evaluateJavaScript("document.body.textContent.trim()", in: descriptionView) as? String,
                     "Latest description")
    }) { $0 > 0 }
  }

  func test_GivenFailedNavigation_WhenViewReattaches_ThenRetriesHTMLLoad() {
    let failingDelegate = FailingNavigationDelegate()
    loadHTML("Text", afterAttach: { descriptionView in
      let webView = descriptionView.subviews.compactMap { $0 as? WKWebView }.first!
      let container = descriptionView.superview!
      failingDelegate.originalDelegate = webView.navigationDelegate
      failingDelegate.onFailure = {
        descriptionView.removeFromSuperview()
        container.addSubview(descriptionView)
      }
      webView.navigationDelegate = failingDelegate
    }, afterLoad: { descriptionView in
      XCTAssertTrue(failingDelegate.didFail)
      XCTAssertEqual(self.evaluateJavaScript("document.body.textContent.trim()", in: descriptionView) as? String, "Text")
    }) { $0 > 0 }
  }

  func test_GivenFinishedDocument_WhenNilFailureArrives_ThenKeepsMeasuringContent() {
    loadHTML("Text", afterLoad: { descriptionView in
      let webView = descriptionView.subviews.compactMap { $0 as? WKWebView }.first!
      descriptionView.webView(webView, didFailProvisionalNavigation: nil, withError: NSError(domain: "Test", code: 1))
      self.changeRenderedHeight(in: descriptionView)
    }) { $0 > 0 }
  }

  func test_GivenWebContentProcessTerminates_WhenViewIsVisible_ThenReloadsCurrentHTML() {
    loadHTML("Text", afterLoad: { descriptionView in
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

  private func attach(_ description: PlacePageUserDescriptionWebView) {
    description.frame = CGRect(x: 0, y: 0, width: width, height: description.collapsedHeight(for: width))
    host.view.addSubview(description)
  }

  @discardableResult
  private func loadHTML(_ html: String,
                        afterAttach: (PlacePageUserDescriptionWebView) -> Void = { _ in },
                        afterLoad: (PlacePageUserDescriptionWebView) -> Void = { _ in },
                        until isExpectedHeight: @escaping (CGFloat) -> Bool) -> [CGFloat] {
    // Drain UIKit's temporary references so the next document can take the same WKWebView from the pool.
    autoreleasepool {
      let measured = XCTestExpectation(description: "HTML height measured")
      let description = PlacePageUserDescriptionWebView(htmlString: html)
      var heights = [CGFloat]()
      var fulfilled = false
      description.onContentHeightChanged = { [weak description] in
        guard let description else { return }
        let height = description.expandedHeight(for: self.width)
        heights.append(height)
        if !fulfilled, isExpectedHeight(height) {
          fulfilled = true
          measured.fulfill()
        }
      }
      attach(description)
      defer { description.removeFromSuperview() }
      afterAttach(description)
      description.layoutIfNeeded()
      guard XCTWaiter.wait(for: [measured], timeout: 30) == .completed else {
        XCTFail("Expected HTML height was never measured. Received: \(heights)")
        return heights
      }
      description.onContentHeightChanged = nil
      afterLoad(description)
      return heights
    }
  }

  private func withLoadedFragment(creationStyle: UIUserInterfaceStyle = .unspecified,
                                  hostStyle: UIUserInterfaceStyle,
                                  check: (PlacePageUserDescriptionWebView) -> Void) {
    autoreleasepool {
      let marker = UUID().uuidString
      var description: PlacePageUserDescriptionWebView!
      UITraitCollection(userInterfaceStyle: creationStyle).performAsCurrent {
        description = PlacePageUserDescriptionWebView(htmlString: "<span id='\(marker)'>Text</span><br>More text")
      }
      host.overrideUserInterfaceStyle = hostStyle
      attach(description)
      defer { description.removeFromSuperview() }
      description.layoutIfNeeded()
      let embedded = description.subviews.compactMap { $0 as? WKWebView }.first!
      if embedded.isLoading {
        let finished = XCTKVOExpectation(keyPath: #keyPath(WKWebView.isLoading), object: embedded, expectedValue: false)
        guard XCTWaiter.wait(for: [finished], timeout: 30) == .completed else {
          XCTFail("Fragment navigation did not finish")
          return
        }
      }
      // A pooled view may still contain an earlier document.
      XCTAssertEqual(evaluateJavaScript("document.body.firstElementChild.id", in: description) as? String, marker)
      check(description)
    }
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
