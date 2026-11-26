@objc
protocol SettingsTableViewSwitchCellDelegate {
  func switchCell(_ cell: SettingsTableViewSwitchCell, didChangeValue value: Bool)
}

@objc
class SettingsTableViewSwitchCell: MWMTableViewCell {
  let switchButton = UISwitch()

  @IBOutlet weak var delegate: SettingsTableViewSwitchCellDelegate?

  @objc
  var isEnabled = true {
    didSet {
      styleTitle()
      switchButton.isUserInteractionEnabled = isEnabled
      if isEnabled {
        switchButton.accessibilityTraits.remove(.notEnabled)
      } else {
        switchButton.accessibilityTraits.insert(.notEnabled)
      }
    }
  }

  @objc
  var isOn: Bool {
    get { switchButton.isOn }
    set { switchButton.isOn = newValue }
  }

  @objc
  func setOn(_ isOn: Bool, animated: Bool) {
    switchButton.setOn(isOn, animated: animated)
  }

  override init(style _: UITableViewCell.CellStyle, reuseIdentifier: String?) {
    super.init(style: .subtitle, reuseIdentifier: reuseIdentifier)
    setupCell()
  }

  @available(*, unavailable)
  required init?(coder: NSCoder) {
    super.init(coder: coder)
  }

  override func awakeFromNib() {
    super.awakeFromNib()
    setupCell()
  }

  @objc
  func config(delegate: SettingsTableViewSwitchCellDelegate, title: String, subtitile: String? = nil, isOn: Bool) {
    setStyle(.background)

    self.delegate = delegate

    textLabel?.text = title
    styleTitle()

    detailTextLabel?.text = subtitile

    switchButton.isOn = isOn
    accessoryType = .none
    accessoryView = switchButton
    selectionStyle = .none

    isAccessibilityElement = false
    textLabel?.isAccessibilityElement = false
    detailTextLabel?.isAccessibilityElement = false
    switchButton.isAccessibilityElement = true
    switchButton.accessibilityLabel = [title, subtitile].compactMap { $0?.isEmpty == false ? $0 : nil }.joined(separator: ", ")
    accessibilityElements = [switchButton]
  }

  @objc
  func setDetail(_ text: String?) {
    detailTextLabel?.text = text
    setNeedsLayout()
  }

  private func setupCell() {
    setStyle(.background)
    styleTitle()
    textLabel?.numberOfLines = 0
    textLabel?.lineBreakMode = .byWordWrapping

    switchButton.onTintColor = .linkBlue
    switchButton.addTarget(self, action: #selector(switchChanged), for: .valueChanged)
    accessoryView = switchButton

    detailTextLabel?.setFontStyleAndApply(.regular12, color: .blackSecondary)
    detailTextLabel?.numberOfLines = 0
    detailTextLabel?.lineBreakMode = .byWordWrapping
  }

  @objc
  private func switchChanged() {
    delegate?.switchCell(self, didChangeValue: switchButton.isOn)
  }

  private func styleTitle() {
    textLabel?.setFontStyleAndApply(.regular17, color: isEnabled ? .blackPrimary : .blackSecondary)
  }
}
