import ActivityKit

protocol TrackRecordingActivityManager {
  func start()
  func update(state: TrackRecordingActivityAttributes.ContentState)
  func stop()
}

#if canImport(ActivityKit)

@available(iOS 16.2, *)
final class TrackRecordingLiveActivityManager: TrackRecordingActivityManager {
  private var activity: Activity<TrackRecordingActivityAttributes>?

  static let shared = TrackRecordingLiveActivityManager()

  init() {}

  func start() {
    guard activity == nil else { return }
    let content = ActivityContent<TrackRecordingActivityAttributes.ContentState>(state: .initial, staleDate: nil)
    let attributes = TrackRecordingActivityAttributes(stopRecordingDeepLink: TrackRecordingAction.stop.buildDeeplink(for: .liveActivityWidget))
    activity = try? LiveActivityManager.startActivity(attributes, content: content)
  }

  func update(state: TrackRecordingActivityAttributes.ContentState) {
    guard let activity else { return }
    let content = ActivityContent<TrackRecordingActivityAttributes.ContentState>(state: state, staleDate: nil)
    LiveActivityManager.update(activity, content: content)
  }

  func stop() {
    guard let activity else { return }
    LiveActivityManager.stop(activity)
    self.activity = nil
  }
}

#endif
