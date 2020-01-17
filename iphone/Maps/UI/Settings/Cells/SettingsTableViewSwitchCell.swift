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

  @IBOutlet weak var delegate: SettingsTableViewSwitchCellDelegate?

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

  override func awakeFromNib() {
    super.awakeFromNib()
    styleTitle()
  }

  @objc func config(delegate: SettingsTableViewSwitchCellDelegate, title: String, isOn: Bool) {
    backgroundColor = UIColor.white()

    self.delegate = delegate

    self.title.text = title
    styleTitle()

    switchButton.isOn = isOn
  }

  @IBAction fileprivate func switchChanged() {
    delegate?.switchCell(self, didChangeValue: switchButton.isOn)
  }

  fileprivate func styleTitle() {
    let style = "regular17:" + (isEnabled ? "blackPrimaryText" : "blackSecondaryText")
    title.setStyleAndApply(style)
  }
}
