final class TTSLanguageSettingsInteractor {
  var presenter: TTSLanguageSettingsPresenter?

  private let textToSpeech: MWMTextToSpeech.Type
  private lazy var previewPlayer: TTSVoicePreviewPlayer = {
    let player = TTSVoicePreviewPlayer()
    player.onFinished = { [weak self] in self?.present(animatingDifferences: false) }
    return player
  }()

  private var languages: [TTSLanguage] = []
  private var voices: [TTSLanguage: TTSVoice] = [:]
  private var voiceCounts: [TTSLanguage: Int] = [:]
  private var voicesObserver: NSObjectProtocol?

  init(textToSpeech: MWMTextToSpeech.Type = MWMTextToSpeech.self) {
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
    languages = textToSpeech.availableLanguages()
    let currentLanguage = textToSpeech.currentLanguage()
    // The selected language is read by the saved voice, which need not be the best installed one -
    // picking a voice by hand and then installing a better one leaves the two apart. Every other
    // language would switch to its best voice, since selecting one does not pin a voice.
    voices = languages.reduce(into: [:]) { result, language in
      result[language] = language == currentLanguage ? textToSpeech.currentVoice()
        : textToSpeech.bestVoice(for: language)
    }
    voiceCounts = languages.reduce(into: [:]) { $0[$1] = textToSpeech.voices(for: $1).count }
    present(animatingDifferences: false)
  }

  private func present(animatingDifferences: Bool) {
    presenter?.present(TTSLanguageSettingsState(voices: voices,
                                                voiceCounts: voiceCounts,
                                                languages: languages,
                                                currentLanguage: textToSpeech.currentLanguage(),
                                                playingVoice: previewPlayer.playingVoice),
                       animatingDifferences: animatingDifferences)
  }

  /// A language read by several voices opens the voice picker; one with a single voice is selected
  /// right away. Selecting never pins the voice, so a better one installed later is picked up.
  private func select(_ language: TTSLanguage) {
    guard voiceCounts[language] == 1 else {
      presenter?.presentTTSVoiceSettings(language)
      return
    }
    previewPlayer.stop()
    textToSpeech.setLanguage(language)
    presenter?.presentTTSSettings()
  }

  private func togglePreview(_ language: TTSLanguage) {
    guard let voice = voices[language] else {
      assertionFailure("A listed language always has a voice: \(language.code)")
      return
    }
    if voice == previewPlayer.playingVoice {
      previewPlayer.stop()
    } else {
      previewPlayer.play(voice)
    }
    present(animatingDifferences: false)
  }
}

extension TTSLanguageSettingsInteractor: SettingsViewControllerInteractor {
  typealias Section = TTSLanguageSettingsSection
  typealias Item = TTSLanguage

  func handle(_ action: SettingsViewControllerAction<TTSLanguage>) {
    switch action {
    // viewWillAppear always follows viewDidLoad, so loading here covers the first appearance too.
    case .willAppear:
      loadSettings()
    case .willDisappear:
      previewPlayer.stop()
    case .didSelect(let language):
      select(language)
    case .didTapPreview(let language):
      togglePreview(language)
    default:
      break
    }
  }
}
