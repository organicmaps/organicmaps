extension UIButton {
  func setImagePadding(_ insets: UIEdgeInsets) {
    let isRightToLeft = UIView.userInterfaceLayoutDirection(for: semanticContentAttribute) == .rightToLeft
    imageEdgeInsets = insets.resolved(isRightToLeft: isRightToLeft)
  }
}

private extension UIEdgeInsets {
  func resolved(isRightToLeft: Bool) -> UIEdgeInsets {
    UIEdgeInsets(top: top,
                 left: isRightToLeft ? right : left,
                 bottom: bottom,
                 right: isRightToLeft ? left : right)
  }
}
