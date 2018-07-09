@objc
final class SettingsTableViewSelectableCell: MWMTableViewCell {
  @IBOutlet fileprivate weak var title: UILabel!

  @objc func config(title: String) {
    backgroundColor = UIColor.white()

    self.title.text = title
    styleTitle()
  }

  fileprivate func styleTitle() {
    title.textColor = UIColor.blackPrimaryText()
    title.font = UIFont.regular17()
  }
}
