protocol SettingsSliderCellDelegate: AnyObject {
  func sliderCell(_ cell: SettingsSliderCell, didChangeValue value: Float)
}

final class SettingsSliderCell: MWMTableViewCell {
  private let slider = UISlider(frame: .zero)
  private let valueLabel = UILabel(frame: .zero)
  private weak var delegate: SettingsSliderCellDelegate?

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
    valueLabel.text = nil
  }

  func configure(delegate: SettingsSliderCellDelegate,
                 value: Float,
                 minimumValue: Float,
                 maximumValue: Float,
                 valueTitle: String,
                 isEnabled: Bool) {
    self.delegate = delegate
    slider.minimumValue = minimumValue
    slider.maximumValue = maximumValue
    slider.value = value
    slider.isEnabled = isEnabled
    valueLabel.text = valueTitle
    valueLabel.textColor = isEnabled ? .label : .tertiaryLabel
  }

  private func setupCell() {
    setStyle(.background)
    selectionStyle = .none

    slider.translatesAutoresizingMaskIntoConstraints = false
    slider.addTarget(self, action: #selector(sliderChanged), for: .valueChanged)

    valueLabel.translatesAutoresizingMaskIntoConstraints = false
    valueLabel.setContentHuggingPriority(.required, for: .horizontal)

    contentView.addSubview(slider)
    contentView.addSubview(valueLabel)

    let margins = contentView.layoutMarginsGuide
    NSLayoutConstraint.activate([
      slider.leadingAnchor.constraint(equalTo: margins.leadingAnchor),
      slider.topAnchor.constraint(equalTo: margins.topAnchor),
      slider.bottomAnchor.constraint(equalTo: margins.bottomAnchor),
      valueLabel.leadingAnchor.constraint(equalTo: slider.trailingAnchor, constant: 8),
      valueLabel.trailingAnchor.constraint(equalTo: margins.trailingAnchor),
      valueLabel.centerYAnchor.constraint(equalTo: slider.centerYAnchor),
    ])
  }

  @objc private func sliderChanged() {
    delegate?.sliderCell(self, didChangeValue: slider.value)
  }
}
