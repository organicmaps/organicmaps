struct UserDescriptionHTMLDocumentBuilder {
  func buildHTML(with htmlString: String) -> String {
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
          font-size: \(UIFont.regular14.dynamic.pointSize)px;
          font-family: -apple-system, sans-serif;
          overflow-wrap: break-word;
        }
        img,
        video {
          max-width: 100%;
          height: auto;
        }
        iframe {
          max-width: 100%;
        }
      </style>
      </head>
      <body>
      \(htmlBody)
      </body>
      </html>
    """
  }

  private func isHTMLDocument(_ html: String) -> Bool {
    html.range(of: #"^\s*(?:<!doctype\s+html[^>]*>\s*)?<html\b"#,
               options: [.regularExpression, .caseInsensitive]) != nil
  }

  private func extractHTMLBody(from html: String) -> String {
    guard let bodyStartRange = html.range(of: "<body[^>]*>", options: [.regularExpression, .caseInsensitive]),
          let bodyEndRange = html.range(of: "</body>", options: [.caseInsensitive]) else {
      return html
    }
    return String(html[bodyStartRange.upperBound ..< bodyEndRange.lowerBound])
  }
}
