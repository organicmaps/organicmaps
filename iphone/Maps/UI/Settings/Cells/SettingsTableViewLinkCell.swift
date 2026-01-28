@objc
final class SettingsTableViewLinkCell: MWMTableViewCell {

  override func awakeFromNib() {
    super.awakeFromNib()
    setupCell()
  }

  private func setupCell() {
    setStyle(.background)
    textLabel?.setFontStyle(.regular17, color: .blackPrimary)
    textLabel?.numberOfLines = 0
    textLabel?.lineBreakMode = .byWordWrapping
    detailTextLabel?.setFontStyle(.regular17, color: .blackSecondary)
  }

  @objc func config(title: String, info: String?) {
    textLabel?.text = title
    detailTextLabel?.text = info
    detailTextLabel?.isHidden = info == nil
  }
}
