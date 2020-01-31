import Foundation

class InsetsLabel: UILabel {
  var insets = UIEdgeInsets.zero

  override func drawText(in rect: CGRect) {
    super.drawText(in: rect.inset(by: insets));
  }

  override var intrinsicContentSize: CGSize {
    return CGSize(width: super.intrinsicContentSize.width + insets.left + insets.right,
                  height: super.intrinsicContentSize.height + insets.top + insets.bottom)
  }
}
