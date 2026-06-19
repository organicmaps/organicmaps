enum MapTilesSettingsSection: String {
  case enable
  case url
  case cacheSize
  case opacity
}

extension MapTilesSettingsSection {
  var title: String? {
    switch self {
    case .enable: return nil
    case .url: return L("pref_bg_tiles_url_title")
    case .cacheSize: return L("pref_bg_tiles_size_title")
    case .opacity: return L("pref_bg_tiles_opacity_title")
    }
  }

  var footer: String? {
    switch self {
    case .url: L("pref_bg_tiles_disclaimer")
    case .enable, .cacheSize, .opacity: nil
    }
  }
}

enum MapTilesSettingsItem {
  case enable
  case url
  case urlError
  case cacheSize
  case opacity
}

struct MapTilesSettingsState: Equatable {
  var isEnabled: Bool
  var url: String
  var cacheSizeMB: Int
  var opacityPct: Int
  var isConfigValid: Bool
}

enum MapTilesSettingsLimits {
  static let minCacheSizeMB = 1
  static let maxCacheSizeMB = 1000
  static let minOpacityPct = 0
  static let maxOpacityPct = 100
}

typealias MapTilesSettingsViewController = SettingsViewController<MapTilesSettingsSection, MapTilesSettingsItem>
typealias MapTilesSettingsSectionViewModel = SettingsSectionViewModel<MapTilesSettingsSection, MapTilesSettingsItem>
typealias MapTilesSettingsItemViewModel = SettingsItemViewModel<MapTilesSettingsItem>
