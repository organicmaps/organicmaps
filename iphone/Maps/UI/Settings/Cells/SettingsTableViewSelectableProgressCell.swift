@objc
final class SettingsTableViewSelectableProgressCell: MWMTableViewCell {
  @IBOutlet private weak var title: UILabel!
  @IBOutlet weak var progress: UIActivityIndicatorView!

  @objc func config(title: String) {
    backgroundColor = UIColor.white()
    progress.activityIndicatorViewStyle =  UIColor.isNightMode() ? .white : .gray

    self.title.text = title
    styleTitle()
  }

  fileprivate func styleTitle() {
    title.textColor = UIColor.blackPrimaryText()
    title.font = UIFont.regular17()
  }
}
