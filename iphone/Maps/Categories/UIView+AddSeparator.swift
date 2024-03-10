extension UIView {
  func addSeparator(thickness: CGFloat = 1.0,
                    color: UIColor? = StyleManager.shared.theme?.colors.blackDividers,
                    insets: UIEdgeInsets = .zero) {
    let lineView = UIView()
    lineView.backgroundColor = color
    lineView.isUserInteractionEnabled = false
    lineView.translatesAutoresizingMaskIntoConstraints = false
    addSubview(lineView)
    NSLayoutConstraint.activate([
      lineView.heightAnchor.constraint(equalToConstant: thickness),
      lineView.leadingAnchor.constraint(equalTo: leadingAnchor, constant: insets.left),
      lineView.trailingAnchor.constraint(equalTo: trailingAnchor, constant: -insets.right),
      lineView.topAnchor.constraint(equalTo: topAnchor, constant: insets.top),
    ])
  }
}
