final class TTSVoiceSettingsInteractor {
  var presenter: TTSVoiceSettingsPresenter?

  private let textToSpeech: MWMTextToSpeech.Type
  private let language: TTSLanguage
  private lazy var previewPlayer: TTSVoicePreviewPlayer = {
    let player = TTSVoicePreviewPlayer()
    player.onFinished = { [weak self] in self?.present(animatingDifferences: false) }
    return player
  }()

  private var voices: [TTSVoice] = []
  private var voicesObserver: NSObjectProtocol?

  init(language: TTSLanguage, textToSpeech: MWMTextToSpeech.Type = MWMTextToSpeech.self) {
    self.language = language
    self.textToSpeech = textToSpeech
    // This is the screen the user leaves to install a voice, and coming back does not re-appear it.
    voicesObserver = NotificationCenter.default
      .addObserver(forName: .MWMTextToSpeechVoicesDidChange,
                   object: nil,
                   queue: .main) { [weak self] _ in
        self?.loadSettings()
      }
  }

  deinit {
    voicesObserver.map(NotificationCenter.default.removeObserver(_:))
  }

  private func loadSettings() {
    voices = textToSpeech.voices(for: language)
    present(animatingDifferences: false)
  }

  private func present(animatingDifferences: Bool) {
    presenter?.present(TTSVoiceSettingsState(language: language,
                                             voices: voices,
                                             selectedVoice: textToSpeech.currentVoice(),
                                             playingVoice: previewPlayer.playingVoice),
                       animatingDifferences: animatingDifferences)
  }

  private func select(_ voice: TTSVoice) {
    previewPlayer.stop()
    textToSpeech.setVoice(voice)
    presenter?.presentTTSSettings()
  }

  private func togglePreview(_ voice: TTSVoice) {
    if voice == previewPlayer.playingVoice {
      previewPlayer.stop()
    } else {
      previewPlayer.play(voice)
    }
    present(animatingDifferences: false)
  }
}

extension TTSVoiceSettingsInteractor: SettingsViewControllerInteractor {
  typealias Section = TTSVoiceGroup
  typealias Item = TTSVoice

  func handle(_ action: SettingsViewControllerAction<TTSVoice>) {
    switch action {
    // viewWillAppear always follows viewDidLoad, so loading here covers the first appearance too.
    case .willAppear:
      loadSettings()
    case .willDisappear:
      previewPlayer.stop()
    case .didSelect(let voice):
      select(voice)
    case .didTapPreview(let voice):
      togglePreview(voice)
    default:
      break
    }
  }
}
