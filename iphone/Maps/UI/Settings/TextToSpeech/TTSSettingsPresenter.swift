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
                               items: languageItems(state)),
      SettingsSectionViewModel(section: .speedCameras,
                               header: TTSSettingsSection.speedCameras.header,
                               items: SpeedCameraManagerMode.settingsOptions.map { item(.speedCamera($0), state: state) }),
    ]
  }

  private func languageItems(_ state: TTSSettingsState) -> [TTSSettingsItemViewModel] {
    state.preferredLanguages.map { item(.language($0), state: state) } +
      [
        item(.otherLanguage, state: state),
        item(.testVoice, state: state),
      ]
  }

  private func item(_ item: TTSSettingsItem, state: TTSSettingsState) -> TTSSettingsItemViewModel {
    SettingsItemViewModel(item: item,
                          title: item.title,
                          kind: kind(item, state: state))
  }

  private func kind(_ item: TTSSettingsItem, state: TTSSettingsState) -> SettingsItemKind {
    switch item {
    case .voiceInstructions:
      .switcher(isOn: state.isTTSEnabled, isEnabled: true)
    case .streetNames:
      .switcher(isOn: state.isStreetNamesTTSEnabled, isEnabled: true)
    case .language(let language):
      .selectable(isSelected: language.bcp47 == state.savedLanguage)
    case .otherLanguage:
      .link
    case .testVoice:
      .selectable(isSelected: false)
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
