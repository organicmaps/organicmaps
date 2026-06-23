final class BookmarksListCell: MWMTableViewSubtitleCell {
  private enum Constants {
    static let extendedLeadingImageTappableMargin = CGFloat(-15)
    static let accessoryButtonSize = CGSize(width: 44, height: 44)
  }

  private static let titleStyleName = "regular16:blackPrimaryText"
  private static let subtitleStyleName = "regular14:blackSecondaryText"

  private let accessoryButton = UIButton(type: .custom)
  private var configuration: Configuration = .default
  private var leadingButtonDidTapAction: ((_ anchor: UIView) -> Void)?
  private var accessoryButtonDidTapAction: ((_ anchor: UIView) -> Void)?

  override init(style _: UITableViewCell.CellStyle, reuseIdentifier: String?) {
    super.init(style: .subtitle, reuseIdentifier: reuseIdentifier)
    setupCell()
  }

  @available(*, unavailable)
  required init?(coder _: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func prepareForReuse() {
    super.prepareForReuse()

    configuration = .default
    leadingButtonDidTapAction = nil
    accessoryButtonDidTapAction = nil

    textLabel?.text = nil
    detailTextLabel?.text = nil
    imageView?.image = nil
    imageView?.tintColor = nil

    accessoryType = .none
    accessoryView = nil
    selectionStyle = .default
  }

  func configure(_ configuration: Configuration) {
    self.configuration = configuration
    applyCurrentAppearance()
  }

  override func applyTheme() {
    super.applyTheme()
    applyCurrentAppearance()
  }

  /// Extends the imageView tappable area.
  override func hitTest(_ point: CGPoint, with event: UIEvent?) -> UIView? {
    if let imageView,
       leadingButtonDidTapAction != nil,
       imageView.convert(imageView.bounds, to: self)
       .insetBy(dx: Constants.extendedLeadingImageTappableMargin, dy: Constants.extendedLeadingImageTappableMargin)
       .contains(point) {
      return imageView
    }

    return super.hitTest(point, with: event)
  }

  @objc private func onLeadingButtonTap(_ sender: UITapGestureRecognizer) {
    leadingButtonDidTapAction?(sender.view!)
  }

  @objc private func onAccessoryButtonTap() {
    accessoryButtonDidTapAction?(accessoryButton)
  }

  private func setupCell() {
    setStyle(.tableViewCell)

    textLabel?.numberOfLines = 0
    textLabel?.lineBreakMode = .byWordWrapping

    detailTextLabel?.numberOfLines = 0
    detailTextLabel?.lineBreakMode = .byWordWrapping

    let tapGesture = UITapGestureRecognizer(target: self, action: #selector(onLeadingButtonTap(_:)))
    imageView?.addGestureRecognizer(tapGesture)
    imageView?.isUserInteractionEnabled = true

    accessoryButton.frame = CGRect(origin: .zero, size: Constants.accessoryButtonSize)
    accessoryButton.addTarget(self, action: #selector(onAccessoryButtonTap), for: .touchUpInside)

    applyCurrentAppearance()
  }

  private func applyCurrentAppearance() {
    applyLabelStyles()
    applyLeadingItem()
    applyAccessoryItem()
    selectionStyle = configuration.selectionStyle
  }

  private func applyLabelStyles() {
    textLabel?.text = configuration.title
    detailTextLabel?.text = configuration.subtitle
    textLabel?.setStyleNameAndApply(Self.titleStyleName)
    detailTextLabel?.setStyleNameAndApply(Self.subtitleStyleName)
  }

  private func applyLeadingItem() {
    switch configuration.leadingItem {
    case .none:
      imageView?.image = nil
      imageView?.tintColor = nil
      imageView?.isUserInteractionEnabled = false
    case .image(let image, let tintColor, let action):
      imageView?.image = image
      imageView?.tintColor = tintColor
      imageView?.isUserInteractionEnabled = action != nil
      leadingButtonDidTapAction = action
    }
  }

  private func applyAccessoryItem() {
    switch configuration.accessoryItem {
    case .none:
      accessoryType = .none
      accessoryView = nil
    case .detailButton:
      accessoryView = nil
      accessoryType = .detailButton
    case .image(let image, let tintColor, let action):
      accessoryType = .none
      accessoryButton.tintColor = tintColor
      accessoryButton.setImage(image, for: .normal)
      accessoryButton.accessibilityLabel = accessibilityLabel
      accessoryView = accessoryButton
      accessoryButtonDidTapAction = action
    }
  }
}
