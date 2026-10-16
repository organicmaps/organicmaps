enum UnitsSettingsSection: String {
  case distance
  case altitude
}

enum UnitsSettingsItem: Hashable {
  case distance(Units)
  case altitude(Units)
}

extension Units {
  static let settingsOptions: [Units] = [.metric, .imperial]

  var altitudeTitle: String {
    switch self {
    case .metric: return L("altitude_units_meters")
    case .imperial: return L("altitude_units_feet")
    @unknown default: return ""
    }
  }
}

struct UnitsSettingsState {
  let units: Units
  let altitudeUnits: Units
}

typealias UnitsSettingsViewController = SettingsViewController<UnitsSettingsSection, UnitsSettingsItem>
typealias UnitsSettingsSectionViewModel = SettingsSectionViewModel<UnitsSettingsSection, UnitsSettingsItem>
typealias UnitsSettingsItemViewModel = SettingsItemViewModel<UnitsSettingsItem>
