enum TTSLanguageSettingsSection: String {
  case languages
}

struct TTSLanguageSettingsState {
  let languages: [TTSLanguage]
}

typealias TTSLanguageSettingsViewController = SettingsViewController<TTSLanguageSettingsSection, TTSLanguage>
typealias TTSLanguageSettingsSectionViewModel = SettingsSectionViewModel<TTSLanguageSettingsSection, TTSLanguage>
typealias TTSLanguageSettingsItemViewModel = SettingsItemViewModel<TTSLanguage>
