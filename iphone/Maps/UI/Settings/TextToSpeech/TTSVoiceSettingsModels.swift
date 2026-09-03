struct TTSVoiceSettingsState {
  let language: TTSLanguage
  let voices: [TTSVoice]
  let selectedVoice: TTSVoice?
  let playingVoice: TTSVoice?
}

extension TTSVoiceGroup {
  /// Standard voices first: the others are legacy or joke sets that suit reading out directions less.
  static let displayOrder: [TTSVoiceGroup] = [.standard, .eloquence, .novelty]

  var header: String? {
    switch self {
    case .standard:
      L("pref_tts_voice_title")
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

typealias TTSVoiceSettingsViewController = SettingsViewController<TTSVoiceGroup, TTSVoice>
typealias TTSVoiceSettingsSectionViewModel = SettingsSectionViewModel<TTSVoiceGroup, TTSVoice>
typealias TTSVoiceSettingsItemViewModel = SettingsItemViewModel<TTSVoice>
