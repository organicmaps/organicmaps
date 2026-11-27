final class SettingsTableViewiCloudSwitchCell: SettingsTableViewSwitchCell {
  var accessibilityAction: (() -> Void)?

  override func accessibilityActivate() -> Bool {
    guard let accessibilityAction else { return super.accessibilityActivate() }
    accessibilityAction()
    return true
  }

  func config(delegate: SettingsTableViewSwitchCellDelegate,
              title: String,
              subtitile: String? = nil,
              isAvailable: Bool,
              isOn: Bool) {
    config(delegate: delegate, title: title, subtitile: subtitile, isOn: isOn)
    accessibilityHint = nil
    guard isAvailable else {
      setOn(isOn, animated: true)
      accessoryView = nil
      accessoryType = .detailButton
      accessibilityElements = nil
      switchButton.isAccessibilityElement = false
      isAccessibilityElement = true
      accessibilityLabel = title
      accessibilityValue = subtitile ?? L("icloud_disabled_title")
      accessibilityHint = L("icloud_disabled_message")
      accessibilityTraits = .button
      return
    }
    accessoryView = switchButton
    accessoryType = .none
    setOn(isOn, animated: true)
    setNeedsLayout()
  }
}
