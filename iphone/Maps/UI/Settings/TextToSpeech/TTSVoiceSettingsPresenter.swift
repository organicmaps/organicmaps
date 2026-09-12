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
    var sections: [TTSVoiceSettingsSectionViewModel] = []
    // Choosing the language rather than one of its voices keeps a better voice installed later from
    // being ignored, and is the way back from a voice picked by hand.
    if let automaticVoice = state.automaticVoice {
      sections.append(SettingsSectionViewModel(section: .automatic,
                                               items: [item(.automatic,
                                                            title: L("auto"),
                                                            detail: automaticVoice.title,
                                                            state: state)]))
    }

    // Voices of one language differ mostly by accent, which their names do not convey. The set a
    // voice comes from is the section it sits in.
    sections += TTSVoiceGroup.displayOrder.compactMap { group -> TTSVoiceSettingsSectionViewModel? in
      let voices = state.voices.filter { $0.group == group }
      guard !voices.isEmpty else { return nil }
      return SettingsSectionViewModel(section: .group(group),
                                      header: group.header,
                                      items: voices.map { item(.voice($0), title: $0.title, detail: $0.region,
                                                               state: state) })
    }

    // The hint about installing more voices belongs under the whole list.
    if !sections.isEmpty {
      sections[sections.count - 1].footer = L("pref_tts_download_voices_description")
    }
    return sections
  }

  private func item(_ item: TTSVoiceSettingsItem,
                    title: String,
                    detail: String?,
                    state: TTSVoiceSettingsState) -> TTSVoiceSettingsItemViewModel {
    SettingsItemViewModel(item: item,
                          title: title,
                          detail: detail,
                          kind: .preview(isSelected: item == state.selectedItem,
                                         isPlaying: item == state.playingItem,
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
