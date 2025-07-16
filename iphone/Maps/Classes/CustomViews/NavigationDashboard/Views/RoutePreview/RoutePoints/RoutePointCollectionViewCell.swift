final class RoutePointCollectionViewCell: UICollectionViewCell {

  struct ViewModel {
    let title: String
    let subtitle: String?
    let image: UIImage
    let imageStyle: GlobalStyleSheet
    let isPlaceholder: Bool
    let isCloseButtonVisible: Bool
    let onCloseHandler: (() -> Void)?
  }

  private enum Constants {
    static let logoSize: CGFloat = 28
    static let contentBackgroundInsets = UIEdgeInsets(top: 2, left: 12, bottom: 2, right: 12)
    static let logoSizeRatio: CGFloat = 1.0
    static let logoImageLeadingInset: CGFloat = 12
    static let reorderButtonSize: CGFloat = 24
    static let closeButtonSize: CGFloat = 24
    static let horizontalSpacing: CGFloat = 12
    static let titleToReorderSpacing: CGFloat = 12
  }

  private let logoImageView = UIImageView()
  private let contentBackgroundView = UIView()
  private let titleLabel = UILabel()
  private let textStackView = UIStackView()
  private let reorderButton = UIButton(type: .system)
  private let closeButton = UIButton(type: .system)
  private var didTapClose: (() -> Void)?

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
    logoImageView.contentMode = .scaleAspectFill
    logoImageView.clipsToBounds = true

    contentBackgroundView.setStyle(.pressBackground)
    contentBackgroundView.layer.setCornerRadius(.buttonDefault)

    textStackView.axis = .vertical
    textStackView.alignment = .leading

    reorderButton.setImage(UIImage(resource: .icMoveList), for: .normal)
    reorderButton.setStyle(.gray)

    closeButton.setImage(UIImage(resource: .icSearchClear), for: .normal)
    closeButton.setStyle(.gray)
    closeButton.addTarget(self, action: #selector(didTapCloseButton), for: .touchUpInside)
  }

  private func layout() {
    contentView.addSubview(contentBackgroundView)
    contentView.addSubview(reorderButton)
    contentBackgroundView.addSubview(logoImageView)
    contentBackgroundView.addSubview(textStackView)
    textStackView.addArrangedSubview(titleLabel)
    contentBackgroundView.addSubview(closeButton)

    logoImageView.translatesAutoresizingMaskIntoConstraints = false
    contentBackgroundView.translatesAutoresizingMaskIntoConstraints = false
    textStackView.translatesAutoresizingMaskIntoConstraints = false
    reorderButton.translatesAutoresizingMaskIntoConstraints = false
    closeButton.translatesAutoresizingMaskIntoConstraints = false

    NSLayoutConstraint.activate([
      contentBackgroundView.leadingAnchor.constraint(equalTo: leadingAnchor, constant: Constants.contentBackgroundInsets.left),
      contentBackgroundView.topAnchor.constraint(equalTo: contentView.topAnchor, constant: Constants.contentBackgroundInsets.top),
      contentBackgroundView.trailingAnchor.constraint(equalTo: reorderButton.leadingAnchor, constant: -Constants.contentBackgroundInsets.right),
      contentBackgroundView.bottomAnchor.constraint(equalTo: contentView.bottomAnchor, constant: -Constants.contentBackgroundInsets.bottom),

      logoImageView.leadingAnchor.constraint(equalTo: contentBackgroundView.leadingAnchor, constant: Constants.logoImageLeadingInset),
      logoImageView.centerYAnchor.constraint(equalTo: contentBackgroundView.centerYAnchor),
      logoImageView.widthAnchor.constraint(equalToConstant: Constants.logoSize),
      logoImageView.heightAnchor.constraint(equalToConstant: Constants.logoSize),

      textStackView.leadingAnchor.constraint(equalTo: logoImageView.trailingAnchor, constant: Constants.titleToReorderSpacing),
      textStackView.centerYAnchor.constraint(equalTo: contentBackgroundView.centerYAnchor),
      textStackView.trailingAnchor.constraint(lessThanOrEqualTo: closeButton.leadingAnchor, constant: -Constants.horizontalSpacing),

      closeButton.trailingAnchor.constraint(equalTo: contentBackgroundView.trailingAnchor, constant: -Constants.horizontalSpacing),
      closeButton.centerYAnchor.constraint(equalTo: contentBackgroundView.centerYAnchor),
      closeButton.widthAnchor.constraint(equalToConstant: Constants.closeButtonSize),
      closeButton.heightAnchor.constraint(equalToConstant: Constants.closeButtonSize),

      reorderButton.leadingAnchor.constraint(equalTo: contentBackgroundView.trailingAnchor, constant: Constants.horizontalSpacing),
      reorderButton.trailingAnchor.constraint(equalTo: trailingAnchor, constant: -Constants.horizontalSpacing),
      reorderButton.centerYAnchor.constraint(equalTo: contentBackgroundView.centerYAnchor),
      reorderButton.widthAnchor.constraint(equalToConstant: Constants.reorderButtonSize),
      reorderButton.heightAnchor.constraint(equalToConstant: Constants.reorderButtonSize),
    ])
  }

  func configure(with viewModel: ViewModel) {
    titleLabel.text = viewModel.title
    logoImageView.image = viewModel.image
    logoImageView.setStyleAndApply(viewModel.imageStyle)
    didTapClose = viewModel.onCloseHandler
    titleLabel.setFontStyleAndApply(.semibold14, color:  viewModel.isPlaceholder ? .blackSecondary : .blackPrimary)
    closeButton.isHidden = !viewModel.isCloseButtonVisible
  }

  func updateImage(with image: UIImage, style: GlobalStyleSheet) {
    logoImageView.image = image
    logoImageView.setStyleAndApply(style)
  }

  @objc
  private func didTapCloseButton() {
    didTapClose?()
  }
}
