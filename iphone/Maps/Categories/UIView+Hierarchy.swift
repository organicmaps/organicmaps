extension UIView {
  @objc func hasSubview(viewClass: AnyClass) -> Bool {
    return !subviews.filter { type(of: $0) == viewClass }.isEmpty
  }
}
