final class TTSLanguageSettingsPresenter {
  private weak var viewController: TTSLanguageSettingsViewController?

  init(viewController: TTSLanguageSettingsViewController) {
    self.viewController = viewController
  }

  func present(_ state: TTSLanguageSettingsState,
               animatingDifferences: Bool = true) {
    viewController?.display(SettingsViewModel(title: TTSSettingsItem.otherLanguage.title ?? "",
                                              sections: sections(from: state),
                                              animatingDifferences: animatingDifferences))
  }

  func presentPreviousScreen() {
    viewController?.displayPreviousScreen()
  }

  private func sections(from state: TTSLanguageSettingsState) -> [TTSLanguageSettingsSectionViewModel] {
    [
      SettingsSectionViewModel(section: .languages,
                               items: state.languages.map(item(_:))),
    ]
  }

  private func item(_ language: TTSLanguage) -> TTSLanguageSettingsItemViewModel {
    SettingsItemViewModel(item: language,
                          title: language.title,
                          kind: .selectable(isSelected: false))
  }
}

extension TTSLanguageSettingsViewController {
  func displayPreviousScreen() {
    navigationController?.popViewController(animated: true)
  }
}
