class CatalogWebViewController: WebViewController {

  override func viewDidLoad() {
    super.viewDidLoad()

  }

  override func webView(_ webView: WKWebView,
                        decidePolicyFor navigationAction: WKNavigationAction,
                        decisionHandler: @escaping (WKNavigationActionPolicy) -> Void) {
    if let url = navigationAction.request.url {
      if url.scheme == "mapsme" {
        processDeeplink(url)
        decisionHandler(.cancel)
        return
      }
    }
    super.webView(webView, decidePolicyFor: navigationAction, decisionHandler: decisionHandler)
  }

  func processDeeplink(_ url: URL) {
    let components = URLComponents(url: url, resolvingAgainstBaseURL: false)
    var id = ""
    var name = ""
    components?.queryItems?.forEach({ (item) in
      if (item.name == "name") {
        name = item.value ?? ""
      }
      if (item.name == "id") {
        id = item.value ?? ""
      }
    })
    MWMBookmarksManager.downloadItem(withId: id, name: name) { (error) in
      
    }
  }
}
