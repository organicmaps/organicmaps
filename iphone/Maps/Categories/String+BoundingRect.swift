import UIKit

extension String {
  func size(width: CGFloat, font: UIFont, maxNumberOfLines: Int = 0) -> CGSize {
    let maximumHeight = maxNumberOfLines == 0 ? CGFloat.greatestFiniteMagnitude : font.lineHeight * CGFloat(maxNumberOfLines + 1)
    let constraintSize = CGSize(width: width, height: maximumHeight)
    let options: NSStringDrawingOptions = [.usesLineFragmentOrigin, .usesFontLeading]
    let attributes = [NSAttributedStringKey.font: font]
    let rect = (self as NSString).boundingRect(with: constraintSize, options: options, attributes: attributes, context: nil)
    var numberOfLines = ceil(ceil(rect.height) / font.lineHeight)
    if maxNumberOfLines != 0 {
      numberOfLines = min(numberOfLines, CGFloat(maxNumberOfLines))
    }
    return CGSize(width: ceil(rect.width), height: ceil(numberOfLines * font.lineHeight))
  }
}
