import ActivityKit

#if canImport(ActivityKit)

protocol TrackRecordingActivityManager {
  func start(with info: TrackInfo) throws
  func update(_ info: TrackInfo)
  func stop()
}

@available(iOS 16.2, *)
final class TrackRecordingLiveActivityManager {

  static let shared = TrackRecordingLiveActivityManager()

  private var activity: Activity<TrackRecordingLiveActivityAttributes>?

  private init() {}
}

// MARK: - TrackRecordingActivityManager

@available(iOS 16.2, *)
extension TrackRecordingLiveActivityManager: TrackRecordingActivityManager {
  func start(with info: TrackInfo) throws {
    guard activity == nil else { return }
    let state = TrackRecordingLiveActivityAttributes.ContentState(trackInfo: info)
    let content = ActivityContent<TrackRecordingLiveActivityAttributes.ContentState>(state: state, staleDate: nil)
    let attributes = TrackRecordingLiveActivityAttributes()
    activity = try LiveActivityManager.startActivity(attributes, content: content)
  }

  func update(_ info: TrackInfo) {
    guard let activity else { return }
    let state = TrackRecordingLiveActivityAttributes.ContentState(trackInfo: info)
    let content = ActivityContent<TrackRecordingLiveActivityAttributes.ContentState>(state: state, staleDate: nil)
    LiveActivityManager.update(activity, content: content)
  }

  func stop() {
    guard let activity else { return }
    LiveActivityManager.stop(activity)
    self.activity = nil
  }
}

// MARK: - Wrap TrackRecordingInfo to TrackRecordingLiveActivityAttributes.ContentState

private extension TrackRecordingLiveActivityAttributes.ContentState {
  init(trackInfo: TrackInfo) {
    let distance = DistanceFormatter.distanceString(fromMeters: trackInfo.distance)
    let duration = DurationFormatter.durationString(from: trackInfo.duration)
    let maxElevation = AltitudeFormatter.altitudeString(fromMeters: Double(trackInfo.maxElevation))
    let minElevation = AltitudeFormatter.altitudeString(fromMeters: Double(trackInfo.minElevation))
    let ascent = AltitudeFormatter.altitudeString(fromMeters: Double(trackInfo.ascent))
    let descent = AltitudeFormatter.altitudeString(fromMeters: Double(trackInfo.descent))

    self.distance = ValueViewModel(value: distance)
    self.duration = ValueViewModel(value: duration)
    self.ascent = DetailViewModel(.ascent, value: ascent)
    self.descent = DetailViewModel(.descent, value: descent)
    self.minElevation = DetailViewModel(.minElevation, value: minElevation)
    self.maxElevation = DetailViewModel(.maxElevation, value: maxElevation)
  }
}

private extension TrackRecordingLiveActivityAttributes.ContentState.DetailViewModel {
  init(_ elevationDescription: ElevationDescription, value: String) {
    self.value = value
    self.key = elevationDescription.title
    self.imageName = elevationDescription.imageName
  }
}

#endif
