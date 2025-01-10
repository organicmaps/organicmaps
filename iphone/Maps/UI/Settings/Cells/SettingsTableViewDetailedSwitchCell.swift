class SettingsTableViewDetailedSwitchCell: SettingsTableViewSwitchCell {

  override func awakeFromNib() {
    super.awakeFromNib()
    styleDetail()
  }

  @objc
  func setDetail(_ text: String?) {
    detailTextLabel?.text = text
    setNeedsLayout()
  }

  private func styleDetail() {
    detailTextLabel?.setFontStyleAndApply(.regular12, color: .blackSecondary)
    detailTextLabel?.numberOfLines = 0
    detailTextLabel?.lineBreakMode = .byWordWrapping
  }
}
