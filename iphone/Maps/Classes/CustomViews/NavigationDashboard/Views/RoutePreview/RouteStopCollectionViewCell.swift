final class RouteStopCollectionViewCell: UICollectionViewCell {

  struct ViewModel {
    let title: String
    let subtitle: String?
    let image: UIImage
    let imageStyle: GlobalStyleSheet
    let isPlaceholder: Bool
    let onCloseHandler: (() -> Void)?
  }

  private enum Constants {
    static let logoSize: CGFloat = 28
    static let contentBackgroundInsets = UIEdgeInsets(top: 2, left: 12, bottom: 2, right: 16)
    static let logoSizeRatio: CGFloat = 1.0
    static let logoImageLeadingInset: CGFloat = 16
    static let reorderButtonSize: CGFloat = 24
    static let closeButtonSize: CGFloat = 24
    static let horizontalSpacing: CGFloat = 12
    static let titleToSubtitleSpacing: CGFloat = 2
    static let titleToReorderSpacing: CGFloat = 12
  }

  private let logoImageView = UIImageView()
  private let contentBackgroundView = UIView()
  private let titleLabel = UILabel()
  private let subtitleLabel = UILabel()
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

    subtitleLabel.setFontStyleAndApply(.regular12, color: .blackSecondary)

    textStackView.axis = .vertical
    textStackView.spacing = Constants.titleToSubtitleSpacing
    textStackView.alignment = .leading

    reorderButton.setImage(UIImage(resource: .icMoveList), for: .normal)
    reorderButton.setStyle(.gray)

    closeButton.setImage(UIImage(resource: .icSearchClear), for: .normal)
    closeButton.setStyle(.gray)
    closeButton.addTarget(self, action: #selector(didTapCloseButton), for: .touchUpInside)
  }

  private func layout() {
    contentView.addSubview(logoImageView)
    contentView.addSubview(contentBackgroundView)
    let dot = makeDotSeparator()
    contentBackgroundView.addSubview(dot)
    contentBackgroundView.addSubview(textStackView)
    textStackView.addArrangedSubview(titleLabel)
    textStackView.addArrangedSubview(subtitleLabel)
    contentBackgroundView.addSubview(reorderButton)
    contentBackgroundView.addSubview(closeButton)

    logoImageView.translatesAutoresizingMaskIntoConstraints = false
    contentBackgroundView.translatesAutoresizingMaskIntoConstraints = false
    textStackView.translatesAutoresizingMaskIntoConstraints = false
    reorderButton.translatesAutoresizingMaskIntoConstraints = false
    closeButton.translatesAutoresizingMaskIntoConstraints = false

    NSLayoutConstraint.activate([
      logoImageView.leadingAnchor.constraint(equalTo: contentView.leadingAnchor, constant: Constants.logoImageLeadingInset),
      logoImageView.centerYAnchor.constraint(equalTo: contentView.centerYAnchor),
      logoImageView.widthAnchor.constraint(equalToConstant: Constants.logoSize),
      logoImageView.heightAnchor.constraint(equalToConstant: Constants.logoSize),

      contentBackgroundView.leadingAnchor.constraint(equalTo: logoImageView.trailingAnchor, constant: Constants.contentBackgroundInsets.left),
      contentBackgroundView.topAnchor.constraint(equalTo: contentView.topAnchor, constant: Constants.contentBackgroundInsets.top),
      contentBackgroundView.trailingAnchor.constraint(equalTo: contentView.trailingAnchor, constant: -Constants.contentBackgroundInsets.right),
      contentBackgroundView.bottomAnchor.constraint(equalTo: contentView.bottomAnchor, constant: -Constants.contentBackgroundInsets.bottom),

      reorderButton.leadingAnchor.constraint(equalTo: contentBackgroundView.leadingAnchor, constant: Constants.horizontalSpacing),
      reorderButton.centerYAnchor.constraint(equalTo: contentBackgroundView.centerYAnchor),
      reorderButton.widthAnchor.constraint(equalToConstant: Constants.reorderButtonSize),
      reorderButton.heightAnchor.constraint(equalToConstant: Constants.reorderButtonSize),

      dot.centerXAnchor.constraint(equalTo: logoImageView.centerXAnchor),
      dot.centerYAnchor.constraint(equalTo: logoImageView.centerYAnchor, constant: -Constants.logoSize),

      textStackView.leadingAnchor.constraint(equalTo: reorderButton.trailingAnchor, constant: Constants.titleToReorderSpacing),
      textStackView.centerYAnchor.constraint(equalTo: contentBackgroundView.centerYAnchor),
      textStackView.trailingAnchor.constraint(lessThanOrEqualTo: closeButton.leadingAnchor, constant: -Constants.horizontalSpacing),

      closeButton.trailingAnchor.constraint(equalTo: contentBackgroundView.trailingAnchor, constant: -Constants.horizontalSpacing),
      closeButton.centerYAnchor.constraint(equalTo: contentBackgroundView.centerYAnchor),
      closeButton.widthAnchor.constraint(equalToConstant: Constants.closeButtonSize),
      closeButton.heightAnchor.constraint(equalToConstant: Constants.closeButtonSize)
    ])
  }

  private func makeDotSeparator() -> UIView {
    let view = UIView()
    view.translatesAutoresizingMaskIntoConstraints = false
    view.backgroundColor = .systemGray
    view.layer.cornerRadius = 2
    NSLayoutConstraint.activate([
      view.widthAnchor.constraint(equalToConstant: 4),
      view.heightAnchor.constraint(equalToConstant: 4)
    ])
    return view
  }

  func configure(with viewModel: ViewModel) {
    titleLabel.text = viewModel.title
    subtitleLabel.text = viewModel.subtitle
    logoImageView.image = viewModel.image
    logoImageView.setStyleAndApply(viewModel.imageStyle)
    didTapClose = viewModel.onCloseHandler
    titleLabel.setFontStyleAndApply(.semibold14, color:  viewModel.isPlaceholder ? .blackSecondary : .blackPrimary)
    closeButton.isHidden = viewModel.isPlaceholder
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
