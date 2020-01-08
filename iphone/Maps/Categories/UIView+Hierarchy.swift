extension UIView {
  @objc func hasSubview(viewClass: AnyClass) -> Bool {
    return !subviews.filter { type(of: $0) == viewClass }.isEmpty
  }

  func clearTreeBackground() {
    backgroundColor = UIColor.clear
    subviews.forEach { $0.clearTreeBackground() }
  }

  func alignToSuperview(_ insets: UIEdgeInsets = .zero) {
    translatesAutoresizingMaskIntoConstraints = false
    NSLayoutConstraint.activate([
      topAnchor.constraint(equalTo: superview!.topAnchor, constant: insets.top),
      leftAnchor.constraint(equalTo: superview!.leftAnchor, constant: insets.left),
      bottomAnchor.constraint(equalTo: superview!.bottomAnchor, constant: insets.bottom),
      rightAnchor.constraint(equalTo: superview!.rightAnchor, constant: insets.right)
    ])
  }
}
