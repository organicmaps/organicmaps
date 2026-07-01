enum UnitsSettingsSection: String {
  case options
}

extension Units {
  static let settingsOptions: [Units] = [.metric, .imperial]
}

struct UnitsSettingsState {
  let units: Units
}

typealias UnitsSettingsViewController = SettingsViewController<UnitsSettingsSection, Units>
typealias UnitsSettingsSectionViewModel = SettingsSectionViewModel<UnitsSettingsSection, Units>
typealias UnitsSettingsItemViewModel = SettingsItemViewModel<Units>
