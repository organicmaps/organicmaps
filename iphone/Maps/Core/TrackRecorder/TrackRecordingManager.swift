enum TrackRecordingState: Equatable {
  case inactive
  case active
  case error(TrackRecordingError)
}

enum TrackRecordingAction {
  case start
  case stop
}

enum TrackRecordingError: Error {
  case locationIsProhibited
}

typealias TrackRecordingStateHandler = (TrackRecordingState) -> Void

@objcMembers
final class TrackRecordingManager: NSObject {

  typealias CompletionHandler = () -> Void

  private enum SavingOption {
    case withoutSaving
    case saveWithName(String? = nil)
  }

  fileprivate struct Observation {
    weak var observer: AnyObject?
    var recordingStateDidChangeHandler: TrackRecordingStateHandler?
  }

  static let shared: TrackRecordingManager = TrackRecordingManager(trackRecorder: FrameworkHelper.self)

  private let trackRecorder: TrackRecorder.Type
  private var observations: [Observation] = []
  private(set) var recordingState: TrackRecordingState = .inactive {
    didSet {
      notifyObservers()
    }
  }

  private init(trackRecorder: TrackRecorder.Type) {
    self.trackRecorder = trackRecorder
    super.init()
    self.recordingState = getCurrentRecordingState()
  }

  func processAction(_ action: TrackRecordingAction, completion: (CompletionHandler)? = nil) {
    switch action {
    case .start:
      start(completion: completion)
    case .stop:
      stop(completion: completion)
    }
  }

  func addObserver(_ observer: AnyObject, recordingStateDidChangeHandler: @escaping TrackRecordingStateHandler) {
    let observation = Observation(observer: observer, recordingStateDidChangeHandler: recordingStateDidChangeHandler)
    observations.append(observation)
    recordingStateDidChangeHandler(recordingState)
  }

  func removeObserver(_ observer: AnyObject) {
    observations.removeAll { $0.observer === observer }
  }

  private func notifyObservers() {
    observations = observations.filter { $0.observer != nil }
    observations.forEach { $0.recordingStateDidChangeHandler?(recordingState) }
  }

  private func handleError(_ error: TrackRecordingError, completion: (CompletionHandler)? = nil) {
    switch error {
    case .locationIsProhibited:
      // Show alert to enable location
      LocationManager.checkLocationStatus()
    }
    stopRecording(.withoutSaving, completion: completion)
  }

  private func getCurrentRecordingState() -> TrackRecordingState {
    guard !LocationManager.isLocationProhibited() else {
      return .error(.locationIsProhibited)
    }
    return trackRecorder.isTrackRecordingEnabled() ? .active : .inactive
  }

  private func start(completion: (CompletionHandler)? = nil) {
    let state = getCurrentRecordingState()
    switch state {
    case .inactive:
      trackRecorder.startTrackRecording()
      recordingState = .active
      completion?()
    case .active:
      completion?()
    case .error(let trackRecordingError):
      handleError(trackRecordingError, completion: completion)
    }
  }

  private func stop(completion: (CompletionHandler)? = nil) {
    guard !trackRecorder.isTrackRecordingEmpty() else {
      Toast.toast(withText: L("track_recording_toast_nothing_to_save")).show()
      stopRecording(.withoutSaving, completion: completion)
      return
    }
    Self.showOnFinishRecordingAlert(onSave: { [weak self] in
      guard let self else { return }
      // TODO: (KK) pass the user provided name from the track saving screen (when it will be implemented)
      self.stopRecording(.saveWithName(), completion: completion)
    },
                                    onStop: { [weak self] in
      guard let self else { return }
      self.stopRecording(.withoutSaving, completion: completion)
    },
                                    onContinue: {
      completion?()
    })
  }

  private func stopRecording(_ savingOption: SavingOption, completion: (CompletionHandler)? = nil) {
    trackRecorder.stopTrackRecording()
    switch savingOption {
    case .withoutSaving:
      break
    case .saveWithName(let name):
      trackRecorder.saveTrackRecording(withName: name)
    }
    recordingState = .inactive
    completion?()
  }
  
  private static func showOnFinishRecordingAlert(onSave: @escaping CompletionHandler, 
                                                 onStop: @escaping CompletionHandler,
                                                 onContinue: @escaping CompletionHandler) {
    let alert = UIAlertController(title: L("track_recording_alert_title"), message: nil, preferredStyle: .alert)
    alert.addAction(UIAlertAction(title: L("continue_recording"), style: .default, handler: { _ in onContinue() }))
    alert.addAction(UIAlertAction(title: L("stop_without_saving"), style: .default, handler: { _ in onStop() }))
    alert.addAction(UIAlertAction(title: L("save"), style: .cancel, handler: { _ in onSave() }))
    UIViewController.topViewController().present(alert, animated: true)
  }
}
