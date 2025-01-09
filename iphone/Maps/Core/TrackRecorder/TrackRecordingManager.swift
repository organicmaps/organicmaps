@objc
enum TrackRecordingState: Int, Equatable {
  case inactive
  case active
}

enum TrackRecordingAction {
  case start
  case stopAndSave(name: String)
}

enum TrackRecordingError: Error {
  case locationIsProhibited
  case trackIsEmpty
  case systemError(Error)
}

enum TrackRecordingActionResult {
  case success
  case error(TrackRecordingError)
}

@objc
protocol TrackRecordingObservable: AnyObject {
  var recordingState: TrackRecordingState { get }
  var trackRecordingInfo: TrackInfo { get }

  func addObserver(_ observer: AnyObject, recordingIsActiveDidChangeHandler: @escaping TrackRecordingStateHandler)
  func removeObserver(_ observer: AnyObject)
  func contains(_ observer: AnyObject) -> Bool
}

typealias TrackRecordingStateHandler = (TrackRecordingState, TrackInfo) -> Void

@objcMembers
final class TrackRecordingManager: NSObject {

  typealias CompletionHandler = (TrackRecordingActionResult) -> Void

  fileprivate struct Observation {
    weak var observer: AnyObject?
    var recordingStateDidChangeHandler: TrackRecordingStateHandler?
  }

  static let shared: TrackRecordingManager = {
    let trackRecorder = FrameworkHelper.self
    let locationManager = LocationManager.self
    var activityManager: TrackRecordingActivityManager? = nil
    #if canImport(ActivityKit)
    if #available(iOS 16.2, *) {
      activityManager = TrackRecordingLiveActivityManager.shared
    }
    #endif
    return TrackRecordingManager(trackRecorder: trackRecorder,
                                 locationService: locationManager,
                                 activityManager: activityManager)
  }()

  private let trackRecorder: TrackRecorder.Type
  private var locationService: LocationService.Type
  private var activityManager: TrackRecordingActivityManager?
  private var observations: [Observation] = []
  private(set) var trackRecordingInfo: TrackInfo = .empty()

  var recordingState: TrackRecordingState {
    trackRecorder.isTrackRecordingEnabled() ? .active : .inactive
  }

  init(trackRecorder: TrackRecorder.Type,
       locationService: LocationService.Type,
       activityManager: TrackRecordingActivityManager?) {
    self.trackRecorder = trackRecorder
    self.locationService = locationService
    self.activityManager = activityManager
    super.init()
  }

  // MARK: - Public methods

  @objc
  func setup() {
    do {
      try checkIsLocationEnabled()
      switch recordingState {
      case .inactive:
        break
      case .active:
        subscribeOnTrackRecordingProgressUpdates()
      }
    } catch {
      handleError(error)
    }
  }

  @objc
  func isActive() -> Bool {
    recordingState == .active
  }

  func processAction(_ action: TrackRecordingAction, completion: (CompletionHandler)? = nil) {
    do {
      switch action {
      case .start:
        try startRecording()
      case .stopAndSave(let name):
        stopRecording()
        try checkIsTrackNotEmpty()
        saveTrackRecording(name: name)
      }
      completion?(.success)
    } catch {
      handleError(error, completion: completion)
    }
  }

  // MARK: - Private methods

  private func checkIsLocationEnabled() throws(TrackRecordingError) {
    if locationService.isLocationProhibited() {
      throw TrackRecordingError.locationIsProhibited
    }
  }

  private func checkIsTrackNotEmpty() throws(TrackRecordingError) {
    if trackRecorder.isTrackRecordingEmpty() {
      throw TrackRecordingError.trackIsEmpty
    }
  }

  // MARK: - Handle track recording process

  private func subscribeOnTrackRecordingProgressUpdates() {
    trackRecorder.setTrackRecordingUpdateHandler { [weak self] info in
      guard let self else { return }
      self.trackRecordingInfo = info
      self.notifyObservers()
      self.activityManager?.update(info)
    }
  }

  private func unsubscribeFromTrackRecordingProgressUpdates() {
    trackRecorder.setTrackRecordingUpdateHandler(nil)
  }

  // MARK: - Handle Start/Stop event and Errors

  private func startRecording() throws(TrackRecordingError) {
    switch recordingState {
    case .inactive:
      try checkIsLocationEnabled()
      subscribeOnTrackRecordingProgressUpdates()
      trackRecorder.startTrackRecording()
      notifyObservers()
      do {
        try activityManager?.start(with: trackRecordingInfo)
      } catch {
        LOG(.warning, "Failed to start activity manager")
        handleError(.systemError(error))
      }
    case .active:
      break
    }
  }

  private func stopRecording() {
    unsubscribeFromTrackRecordingProgressUpdates()
    trackRecorder.stopTrackRecording()
    trackRecordingInfo = .empty()
    activityManager?.stop()
    notifyObservers()
  }

  private func saveTrackRecording(name: String) {
    trackRecorder.saveTrackRecording(withName: name)
  }

  private func handleError(_ error: TrackRecordingError, completion: (CompletionHandler)? = nil) {
    switch error {
    case TrackRecordingError.locationIsProhibited:
      // Show alert to enable location
      locationService.checkLocationStatus()
    case TrackRecordingError.trackIsEmpty:
      Toast.toast(withText: L("track_recording_toast_nothing_to_save")).show()
    case TrackRecordingError.systemError(let error):
      LOG(.error, error.localizedDescription)
      break
    }
    DispatchQueue.main.async {
      completion?(.error(error))
    }
  }
}

// MARK: - TrackRecordingObserver

extension TrackRecordingManager: TrackRecordingObservable {
  @objc
  func addObserver(_ observer: AnyObject, recordingIsActiveDidChangeHandler: @escaping TrackRecordingStateHandler) {
    guard !observations.contains(where: { $0.observer === observer }) else { return }
    let observation = Observation(observer: observer, recordingStateDidChangeHandler: recordingIsActiveDidChangeHandler)
    observations.append(observation)
    recordingIsActiveDidChangeHandler(recordingState, trackRecordingInfo)
  }

  @objc
  func removeObserver(_ observer: AnyObject) {
    observations.removeAll { $0.observer === observer }
  }

  @objc
  func contains(_ observer: AnyObject) -> Bool {
    observations.contains { $0.observer === observer }
  }

  private func notifyObservers() {
    observations = observations.filter { $0.observer != nil }
    observations.forEach { $0.recordingStateDidChangeHandler?(recordingState, trackRecordingInfo) }
  }
}
