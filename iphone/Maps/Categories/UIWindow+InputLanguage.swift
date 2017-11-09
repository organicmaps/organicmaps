extension UIWindow {
  private func findFirstResponder(view: UIView) -> UIResponder? {
    guard !view.isFirstResponder else { return view }
    for subView in view.subviews {
      if let responder = findFirstResponder(view: subView) {
        return responder
      }
    }
    return nil
  }

  @objc func firstResponder() -> UIResponder? {
    return findFirstResponder(view: self)
  }
}
