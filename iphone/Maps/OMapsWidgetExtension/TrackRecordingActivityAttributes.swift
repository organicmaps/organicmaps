import ActivityKit
import AppIntents

struct TrackRecordingActivityAttributes: ActivityAttributes {
  public struct ContentState: Codable, Hashable {
    let recordingTime: String
    let trackLength: String

    static let initial = ContentState(recordingTime: "", trackLength: "")
  }

  let stopRecordingDeepLink: URL
}
