protocol TTSSettingsPreviewCellDelegate: AnyObject {
  func previewCellDidTapPlay(_ cell: TTSSettingsPreviewCell)
}

/// A cell whose leading button plays a sample of what the row offers, mirroring the iOS VoiceOver
/// voice picker. The row itself stays selectable, so the button carries its own accessibility label.
final class TTSSettingsPreviewCell: MWMTableViewCell {
  private enum Constants {
    static let buttonSize: CGFloat = 44
    static let iconSize: CGFloat = 20
    static let titleOffset: CGFloat = 4
    static let detailOffset: CGFloat = 8
    static let verticalInset: CGFloat = 11
    /// Keeps the row as tall as the plain cells of the other settings screens.
    static let minimumHeight: CGFloat = 44
  }

  private let playButton = UIButton(type: .system)
  private let titleLabel = UILabel(frame: .zero)
  private let detailLabel = UILabel(frame: .zero)
  private weak var delegate: TTSSettingsPreviewCellDelegate?

  override init(style _: UITableViewCell.CellStyle, reuseIdentifier: String?) {
    super.init(style: .default, reuseIdentifier: reuseIdentifier)
    setupCell()
  }

  @available(*, unavailable)
  required init?(coder: NSCoder) {
    super.init(coder: coder)
  }

  override func prepareForReuse() {
    super.prepareForReuse()
    delegate = nil
    titleLabel.text = nil
    detailLabel.text = nil
    accessoryType = .none
    accessoryView = nil
  }

  func configure(delegate: TTSSettingsPreviewCellDelegate,
                 title: String,
                 detail: String?,
                 isSelected: Bool,
                 isPlaying: Bool,
                 showsDisclosure: Bool) {
    self.delegate = delegate
    titleLabel.text = title
    detailLabel.text = detail
    detailLabel.isHidden = detail == nil
    if isSelected {
      accessoryType = .none
      accessoryView = Self.selectedAccessoryView(showsDisclosure: showsDisclosure)
    } else {
      accessoryView = nil
      accessoryType = showsDisclosure ? .disclosureIndicator : .none
    }
    let configuration = UIImage.SymbolConfiguration(pointSize: Constants.iconSize, weight: .regular)
    playButton.setImage(UIImage(systemName: isPlaying ? "stop.circle" : "play.circle",
                                withConfiguration: configuration),
                        for: .normal)
    playButton.accessibilityLabel = isPlaying ? L("pref_tts_voice_preview_stop") : L("pref_tts_voice_preview_play")
  }

  private func setupCell() {
    setStyle(.background)

    playButton.translatesAutoresizingMaskIntoConstraints = false
    playButton.tintColor = .linkBlue
    playButton.addTarget(self, action: #selector(onPlayButtonTap), for: .touchUpInside)

    titleLabel.translatesAutoresizingMaskIntoConstraints = false
    titleLabel.numberOfLines = 0
    titleLabel.lineBreakMode = .byWordWrapping
    titleLabel.setFontStyle(.regular17, color: .blackPrimary)
    titleLabel.setContentHuggingPriority(.defaultLow, for: .horizontal)
    titleLabel.setContentCompressionResistancePriority(.defaultLow, for: .horizontal)

    detailLabel.translatesAutoresizingMaskIntoConstraints = false
    detailLabel.textAlignment = .right
    detailLabel.numberOfLines = 0
    detailLabel.lineBreakMode = .byWordWrapping
    detailLabel.setFontStyle(.regular17, color: .blackSecondary)
    detailLabel.setContentHuggingPriority(.defaultHigh, for: .horizontal)
    detailLabel.setContentCompressionResistancePriority(.defaultHigh, for: .horizontal)

    contentView.addSubview(playButton)
    contentView.addSubview(titleLabel)
    contentView.addSubview(detailLabel)

    let margins = contentView.layoutMarginsGuide
    NSLayoutConstraint.activate([
      contentView.heightAnchor.constraint(greaterThanOrEqualToConstant: Constants.minimumHeight),

      playButton.leadingAnchor.constraint(equalTo: margins.leadingAnchor, constant: -(Constants.buttonSize - Constants.iconSize) / 2),
      playButton.centerYAnchor.constraint(equalTo: contentView.centerYAnchor),
      playButton.widthAnchor.constraint(equalToConstant: Constants.buttonSize),
      playButton.heightAnchor.constraint(equalToConstant: Constants.buttonSize),

      titleLabel.leadingAnchor.constraint(equalTo: playButton.trailingAnchor, constant: Constants.titleOffset),
      titleLabel.topAnchor.constraint(equalTo: contentView.topAnchor, constant: Constants.verticalInset),
      titleLabel.bottomAnchor.constraint(equalTo: contentView.bottomAnchor, constant: -Constants.verticalInset),

      detailLabel.leadingAnchor.constraint(greaterThanOrEqualTo: titleLabel.trailingAnchor,
                                           constant: Constants.detailOffset),
      detailLabel.trailingAnchor.constraint(equalTo: margins.trailingAnchor),
      detailLabel.centerYAnchor.constraint(equalTo: contentView.centerYAnchor),
      detailLabel.topAnchor.constraint(greaterThanOrEqualTo: contentView.topAnchor,
                                       constant: Constants.verticalInset),
      detailLabel.bottomAnchor.constraint(lessThanOrEqualTo: contentView.bottomAnchor,
                                          constant: -Constants.verticalInset),
    ])
  }

  /// A custom accessory keeps the selected-state checkmark consistent when a disclosure indicator
  /// is also present.
  private static func selectedAccessoryView(showsDisclosure: Bool) -> UIView {
    let checkmarkConfiguration = UIImage.SymbolConfiguration(textStyle: .body)
      .applying(UIImage.SymbolConfiguration(weight: .semibold))
    let checkmark = UIImageView(image: UIImage(systemName: "checkmark", withConfiguration: checkmarkConfiguration))
    checkmark.tintColor = .linkBlue

    var accessoryViews: [UIView] = [checkmark]
    if showsDisclosure {
      let disclosureConfiguration = UIImage.SymbolConfiguration(textStyle: .body)
      let disclosure = UIImageView(image: UIImage(systemName: "chevron.right",
                                                  withConfiguration: disclosureConfiguration))
      disclosure.tintColor = .blackHintText
      accessoryViews.append(disclosure)
    }

    let stack = UIStackView(arrangedSubviews: accessoryViews)
    stack.alignment = .center
    stack.spacing = Constants.detailOffset
    stack.frame = CGRect(origin: .zero, size: stack.systemLayoutSizeFitting(UIView.layoutFittingCompressedSize))
    return stack
  }

  @objc private func onPlayButtonTap() {
    delegate?.previewCellDidTapPlay(self)
  }
}
