import ActivityKit

#if canImport(ActivityKit)

private let kCurrentTrackRecordingLiveActivityIDKey = "kCurrentTrackRecordingLiveActivityIDKey"

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
    stop()
    let state = TrackRecordingLiveActivityAttributes.ContentState(trackInfo: info)
    let content = ActivityContent<TrackRecordingLiveActivityAttributes.ContentState>(state: state, staleDate: nil)
    let attributes = TrackRecordingLiveActivityAttributes()
    let activity = try LiveActivityManager.startActivity(attributes, content: content)
    self.activity = activity
    UserDefaults.standard.set(activity.id, forKey: kCurrentTrackRecordingLiveActivityIDKey)
  }

  func update(_ info: TrackInfo) {
    guard let activity = activity ?? fetchCurrentActivity() else {
      LOG(.warning, "No active TrackRecordingLiveActivity found to update.")
      return
    }
    let state = TrackRecordingLiveActivityAttributes.ContentState(trackInfo: info)
    let content = ActivityContent<TrackRecordingLiveActivityAttributes.ContentState>(state: state, staleDate: nil)
    self.activity = activity
    LiveActivityManager.update(activity, content: content)
  }

  func stop() {
    let activities = Activity<TrackRecordingLiveActivityAttributes>.activities
    activities.forEach(LiveActivityManager.stop)
    activity = nil
    UserDefaults.standard.removeObject(forKey: kCurrentTrackRecordingLiveActivityIDKey)
  }

  private func fetchCurrentActivity() -> Activity<TrackRecordingLiveActivityAttributes>? {
    guard let id = UserDefaults.standard.string(forKey: kCurrentTrackRecordingLiveActivityIDKey) else { return nil }
    let activities = Activity<TrackRecordingLiveActivityAttributes>.activities
    return activities.first(where: { $0.id == id })
  }
}

// MARK: - Wrap TrackRecordingInfo to TrackRecordingLiveActivityAttributes.ContentState

private extension TrackRecordingLiveActivityAttributes.ContentState {
  init(trackInfo: TrackInfo) {
    self.distance = StatisticsViewModel(key: "", value: trackInfo.distance)
    self.duration = StatisticsViewModel(key: "", value: trackInfo.duration)
    self.maxElevation = StatisticsViewModel(key: L("elevation_profile_max_elevation"), value: trackInfo.maxElevation)
    self.minElevation = StatisticsViewModel(key: L("elevation_profile_min_elevation"), value: trackInfo.minElevation)
    self.ascent = StatisticsViewModel(key: L("elevation_profile_ascent"), value: trackInfo.ascent)
    self.descent = StatisticsViewModel(key: L("elevation_profile_descent"), value: trackInfo.descent)
  }
}

#endif
