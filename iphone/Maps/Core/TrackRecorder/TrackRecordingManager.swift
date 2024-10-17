enum TrackRecordingState: Equatable {
  case inactive
  case active
  case error(TrackRecordingError)
}

enum TrackRecordingAction: String, CaseIterable {
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

  static let shared: TrackRecordingManager = {
    let trackRecorder = FrameworkHelper.self
    var activityManager: TrackRecordingActivityManager? = nil
    #if canImport(ActivityKit)
    if #available(iOS 16.2, *) {
      activityManager = TrackRecordingLiveActivityManager.shared
    }
    #endif
    return TrackRecordingManager(trackRecorder: trackRecorder, activityManager: activityManager)
  }()

  private let trackRecorder: TrackRecorder.Type
  private(set) var recordingState: TrackRecordingState = .inactive
  private var activityManager: TrackRecordingActivityManager?

  init(trackRecorder: TrackRecorder.Type, activityManager: TrackRecordingActivityManager?) {
    self.trackRecorder = trackRecorder
    self.activityManager = activityManager
    super.init()
    subscribeOnAppLifecycleEvents()
  }

  @objc
  func setup() {
    recordingState = getCurrentRecordingState()
    switch recordingState {
    case .inactive:
      break
    case .active:
      activityManager?.start()
    case .error(let trackRecordingError):
      handleError(trackRecordingError)
    }
  }

  func processAction(_ action: TrackRecordingAction, completion: (CompletionHandler)? = nil) {
    switch action {
    case .start:
      start(completion: completion)
    case .stop:
      stop(completion: completion)
    }
  }

  private func subscribeOnAppLifecycleEvents() {
    NotificationCenter.default.addObserver(self, selector: #selector(prepareForTermination), name: UIApplication.willTerminateNotification, object: nil)
  }

  @objc
  private func prepareForTermination() {
    activityManager?.stop()
  }

  private func handleError(_ error: TrackRecordingError, completion: (CompletionHandler)? = nil) {
    switch error {
    case .locationIsProhibited:
      completion?()
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
    recordingState = getCurrentRecordingState()
    switch recordingState {
    case .inactive:
      trackRecorder.startTrackRecording()
      activityManager?.start()
      completion?()
    case .active:
      break
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
    activityManager?.stop()
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
