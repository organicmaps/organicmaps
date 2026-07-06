final class BookmarksListCell: MWMTableViewSubtitleCell {
  private enum Constants {
    static let leadingButtonSize = CGSize(width: 44, height: 44)
    static let accessoryButtonSize = CGSize(width: 44, height: 44)
  }

  private let leadingButton = UIButton(type: .custom)
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
    updateButtonInteractions()
  }

  func configure(_ configuration: Configuration) {
    self.configuration = configuration
    applyCurrentAppearance()
  }

  override func setEditing(_ editing: Bool, animated: Bool) {
    super.setEditing(editing, animated: animated)
    updateButtonInteractions()
  }

  override func applyTheme() {
    super.applyTheme()
    applyCurrentAppearance()
  }

  override func layoutSubviews() {
    super.layoutSubviews()
    guard let imageView else { return }
    leadingButton.center = imageView.convert(CGPoint(x: imageView.bounds.midX, y: imageView.bounds.midY),
                                             to: contentView)
  }

  @objc private func onLeadingButtonTap() {
    leadingButtonDidTapAction?(leadingButton)
  }

  @objc private func onAccessoryButtonTap() {
    accessoryButtonDidTapAction?(accessoryButton)
  }

  private func setupCell() {
    setStyle(.tableViewCell)

    textLabel?.numberOfLines = 0
    textLabel?.lineBreakMode = .byWordWrapping
    textLabel?.setFontStyle(.regular16, color: .blackPrimary)

    detailTextLabel?.numberOfLines = 0
    detailTextLabel?.lineBreakMode = .byWordWrapping
    detailTextLabel?.setFontStyle(.regular14, color: .blackSecondary)

    leadingButton.bounds = CGRect(origin: .zero, size: Constants.leadingButtonSize)
    leadingButton.addTarget(self, action: #selector(onLeadingButtonTap), for: .touchUpInside)
    leadingButton.isAccessibilityElement = false
    contentView.addSubview(leadingButton)

    accessoryButton.frame = CGRect(origin: .zero, size: Constants.accessoryButtonSize)
    accessoryButton.addTarget(self, action: #selector(onAccessoryButtonTap), for: .touchUpInside)
    selectionStyle = .default

    applyCurrentAppearance()
  }

  private func applyCurrentAppearance() {
    applyLabelStyles()
    applyLeadingItem()
    applyAccessoryItem()
    updateButtonInteractions()
  }

  private func applyLabelStyles() {
    textLabel?.text = configuration.title
    detailTextLabel?.text = configuration.subtitle
  }

  private func applyLeadingItem() {
    switch configuration.leadingItem {
    case .none:
      imageView?.image = nil
      imageView?.tintColor = nil
      leadingButtonDidTapAction = nil
    case .image(let image, let tintColor, let action):
      imageView?.image = image
      imageView?.tintColor = tintColor
      leadingButtonDidTapAction = action
    }
  }

  private func applyAccessoryItem() {
    switch configuration.accessoryItem {
    case .none:
      accessoryType = .none
      accessoryView = nil
      accessoryButtonDidTapAction = nil
    case .detailButton:
      accessoryView = nil
      accessoryType = .detailButton
      accessoryButtonDidTapAction = nil
    case .image(let image, let tintColor, let action):
      accessoryType = .none
      accessoryButton.tintColor = tintColor
      accessoryButton.setImage(image, for: .normal)
      accessoryView = accessoryButton
      accessoryButtonDidTapAction = action
    }
  }

  private func updateButtonInteractions() {
    leadingButton.isUserInteractionEnabled = !isEditing && leadingButtonDidTapAction != nil
    accessoryButton.isUserInteractionEnabled = !isEditing && accessoryButtonDidTapAction != nil
  }
}
