extension UIView {
  func hasSubview(viewClass: AnyClass) -> Bool {
    return !subviews.filter { type(of: $0) == viewClass }.isEmpty
  }
}
