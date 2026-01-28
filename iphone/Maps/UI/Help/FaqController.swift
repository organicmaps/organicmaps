final class FaqController: MWMViewController {
  override func loadView() {
    super.loadView()

    // TODO: FAQ?
    self.title = L("help")

    let path = Bundle.main.path(forResource: "faq", ofType: "html")!
    let html = try! String(contentsOfFile: path, encoding: String.Encoding.utf8)
    let webViewController = WebViewController.init(html: html, baseUrl: nil, title: nil)!
    webViewController.openInSafari = true
    addChild(webViewController)
    let aboutView = webViewController.view!
    view.addSubview(aboutView)

    aboutView.translatesAutoresizingMaskIntoConstraints = false
    NSLayoutConstraint.activate([
      aboutView.leadingAnchor.constraint(equalTo: view.leadingAnchor),
      aboutView.trailingAnchor.constraint(equalTo: view.trailingAnchor),
      aboutView.topAnchor.constraint(equalTo: view.topAnchor),
      aboutView.bottomAnchor.constraint(equalTo: view.bottomAnchor)
    ])
  }
}
