import Foundation

class BookmarksSubscriptionDiscountLabel: UILabel {
  let offsetX:CGFloat = 10

  override func drawText(in rect: CGRect) {
    let insets = UIEdgeInsets(top: 0, left: offsetX, bottom: 0, right: offsetX)
    super.drawText(in: rect.inset(by: insets));
  }

  override var intrinsicContentSize: CGSize {
    return CGSize(width: super.intrinsicContentSize.width + offsetX*2, height: super.intrinsicContentSize.height)
  }
}
