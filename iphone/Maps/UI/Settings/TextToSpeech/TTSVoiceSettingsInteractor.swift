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
  private var automaticVoice: TTSVoice?
  private var previewedItem: TTSVoiceSettingsItem?
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
    automaticVoice = textToSpeech.bestVoice(for: language)
    present(animatingDifferences: false)
  }

  private func present(animatingDifferences: Bool) {
    presenter?.present(TTSVoiceSettingsState(language: language,
                                             voices: voices,
                                             automaticVoice: automaticVoice,
                                             selectedItem: selectedItem,
                                             // The player also stops when the app resigns active.
                                             playingItem: previewPlayer.playingVoice == nil ? nil : previewedItem),
                       animatingDifferences: animatingDifferences)
  }

  /// Nothing is selected while another language is being read out: its voices are not in this list.
  private var selectedItem: TTSVoiceSettingsItem? {
    guard language == textToSpeech.currentLanguage() else { return nil }
    guard textToSpeech.isVoicePinned(), let voice = textToSpeech.currentVoice() else { return .automatic }
    return .voice(voice)
  }

  private func voice(for item: TTSVoiceSettingsItem) -> TTSVoice? {
    switch item {
    case .automatic: automaticVoice
    case .voice(let voice): voice
    }
  }

  private func select(_ item: TTSVoiceSettingsItem) {
    previewPlayer.stop()
    switch item {
    case .automatic: textToSpeech.setLanguage(language)
    case .voice(let voice): textToSpeech.setVoice(voice)
    }
    presenter?.presentTTSSettings()
  }

  private func togglePreview(_ item: TTSVoiceSettingsItem) {
    if item == previewedItem, previewPlayer.playingVoice != nil {
      previewPlayer.stop()
    } else if let voice = voice(for: item) {
      previewPlayer.play(voice)
      previewedItem = item
    }
    present(animatingDifferences: false)
  }
}

extension TTSVoiceSettingsInteractor: SettingsViewControllerInteractor {
  typealias Section = TTSVoiceSettingsSection
  typealias Item = TTSVoiceSettingsItem

  func handle(_ action: SettingsViewControllerAction<TTSVoiceSettingsItem>) {
    switch action {
    // viewWillAppear always follows viewDidLoad, so loading here covers the first appearance too.
    case .willAppear:
      loadSettings()
    case .willDisappear:
      previewPlayer.stop()
    case .didSelect(let item):
      select(item)
    case .didTapPreview(let item):
      togglePreview(item)
    default:
      break
    }
  }
}
