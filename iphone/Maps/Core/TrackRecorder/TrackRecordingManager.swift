enum TrackRecordingState {
  case inactive
  case active
  case error(TrackRecordingError)
}

enum TrackRecordingError: Error {
  case locationIsProhibited
}

typealias TrackRecordingStateHandler = (TrackRecordingState) -> Void

protocol TrackRecordingObservation {
  func addObserver(_ observer: AnyObject, trackRecordingStateDidChange handler: @escaping TrackRecordingStateHandler)
  func removeObserver(_ observer: AnyObject)
}

@objcMembers
final class TrackRecordingManager: NSObject {

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
  private var observers = [ObjectIdentifier: TrackRecordingManager.Observation]()
  private(set) var recordingState: TrackRecordingState = .inactive {
    didSet {
      notifyObservers(recordingState)
    }
  }

  private init(trackRecorder: TrackRecorder.Type) {
    self.trackRecorder = trackRecorder
    super.init()
    self.recordingState = getCurrentRecordingState()
  }

  func toggleRecording() {
    let state = getCurrentRecordingState()
    switch state {
    case .inactive:
      start()
    case .active:
      stop()
    case .error(let error):
      handleError(error)
    }
  }

  private func handleError(_ error: TrackRecordingError) {
    switch error {
    case .locationIsProhibited:
      // Show alert to enable location
      LocationManager.checkLocationStatus()
    }
    stopRecording(.withoutSaving)
  }

  private func getCurrentRecordingState() -> TrackRecordingState {
    guard !LocationManager.isLocationProhibited() else {
      return .error(.locationIsProhibited)
    }
    return trackRecorder.isTrackRecordingEnabled() ? .active : .inactive
  }

  private func start() {
    trackRecorder.startTrackRecording()
    recordingState = .active
  }

  private func stop() {
    guard !trackRecorder.isTrackRecordingEmpty() else {
      Toast.toast(withText: L("track_recording_toast_nothing_to_save")).show()
      stopRecording(.withoutSaving)
      return
    }
    Self.showOnFinishRecordingAlert(onSave: { [weak self] in
      guard let self else { return }
      // TODO: (KK) pass the user provided name from the track saving screen (when it will be implemented)
      self.stopRecording(.saveWithName())
    },
                                    onStop: { [weak self] in
      guard let self else { return }
      self.stopRecording(.withoutSaving)
    })
  }

  private func stopRecording(_ savingOption: SavingOption) {
    trackRecorder.stopTrackRecording()
    switch savingOption {
    case .withoutSaving:
      break
    case .saveWithName(let name):
      trackRecorder.saveTrackRecording(withName: name)
    }
    recordingState = .inactive
  }
  
  private static func showOnFinishRecordingAlert(onSave: @escaping () -> Void, onStop: @escaping () -> Void) {
    let alert = UIAlertController(title: L("track_recording_alert_title"),
                                  message: L("track_recording_alert_message"),
                                  preferredStyle: .alert)
    alert.addAction(UIAlertAction(title: L("save"), style: .cancel, handler: { _ in onSave() }))
    alert.addAction(UIAlertAction(title: L("stop_without_saving"), style: .default, handler: { _ in onStop() }))
    alert.addAction(UIAlertAction(title: L("continue_download"), style: .default, handler: nil))
    UIViewController.topViewController().present(alert, animated: true)
  }
}

// MARK: - TrackRecorder + Observation
extension TrackRecordingManager: TrackRecordingObservation {
  func addObserver(_ observer: AnyObject, trackRecordingStateDidChange handler: @escaping TrackRecordingStateHandler) {
    let id = ObjectIdentifier(observer)
    observers[id] = Observation(observer: observer, recordingStateDidChangeHandler: handler)
    notifyObservers(recordingState)
  }

  func removeObserver(_ observer: AnyObject) {
    let id = ObjectIdentifier(observer)
    observers.removeValue(forKey: id)
  }

  private func notifyObservers(_ state: TrackRecordingState) {
    observers = observers.filter { $0.value.observer != nil }
    observers.values.forEach { $0.recordingStateDidChangeHandler?(state) }
  }
}
