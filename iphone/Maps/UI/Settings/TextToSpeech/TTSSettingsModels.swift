enum TTSSettingsSection: Int {
  case voiceInstructions
  case streetNames
  case language
  case speedCameras
}

extension TTSSettingsSection {
  var header: String? {
    switch self {
    case .voiceInstructions, .streetNames:
      nil
    case .language:
      L("pref_tts_language_title")
    case .speedCameras:
      L("speedcams_alert_title")
    }
  }

  var footer: String? {
    switch self {
    case .voiceInstructions, .speedCameras:
      nil
    case .streetNames:
      L("pref_tts_street_names_description")
    case .language:
      L("pref_tts_download_voices_description")
    }
  }
}

enum TTSSettingsItem: Hashable {
  case voiceInstructions
  case streetNames
  case language(TTSLanguage)
  case otherLanguage
  case testVoice
  case speedCamera(SpeedCameraManagerMode)
}

extension TTSSettingsItem {
  var title: String? {
    switch self {
    case .voiceInstructions:
      RootSettings.voiceInstructions.title
    case .streetNames:
      L("pref_tts_street_names_title")
    case .language(let language):
      language.title
    case .otherLanguage:
      L("pref_tts_other_section_title")
    case .testVoice:
      L("pref_tts_test_voice_title")
    case .speedCamera(let mode):
      mode.title
    }
  }
}

extension SpeedCameraManagerMode {
  static let settingsOptions: [SpeedCameraManagerMode] = [.auto, .always, .never]

  var title: String {
    switch self {
    case .auto:
      L("pref_tts_speedcams_auto")
    case .always:
      L("pref_tts_speedcams_always")
    case .never:
      L("pref_tts_speedcams_never")
    @unknown default:
      ""
    }
  }
}

struct TTSSettingsState {
  let isTTSEnabled: Bool
  let isStreetNamesTTSEnabled: Bool
  let savedLanguage: String?
  let preferredLanguages: [TTSLanguage]
  let speedCameraMode: SpeedCameraManagerMode
}

typealias TTSSettingsViewController = SettingsViewController<TTSSettingsSection, TTSSettingsItem>
typealias TTSSettingsSectionViewModel = SettingsSectionViewModel<TTSSettingsSection, TTSSettingsItem>
typealias TTSSettingsItemViewModel = SettingsItemViewModel<TTSSettingsItem>
