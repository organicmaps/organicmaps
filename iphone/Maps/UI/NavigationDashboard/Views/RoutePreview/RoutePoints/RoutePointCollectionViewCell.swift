final class RoutePointCollectionViewCell: UICollectionViewCell {
  static let minimumHeight: CGFloat = 44

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
    static let logoImageLeadingInset: CGFloat = 12
    static let reorderButtonSize: CGFloat = 24
    static let closeButtonSize: CGFloat = 24
    static let horizontalSpacing: CGFloat = 12
    static let horizontalSpacingSmall: CGFloat = 5
    static let verticalInset: CGFloat = 8
    static let accessibilityVerticalInset: CGFloat = 12
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
  private var textStackViewTopConstraint: NSLayoutConstraint!
  private var textStackViewBottomConstraint: NSLayoutConstraint!

  private var verticalInset: CGFloat {
    traitCollection.preferredContentSizeCategory.isAccessibilityCategory
      ? Constants.accessibilityVerticalInset
      : Constants.verticalInset
  }

  override init(frame: CGRect) {
    super.init(frame: frame)
    setupView()
    layout()
  }

  override var isHighlighted: Bool {
    didSet {
      contentBackgroundView.backgroundColor = isHighlighted ? .lightGray : .pressBackground
    }
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    guard previousTraitCollection?.preferredContentSizeCategory != traitCollection.preferredContentSizeCategory else { return }
    updateVerticalInsets()
  }

  @available(*, unavailable)
  required init?(coder _: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  private func setupView() {
    contentView.clipsToBounds = false

    contentBackgroundView.setStyle(.pressBackground)
    contentBackgroundView.layer.setCornerRadius(.buttonDefault)
    contentBackgroundView.clipsToBounds = false

    logoImageView.contentMode = .scaleAspectFill
    logoImageView.clipsToBounds = true

    titleLabel.numberOfLines = 0

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

    textStackViewTopConstraint = textStackView.topAnchor.constraint(greaterThanOrEqualTo: contentBackgroundView.topAnchor)
    textStackViewBottomConstraint = textStackView.bottomAnchor.constraint(lessThanOrEqualTo: contentBackgroundView.bottomAnchor)
    updateVerticalInsets()

    NSLayoutConstraint.activate([
      contentBackgroundView.leadingAnchor.constraint(equalTo: contentView.leadingAnchor),
      contentBackgroundView.topAnchor.constraint(equalTo: contentView.topAnchor),
      contentBackgroundView.trailingAnchor.constraint(equalTo: contentView.trailingAnchor),
      contentBackgroundView.bottomAnchor.constraint(equalTo: contentView.bottomAnchor),
      contentBackgroundView.heightAnchor.constraint(greaterThanOrEqualToConstant: Self.minimumHeight),

      logoImageView.leadingAnchor.constraint(equalTo: contentBackgroundView.leadingAnchor, constant: Constants.logoImageLeadingInset),
      logoImageView.centerYAnchor.constraint(equalTo: contentBackgroundView.centerYAnchor),
      logoImageView.widthAnchor.constraint(equalToConstant: Constants.logoSize),
      logoImageView.heightAnchor.constraint(equalToConstant: Constants.logoSize),

      textStackView.leadingAnchor.constraint(equalTo: logoImageView.trailingAnchor, constant: Constants.horizontalSpacing),
      textStackViewTopConstraint,
      textStackView.centerYAnchor.constraint(equalTo: contentBackgroundView.centerYAnchor),
      textStackView.trailingAnchor.constraint(lessThanOrEqualTo: closeButton.leadingAnchor, constant: -Constants.horizontalSpacing),
      textStackViewBottomConstraint,

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

  private func updateVerticalInsets() {
    textStackViewTopConstraint?.constant = verticalInset
    textStackViewBottomConstraint?.constant = -verticalInset
  }

  override func preferredLayoutAttributesFitting(_ layoutAttributes: UICollectionViewLayoutAttributes) -> UICollectionViewLayoutAttributes {
    let attributes = layoutAttributes.copy() as! UICollectionViewLayoutAttributes
    let width = attributes.size.width > 0 ? attributes.size.width : superview?.bounds.width ?? 0
    guard width > 0 else { return super.preferredLayoutAttributesFitting(layoutAttributes) }

    contentView.bounds.size.width = width
    contentView.setNeedsLayout()
    contentView.layoutIfNeeded()

    let targetSize = CGSize(width: width, height: UIView.layoutFittingCompressedSize.height)
    let size = contentView.systemLayoutSizeFitting(
      targetSize,
      withHorizontalFittingPriority: .required,
      verticalFittingPriority: .fittingSizeLevel
    )
    attributes.size = CGSize(width: width, height: max(Self.minimumHeight, ceil(size.height)))
    return attributes
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

  @objc
  private func didTapCloseButton() {
    didTapClose?()
  }
}
