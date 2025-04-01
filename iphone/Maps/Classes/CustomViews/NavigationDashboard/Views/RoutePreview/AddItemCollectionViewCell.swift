final class AddItemCollectionViewCell: UICollectionViewCell {

  private let plusButton = CircleImageButton()
  var didTapAction: (() -> Void)?

  override init(frame: CGRect) {
    super.init(frame: frame)
    setupView()
    layout()
  }

  @available(*, unavailable)
  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  private func setupView() {
    plusButton.setImage(UIImage(resource: .icAddLight))
    plusButton.addTarget(self, action: #selector(didTapButton), for: .touchUpInside)
  }

  private func layout() {
    contentView.addSubview(plusButton)
    plusButton.translatesAutoresizingMaskIntoConstraints = false
    NSLayoutConstraint.activate([
      plusButton.trailingAnchor.constraint(equalTo: contentView.trailingAnchor),
      plusButton.centerYAnchor.constraint(equalTo: contentView.centerYAnchor),
      plusButton.heightAnchor.constraint(equalTo: contentView.heightAnchor),
      plusButton.widthAnchor.constraint(equalTo: contentView.heightAnchor)
    ])
  }

  @objc
  private func didTapButton() {
    didTapAction?()
  }
}
