protocol SettingsPreviewCellDelegate: AnyObject {
  func previewCellDidTapPlay(_ cell: SettingsPreviewCell)
}

/// A cell whose leading button plays a sample of what the row offers, mirroring the iOS VoiceOver
/// voice picker. The row itself stays selectable, so the button carries its own accessibility label.
final class SettingsPreviewCell: MWMTableViewCell {
  private enum Constants {
    static let buttonSize: CGFloat = 44
    static let iconSize: CGFloat = 26
    static let titleOffset: CGFloat = 4
    static let detailOffset: CGFloat = 8
    static let verticalInset: CGFloat = 11
    /// Keeps the row as tall as the plain cells of the other settings screens.
    static let minimumHeight: CGFloat = 44
  }

  private let playButton = UIButton(type: .system)
  private let titleLabel = UILabel(frame: .zero)
  private let detailLabel = UILabel(frame: .zero)
  private weak var delegate: SettingsPreviewCellDelegate?

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
  }

  func configure(delegate: SettingsPreviewCellDelegate,
                 title: String,
                 detail: String?,
                 isSelected: Bool,
                 isPlaying: Bool,
                 showsDisclosure: Bool) {
    self.delegate = delegate
    titleLabel.text = title
    detailLabel.text = detail
    detailLabel.isHidden = detail == nil
    accessoryType = isSelected ? .checkmark : (showsDisclosure ? .disclosureIndicator : .none)
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
    detailLabel.setFontStyle(.regular17, color: .blackSecondary)
    detailLabel.setContentHuggingPriority(.defaultHigh, for: .horizontal)
    detailLabel.setContentCompressionResistancePriority(.required, for: .horizontal)

    contentView.addSubview(playButton)
    contentView.addSubview(titleLabel)
    contentView.addSubview(detailLabel)

    let margins = contentView.layoutMarginsGuide
    NSLayoutConstraint.activate([
      contentView.heightAnchor.constraint(greaterThanOrEqualToConstant: Constants.minimumHeight),

      playButton.leadingAnchor.constraint(equalTo: margins.leadingAnchor),
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
    ])
  }

  @objc private func onPlayButtonTap() {
    delegate?.previewCellDidTapPlay(self)
  }
}
