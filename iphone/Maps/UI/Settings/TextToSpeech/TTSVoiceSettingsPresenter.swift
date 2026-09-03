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
    if state.automaticVoice != nil {
      sections.append(SettingsSectionViewModel(section: .automatic,
                                               items: [item(.automatic,
                                                            title: L("auto"),
                                                            state: state)]))
    }

    // Match the system voice picker: standard voices are grouped by locale instead of repeating
    // their region beside every row.
    sections += standardSections(from: state)

    sections += TTSVoiceGroup.secondaryDisplayOrder.compactMap { group -> TTSVoiceSettingsSectionViewModel? in
      let voices = state.voices.filter { $0.group == group }
      guard !voices.isEmpty else { return nil }
      return SettingsSectionViewModel(section: .group(group),
                                      header: group.header,
                                      items: voices.map { item(.voice($0), title: $0.title, state: state) })
    }

    // The hint about installing more voices belongs under the whole list.
    if !sections.isEmpty {
      sections[sections.count - 1].footer = L("pref_tts_download_voices_description")
    }
    return sections
  }

  private func standardSections(from state: TTSVoiceSettingsState) -> [TTSVoiceSettingsSectionViewModel] {
    Dictionary(grouping: state.voices.filter { $0.group == .standard }, by: \TTSVoice.region)
      .map { region, voices in
        let header = region.map { "\(state.language.title) (\($0))" } ?? state.language.title
        return SettingsSectionViewModel(section: .locale(header),
                                        header: header,
                                        items: voices.map { item(.voice($0), title: $0.title, state: state) })
      }
      .sorted { $0.header?.localizedStandardCompare($1.header ?? "") == .orderedAscending }
  }

  private func item(_ item: TTSVoiceSettingsItem,
                    title: String,
                    state: TTSVoiceSettingsState) -> TTSVoiceSettingsItemViewModel {
    SettingsItemViewModel(item: item,
                          title: title,
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
