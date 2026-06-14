import WebKit

final class PlacePageUserDescriptionWebView: UIView {
  private enum Constants {
    static let previewHeight: CGFloat = 70
    static let baseURL = URL(string: "https://organicmaps.app")
  }

  /** This renderer is used only for bookmark/track user descriptions (PlacePageUserDescriptionContainerFactory),
   and a place page builds at most one WebView-backed section. There is never a second concurrent owner contending for the cache.
   */
  private static let webViewCache = PlacePageUserDescriptionWebViewCache()

  private var webView: WKWebView?
  private var ownsCachedWebView: Bool { Self.webViewCache.isOwned(by: self) }

  private var contentSizeObservation: NSKeyValueObservation?
  private var webViewConstraints = [NSLayoutConstraint]()
  private var isLoadingHTMLString = false
  private var measuredHTMLHeight: CGFloat = 0
  private var htmlString: String

  var contentView: UIView { self }
  var onContentHeightChanged: (() -> Void)?

  init(htmlString: String) {
    self.htmlString = htmlString
    super.init(frame: .zero)
    setupView()
    loadHTML(htmlString)
  }

  @available(*, unavailable)
  required init?(coder _: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  deinit {
    detachWebView()
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    guard ownsCachedWebView else { return }
    guard previousTraitCollection?.userInterfaceStyle != traitCollection.userInterfaceStyle else { return }
    loadHTML(htmlString)
  }

  private func setupView() {
    backgroundColor = .clear
    clipsToBounds = false
  }

  private func attachWebView() {
    guard !ownsCachedWebView else { return }

    let webView = Self.webViewCache.acquire(for: self)
    self.webView = webView

    webView.navigationDelegate = self
    webView.removeFromSuperview()
    addSubview(webView)
    webViewConstraints = [
      webView.topAnchor.constraint(equalTo: topAnchor),
      webView.leadingAnchor.constraint(equalTo: leadingAnchor),
      webView.trailingAnchor.constraint(equalTo: trailingAnchor),
      webView.bottomAnchor.constraint(equalTo: bottomAnchor),
    ]
    NSLayoutConstraint.activate(webViewConstraints)

    contentSizeObservation = webView.scrollView.observe(\.contentSize, options: [.new]) { [weak self] scrollView, _ in
      guard let self, self.ownsCachedWebView else { return }
      self.applyMeasuredHeight(scrollView.contentSize.height)
    }
  }

  fileprivate func detachWebView() {
    contentSizeObservation = nil
    NSLayoutConstraint.deactivate(webViewConstraints)
    webViewConstraints.removeAll()

    guard let webView else { return }
    Self.webViewCache.detach(webView, from: self)
    self.webView = nil
  }

  private func loadHTML(_ htmlString: String) {
    attachWebView()
    guard let webView else { return }
    self.htmlString = htmlString
    measuredHTMLHeight = 0
    isLoadingHTMLString = true
    let html = Self.buildHTML(with: htmlString)
    webView.loadHTMLString(html, baseURL: Constants.baseURL)
  }

  private func applyMeasuredHeight(_ height: CGFloat) {
    guard height > 0 else { return }
    let measuredHeight = ceil(height)
    guard measuredHeight != measuredHTMLHeight else { return }
    measuredHTMLHeight = measuredHeight
    onContentHeightChanged?()
  }

  func updateContentHeight() {
    guard ownsCachedWebView, let webView else { return }
    webView.evaluateJavaScript("""
      Math.ceil(Math.max(
        document.body.scrollHeight,
        document.body.offsetHeight,
        document.documentElement.scrollHeight,
        document.documentElement.offsetHeight
      ))
    """) { [weak self] result, _ in
      guard let self, self.ownsCachedWebView, let webView = self.webView else { return }
      let domHeight = CGFloat(htmlHeight: result) ?? 0
      let scrollHeight = webView.scrollView.contentSize.height
      let height = max(domHeight, scrollHeight)
      self.applyMeasuredHeight(height)
    }
  }

  private func navigationPolicy(for navigationAction: WKNavigationAction) -> WKNavigationActionPolicy {
    guard ownsCachedWebView else { return .cancel }

    if navigationAction.navigationType == .linkActivated {
      if let urlString = navigationAction.request.url?.absoluteString {
        MapViewController.shared()?.openUrl(urlString)
      }
      return .cancel
    }

    if navigationAction.targetFrame?.isMainFrame == false {
      return .allow
    }
    defer { isLoadingHTMLString = false }
    guard isLoadingHTMLString else { return .cancel }
    guard navigationAction.navigationType == .other else { return .cancel }
    return .allow
  }

  private static func buildHTML(with htmlString: String) -> String {
    if isHTMLDocument(htmlString) {
      return htmlString
    }
    // Convert fragment HTML to full document.
    let htmlBody = extractHTMLBody(from: htmlString)
    return """
      <!doctype html>
      <html>
      <head>
      <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0">
      <style>
        html, body {
          margin: 0;
          padding: 0;
          background: transparent;
        }
        body {
          color: \(UIColor.blackPrimaryText.hexString);
          font: 14px -apple-system, sans-serif;
          overflow-wrap: break-word;
        }
        img {
          max-width: 100%;
          height: auto;
        }
      </style>
      </head>
      <body>
      \(htmlBody)
      </body>
      </html>
    """
  }

  private static func isHTMLDocument(_ html: String) -> Bool {
    html.range(of: #"^\s*(?:<!doctype\s+html[^>]*>\s*)?<html\b"#,
               options: [.regularExpression, .caseInsensitive]) != nil
  }

  private static func extractHTMLBody(from html: String) -> String {
    guard let bodyStartRange = html.range(of: "<body[^>]*>", options: [.regularExpression, .caseInsensitive]),
          let bodyEndRange = html.range(of: "</body>", options: [.caseInsensitive]) else {
      return html
    }
    return String(html[bodyStartRange.upperBound ..< bodyEndRange.lowerBound])
  }
}

extension PlacePageUserDescriptionWebView: WKNavigationDelegate {
  func webView(_: WKWebView, didFinish _: WKNavigation!) {
    updateContentHeight()
  }

