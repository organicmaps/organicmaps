extension UITableViewCell {
  /// Starts the separator at the view's leading edge. Measuring its laid-out geometry includes the
  /// safe area inset applied to `contentView`; UIKit mirrors `separatorInset` for RTL layouts, while
  /// separators end at the cell's trailing edge throughout the app.
  @objc(alignSeparatorWithView:)
  func alignSeparator(with view: UIView) {
    contentView.layoutIfNeeded()
    let frame = convert(view.bounds, from: view)
    let leading = effectiveUserInterfaceLayoutDirection == .rightToLeft
      ? bounds.maxX - frame.maxX
      : frame.minX
    let inset = UIEdgeInsets(top: 0, left: leading, bottom: 0, right: 0)
    // Avoid invalidating the cell's layout when called from layoutSubviews().
    guard separatorInset != inset else { return }
    separatorInset = inset
  }
}
