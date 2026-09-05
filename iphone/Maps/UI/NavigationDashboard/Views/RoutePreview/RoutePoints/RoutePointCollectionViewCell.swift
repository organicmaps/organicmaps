final class RoutePointCollectionViewCell: UICollectionViewCell {
  enum CellType {
    case point(PointViewModel)
    case addPoint
  }

  struct PointViewModel {
    enum TrailingButton {
      case close(() -> Void)
      case swap(() -> Void)

      var image: UIImage {
        switch self {
        case .close: UIImage.icSearchClear
        case .swap: UIImage.icSwap
        }
      }

      var style: GlobalStyleSheet {
        switch self {
        case .close: .gray
        case .swap: .blue
        }
      }

      var action: () -> Void {
        switch self {
        case .close(let action), .swap(let action): action
        }
      }
    }

    let title: String
    let image: UIImage
    let trailingButton: TrailingButton?
    let maskedCorners: CACornerMask
    let isPlaceholder: Bool
    let showSeparator: Bool
  }

  private enum Constants {
    static let fontStyle = FontStyleSheet.semibold14
    static let minimumHeight: CGFloat = 44
    static let titleNumberOfLines: Int = 2
    static let verticalInset: CGFloat = 8
    static let logoSize: CGFloat = 28
    static let logoImageLeadingInset: CGFloat = 12
    static let reorderButtonSize: CGFloat = 24
    static let actionButtonSize: CGFloat = 24
    static let horizontalSpacing: CGFloat = 12
    static let horizontalSpacingSmall: CGFloat = 5
  }

  private let logoImageView = UIImageView()
  private let contentBackgroundView = UIView()
  private let titleLabel = UILabel()
  private let textStackView = UIStackView()
  private let reorderButton = UIButton(type: .system)
  private let actionButton = UIButton(type: .system)
  private lazy var separatorView: UIView = {
    let separatorInsets = UIEdgeInsets(top: 0, left: Constants.logoImageLeadingInset + Constants.logoSize + Constants.horizontalSpacing, bottom: 0, right: 0)
    return contentBackgroundView.addSeparator(.bottom, insets: separatorInsets)
  }()

  private var didTapTrailingButton: (() -> Void)?
  private var actionButtonStyle: GlobalStyleSheet?

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

  @available(*, unavailable)
  required init?(coder _: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  private func setupView() {
    contentView.clipsToBounds = false

    contentBackgroundView.setStyle(.pressBackground)
    contentBackgroundView.layer.setCornerRadius(.buttonDefaultBig)
    contentBackgroundView.clipsToBounds = false

    logoImageView.contentMode = .scaleAspectFill
    logoImageView.clipsToBounds = true

    titleLabel.numberOfLines = Constants.titleNumberOfLines

    textStackView.axis = .vertical
    textStackView.alignment = .leading

    reorderButton.setImage(UIImage(resource: .icMoveList), for: .normal)
    reorderButton.setStyle(.gray)

    actionButton.addTarget(self, action: #selector(didTapActionButton), for: .touchUpInside)
  }

  private func layout() {
    contentView.addSubview(contentBackgroundView)
    textStackView.addArrangedSubview(titleLabel)
    contentBackgroundView.addSubview(logoImageView)
    contentBackgroundView.addSubview(textStackView)
    contentBackgroundView.addSubview(actionButton)
    contentBackgroundView.addSubview(reorderButton)

    logoImageView.translatesAutoresizingMaskIntoConstraints = false
    contentBackgroundView.translatesAutoresizingMaskIntoConstraints = false
    textStackView.translatesAutoresizingMaskIntoConstraints = false
    reorderButton.translatesAutoresizingMaskIntoConstraints = false
    actionButton.translatesAutoresizingMaskIntoConstraints = false

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
      textStackView.trailingAnchor.constraint(lessThanOrEqualTo: actionButton.leadingAnchor, constant: -Constants.horizontalSpacing),

      actionButton.trailingAnchor.constraint(equalTo: reorderButton.leadingAnchor, constant: -Constants.horizontalSpacingSmall),
      actionButton.centerYAnchor.constraint(equalTo: contentBackgroundView.centerYAnchor),
      actionButton.widthAnchor.constraint(equalToConstant: Constants.actionButtonSize),
      actionButton.heightAnchor.constraint(equalToConstant: Constants.actionButtonSize),

      reorderButton.trailingAnchor.constraint(equalTo: contentBackgroundView.trailingAnchor, constant: -Constants.horizontalSpacing),
      reorderButton.centerYAnchor.constraint(equalTo: contentBackgroundView.centerYAnchor),
      reorderButton.widthAnchor.constraint(equalToConstant: Constants.reorderButtonSize),
      reorderButton.heightAnchor.constraint(equalToConstant: Constants.reorderButtonSize),
    ])
  }

  func configure(with viewModel: CellType) {
    switch viewModel {
    case .point(let viewModel):
      titleLabel.text = viewModel.title
      logoImageView.image = viewModel.image
      logoImageView.setStyleAndApply(.black)
      titleLabel.setFontStyleAndApply(Constants.fontStyle, color: viewModel.isPlaceholder ? .blackSecondary : .blackPrimary)
      configureTrailingButton(viewModel.trailingButton)
      reorderButton.isHidden = false
      contentBackgroundView.layer.maskedCorners = viewModel.maskedCorners
      separatorView.isHidden = !viewModel.showSeparator
    case .addPoint:
      titleLabel.text = L("placepage_add_stop")
      logoImageView.image = UIImage(resource: .icAddButton)
      logoImageView.setStyleAndApply(.blue)
      titleLabel.setFontStyleAndApply(Constants.fontStyle, color: .linkBlue)
      configureTrailingButton(nil)
      reorderButton.isHidden = true
      contentBackgroundView.layer.maskedCorners = [.layerMinXMaxYCorner, .layerMaxXMaxYCorner]
      separatorView.isHidden = true
    }
  }

  private func configureTrailingButton(_ button: PointViewModel.TrailingButton?) {
    actionButton.isHidden = button == nil
    actionButton.setImage(button?.image, for: .normal)
    didTapTrailingButton = button?.action

    guard let style = button?.style, actionButtonStyle != style else { return }
    actionButtonStyle = style
    actionButton.setStyleAndApply(style)
  }

  @objc
  private func didTapActionButton() {
    didTapTrailingButton?()
  }

  static func height() -> CGFloat {
    let titleHeight = Constants.fontStyle.font.dynamic.lineHeight * CGFloat(Constants.titleNumberOfLines)
    return max(Constants.minimumHeight, ceil(titleHeight + Constants.verticalInset * 2))
  }
}
