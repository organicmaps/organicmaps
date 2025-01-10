@objc
final class SettingsTableViewSelectableCell: MWMTableViewCell {

  override func awakeFromNib() {
    super.awakeFromNib()
    setupCell()
  }

  override func applyTheme() {
    super.applyTheme()
    textLabel?.applyTheme()
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
}
