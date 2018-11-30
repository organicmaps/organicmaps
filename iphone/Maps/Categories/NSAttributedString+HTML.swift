extension NSAttributedString {
  @objc
  public class func string(withHtml htmlString:String, defaultAttributes attributes:[NSAttributedStringKey : Any]) -> NSAttributedString? {
    guard let data = htmlString.data(using: .utf8) else { return nil }
    guard let text = try? NSMutableAttributedString(data: data,
                                                    options: [.documentType: NSAttributedString.DocumentType.html,
                                                              .characterEncoding: String.Encoding.utf8.rawValue],
                                                    documentAttributes: nil) else { return nil }
    text.addAttributes(attributes, range: NSMakeRange(0, text.length))
    return text
  }

}

extension NSMutableAttributedString {
  @objc convenience init?(htmlString: String, baseFont: UIFont) {
    guard let data = htmlString.data(using: .utf8) else { return nil }

    do {
      try self.init(data: data,
                    options: [.documentType : NSAttributedString.DocumentType.html,
                              .characterEncoding: String.Encoding.utf8.rawValue],
                    documentAttributes: nil)
    } catch {
      return nil
    }

    enumerateAttribute(.font, in: NSMakeRange(0, length), options: []) { (value, range, _) in
      if let font = value as? UIFont,
        let descriptor = baseFont.fontDescriptor.withSymbolicTraits(font.fontDescriptor.symbolicTraits) {
        let newFont = UIFont(descriptor: descriptor, size: baseFont.pointSize)
        addAttribute(.font, value: newFont, range: range)
      } else {
        addAttribute(.font, value: baseFont, range: range)
      }
    }
  }
}
