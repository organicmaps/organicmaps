final class AddPointCollectionViewCell: UICollectionViewCell {

  private enum Constants {
    static let trailingInsets = CGFloat(16)
  }

  private let plusButton = UIButton()
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
    plusButton.setStyle(.flatNormalTransButton)
    plusButton.setImage(UIImage(resource: .icAddButton), for: .normal)
    plusButton.setTitle(L("placepage_add_stop"), for: .normal)
    plusButton.addTarget(self, action: #selector(didTapButton), for: .touchUpInside)
  }

  private func layout() {
    contentView.addSubview(plusButton)
    plusButton.translatesAutoresizingMaskIntoConstraints = false
    NSLayoutConstraint.activate([
      plusButton.trailingAnchor.constraint(equalTo: contentView.trailingAnchor, constant: -Constants.trailingInsets),
      plusButton.centerYAnchor.constraint(equalTo: contentView.centerYAnchor),
      plusButton.heightAnchor.constraint(equalTo: contentView.heightAnchor),
    ])
  }

  @objc
  private func didTapButton() {
    didTapAction?()
  }
}
