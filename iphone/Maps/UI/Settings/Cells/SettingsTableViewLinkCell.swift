@objc
final class SettingsTableViewLinkCell: MWMTableViewCell {

  override func awakeFromNib() {
    super.awakeFromNib()
    setupCell()
  }

  private func setupCell() {
    styleName = "Background"
    textLabel?.styleName = "regular17:blackPrimaryText"
    textLabel?.numberOfLines = 0
    textLabel?.lineBreakMode = .byWordWrapping
    detailTextLabel?.styleName = "regular17:blackSecondaryText"
  }

  @objc func config(title: String, info: String?) {
    textLabel?.text = title
    detailTextLabel?.text = info
    detailTextLabel?.isHidden = info == nil
  }
}
