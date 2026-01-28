extension NSAttributedString {
  @objc
  public class func string(withHtml htmlString:String, defaultAttributes attributes:[NSAttributedString.Key : Any]) -> NSAttributedString? {
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
  @objc convenience init?(htmlString: String, baseFont: UIFont, paragraphStyle: NSParagraphStyle?, estimatedWidth: CGFloat = 0) {
    self.init(htmlString: htmlString, baseFont: baseFont)
    if let paragraphStyle = paragraphStyle {
      addAttribute(.paragraphStyle, value: paragraphStyle, range: NSMakeRange(0, length))
    }
    
    guard estimatedWidth > 0 else { return }
    enumerateAttachments(estimatedWidth: estimatedWidth)
  }
  
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

    enumerateFont(baseFont)
  }
  
  @objc convenience init?(htmlString: String, baseFont: UIFont, estimatedWidth: CGFloat) {
    guard let data = htmlString.data(using: .utf8) else { return nil }
    
    do {
      try self.init(data: data,
                    options: [.documentType : NSAttributedString.DocumentType.html,
                              .characterEncoding: String.Encoding.utf8.rawValue],
                    documentAttributes: nil)
    } catch {
      return nil
    }
    
    enumerateFont(baseFont)
    
    guard estimatedWidth > 0 else { return }

    enumerateAttachments(estimatedWidth: estimatedWidth)
  }
  
  func enumerateFont(_ baseFont: UIFont) {
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
  
  func enumerateAttachments(estimatedWidth: CGFloat) {
    enumerateAttribute(.attachment, in: NSMakeRange(0, length), options: []) { (value, range, _) in
      if let attachement = value as? NSTextAttachment,
        let image = attachement.image(forBounds: attachement.bounds, textContainer: NSTextContainer(), characterIndex: range.location) {
        if image.size.width > estimatedWidth || attachement.bounds.width > estimatedWidth {
          let resizedAttachment = NSTextAttachment()
          if image.size.width > estimatedWidth {
            let newImage = resizeImage(image: image, scale: estimatedWidth/image.size.width) ?? image
            resizedAttachment.image = newImage
          } else {
            resizedAttachment.image = image
          }
          addAttribute(.attachment, value: resizedAttachment, range: range)
        }
      }
    }
  }
  
  func resizeImage(image: UIImage, scale: CGFloat) -> UIImage? {
    let newSize = CGSize(width: image.size.width*scale, height: image.size.height*scale)
    let rect = CGRect(origin: CGPoint.zero, size: newSize)

    let renderer = UIGraphicsImageRenderer(size: newSize)
    let newImage = renderer.image { context in
      image.draw(in: rect)
    }
    return newImage
  }
}
