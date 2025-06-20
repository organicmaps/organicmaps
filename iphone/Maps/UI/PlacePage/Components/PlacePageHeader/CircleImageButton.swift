final class CircleImageButton: UIButton {

  private static let expandedTappableAreaInsets = UIEdgeInsets(top: -5, left: -5, bottom: -5, right: -5)

  private let circleImageView = UIImageView()

  override init(frame: CGRect) {
    super.init(frame: frame)
    setupView()
  }

  @available(*, unavailable)
  required init?(coder: NSCoder) {
    super.init(coder: coder)
    setupView()
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    if #available(iOS 13.0, *), traitCollection.hasDifferentColorAppearance(comparedTo: previousTraitCollection) {
      circleImageView.applyTheme()
    }
  }

  private func setupView() {
    backgroundColor = .clear
    circleImageView.setStyle(.ppHeaderCircleIcon)
    circleImageView.contentMode = .scaleAspectFill
    circleImageView.clipsToBounds = true
    circleImageView.isUserInteractionEnabled = false
    circleImageView.layer.masksToBounds = true
    circleImageView.translatesAutoresizingMaskIntoConstraints = false
    addSubview(circleImageView)

    let aspectRatioConstraint = circleImageView.widthAnchor.constraint(equalTo: circleImageView.heightAnchor)
    aspectRatioConstraint.priority = .defaultHigh
    NSLayoutConstraint.activate([
      circleImageView.centerXAnchor.constraint(equalTo: centerXAnchor),
      circleImageView.centerYAnchor.constraint(equalTo: centerYAnchor),
      circleImageView.widthAnchor.constraint(lessThanOrEqualTo: widthAnchor),
      circleImageView.heightAnchor.constraint(lessThanOrEqualTo: heightAnchor),
      aspectRatioConstraint
    ])
  }

  override func layoutSubviews() {
    super.layoutSubviews()
    circleImageView.layer.cornerRadius = circleImageView.bounds.width / 2.0
  }

  override func point(inside point: CGPoint, with event: UIEvent?) -> Bool {
    let expandedBounds = bounds.inset(by: Self.expandedTappableAreaInsets)
    return expandedBounds.contains(point)
  }

  func setImage(_ image: UIImage?) {
    circleImageView.image = image
  }
}
