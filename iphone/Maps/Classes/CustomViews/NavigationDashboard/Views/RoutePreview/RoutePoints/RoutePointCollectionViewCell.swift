final class RoutePointCollectionViewCell: UICollectionViewCell {

  enum CellType {
    case point(PointViewModel)
    case addPoint
  }

  struct PointViewModel {
    let title: String
    let image: UIImage
    let showCloseButton: Bool
    let maskedCorners: CACornerMask
    let isPlaceholder: Bool
    let showSeparator: Bool
    let onCloseHandler: (() -> Void)?
  }

  private enum Constants {
    static let logoSize: CGFloat = 28
    static let logoSizeRatio: CGFloat = 1.0
    static let logoImageLeadingInset: CGFloat = 12
    static let reorderButtonSize: CGFloat = 24
    static let closeButtonSize: CGFloat = 24
    static let horizontalSpacing: CGFloat = 12
    static let horizontalSpacingSmall: CGFloat = 5
  }

  private let logoImageView = UIImageView()
  private let contentBackgroundView = UIView()
  private let titleLabel = UILabel()
  private let textStackView = UIStackView()
  private let reorderButton = UIButton(type: .system)
  private let closeButton = UIButton(type: .system)
  private lazy var separatorView: UIView = {
    let separatorInsets = UIEdgeInsets(top: 0, left: Constants.logoImageLeadingInset + Constants.logoSize + Constants.horizontalSpacing, bottom: 0, right: 0)
    return contentBackgroundView.addSeparator(.bottom, insets: separatorInsets)
  }()
  private var didTapClose: (() -> Void)?

  override init(frame: CGRect) {
    super.init(frame: frame)
    setupView()
    layout()
  }

  override var isHighlighted: Bool {
    didSet {
      contentBackgroundView.backgroundColor = isHighlighted ? .lightGray : .pressBackground()
    }
  }

  @available(*, unavailable)
  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  private func setupView() {
    contentView.clipsToBounds = false

    contentBackgroundView.setStyle(.pressBackground)
    contentBackgroundView.layer.setCornerRadius(.buttonDefault)
    contentBackgroundView.clipsToBounds = false

    logoImageView.contentMode = .scaleAspectFill
    logoImageView.clipsToBounds = true

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
    textStackView.addArrangedSubview(titleLabel)
    contentBackgroundView.addSubview(logoImageView)
    contentBackgroundView.addSubview(textStackView)
    contentBackgroundView.addSubview(closeButton)
    contentBackgroundView.addSubview(reorderButton)

    logoImageView.translatesAutoresizingMaskIntoConstraints = false
    contentBackgroundView.translatesAutoresizingMaskIntoConstraints = false
    textStackView.translatesAutoresizingMaskIntoConstraints = false
    reorderButton.translatesAutoresizingMaskIntoConstraints = false
    closeButton.translatesAutoresizingMaskIntoConstraints = false

    NSLayoutConstraint.activate([
      contentBackgroundView.leadingAnchor.constraint(equalTo: contentView.leadingAnchor),
      contentBackgroundView.topAnchor.constraint(equalTo: contentView.topAnchor),
      contentBackgroundView.trailingAnchor.constraint(equalTo: contentView.trailingAnchor),
      contentBackgroundView.bottomAnchor.constraint(equalTo: contentView.bottomAnchor),

      logoImageView.leadingAnchor.constraint(equalTo: contentBackgroundView.leadingAnchor, constant: Constants.logoImageLeadingInset),
      logoImageView.centerYAnchor.constraint(equalTo: contentBackgroundView.centerYAnchor),
      logoImageView.widthAnchor.constraint(equalToConstant: Constants.logoSize),
      logoImageView.heightAnchor.constraint(equalToConstant: Constants.logoSize),

      textStackView.leadingAnchor.constraint(equalTo: logoImageView.trailingAnchor, constant: Constants.horizontalSpacing),
      textStackView.centerYAnchor.constraint(equalTo: contentBackgroundView.centerYAnchor),
      textStackView.trailingAnchor.constraint(lessThanOrEqualTo: closeButton.leadingAnchor, constant: -Constants.horizontalSpacing),

      closeButton.trailingAnchor.constraint(equalTo: reorderButton.leadingAnchor, constant: -Constants.horizontalSpacingSmall),
      closeButton.centerYAnchor.constraint(equalTo: contentBackgroundView.centerYAnchor),
      closeButton.widthAnchor.constraint(equalToConstant: Constants.closeButtonSize),
      closeButton.heightAnchor.constraint(equalToConstant: Constants.closeButtonSize),

      reorderButton.trailingAnchor.constraint(equalTo: contentBackgroundView.trailingAnchor, constant: -Constants.horizontalSpacing),
      reorderButton.centerYAnchor.constraint(equalTo: contentBackgroundView.centerYAnchor),
      reorderButton.widthAnchor.constraint(equalToConstant: Constants.reorderButtonSize),
      reorderButton.heightAnchor.constraint(equalToConstant: Constants.reorderButtonSize),
    ])
  }

  private func makeDotSeparator() -> UIView {
    let view = UIView()
    view.setStyle(.gray)
    view.translatesAutoresizingMaskIntoConstraints = false
    view.layer.cornerRadius = 2
    NSLayoutConstraint.activate([
      view.widthAnchor.constraint(equalToConstant: 4),
      view.heightAnchor.constraint(equalToConstant: 4)
    ])
    return view
  }

  func configure(with viewModel: CellType) {
    switch viewModel {
    case .point(let viewModel):
      titleLabel.text = viewModel.title
      logoImageView.image = viewModel.image
      logoImageView.setStyleAndApply(.black)
      didTapClose = viewModel.onCloseHandler
      titleLabel.setFontStyleAndApply(.semibold14, color: viewModel.isPlaceholder ? .blackSecondary : .blackPrimary)
      closeButton.isHidden = !viewModel.showCloseButton
      reorderButton.isHidden = false
      contentBackgroundView.layer.maskedCorners = viewModel.maskedCorners
      separatorView.isHidden = !viewModel.showSeparator
    case .addPoint:
      titleLabel.text = L("placepage_add_stop")
      logoImageView.image = UIImage(resource: .icAddButton)
      logoImageView.setStyleAndApply(.blue)
      titleLabel.setFontStyleAndApply(.semibold14, color: .linkBlue)
      closeButton.isHidden = true
      reorderButton.isHidden = true
      contentBackgroundView.layer.maskedCorners = [.layerMinXMaxYCorner, .layerMaxXMaxYCorner]
      separatorView.isHidden = true
    }
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
