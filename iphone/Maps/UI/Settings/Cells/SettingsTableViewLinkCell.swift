@objc
final class SettingsTableViewLinkCell: MWMTableViewCell {
  @IBOutlet fileprivate weak var title: UILabel!
  @IBOutlet fileprivate weak var info: UILabel?

  override func awakeFromNib() {
    super.awakeFromNib()
    self.styleName = "Background"
    self.title.styleName = "regular17:blackPrimaryText"
    self.info?.styleName = "regular17:blackSecondaryText"
  }

  @objc func config(title: String, info: String?) {
    self.title.text = title

    self.info?.text = info
    self.info?.isHidden = info == nil
  }
}
