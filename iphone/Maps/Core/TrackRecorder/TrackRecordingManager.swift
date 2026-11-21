enum LocationError: Error {
  case locationIsProhibited
}

enum StartTrackRecordingResult {
  case success
  case failure(Error)
}

@objc
protocol TrackRecordingObservable: AnyObject {
  var recordingState: TrackRecordingState { get }
  var trackRecordingInfo: TrackInfo { get }
  var trackRecordingElevationProfileData: ElevationProfileData? { get }

  func addObserver(_ observer: AnyObject, recordingIsActiveDidChangeHandler: @escaping TrackRecordingStateHandler)
  func removeObserver(_ observer: AnyObject)
  func contains(_ observer: AnyObject) -> Bool
}

/// A handler type for extracting elevation profile data on demand.
typealias ElevationProfileDataExtractionHandler = () -> ElevationProfileData?

/// A callback type that notifies observers about track recording state changes.
/// - Parameters:
///   - state: The current recording state.
///   - info: The current track recording info.
///   - elevationProfileExtractor: A closure to fetch elevation profile data lazily.
typealias TrackRecordingStateHandler = (TrackRecordingState, TrackInfo, ElevationProfileDataExtractionHandler) -> Void

@objcMembers
final class TrackRecordingManager: NSObject {
  fileprivate struct Observation {
    weak var observer: AnyObject?
    var recordingStateDidChangeHandler: TrackRecordingStateHandler?
  }

  static let shared: TrackRecordingManager = {
    let trackRecorder = FrameworkHelper.self
    let locationManager = LocationManager.self
    var activityManager: TrackRecordingActivityManager? = nil
#if canImport(ActivityKit)
    if #available(iOS 16.2, *), !ProcessInfo.processInfo.isiOSAppOnMac {
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

  var trackRecordingElevationProfileData: ElevationProfileData? {
    trackRecorder.trackRecordingElevationInfo()
  }

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
    subscribeOnTheAppLifecycleEvents()
  }

  // MARK: - Public methods

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

  func isActive() -> Bool {
    recordingState == .active
  }

  func start() {
    start(completion: nil)
  }

  func start(completion: ((StartTrackRecordingResult) -> Void)? = nil) {
    do {
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
          handleError(error)
        }
      case .active:
        break
      }
      completion?(.success)
    } catch {
      handleError(error)
      completion?(.failure(error))
    }
  }

  func stopAndSave(withName name: String = "", completion: ((TrackRecordingSaveResult) -> Void)? = nil) {
    let result = trackRecorder.saveTrackRecording(withName: name)
    if result == .fileError {
      Toast.show(withText: L("track_recording_save_failed"))
    } else {
      unsubscribeFromTrackRecordingProgressUpdates()
      trackRecordingInfo = .empty()
      activityManager?.stop()
      notifyObservers()
      if result == .empty {
        Toast.show(withText: L("track_recording_toast_nothing_to_save"))
      }
    }
    completion?(result)
  }

  func discard() {
    unsubscribeFromTrackRecordingProgressUpdates()
    trackRecorder.discardTrackRecording()
    trackRecordingInfo = .empty()
    activityManager?.stop()
    notifyObservers()
  }

  // MARK: - Private methods

  private func subscribeOnTheAppLifecycleEvents() {
    NotificationCenter.default.addObserver(self,
                                           selector: #selector(notifyObservers),
                                           name: UIApplication.didBecomeActiveNotification,
                                           object: nil)
  }

  private func checkIsLocationEnabled() throws {
    if locationService.isLocationProhibited() {
      throw LocationError.locationIsProhibited
    }
  }

  private func subscribeOnTrackRecordingProgressUpdates() {
    trackRecorder.setTrackRecordingUpdateHandler { [weak self] info in
      // SafeCallback may deliver an update queued before unsubscribing.
      guard let self, self.isActive() else { return }
      self.trackRecordingInfo = info
      self.notifyObservers()
      self.activityManager?.update(info)
    }
  }

  private func unsubscribeFromTrackRecordingProgressUpdates() {
    trackRecorder.setTrackRecordingUpdateHandler(nil)
  }

  private func handleError(_ error: Error) {
    switch error {
    case LocationError.locationIsProhibited:
      // Show alert to enable location
      locationService.checkLocationStatus()
    default:
      LOG(.error, error.localizedDescription)
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
    recordingIsActiveDidChangeHandler(recordingState, trackRecordingInfo) {
      self.trackRecordingElevationProfileData
    }
  }

  @objc
  func removeObserver(_ observer: AnyObject) {
    observations.removeAll { $0.observer === observer }
  }

  @objc
  func contains(_ observer: AnyObject) -> Bool {
    observations.contains { $0.observer === observer }
  }

  @objc
  private func notifyObservers() {
    observations.removeAll { $0.observer == nil }
    for observation in observations {
      observation.recordingStateDidChangeHandler?(recordingState, trackRecordingInfo) { self.trackRecordingElevationProfileData }
    }
  }
}
