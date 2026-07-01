final class TTSSettingsInteractor {
  var presenter: TTSSettingsPresenter?

  private let textToSpeech: MWMTextToSpeech.Type
  private let routingManager: RoutingManager
  private var skipsNextWillAppearUpdate = true
  private var preferredLanguages: [TTSLanguage] = []

  init(textToSpeech: MWMTextToSpeech.Type = MWMTextToSpeech.self,
       routingManager: RoutingManager = .routingManager) {
    self.textToSpeech = textToSpeech
    self.routingManager = routingManager
  }

  private func loadSettings(animatingDifferences: Bool) {
    present(state(), animatingDifferences: animatingDifferences)
  }

  private func state() -> TTSSettingsState {
    updatePreferredLanguages()
    return TTSSettingsState(isTTSEnabled: textToSpeech.isTTSEnabled(),
                            isStreetNamesTTSEnabled: textToSpeech.isStreetNamesTTSEnabled(),
                            savedLanguage: textToSpeech.savedLanguage(),
                            preferredLanguages: preferredLanguages,
                            speedCameraMode: routingManager.speedCameraMode)
  }

  private func updatePreferredLanguages() {
    let latestLanguages = textToSpeech.preferredLanguages()
    guard !preferredLanguages.isEmpty, latestLanguages.count < preferredLanguages.count else {
      preferredLanguages = latestLanguages
      return
    }

    for language in latestLanguages where !preferredLanguages.contains(language) {
      preferredLanguages.append(language)
    }
  }

  private func select(_ item: TTSSettingsItem) {
    switch item {
    case .language(let language):
      textToSpeech.setNotificationsLanguage(language)
      loadSettings(animatingDifferences: false)
    case .otherLanguage:
      presenter?.presentTTSLanguageSettings()
    case .testVoice:
      textToSpeech.playRandomTestString()
    case .speedCamera(let mode):
      routingManager.speedCameraMode = mode
      loadSettings(animatingDifferences: false)
    case .voiceInstructions, .streetNames:
      break
    }
  }

  private func changeSwitch(_ item: TTSSettingsItem, isOn: Bool) {
    switch item {
    case .voiceInstructions:
      textToSpeech.setTTSEnabled(isOn)
      loadSettings(animatingDifferences: true)
    case .streetNames:
      textToSpeech.setStreetNamesTTSEnabled(isOn)
      loadSettings(animatingDifferences: true)
    default:
      break
    }
  }

  private func present(_ state: TTSSettingsState, animatingDifferences: Bool = true) {
    presenter?.present(state, animatingDifferences: animatingDifferences)
  }
}

extension TTSSettingsInteractor: SettingsViewControllerInteractor {
  typealias Section = TTSSettingsSection
  typealias Item = TTSSettingsItem

  func handle(_ action: SettingsViewControllerAction<Item>) {
    switch action {
    case .didLoad:
      loadSettings(animatingDifferences: false)
    case .willAppear:
      guard skipsNextWillAppearUpdate else {
        loadSettings(animatingDifferences: true)
        return
      }
      skipsNextWillAppearUpdate = false
    case .didSelect(let item):
      select(item)
    case .didChangeSwitch(let item, let isOn):
      changeSwitch(item, isOn: isOn)
    default:
      break
    }
  }
}
