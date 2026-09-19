import AVFoundation

/// Plays a sample phrase in a given voice for the settings screens. Previewing never changes the
/// voice the turn notifications are read in.
final class TTSVoicePreviewPlayer {
  /// The voice being previewed, or `nil` when nothing is playing.
  private(set) var playingVoice: TTSVoice?
  /// Called on the main queue when playback ends on its own. Not called by an explicit `stop()`.
  var onFinished: (() -> Void)?

  private let tester = TTSTester()
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

  func play(_ voice: TTSVoice) {
    guard let speechVoice = AVSpeechSynthesisVoice(identifier: voice.identifier) else { return }
    let phrase = tester.nextTest(speechVoice.language) ?? speechVoice.name
    guard !phrase.isEmpty else { return }

    playingVoice = voice
    MWMTextToSpeech.tts().speakPreview(phrase, voiceIdentifier: voice.identifier) { [weak self] in
      self?.playingVoice = nil
      self?.onFinished?()
    }
  }

  func stop() {
    guard playingVoice != nil else { return }
    playingVoice = nil
    MWMTextToSpeech.tts().stopPreview()
  }
}
