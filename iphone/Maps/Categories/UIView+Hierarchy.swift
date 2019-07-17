extension UIView {
  @objc func hasSubview(viewClass: AnyClass) -> Bool {
    return !subviews.filter { type(of: $0) == viewClass }.isEmpty
  }

  func clearTreeBackground() {
    backgroundColor = UIColor.clear
    subviews.forEach { $0.clearTreeBackground() }
  }

  func alignToSuperview() {
    translatesAutoresizingMaskIntoConstraints = false
    NSLayoutConstraint.activate([
      topAnchor.constraint(equalTo: superview!.topAnchor),
      leftAnchor.constraint(equalTo: superview!.leftAnchor),
      bottomAnchor.constraint(equalTo: superview!.bottomAnchor),
      rightAnchor.constraint(equalTo: superview!.rightAnchor)
    ])
  }
}
