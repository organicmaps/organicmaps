import WebKit

final class PlacePageUserDescriptionWebView: UIView {
  private enum Constants {
    static let previewHeight: CGFloat = 70
    static let baseURL = URL(string: "https://organicmaps.app")
  }

  private static let webViewPool = WebViewPool()
  private static let htmlDocumentBuilder = UserDescriptionHTMLDocumentBuilder()

  private var webView: WKWebView?

  private var contentSizeObservation: NSKeyValueObservation?
  private var webViewConstraints = [NSLayoutConstraint]()
  private var isLoadingHTMLString = false
  private var measuredHTMLHeight: CGFloat = 0
  private var htmlString: String
  private var networkPolicy: NetworkPolicy

  var contentView: UIView { self }
  var onContentHeightChanged: (() -> Void)?

  init(networkPolicy: NetworkPolicy = .shared(), htmlString: String) {
    self.networkPolicy = networkPolicy
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
    guard webView != nil else { return }
    let userInterfaceStyleChanged = previousTraitCollection?.userInterfaceStyle != traitCollection.userInterfaceStyle
    let contentSizeCategoryChanged = previousTraitCollection?.preferredContentSizeCategory !=
      traitCollection.preferredContentSizeCategory
    guard userInterfaceStyleChanged || contentSizeCategoryChanged else { return }
    loadHTML(htmlString)
  }

  private func setupView() {
    backgroundColor = .clear
    clipsToBounds = false
  }

  private func attachWebView() {
    guard webView == nil else { return }

    let webView = Self.webViewPool.getWebView()
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
      guard let self, self.webView === webView else { return }
      self.applyMeasuredHeight(scrollView.contentSize.height)
    }
  }

  private func detachWebView() {
    contentSizeObservation = nil
    NSLayoutConstraint.deactivate(webViewConstraints)
    webViewConstraints.removeAll()

    guard let webView else { return }
    self.webView = nil
    Self.webViewPool.release(webView)
  }

  private func loadHTML(_ htmlString: String) {
    attachWebView()
    guard let webView else { return }
    self.htmlString = htmlString
    measuredHTMLHeight = 0
    isLoadingHTMLString = true
    let html = Self.htmlDocumentBuilder.buildHTML(with: htmlString)
    webView.loadHTMLString(html, baseURL: Constants.baseURL)
  }

  private func applyMeasuredHeight(_ height: CGFloat) {
    guard height > 0 else { return }
    let measuredHeight = ceil(height)
    guard measuredHeight != measuredHTMLHeight else { return }
    measuredHTMLHeight = measuredHeight
    onContentHeightChanged?()
  }

  private func navigationPolicy(for navigationAction: WKNavigationAction) -> WKNavigationActionPolicy {
    guard webView != nil else { return .cancel }

    if navigationAction.navigationType == .linkActivated, let url = navigationAction.request.url {
      MapViewController.shared()?.openUrl(url.absoluteString, externally: !url.isHTTPOrHTTPS)
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

  private func navigationNeedsNetworkPermission(_ navigationAction: WKNavigationAction) -> Bool {
    guard navigationAction.navigationType != .linkActivated else { return false }
    guard let url = navigationAction.request.url else { return false }
    if isLoadingHTMLString, navigationAction.targetFrame?.isMainFrame != false, navigationAction.navigationType == .other {
      return false
    }
    return url.isHTTPOrHTTPS
  }
}

// MARK: - WKNavigationDelegate

extension PlacePageUserDescriptionWebView: WKNavigationDelegate {
  func webView(_ webView: WKWebView, didFinish _: WKNavigation!) {
    guard self.webView === webView else { return }
    updateContentHeight()
  }

  func webView(_ webView: WKWebView,
               decidePolicyFor navigationAction: WKNavigationAction,
               preferences: WKWebpagePreferences,
               decisionHandler: @escaping (WKNavigationActionPolicy, WKWebpagePreferences) -> Void) {
    preferences.allowsContentJavaScript = networkPolicy.canUseNetwork
    guard navigationNeedsNetworkPermission(navigationAction) else {
      decisionHandler(navigationPolicy(for: navigationAction), preferences)
      return
    }

    networkPolicy.callOnlineApi({ [weak self] permitted in
      guard let self, self.webView === webView else {
        decisionHandler(.cancel, preferences)
        return
      }
      preferences.allowsContentJavaScript = permitted
      decisionHandler(permitted ? self.navigationPolicy(for: navigationAction) : .cancel, preferences)
    }, forceAskPermission: false)
  }
}

// MARK: - ExpandableTextContainer

extension PlacePageUserDescriptionWebView: ExpandableTextContainer {
  func configure(with text: String) {
    guard htmlString != text else { return }
    loadHTML(text)
  }

  func updateContentHeight() {
    guard let webView else { return }
    webView.evaluateJavaScript("document.documentElement.scrollHeight") { [weak self] result, _ in
      guard let self, self.webView === webView else { return }
      let domHeight = CGFloat(htmlHeight: result) ?? 0
      let scrollHeight = webView.scrollView.contentSize.height
      let height = max(domHeight, scrollHeight)
      self.applyMeasuredHeight(height)
    }
  }

  func expandedHeight(for _: CGFloat) -> CGFloat {
    measuredHTMLHeight
  }

  func collapsedHeight(for _: CGFloat) -> CGFloat {
    Constants.previewHeight
  }
}

// MARK: - WebViewPool

private extension PlacePageUserDescriptionWebView {
  final class WebViewPool {
    private var cachedWebView: WKWebView?
    private var memoryWarningObserver: NSObjectProtocol?

    init() {
      memoryWarningObserver = NotificationCenter.default.addObserver(forName: UIApplication.didReceiveMemoryWarningNotification,
                                                                     object: nil,
                                                                     queue: .main) { [weak self] _ in
        self?.cachedWebView = nil
      }
    }

    deinit {
      if let memoryWarningObserver {
        NotificationCenter.default.removeObserver(memoryWarningObserver)
      }
    }

    func getWebView() -> WKWebView {
      let webView = cachedWebView ?? makeWebView()
      cachedWebView = nil
      return webView
    }

    func release(_ webView: WKWebView) {
      webView.stopLoading()
      webView.navigationDelegate = nil
      webView.removeFromSuperview()
      cachedWebView = webView
    }

    private func makeWebView() -> WKWebView {
      let configuration = WKWebViewConfiguration()
      configuration.allowsInlineMediaPlayback = false
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
}

// MARK: - URL + HTTP

private extension URL {
  var isHTTPOrHTTPS: Bool {
    guard let scheme = scheme?.lowercased() else { return false }
    return scheme == "http" || scheme == "https"
  }
}

// MARK: - CGFloat + HtmlHeight

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
