extension UIButton {
  @objc func setImagePadding(_ padding: CGFloat) {
    let isRightToLeft = UIView.userInterfaceLayoutDirection(for: semanticContentAttribute) == .rightToLeft
    if isRightToLeft {
      contentEdgeInsets = UIEdgeInsets(top: 0, left: 0, bottom: 0, right: padding)
      imageEdgeInsets = UIEdgeInsets(top: 0, left: 0, bottom: 0, right: -2 * padding)
    } else {
      contentEdgeInsets = UIEdgeInsets(top: 0, left: padding, bottom: 0, right: 0)
      imageEdgeInsets = UIEdgeInsets(top: 0, left: -2 * padding, bottom: 0, right: 0)
    }
  }
}
