final class RoutePointCollectionViewCell: UICollectionViewCell {
  enum CellType {
    case point(PointViewModel)
    case addPoint
  }

  enum ConnectorStyle {
    case none
    case solid
    case threeDots
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
    static let fontStyle = FontStyleSheet.semibold14
    static let minimumHeight: CGFloat = 46
    static let titleNumberOfLines: Int = 2
    static let verticalInset: CGFloat = 8
    static let logoSize: CGFloat = 28
    static let logoImageLeadingInset: CGFloat = 12
    static let reorderButtonSize: CGFloat = 24
    static let closeButtonSize: CGFloat = 24
    static let horizontalSpacing: CGFloat = 12
    static let horizontalSpacingSmall: CGFloat = 5
    static let connectorWidth: CGFloat = 2.5
    static let connectorIconInset: CGFloat = 3
    static let dotDiameter: CGFloat = 3.5
  }

  private let logoImageView = UIImageView()
  private let contentBackgroundView = UIView()
  private let titleLabel = UILabel()
  private let textStackView = UIStackView()
  private let reorderButton = UIButton(type: .system)
  private let closeButton = UIButton(type: .system)
  private let connectorLayer = CAShapeLayer()
  private lazy var separatorView: UIView = {
    let separatorInsets = UIEdgeInsets(top: 0, left: Constants.logoImageLeadingInset + Constants.logoSize + Constants.horizontalSpacing, bottom: 0, right: 0)
    return contentBackgroundView.addSeparator(.bottom, insets: separatorInsets)
  }()

  private var didTapClose: (() -> Void)?
  private var topConnector: ConnectorStyle = .none
  private var bottomConnector: ConnectorStyle = .none

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

  override func layoutSubviews() {
    super.layoutSubviews()
    let leadingX = Constants.logoImageLeadingInset + Constants.logoSize / 2
    let x = effectiveUserInterfaceLayoutDirection == .rightToLeft ? bounds.width - leadingX : leadingX
    let iconTop = (bounds.height - Constants.logoSize) / 2
    let iconBottom = iconTop + Constants.logoSize
    let connectorGap = iconTop - Constants.connectorIconInset
    // Both rows draw the same dotted segment; clipping keeps each half inside its own row.
    let topStart = topConnector == .threeDots ? -connectorGap : 0
    let bottomEnd = bottomConnector == .threeDots ? bounds.height + connectorGap : bounds.height
    let path = UIBezierPath()
    appendConnector(to: path, style: topConnector, x: x,
                    from: topStart, to: connectorGap, roundAtStart: false)
    appendConnector(to: path, style: bottomConnector, x: x,
                    from: iconBottom + Constants.connectorIconInset, to: bottomEnd, roundAtStart: true)
    connectorLayer.frame = bounds
    connectorLayer.path = path.cgPath
    connectorLayer.fillColor = UIColor.blackSecondaryText.cgColor
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    setNeedsLayout()
  }

  @available(*, unavailable)
  required init?(coder _: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  private func setupView() {
    clipsToBounds = false
    contentView.clipsToBounds = false

    contentBackgroundView.setStyle(.pressBackground)
    contentBackgroundView.layer.setCornerRadius(.buttonDefaultBig)
    contentBackgroundView.clipsToBounds = false
    connectorLayer.masksToBounds = true
    contentBackgroundView.layer.addSublayer(connectorLayer)

    logoImageView.contentMode = .scaleAspectFill
    logoImageView.clipsToBounds = true

    titleLabel.numberOfLines = Constants.titleNumberOfLines

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

  func configure(with viewModel: CellType, topConnector: ConnectorStyle, bottomConnector: ConnectorStyle) {
    self.topConnector = topConnector
    self.bottomConnector = bottomConnector
    setNeedsLayout()
    switch viewModel {
    case .point(let viewModel):
      titleLabel.text = viewModel.title
      logoImageView.image = viewModel.image
      logoImageView.setStyleAndApply(.black)
      didTapClose = viewModel.onCloseHandler
      titleLabel.setFontStyleAndApply(Constants.fontStyle, color: viewModel.isPlaceholder ? .blackSecondary : .blackPrimary)
      closeButton.isHidden = !viewModel.showCloseButton
      reorderButton.isHidden = false
      contentBackgroundView.layer.maskedCorners = viewModel.maskedCorners
      separatorView.isHidden = !viewModel.showSeparator
    case .addPoint:
      titleLabel.text = L("route_add_destination")
      logoImageView.image = UIImage(resource: .icAddButton)
      logoImageView.setStyleAndApply(.blue)
      titleLabel.setFontStyleAndApply(Constants.fontStyle, color: .linkBlue)
      closeButton.isHidden = true
      reorderButton.isHidden = true
      contentBackgroundView.layer.maskedCorners = [.layerMinXMaxYCorner, .layerMaxXMaxYCorner]
      separatorView.isHidden = true
    }
  }

  private func appendConnector(to path: UIBezierPath, style: ConnectorStyle, x: CGFloat,
                               from startY: CGFloat, to endY: CGFloat, roundAtStart: Bool) {
    switch style {
    case .none:
      break
    case .solid:
      let radius = Constants.connectorWidth / 2
      let rect = CGRect(x: x - radius, y: startY, width: Constants.connectorWidth, height: endY - startY)
      let corners: UIRectCorner = roundAtStart ? [.topLeft, .topRight] : [.bottomLeft, .bottomRight]
      path.append(UIBezierPath(roundedRect: rect, byRoundingCorners: corners,
                               cornerRadii: CGSize(width: radius, height: radius)))
    case .threeDots:
      let radius = Constants.dotDiameter / 2
      let centerSpacing = (endY - startY - Constants.dotDiameter) / 2
      for index in 0 ..< 3 {
        let y = startY + radius + CGFloat(index) * centerSpacing
        let dotRect = CGRect(x: x - radius, y: y - radius,
                             width: Constants.dotDiameter, height: Constants.dotDiameter)
        path.append(UIBezierPath(ovalIn: dotRect))
      }
    }
  }

  @objc
  private func didTapCloseButton() {
    didTapClose?()
  }

  static func height() -> CGFloat {
    let titleHeight = Constants.fontStyle.font.dynamic.lineHeight * CGFloat(Constants.titleNumberOfLines)
    return max(Constants.minimumHeight, ceil(titleHeight + Constants.verticalInset * 2))
  }
}
