enum TTSLanguageSettingsSection: String {
  case languages
}

struct TTSLanguageSettingsState {
  /// The voice each language is read in today, and how many others could replace it.
  let voices: [TTSLanguage: TTSVoice]
  let voiceCounts: [TTSLanguage: Int]
  let languages: [TTSLanguage]
  let currentLanguage: TTSLanguage?
  let playingVoice: TTSVoice?
}

typealias TTSLanguageSettingsViewController = SettingsViewController<TTSLanguageSettingsSection, TTSLanguage>
typealias TTSLanguageSettingsSectionViewModel = SettingsSectionViewModel<TTSLanguageSettingsSection, TTSLanguage>
typealias TTSLanguageSettingsItemViewModel = SettingsItemViewModel<TTSLanguage>
