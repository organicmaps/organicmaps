final class TTSLanguageSettingsInteractor {
  var presenter: TTSLanguageSettingsPresenter?

  private let textToSpeech: MWMTextToSpeech.Type

  init(textToSpeech: MWMTextToSpeech.Type = MWMTextToSpeech.self) {
    self.textToSpeech = textToSpeech
  }

  private func loadSettings() {
    presenter?.present(TTSLanguageSettingsState(languages: textToSpeech.availableLanguages()),
                       animatingDifferences: false)
  }

  private func select(_ language: TTSLanguage) {
    textToSpeech.setNotificationsLanguage(language)
    presenter?.presentPreviousScreen()
  }
}

extension TTSLanguageSettingsInteractor: SettingsViewControllerInteractor {
  typealias Section = TTSLanguageSettingsSection
  typealias Item = TTSLanguage

  func handle(_ action: SettingsViewControllerAction<TTSLanguage>) {
    switch action {
    case .didLoad:
      loadSettings()
    case .didSelect(let language):
      select(language)
    default:
      break
    }
  }
}
