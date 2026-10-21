final class TTSSettingsPresenter {
  private weak var viewController: TTSSettingsViewController?

  init(viewController: TTSSettingsViewController) {
    self.viewController = viewController
  }

  func present(_ state: TTSSettingsState,
               animatingDifferences: Bool = true) {
    viewController?.display(SettingsViewModel(title: RootSettings.voiceInstructions.title,
                                              sections: sections(from: state),
                                              animatingDifferences: animatingDifferences))
  }

  func presentTTSLanguageSettings() {
    viewController?.displayTTSLanguageSettings()
  }

  private func sections(from state: TTSSettingsState) -> [TTSSettingsSectionViewModel] {
    let voiceSection = SettingsSectionViewModel(section: TTSSettingsSection.voiceInstructions,
                                                footer: TTSSettingsSection.voiceInstructions.footer,
                                                items: [item(.voiceInstructions, state: state)])
    guard state.isTTSEnabled else { return [voiceSection] }

    return [
      voiceSection,
      SettingsSectionViewModel(section: .streetNames,
                               footer: TTSSettingsSection.streetNames.footer,
                               items: [item(.streetNames, state: state)]),
      SettingsSectionViewModel(section: .language,
                               header: TTSSettingsSection.language.header,
                               footer: TTSSettingsSection.language.footer,
                               items: [item(.language, state: state)]),
      SettingsSectionViewModel(section: .speedCameras,
                               header: TTSSettingsSection.speedCameras.header,
                               items: SpeedCameraManagerMode.settingsOptions.map { item(.speedCamera($0), state: state) }),
    ]
  }

  private func item(_ item: TTSSettingsItem, state: TTSSettingsState) -> TTSSettingsItemViewModel {
    SettingsItemViewModel(item: item,
                          title: title(item, state: state),
                          detail: detail(item, state: state),
                          kind: kind(item, state: state))
  }

  private func title(_ item: TTSSettingsItem, state: TTSSettingsState) -> String? {
    switch item {
    case .language:
      state.language?.title
    default:
      item.title
    }
  }

  private func detail(_ item: TTSSettingsItem, state: TTSSettingsState) -> String? {
    switch item {
    case .language:
      state.voice?.title
    default:
      nil
    }
  }

  private func kind(_ item: TTSSettingsItem, state: TTSSettingsState) -> SettingsItemKind {
    switch item {
    case .voiceInstructions:
      .switcher(isOn: state.isTTSEnabled, isEnabled: true)
    case .streetNames:
      .switcher(isOn: state.isStreetNamesTTSEnabled, isEnabled: true)
    // The play button samples the selected voice, the row itself opens the language picker.
    case .language:
      .preview(isSelected: false,
               isPlaying: state.voice != nil && state.voice == state.playingVoice,
               showsDisclosure: true)
    case .speedCamera(let mode):
      .selectable(isSelected: mode == state.speedCameraMode)
    }
  }
}

extension TTSSettingsViewController {
  func displayTTSLanguageSettings() {
    navigationController?.pushViewController(SettingsBuilder.buildTTSLanguage(), animated: true)
  }
}
