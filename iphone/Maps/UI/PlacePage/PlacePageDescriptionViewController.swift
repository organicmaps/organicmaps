final class PlacePageDescriptionViewController: WebViewController {
  override func viewDidLoad() {
    super.viewDidLoad()
    webView.scrollView.delegate = self
  }

  override func viewDidAppear(_ animated: Bool) {
    super.viewDidAppear(animated)
    if isOnBottom(webView.scrollView) {
      Statistics.logEvent(kStatPlacePageDescriptionViewAll)
    }
  }
  
  override func configuredHtml(withText htmlText: String) -> String {
    let scale = UIScreen.main.scale
    let styleTags = """
      <head>
      <style type=\"text/css\">
        body{font-family:'-apple-system','HelveticaNeue'; font-size:\(14 * scale); line-height:1.5em;}
      </style>
      </head>
      <body>
    """
    var html = htmlText.replacingOccurrences(of: "<body>", with: styleTags)
    html = html.replacingOccurrences(of: "</body>", with: "<p><b>wikipedia.org</b></p></body>")
    return html
  }

  override func webView(_ webView: WKWebView,
                        decidePolicyFor navigationAction: WKNavigationAction,
                        decisionHandler: @escaping (WKNavigationActionPolicy) -> Void) {
    if navigationAction.navigationType == .linkActivated, let url = navigationAction.request.url {
      Statistics.logEvent(kStatPlacePageDescriptionLinkClick,
                          withParameters: [kStatUrl : url.absoluteString])
    }

    super.webView(webView, decidePolicyFor: navigationAction, decisionHandler: decisionHandler)
  }

  private func isOnBottom(_ scrollView: UIScrollView) -> Bool {
    let bottom = scrollView.contentSize.height + scrollView.contentInset.bottom - scrollView.bounds.height
    return scrollView.contentOffset.y >= bottom
  }
}

extension PlacePageDescriptionViewController: UIScrollViewDelegate {
  func scrollViewDidEndDecelerating(_ scrollView: UIScrollView) {
    if isOnBottom(scrollView) {
      Statistics.logEvent(kStatPlacePageDescriptionViewAll)
    }
  }
}
