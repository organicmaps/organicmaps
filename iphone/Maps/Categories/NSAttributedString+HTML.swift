extension NSAttributedString {
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
