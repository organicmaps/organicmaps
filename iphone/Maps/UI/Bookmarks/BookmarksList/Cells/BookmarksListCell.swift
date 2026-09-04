/// A list cell with a fixed layout: an optional leading icon button, a title/subtitle block and a
/// trailing icon button.
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
    apply(configuration.trailingButton, to: trailingButton)
    updateEditingState()
  }

  override func setEditing(_ editing: Bool, animated: Bool) {
    super.setEditing(editing, animated: animated)
    if !editing {
      isShowingEditControl = false
    }
    updateEditingState()
  }

  override func layoutSubviews() {
    super.layoutSubviews()
    updateSeparatorInset(leading: separatorLeadingInset, trailing: separatorTrailingInset)
  }

  /// The separator starts under the labels, wherever the leading slot and the safe area put them.
  private var separatorLeadingInset: CGFloat {
    leadingInset(of: labelsContainerView)
  }

  /// ...and ends at the trailing icon, the way system cells stop theirs at the accessory. The icon
  /// is centred in a wider button, so its own padding is added to the button's inset.
  private var separatorTrailingInset: CGFloat {
    guard !trailingButton.isHidden, let icon = trailingButton.image(for: .normal) else { return 0 }
    return trailingInset(of: trailingButton) + (trailingButton.bounds.width - icon.size.width) / 2
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
    contentStackView.addArrangedSubview(trailingButton)
    // The leading button's width already includes the padding around its icon.
    contentStackView.setCustomSpacing(0, after: leadingButton)
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

  private func apply(_ configuration: Configuration.TrailingButton?, to button: IconButton) {
    button.setImage(configuration?.image, for: .normal)
    button.tintColor = configuration?.tintColor
    button.accessibilityLabel = configuration?.accessibilityLabel
    button.action = configuration?.action
  }

  private func updateEditingState() {
    // Multiple selection is the only state that removes the button: a visible control that does
    // nothing is still announced by VoiceOver. Swipe actions are deliberately excluded — UIKit turns
    // `isEditing` on for them too, and dropping the button there would re-wrap the labels mid-swipe,
    // while a system accessory keeps the row's layout untouched.
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
  }
}

extension UITableViewCell {
  /// Distance from the cell's leading edge to the view's leading edge. Measured on the laid out
  /// geometry, so the safe area inset the table applies to `contentView` is included.
  func leadingInset(of view: UIView) -> CGFloat {
    let frame = convert(view.bounds, from: view)
    return isRTL ? bounds.maxX - frame.maxX : frame.minX
  }

  /// Distance from the cell's trailing edge to the view's trailing edge.
  func trailingInset(of view: UIView) -> CGFloat {
    let frame = convert(view.bounds, from: view)
    return isRTL ? frame.minX - bounds.minX : bounds.maxX - frame.maxX
  }

  /// Runs the separator between the two insets. UIKit mirrors `separatorInset` for the layout
  /// direction, so the leading value belongs in `left`.
  func updateSeparatorInset(leading: CGFloat, trailing: CGFloat) {
    let inset = UIEdgeInsets(top: 0, left: leading, bottom: 0, right: trailing)
    guard separatorInset != inset else { return }
    separatorInset = inset
  }

  private var isRTL: Bool {
    effectiveUserInterfaceLayoutDirection == .rightToLeft
  }
}

/// A `.custom` button that carries its own action. It stays `.custom` on purpose: a `.system` button
/// renders its image as a template, which would flatten the multicoloured bookmark and track colour
/// dots into a single tint.
private final class IconButton: UIButton {
  var action: ((UIView) -> Void)?

  override init(frame: CGRect) {
    super.init(frame: frame)
    addTarget(self, action: #selector(onTap), for: .touchUpInside)
  }

  @available(*, unavailable)
  required init?(coder _: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  @objc private func onTap() {
    action?(self)
  }
}
