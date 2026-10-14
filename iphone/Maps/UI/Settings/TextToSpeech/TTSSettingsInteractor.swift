final class TTSSettingsInteractor {
  var presenter: TTSSettingsPresenter?

  private let textToSpeech: MWMTextToSpeech.Type
  private let routingManager: RoutingManager
  private var voicesObserver: NSObjectProtocol?
  private lazy var previewPlayer: TTSVoicePreviewPlayer = {
    let player = TTSVoicePreviewPlayer()
    player.onFinished = { [weak self] in self?.loadSettings(animatingDifferences: false) }
    return player
  }()

  init(textToSpeech: MWMTextToSpeech.Type = MWMTextToSpeech.self,
       routingManager: RoutingManager = .routingManager) {
    self.textToSpeech = textToSpeech
    self.routingManager = routingManager
    // The screen stays visible while the user installs a voice in the iOS settings, so refresh on
    // return instead of waiting for the next appearance.
    voicesObserver = NotificationCenter.default
      .addObserver(forName: .MWMTextToSpeechVoicesDidChange,
                   object: nil,
                   queue: .main) { [weak self] _ in
        self?.loadSettings(animatingDifferences: false)
      }
  }

  deinit {
    voicesObserver.map(NotificationCenter.default.removeObserver(_:))
  }

  private func loadSettings(animatingDifferences: Bool) {
    presenter?.present(state(), animatingDifferences: animatingDifferences)
  }

  private func state() -> TTSSettingsState {
    TTSSettingsState(isTTSEnabled: textToSpeech.isTTSEnabled(),
                     isStreetNamesTTSEnabled: textToSpeech.isStreetNamesTTSEnabled(),
                     language: textToSpeech.currentLanguage(),
                     voice: textToSpeech.currentVoice(),
                     playingVoice: previewPlayer.playingVoice,
                     speedCameraMode: routingManager.speedCameraMode)
  }

  private func select(_ item: TTSSettingsItem) {
    switch item {
    case .language:
      presenter?.presentTTSLanguageSettings()
    case .speedCamera(let mode):
      routingManager.speedCameraMode = mode
      loadSettings(animatingDifferences: false)
    case .voiceInstructions, .streetNames:
      break
    }
  }

  private func toggleVoicePreview() {
    guard let voice = textToSpeech.currentVoice() else { return }
    if voice == previewPlayer.playingVoice {
      previewPlayer.stop()
    } else {
      previewPlayer.play(voice)
    }
    loadSettings(animatingDifferences: false)
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
}

extension TTSSettingsInteractor: SettingsViewControllerInteractor {
  typealias Section = TTSSettingsSection
  typealias Item = TTSSettingsItem

  func handle(_ action: SettingsViewControllerAction<Item>) {
    switch action {
    // viewWillAppear always follows viewDidLoad, so loading here covers the first appearance too.
    case .willAppear:
      loadSettings(animatingDifferences: false)
    case .willDisappear:
      previewPlayer.stop()
    case .didSelect(let item):
      select(item)
    case .didTapPreview:
      toggleVoicePreview()
    case .didChangeSwitch(let item, let isOn):
      changeSwitch(item, isOn: isOn)
    default:
      break
    }
  }
}
