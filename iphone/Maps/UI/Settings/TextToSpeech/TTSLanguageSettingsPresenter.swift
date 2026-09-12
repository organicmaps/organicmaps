final class TTSLanguageSettingsPresenter {
  private weak var viewController: TTSLanguageSettingsViewController?

  init(viewController: TTSLanguageSettingsViewController) {
    self.viewController = viewController
  }

  func present(_ state: TTSLanguageSettingsState,
               animatingDifferences: Bool = true) {
    viewController?.display(SettingsViewModel(title: L("pref_tts_language_title"),
                                              sections: sections(from: state),
                                              animatingDifferences: animatingDifferences))
  }

  func presentTTSVoiceSettings(_ language: TTSLanguage) {
    viewController?.displayTTSVoiceSettings(language)
  }

  func presentTTSSettings() {
    viewController?.displayPreviousScreen()
  }

  private func sections(from state: TTSLanguageSettingsState) -> [TTSLanguageSettingsSectionViewModel] {
    [
      SettingsSectionViewModel(section: .languages,
                               footer: L("pref_tts_download_voices_description"),
                               items: state.languages.map { item($0, state: state) }),
    ]
  }

  /// Every row shows and previews the voice it is read in; one with alternatives opens their list.
  private func item(_ language: TTSLanguage, state: TTSLanguageSettingsState) -> TTSLanguageSettingsItemViewModel {
    let voice = state.voices[language]
    return SettingsItemViewModel(item: language,
                                 title: language.title,
                                 detail: voice?.title,
                                 kind: .preview(isSelected: language == state.currentLanguage,
                                                isPlaying: voice != nil && voice == state.playingVoice,
                                                showsDisclosure: state.voiceCounts[language] ?? 0 > 1))
  }
}

extension TTSLanguageSettingsViewController {
  func displayTTSVoiceSettings(_ language: TTSLanguage) {
    navigationController?.pushViewController(SettingsBuilder.buildTTSVoice(language), animated: true)
  }

  func displayPreviousScreen() {
    navigationController?.popViewController(animated: true)
  }
}
