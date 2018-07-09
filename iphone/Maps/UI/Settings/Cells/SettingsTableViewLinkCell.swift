@objc
final class SettingsTableViewLinkCell: MWMTableViewCell {
  @IBOutlet fileprivate weak var title: UILabel!
  @IBOutlet fileprivate weak var info: UILabel!

  @objc func config(title: String, info: String?) {
    backgroundColor = UIColor.white()

    self.title.text = title
    styleTitle()

    self.info.text = info
    self.info.isHidden = info == nil
    styleInfo()
  }

  fileprivate func styleTitle() {
    title.textColor = UIColor.blackPrimaryText()
    title.font = UIFont.regular17()
  }

  fileprivate func styleInfo() {
    info.textColor = UIColor.blackSecondaryText()
    info.font = UIFont.regular17()
  }
}
