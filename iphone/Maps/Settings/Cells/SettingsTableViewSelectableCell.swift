@objc final class SettingsTableViewSelectableCell: MWMTableViewCell {

  static let cellId = "SettingsTableViewSelectableCell"

  @IBOutlet fileprivate weak var title: UILabel!

  func config(title: String) {
    backgroundColor = UIColor.white()

    self.title.text = title
    styleTitle()
  }

  fileprivate func styleTitle() {
    title.textColor = UIColor.blackPrimaryText()
    title.font = UIFont.regular17()
  }

}