  func webView(_: WKWebView,
               decidePolicyFor navigationAction: WKNavigationAction,
               preferences: WKWebpagePreferences,
               decisionHandler: @escaping (WKNavigationActionPolicy, WKWebpagePreferences) -> Void) {
    preferences.allowsContentJavaScript = Settings.placePageDescriptionJavaScriptEnabled()
    decisionHandler(navigationPolicy(for: navigationAction), preferences)
  }
}

extension PlacePageUserDescriptionWebView: ExpandableTextContainer {
  func configure(with text: String) {
    guard htmlString != text else { return }
    loadHTML(text)
  }

  func expandedHeight(for _: CGFloat) -> CGFloat {
    measuredHTMLHeight
  }

  func collapsedHeight(for _: CGFloat) -> CGFloat {
    Constants.previewHeight
  }
}

private extension CGFloat {
  init?(htmlHeight value: Any?) {
    switch value {
    case let value as CGFloat:
      self = value
    case let value as Double:
      self = CGFloat(value)
    case let value as Int:
      self = CGFloat(value)
    default:
      return nil
    }
  }
}

private final class PlacePageUserDescriptionWebViewCache {
  private weak var owner: PlacePageUserDescriptionWebView?
  private var cachedWebView: WKWebView?
  private var memoryWarningObserver: NSObjectProtocol?

  init() {
    memoryWarningObserver = NotificationCenter.default.addObserver(forName: UIApplication.didReceiveMemoryWarningNotification,
                                                                   object: nil,
                                                                   queue: .main) { [weak self] _ in
      self?.cleanupIfIdle()
      self?.cachedWebView = nil
    }
  }

  deinit {
    if let memoryWarningObserver {
      NotificationCenter.default.removeObserver(memoryWarningObserver)
    }
  }

  func isOwned(by owner: PlacePageUserDescriptionWebView) -> Bool {
    self.owner === owner
  }

  func acquire(for owner: PlacePageUserDescriptionWebView) -> WKWebView {
    if isOwned(by: owner), let cachedWebView {
      return cachedWebView
    }

    self.owner?.detachWebView()

    let webView = cachedWebView ?? makeWebView()
    cachedWebView = webView
    self.owner = owner
    return webView
  }

  func detach(_ webView: WKWebView, from owner: PlacePageUserDescriptionWebView) {
    guard isOwned(by: owner) || webView.superview === owner else { return }

    webView.navigationDelegate = nil
    webView.removeFromSuperview()
    if isOwned(by: owner) {
      self.owner = nil
    }
    cleanupIfIdle()
  }

  private func cleanupIfIdle() {
    guard owner == nil, let cachedWebView else { return }

    cachedWebView.stopLoading()
    cachedWebView.evaluateJavaScript("document.body.remove()")
    cachedWebView.navigationDelegate = nil
    cachedWebView.removeFromSuperview()
  }

  private func makeWebView() -> WKWebView {
    let configuration = WKWebViewConfiguration()
    configuration.defaultWebpagePreferences.allowsContentJavaScript = Settings.placePageDescriptionJavaScriptEnabled()
    configuration.allowsInlineMediaPlayback = true
    configuration.mediaTypesRequiringUserActionForPlayback = [.video]

    let webView = WKWebView(frame: .zero, configuration: configuration)
    webView.backgroundColor = .clear
    webView.isOpaque = false
    webView.allowsLinkPreview = false
    webView.scrollView.isScrollEnabled = false
    webView.scrollView.bounces = false
    webView.scrollView.showsVerticalScrollIndicator = false
    webView.scrollView.showsHorizontalScrollIndicator = false
    webView.translatesAutoresizingMaskIntoConstraints = false
    return webView
  }
}
