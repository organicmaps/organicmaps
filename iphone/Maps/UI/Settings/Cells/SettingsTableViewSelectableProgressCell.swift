@objc
final class SettingsTableViewSelectableProgressCell: MWMTableViewCell {
  @IBOutlet private weak var title: UILabel!
  @IBOutlet weak var progress: UIActivityIndicatorView!

  override func awakeFromNib() {
    super.awakeFromNib()
    self.title.styleName = "regular17:blackPrimaryText"
  }
  
  @objc func config(title: String) {
    progress.style =  UIColor.isNightMode() ? .white : .gray

    self.title.text = title
  }
}
