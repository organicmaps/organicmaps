@objc
protocol SettingsTableViewSwitchCellDelegate {
  func switchCell(_ cell: SettingsTableViewSwitchCell, didChangeValue value: Bool)
}

@objc
final class SettingsTableViewSwitchCell: MWMTableViewCell {
  @IBOutlet fileprivate weak var title: UILabel!
  @IBOutlet fileprivate weak var switchButton: UISwitch! {
    didSet {
      switchButton.addTarget(self, action: #selector(switchChanged), for: .valueChanged)
    }
  }

  weak var delegate: SettingsTableViewSwitchCellDelegate?

  @objc
  var isEnabled = true {
    didSet {
      styleTitle()
      switchButton.isUserInteractionEnabled = isEnabled
    }
  }

  @objc
  var isOn: Bool {
    get { return switchButton.isOn }
    set { switchButton.isOn = newValue }
  }

  @objc func config(delegate: SettingsTableViewSwitchCellDelegate, title: String, isOn: Bool) {
    backgroundColor = UIColor.white()

    self.delegate = delegate

    self.title.text = title
    styleTitle()

    switchButton.isOn = isOn
    styleSwitchButton()
  }

  @IBAction fileprivate func switchChanged() {
    delegate?.switchCell(self, didChangeValue: switchButton.isOn)
  }

  fileprivate func styleTitle() {
    title.textColor = isEnabled ? UIColor.blackPrimaryText() : UIColor.blackSecondaryText()
    title.font = UIFont.regular17()
  }

  fileprivate func styleSwitchButton() {
    switchButton.onTintColor = UIColor.linkBlue()
  }
}
