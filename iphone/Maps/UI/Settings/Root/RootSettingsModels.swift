enum RootSettingsSection: String, CaseIterable, Hashable {
  case profile
  case general
  case navigation
}

enum RootSettings: String, Hashable {
  case profile
  case units
  case zoomButtons
  case buildings3D
  case autoDownload
  case showDownloadedRegions
  case mobileInternet
  case powerManagement
  case bookmarksTextPlacement
  case largeFont
  case transliteration
  case compassCalibration
  case appearance
  case iCloud
  case mapTiles
  case logging
  case perspectiveView
  case autoZoom
  case voiceInstructions
  case routingOptions
}

extension RootSettingsSection {
  var title: String? {
    switch self {
    case .profile: nil
    case .general: L("general_settings")
    case .navigation: L("prefs_group_route")
    }
  }

  var footer: String? {
    switch self {
    case .profile: nil
    case .general: L("enable_logging_warning_message")
    case .navigation: nil
    }
  }
}

extension RootSettings {
  var title: String {
    switch self {
    case .profile: return L("profile")
    case .units: return L("measurement_units")
    case .zoomButtons: return L("pref_zoom_title")
    case .buildings3D: return L("pref_map_3d_buildings_title")
    case .autoDownload: return L("autodownload")
    case .showDownloadedRegions: return L("show_downloaded_regions")
    case .mobileInternet: return L("mobile_data")
    case .powerManagement: return L("power_managment_title")
    case .bookmarksTextPlacement: return L("bookmarks_text_placement_title")
    case .largeFont: return L("big_font")
    case .transliteration: return L("transliteration_title")
    case .compassCalibration: return L("pref_calibration_title")
    case .appearance: return L("pref_appearance_title")
    case .iCloud: return "iCloud Synchronization (Beta)"
    case .mapTiles: return L("pref_bg_tiles_title")
    case .logging: return L("enable_logging")
    case .perspectiveView: return L("pref_map_3d_title")
    case .autoZoom: return L("pref_map_auto_zoom")
    case .voiceInstructions: return L("pref_tts_enable_title")
    case .routingOptions: return L("driving_options_title")
    }
  }
}

extension RootSettings {
  var screen: SettingsScreen? {
    switch self {
    case .profile: return .profile
    case .units: return .units
    case .mobileInternet: return .mobileInternet
    case .powerManagement: return .powerManagement
    case .bookmarksTextPlacement: return .bookmarksTextPlacement
    case .appearance: return .appearance
    case .voiceInstructions: return .voiceInstructions
    case .routingOptions: return .drivingOptions
    case .mapTiles: return .mapTiles
    case .zoomButtons,
         .buildings3D,
         .autoDownload,
         .showDownloadedRegions,
         .largeFont,
         .transliteration,
         .compassCalibration,
         .iCloud,
         .logging,
         .perspectiveView,
         .autoZoom:
      return nil
    }
  }
}

extension Placement {
  var title: String {
    switch self {
    case .none: return L("hide")
    case .right: return L("right")
    case .bottom: return L("bottom")
    @unknown default: return ""
    }
  }
}

extension MWMTheme {
  var title: String {
    switch self {
    case .vehicleDay, .day: return L("pref_appearance_light")
    case .vehicleNight, .night: return L("pref_appearance_dark")
    case .auto: return L("auto")
    @unknown default: return ""
    }
  }
}

extension Units {
  var title: String {
    switch self {
    case .metric: return L("kilometres")
    case .imperial: return L("miles")
    @unknown default: return ""
    }
  }
}

extension MWMSettingsPowerManagement {
  var title: String? {
    switch self {
    case .normal: return L("power_managment_setting_never")
    case .economyMaximum: return L("power_managment_setting_manual_max")
    case .auto: return L("power_managment_setting_auto")
    case .none, .economyMedium: return nil
    @unknown default: return nil
    }
  }
}

extension SettingsItemViewModel where Item == RootSettings {
  init(setting: RootSettings, detail: String? = nil, kind: SettingsItemKind) {
    item = setting
    title = setting.title
    self.detail = detail
    self.kind = kind
  }
}

extension SettingsSectionViewModel where Section == RootSettingsSection, Item == RootSettings {
  init(section: RootSettingsSection, items: [SettingsItemViewModel<RootSettings>]) {
    self.section = section
    header = section.title?.capitalized
    footer = section.footer
    self.items = items
  }
}

struct RootSettingsState {
  let osmUserName: String
  let measurementUnits: Units
  let zoomButtonsEnabled: Bool
  let buildings3DEnabled: Bool
  let buildings3DEditable: Bool
  let autoDownloadEnabled: Bool
  let showDownloadedRegions: Bool
  let mobileInternetPermission: MWMNetworkPolicyPermission
  let powerManagement: MWMSettingsPowerManagement
  let bookmarksTextPlacement: Placement
  let largeFontSize: Bool
  let transliteration: Bool
  let compassCalibrationEnabled: Bool
  let theme: MWMTheme
  let iCloudSynchronizationEnabled: Bool
  let iCloudSynchronizationState: SynchronizationManagerState?
  let map3DEnabled: Bool
  let autoZoomEnabled: Bool
  let ttsEnabled: Bool
  let fileLoggingEnabled: Bool
  let logFileSize: UInt64
}

typealias RootSettingsViewController = SettingsViewController<RootSettingsSection, RootSettings>
typealias RootSettingsSectionViewModel = SettingsSectionViewModel<RootSettingsSection, RootSettings>
typealias RootSettingsItemViewModel = SettingsItemViewModel<RootSettings>
