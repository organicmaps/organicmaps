import UIKit

extension String {
  func size(width: CGFloat, font: UIFont, maxNumberOfLines: Int = 0) -> CGSize {
    if isEmpty {
      return CGSize.zero
    }
    let lineHeight = font.lineHeight
    let maximumHeight = maxNumberOfLines == 0 ? CGFloat.greatestFiniteMagnitude : lineHeight * CGFloat(maxNumberOfLines + 1)
    let constraintSize = CGSize(width: width, height: maximumHeight)
    let options: NSStringDrawingOptions = [.usesLineFragmentOrigin, .usesFontLeading, .truncatesLastVisibleLine]
    let paragraph = NSMutableParagraphStyle()
    paragraph.lineBreakMode = .byWordWrapping
    paragraph.alignment = .natural
    let attributes = [
      NSAttributedStringKey.font: font,
      NSAttributedStringKey.paragraphStyle: paragraph,
    ]
    var rect = (self as NSString).boundingRect(with: constraintSize, options: options, attributes: attributes, context: nil)
    var numberOfLines = ceil(rect.height / lineHeight)
    if maxNumberOfLines != 0 {
      if width - rect.width < font.ascender {
        rect.size.width = width - font.ascender
        numberOfLines += 1
      }
      numberOfLines = min(numberOfLines, CGFloat(maxNumberOfLines))
    }
    return CGSize(width: ceil(rect.width), height: ceil(numberOfLines * lineHeight))
  }
}
