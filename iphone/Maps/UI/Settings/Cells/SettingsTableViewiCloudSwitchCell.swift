final class SettingsTableViewiCloudSwitchCell: SettingsTableViewSwitchCell {
  func config(delegate: SettingsTableViewSwitchCellDelegate,
              title: String,
              subtitile: String? = nil,
              isAvailable: Bool,
              isOn: Bool) {
    config(delegate: delegate, title: title, subtitile: subtitile, isOn: isOn)
    guard isAvailable else {
      setOn(isOn, animated: true)
      accessoryView = nil
      accessoryType = .detailButton
      accessibilityElements = nil
      switchButton.isAccessibilityElement = false
      textLabel?.isAccessibilityElement = true
      textLabel?.accessibilityLabel = switchButton.accessibilityLabel
      return
    }
    accessoryView = switchButton
    accessoryType = .none
    setOn(isOn, animated: true)
    setNeedsLayout()
  }
}
