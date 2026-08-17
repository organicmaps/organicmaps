@objc
final class SettingsTableViewSelectableCell: MWMTableViewCell {
  override init(style _: UITableViewCell.CellStyle, reuseIdentifier: String?) {
    super.init(style: .default, reuseIdentifier: reuseIdentifier)
    setupCell()
  }

  @available(*, unavailable)
  required init?(coder: NSCoder) {
    super.init(coder: coder)
  }

  override func awakeFromNib() {
    super.awakeFromNib()
    setupCell()
  }

  private func setupCell() {
    setStyle(.background)
    textLabel?.setFontStyle(.regular17, color: .blackPrimary)
    textLabel?.numberOfLines = 0
    textLabel?.lineBreakMode = .byWordWrapping
  }

  @objc func config(title: String) {
    textLabel?.text = title
  }

  func config(title: String, isSelected: Bool) {
    config(title: title)
    accessoryType = isSelected ? .checkmark : .none
  }
}
