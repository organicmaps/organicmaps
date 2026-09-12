enum TTSVoiceSettingsSection: Hashable {
  case automatic
  case group(TTSVoiceGroup)
}

/// The automatic choice keeps following the best installed voice instead of pinning one.
enum TTSVoiceSettingsItem: Hashable {
  case automatic
  case voice(TTSVoice)
}

struct TTSVoiceSettingsState {
  let language: TTSLanguage
  let voices: [TTSVoice]
  /// The voice the automatic choice reads the language in today, nil when it has none.
  let automaticVoice: TTSVoice?
  let selectedItem: TTSVoiceSettingsItem?
  let playingItem: TTSVoiceSettingsItem?
}

extension TTSVoiceGroup {
  /// Standard voices first: the others are legacy or joke sets that suit reading out directions less.
  static let displayOrder: [TTSVoiceGroup] = [.standard, .eloquence, .novelty]

  var header: String? {
    switch self {
    case .standard:
      L("pref_tts_voice_group_standard")
    // Eloquence is a product name and is not translated.
    case .eloquence:
      "Eloquence"
    case .novelty:
      L("pref_tts_voice_group_novelty")
    @unknown default:
      nil
    }
  }
}

typealias TTSVoiceSettingsViewController = SettingsViewController<TTSVoiceSettingsSection, TTSVoiceSettingsItem>
typealias TTSVoiceSettingsSectionViewModel = SettingsSectionViewModel<TTSVoiceSettingsSection, TTSVoiceSettingsItem>
typealias TTSVoiceSettingsItemViewModel = SettingsItemViewModel<TTSVoiceSettingsItem>
