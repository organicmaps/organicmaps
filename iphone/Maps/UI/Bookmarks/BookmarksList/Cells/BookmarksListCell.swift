/// A list cell with a fixed layout: an optional leading icon button, a title/subtitle block and up
/// to two trailing icon buttons.
///
/// `UITableViewCell`'s built-in layout is deliberately not used. Its system accessories
/// (`accessoryType = .detailButton`) resize with the content size category while custom icons do
/// not, so at large fonts the rows of one list stopped sharing a trailing column. A fixed-size
/// `UIStackView` in `accessoryView` solves that much, but not the leading side: the icon column had
/// to become a fixed width so that bookmark rows, category rows and the list header line up on one
/// grid, and the built-in layout derives that inset from the image's own size. Here every button
/// keeps its size at any content size category and only the labels grow.
final class BookmarksListCell: MWMTableViewCell {
  private enum Constants {
    static let leadingButtonWidth: CGFloat = 52
    static let trailingButtonWidth: CGFloat = 52
    static let innerTrailingButtonWidth: CGFloat = 36
    /// The inner slot is narrower than the 44pt minimum touch target, so it claims the difference
    /// from the side that faces the labels.
    static let innerTrailingButtonTouchInset: CGFloat = 8
    static let minimumHeight: CGFloat = 48
    static let verticalInset: CGFloat = 12
    static let horizontalInset: CGFloat = 16
    static let labelsToButtonSpacing: CGFloat = 8
    static let labelsSpacing: CGFloat = 2
  }

  private let contentStackView = UIStackView()
  private let leadingButton = IconButton(frame: .zero)
  private let labelsContainerView = UIView()
  private let labelsStackView = UIStackView()
  private let titleLabel = UILabel()
  private let subtitleLabel = UILabel()
  private let innerTrailingButton = IconButton(frame: .zero)
  private let trailingButton = IconButton(frame: .zero)

  private var isShowingEditControl = false

