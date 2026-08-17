enum PowerManagementSettingsSection: String {
  case options
}

extension PowerManagementSettingsSection {
  var footer: String? {
    switch self {
    case .options: return L("power_managment_description")
    }
  }
}

extension MWMSettingsPowerManagement {
  static let settingsOptions: [MWMSettingsPowerManagement] = [.normal, .economyMaximum, .auto]
}

struct PowerManagementSettingsState {
  let powerManagement: MWMSettingsPowerManagement
}

typealias PowerManagementSettingsViewController =
  SettingsViewController<PowerManagementSettingsSection, MWMSettingsPowerManagement>
typealias PowerManagementSettingsSectionViewModel =
  SettingsSectionViewModel<PowerManagementSettingsSection, MWMSettingsPowerManagement>
typealias PowerManagementSettingsItemViewModel = SettingsItemViewModel<MWMSettingsPowerManagement>
