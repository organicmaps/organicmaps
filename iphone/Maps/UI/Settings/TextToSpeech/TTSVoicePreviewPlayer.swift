import AVFoundation

/// Plays a sample phrase in a given voice for the settings screens. Previewing never changes the
/// voice the turn notifications are read in.
final class TTSVoicePreviewPlayer {
  /// The voice being previewed, or `nil` when nothing is playing.
  private(set) var playingVoice: TTSVoice?
  /// Called on the main queue when playback ends on its own. Not called by an explicit `stop()`.
  var onFinished: (() -> Void)?

  private var playbackID: UUID?
  private var resignActiveObserver: NSObjectProtocol?

  init() {
    // Nothing calls viewWillDisappear when the app is backgrounded, so a preview would otherwise
    // keep speaking over the lock screen.
    resignActiveObserver = NotificationCenter.default.addObserver(
      forName: UIApplication.willResignActiveNotification,
      object: nil,
      queue: .main
    ) { [weak self] _ in
      self?.stop()
    }
  }

  deinit {
    if let resignActiveObserver {
      NotificationCenter.default.removeObserver(resignActiveObserver)
    }
    stop()
  }

  /// Plays `voice` reading a phrase in `language`, the language the voice reads.
  func play(_ voice: TTSVoice, language: TTSLanguage) {
    guard AVSpeechSynthesisVoice(identifier: voice.identifier) != nil else { return }
    let phrase = Self.nextAppTip(languageCode: language.code)

    let playbackID = UUID()
    self.playbackID = playbackID
    playingVoice = voice
    MWMTextToSpeech.tts().speakPreview(phrase, voiceIdentifier: voice.identifier) { [weak self] in
      // A completion queued on main may arrive after another sample of the same voice has started.
      guard let self, self.playbackID == playbackID else { return }
      self.playbackID = nil
      self.playingVoice = nil
      self.onFinished?()
    }
  }

  func stop() {
    guard playingVoice != nil else { return }
    playbackID = nil
    playingVoice = nil
    MWMTextToSpeech.tts().stopPreview()
  }

  /// The app tips for the language with this internal `code`, in its closest app translation, the way
  /// Android picks resources for the voice's locale: Cantonese gets Traditional Chinese. The keys are
  /// listed one by one, like Android's `R.array.app_tips`, so the strings check can see that iOS uses them.
  static func appTips(languageCode: String) -> [String] {
    let localization = Bundle.preferredLocalizations(from: Bundle.main.localizations,
                                                     forPreferences: [languageCode]).first ?? languageCode
    return [
      L("app_tip_00", languageCode: localization),
      L("app_tip_01", languageCode: localization),
      L("app_tip_02", languageCode: localization),
      L("app_tip_03", languageCode: localization),
      L("app_tip_04", languageCode: localization),
      L("app_tip_05", languageCode: localization),
      L("app_tip_06", languageCode: localization),
      L("app_tip_07", languageCode: localization),
      L("app_tip_08", languageCode: localization),
      L("app_tip_09", languageCode: localization),
    ]
  }

  // One shuffled sequence for every settings screen, so previews cover every tip before repeating one.
  private static var shuffledAppTips: [String] = []
  private static var shuffledAppTipsLanguageCode: String?
  private static var appTipIndex = 0

  private static func nextAppTip(languageCode: String) -> String {
    if languageCode != shuffledAppTipsLanguageCode {
      shuffledAppTips = appTips(languageCode: languageCode).shuffled()
      shuffledAppTipsLanguageCode = languageCode
      appTipIndex = 0
    }
    defer { appTipIndex = (appTipIndex + 1) % shuffledAppTips.count }
    return shuffledAppTips[appTipIndex]
  }
}
