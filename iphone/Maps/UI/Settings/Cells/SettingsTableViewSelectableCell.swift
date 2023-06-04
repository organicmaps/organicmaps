@objc
final class SettingsTableViewSelectableCell: MWMTableViewCell {
  @IBOutlet fileprivate weak var title: UILabel!

  override func awakeFromNib() {
    super.awakeFromNib()
    self.styleName = "Background"
    self.title.styleName = "regular17:blackPrimaryText"
  }

  @objc func config(title: String) {
    self.title.text = title
  }
}
