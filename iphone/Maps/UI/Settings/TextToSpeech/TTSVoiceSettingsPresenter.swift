final class TTSVoiceSettingsPresenter {
  private weak var viewController: TTSVoiceSettingsViewController?

  init(viewController: TTSVoiceSettingsViewController) {
    self.viewController = viewController
  }

  func present(_ state: TTSVoiceSettingsState,
               animatingDifferences: Bool = true) {
    viewController?.display(SettingsViewModel(title: state.language.title,
                                              sections: sections(from: state),
                                              animatingDifferences: animatingDifferences))
  }

  func presentTTSSettings() {
    viewController?.displayTTSSettings()
  }

  private func sections(from state: TTSVoiceSettingsState) -> [TTSVoiceSettingsSectionViewModel] {
    var sections = TTSVoiceGroup.displayOrder.compactMap { group -> TTSVoiceSettingsSectionViewModel? in
      let voices = state.voices.filter { $0.group == group }
      guard !voices.isEmpty else { return nil }
      return SettingsSectionViewModel(section: group,
                                      header: group.header,
                                      items: voices.map { item($0, state: state) })
    }
    // The hint about installing more voices belongs under the whole list.
    if let last = sections.popLast() {
      sections.append(SettingsSectionViewModel(section: last.section,
                                               header: last.header,
                                               footer: L("pref_tts_download_voices_description"),
                                               items: last.items))
    }
    return sections
  }

  private func item(_ voice: TTSVoice, state: TTSVoiceSettingsState) -> TTSVoiceSettingsItemViewModel {
    // Voices of one language differ mostly by accent, which their names do not convey. The set a
    // voice comes from is the section it sits in.
    SettingsItemViewModel(item: voice,
                          title: voice.title,
                          detail: voice.region,
                          kind: .preview(isSelected: voice == state.selectedVoice,
                                         isPlaying: voice == state.playingVoice,
                                         showsDisclosure: false))
  }
}

extension TTSVoiceSettingsViewController {
  /// Selecting a voice returns straight to the TTS settings, skipping the language picker.
  func displayTTSSettings() {
    guard let navigationController,
          let ttsSettings = navigationController.viewControllers.first(where: { $0 is TTSSettingsViewController })
    else {
      assertionFailure("The voice picker is always pushed on top of the TTS settings")
      navigationController?.popViewController(animated: true)
      return
    }
    navigationController.popToViewController(ttsSettings, animated: true)
  }
}
