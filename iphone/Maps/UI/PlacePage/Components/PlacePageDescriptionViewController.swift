final class PlacePageDescriptionViewController: WebViewController {
  override func configuredHtml(withText htmlText: String) -> String {
    let scale = UIScreen.main.scale
    let styleTags = """
      <head>
        <style type=\"text/css\">
          body{font-family: -apple-system, BlinkMacSystemFont, "Helvetica Neue", Helvetica, Arial, sans-serif, "Apple Color Emoji"; font-size:\(14 * scale); line-height:1.5em; color: \(UIColor.blackPrimaryText().hexString); padding: 24px 16px;}
        </style>
      </head>
      <body>
    """
    var html = htmlText.replacingOccurrences(of: "<body>", with: styleTags)
    html = html.replacingOccurrences(of: "</body>", with: "<p><b>wikipedia.org</b></p></body>")
    return html
  }
  
  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    if previousTraitCollection?.userInterfaceStyle != traitCollection.userInterfaceStyle {
      guard let m_htmlText else { return }
      webView.loadHTMLString(configuredHtml(withText: m_htmlText), baseURL: nil)
    }
  }
  
  private func isOnBottom(_ scrollView: UIScrollView) -> Bool {
    let bottom = scrollView.contentSize.height + scrollView.contentInset.bottom - scrollView.bounds.height
    return scrollView.contentOffset.y >= bottom
  }
}