  override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
    super.init(style: style, reuseIdentifier: reuseIdentifier)
    setupViews()
    layoutViews()
    configure(.default)
  }

  @available(*, unavailable)
  required init?(coder _: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  func configure(_ configuration: Configuration) {
    // A pooled cell keeps whatever state it had; UIKit re-applies the real one before display.
    isShowingEditControl = false
    titleLabel.text = configuration.title
    subtitleLabel.text = configuration.subtitle
    subtitleLabel.isHidden = configuration.subtitle?.isEmpty ?? true
    applyLeadingItem(configuration.leadingItem)
    applyTrailingButtons(configuration.trailingButtons)
    updateEditingState()
  }

  override func setEditing(_ editing: Bool, animated: Bool) {
    super.setEditing(editing, animated: animated)
    if !editing {
      isShowingEditControl = false
    }
    updateEditingState()
  }

  override func willTransition(to state: UITableViewCell.StateMask) {
    super.willTransition(to: state)
    isShowingEditControl = state.contains(.showingEditControl)
    updateEditingState()
  }

  private func setupViews() {
    setStyle(.tableViewCell)
    selectionStyle = .default

    titleLabel.numberOfLines = 0
    titleLabel.lineBreakMode = .byWordWrapping
    titleLabel.setFontStyle(.regular16, color: .blackPrimary)

    subtitleLabel.numberOfLines = 0
    subtitleLabel.lineBreakMode = .byWordWrapping
    subtitleLabel.setFontStyle(.regular14, color: .blackSecondary)

    labelsStackView.axis = .vertical
    labelsStackView.spacing = Constants.labelsSpacing
    labelsStackView.addArrangedSubview(titleLabel)
    labelsStackView.addArrangedSubview(subtitleLabel)
    labelsContainerView.addSubview(labelsStackView)
    labelsContainerView.setContentCompressionResistancePriority(.defaultLow, for: .horizontal)

    contentStackView.axis = .horizontal
    contentStackView.alignment = .fill
    contentStackView.spacing = Constants.labelsToButtonSpacing
    contentStackView.isLayoutMarginsRelativeArrangement = true
    contentStackView.addArrangedSubview(leadingButton)
    contentStackView.addArrangedSubview(labelsContainerView)
    contentStackView.addArrangedSubview(innerTrailingButton)
    contentStackView.addArrangedSubview(trailingButton)
    // The leading button's width already includes the padding around its icon, and the two trailing
    // buttons form a single column.
    contentStackView.setCustomSpacing(0, after: leadingButton)
    contentStackView.setCustomSpacing(0, after: innerTrailingButton)
    innerTrailingButton.touchInset = Constants.innerTrailingButtonTouchInset
    contentView.addSubview(contentStackView)
    contentView.shouldGroupAccessibilityChildren = true

    leadingButton.isAccessibilityElement = false
  }

  private func layoutViews() {
    contentStackView.translatesAutoresizingMaskIntoConstraints = false
    labelsStackView.translatesAutoresizingMaskIntoConstraints = false

    // The cell is self-sized, so the bottom constraint yields to UIKit's encapsulated height.
    let bottomConstraint = contentStackView.bottomAnchor.constraint(equalTo: contentView.bottomAnchor)
    bottomConstraint.priority = .required - 1

    NSLayoutConstraint.activate([
      contentStackView.topAnchor.constraint(equalTo: contentView.topAnchor),
      bottomConstraint,
      contentStackView.leadingAnchor.constraint(equalTo: contentView.leadingAnchor),
      contentStackView.trailingAnchor.constraint(equalTo: contentView.trailingAnchor),
      contentView.heightAnchor.constraint(greaterThanOrEqualToConstant: Constants.minimumHeight),

      labelsStackView.leadingAnchor.constraint(equalTo: labelsContainerView.leadingAnchor),
      labelsStackView.trailingAnchor.constraint(equalTo: labelsContainerView.trailingAnchor),
      labelsStackView.centerYAnchor.constraint(equalTo: labelsContainerView.centerYAnchor),
      labelsStackView.topAnchor.constraint(greaterThanOrEqualTo: labelsContainerView.topAnchor,
                                           constant: Constants.verticalInset),
      labelsStackView.bottomAnchor.constraint(lessThanOrEqualTo: labelsContainerView.bottomAnchor,
                                              constant: -Constants.verticalInset),

      leadingButton.widthAnchor.constraint(equalToConstant: Constants.leadingButtonWidth),
      innerTrailingButton.widthAnchor.constraint(equalToConstant: Constants.innerTrailingButtonWidth),
      trailingButton.widthAnchor.constraint(equalToConstant: Constants.trailingButtonWidth),
    ])
  }

  private func applyLeadingItem(_ leadingItem: Configuration.LeadingItem) {
    switch leadingItem {
    case .none:
      leadingButton.isHidden = true
      leadingButton.setImage(nil, for: .normal)
      leadingButton.action = nil
    case .image(let image, let tintColor, let action):
      leadingButton.isHidden = false
      leadingButton.setImage(image, for: .normal)
      leadingButton.tintColor = tintColor
      leadingButton.action = action
    }
  }

  private func applyTrailingButtons(_ buttons: Configuration.TrailingButtons) {
    switch buttons {
    case .none:
      apply(nil, to: innerTrailingButton)
      apply(nil, to: trailingButton)
    case .one(let button):
      apply(nil, to: innerTrailingButton)
      apply(button, to: trailingButton)
    case .two(let inner, let edge):
      apply(inner, to: innerTrailingButton)
      apply(edge, to: trailingButton)
    }
  }

  private func apply(_ configuration: Configuration.TrailingButton?, to button: IconButton) {
    button.setImage(configuration?.image, for: .normal)
    button.tintColor = configuration?.tintColor
    button.accessibilityLabel = configuration?.accessibilityLabel
    button.action = configuration?.action
  }

  private func updateEditingState() {
    // Multiple selection is the only state that removes the buttons: a visible control that does
    // nothing is still announced by VoiceOver. Swipe actions are deliberately excluded — UIKit turns
    // `isEditing` on for them too, and dropping the buttons there would re-wrap the labels mid-swipe,
    // while a system accessory keeps the row's layout untouched.
    innerTrailingButton.isHidden = isShowingEditControl || innerTrailingButton.action == nil
    trailingButton.isHidden = isShowingEditControl || trailingButton.action == nil
    leadingButton.isUserInteractionEnabled = !isEditing && leadingButton.action != nil
    updateInsets()
  }

  private func updateInsets() {
    contentStackView.directionalLayoutMargins = NSDirectionalEdgeInsets(
      top: 0,
      leading: leadingButton.isHidden ? Constants.horizontalInset : 0,
      bottom: 0,
      trailing: trailingButton.isHidden ? Constants.horizontalInset : 0
    )
    let separatorLeftInset = leadingButton.isHidden ? Constants.horizontalInset : Constants.leadingButtonWidth
    separatorInset = UIEdgeInsets(top: 0, left: separatorLeftInset, bottom: 0, right: 0)
  }
}

/// A `.custom` button that carries its own action. It stays `.custom` on purpose: a `.system` button
/// renders its image as a template, which would flatten the multicoloured bookmark and track colour
/// dots into a single tint.
private final class IconButton: UIButton {
  var action: ((UIView) -> Void)?

  /// Extra touch area on both sides. The neighbouring button is later in `subviews` and therefore
  /// wins the overlap, so the button only really gains the side that faces the labels.
  var touchInset: CGFloat = 0

  override init(frame: CGRect) {
    super.init(frame: frame)
    addTarget(self, action: #selector(onTap), for: .touchUpInside)
  }

  @available(*, unavailable)
  required init?(coder _: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func point(inside point: CGPoint, with _: UIEvent?) -> Bool {
    bounds.insetBy(dx: -touchInset, dy: 0).contains(point)
  }

  @objc private func onTap() {
    action?(self)
  }
}
