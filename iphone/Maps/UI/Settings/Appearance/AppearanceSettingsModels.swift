enum AppearanceSettingsSection: String {
  case options
}

extension MWMTheme {
  static let settingsOptions: [MWMTheme] = [.auto, .night, .day]

  var settingsTheme: MWMTheme {
    switch self {
    case .vehicleDay: return .day
    case .vehicleNight: return .night
    default: return self
    }
  }
}

struct AppearanceSettingsState {
  let theme: MWMTheme
}

typealias AppearanceSettingsViewController = SettingsViewController<AppearanceSettingsSection, MWMTheme>
typealias AppearanceSettingsSectionViewModel = SettingsSectionViewModel<AppearanceSettingsSection, MWMTheme>
typealias AppearanceSettingsItemViewModel = SettingsItemViewModel<MWMTheme>
