enum TrafficSettingsSection: String {
  case apiKey
}

extension TrafficSettingsSection {
  var title: String? {
    switch self {
    case .apiKey: return L("pref_traffic_api_key_title")
    }
  }

  var footer: String? {
    switch self {
    case .apiKey: L("pref_traffic_disclaimer")
    }
  }
}

enum TrafficSettingsItem {
  case apiKey
  case keyError
}

struct TrafficSettingsState: Equatable {
  var apiKey: String
  var isKeyValid: Bool
}

typealias TrafficSettingsViewController = SettingsViewController<TrafficSettingsSection, TrafficSettingsItem>
typealias TrafficSettingsSectionViewModel = SettingsSectionViewModel<TrafficSettingsSection, TrafficSettingsItem>
typealias TrafficSettingsItemViewModel = SettingsItemViewModel<TrafficSettingsItem>
