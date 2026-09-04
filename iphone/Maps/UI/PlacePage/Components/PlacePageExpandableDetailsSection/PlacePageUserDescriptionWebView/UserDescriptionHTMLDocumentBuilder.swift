struct UserDescriptionHTMLDocumentBuilder {
  private static let adaptiveColorSchemeStyle = """
  <style>
    :root { color-scheme: light dark; }
    html, body { background: transparent; }
  </style>
  """

  func buildHTML(with htmlString: String, compatibleWith traitCollection: UITraitCollection) -> String {
    if isHTMLDocument(htmlString) {
      return addingAdaptiveColorScheme(to: htmlString)
    }
    // Convert fragment HTML to full document.
    let htmlBody = extractHTMLBody(from: htmlString)
    let appearanceDeclarations = appearanceVariables(compatibleWith: traitCollection)
      .map { "\($0.name): \($0.value);" }
      .joined(separator: "\n          ")
    return """
      <!doctype html>
      <html>
      <head>
      <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0">
      <style>
        :root {
          \(appearanceDeclarations)
        }
        html, body {
          margin: 0;
          padding: 0;
          background: transparent;
        }
        body {
          color: var(--om-text-color);
          font-size: var(--om-font-size);
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

  func appearanceVariables(compatibleWith traitCollection: UITraitCollection) -> [(name: String, value: String)] {
    let textColor = UIColor.blackPrimaryText.resolvedColor(with: traitCollection).hexString
    let fontSize = UIFont.regular14.dynamic(compatibleWith: traitCollection).pointSize
    return [("--om-text-color", textColor), ("--om-font-size", "\(fontSize)px")]
  }

  private func isHTMLDocument(_ html: String) -> Bool {
    html.range(of: #"^\s*(?:<!doctype\s+html[^>]*>\s*)?<html\b"#,
               options: [.regularExpression, .caseInsensitive]) != nil
  }

  private func addingAdaptiveColorScheme(to html: String) -> String {
    var result = html
    let options: String.CompareOptions = [.regularExpression, .caseInsensitive]
    if let headStart = html.range(of: #"<head\b[^>]*>"#, options: options) {
      result.insert(contentsOf: Self.adaptiveColorSchemeStyle, at: headStart.upperBound)
    } else if let htmlStart = html.range(of: #"<html\b[^>]*>"#, options: options) {
      result.insert(contentsOf: "<head>\(Self.adaptiveColorSchemeStyle)</head>", at: htmlStart.upperBound)
    }
    return result
  }

  private func extractHTMLBody(from html: String) -> String {
    guard let bodyStartRange = html.range(of: "<body[^>]*>", options: [.regularExpression, .caseInsensitive]),
          let bodyEndRange = html.range(of: "</body>", options: [.caseInsensitive]) else {
      return html
    }
    return String(html[bodyStartRange.upperBound ..< bodyEndRange.lowerBound])
  }
}
